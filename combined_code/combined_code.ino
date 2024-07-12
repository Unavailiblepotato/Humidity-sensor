#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <esp_now.h>

// WiFi and server setup
const char* hostname = "report";
const char* ssid = "Humidity Sensor";
const char* password = "12345670";
AsyncWebServer server(80);

// Data structures
const int maxNodes = 20;
const int maxDataPoints = 100;

typedef struct BoardData {
  int boardNum;
  float temp;
  float humd;
  bool active;
  uint8_t macAddress[6];
} BoardData;

BoardData nodeData[maxNodes];
float humidityData[maxNodes][maxDataPoints];
float temperatureData[maxNodes][maxDataPoints];
int dataIndex[maxNodes];

// ESP-NOW message structure
typedef struct struct_message {
  uint8_t macAddress[6];
  float humd;
  float temp;
} struct_message;

// Function prototypes
void initDataPoints();
void updateDataPoints(int node);
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len);
String processor(const String& var);
void setupWiFi();
void setupESPNOW();
void setupWebServer();

void setup() {
  Serial.begin(115200);
  
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  initDataPoints();
  setupWiFi();
  setupESPNOW();
  setupWebServer();

  Serial.println("Setup complete.");
}

void loop() {
  // The main functionality is now handled by callbacks and the web server
  delay(10);
}

void initDataPoints() {
  Serial.println("Initializing data points...");
  for (int i = 0; i < maxNodes; i++) {
    nodeData[i].boardNum = 0;
    nodeData[i].active = false;
    for (int j = 0; j < maxDataPoints; j++) {
      humidityData[i][j] = 0;
      temperatureData[i][j] = 0;
    }
    dataIndex[i] = 0;
  }
  Serial.println("Data points initialized.");
}

void updateDataPoints(int node) {
  float humidity = nodeData[node].humd;
  float temperature = nodeData[node].temp;

  if (!isnan(humidity) && !isnan(temperature)) {
    humidityData[node][dataIndex[node]] = humidity;
    temperatureData[node][dataIndex[node]] = temperature;
    dataIndex[node] = (dataIndex[node] + 1) % maxDataPoints;
    Serial.printf("Updated data points for node %d: Humidity=%.2f, Temperature=%.2f\n", 
                  node, humidity, temperature);
  } else {
    Serial.println("Received invalid data. Skipping update.");
  }
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(struct_message)) {
    struct_message* data = (struct_message*)incomingData;
    
    // Find or create a slot for this board
    int nodeIndex = -1;
    for (int i = 0; i < maxNodes; i++) {
      if (!nodeData[i].active || memcmp(nodeData[i].macAddress, data->macAddress, 6) == 0) {
        nodeIndex = i;
        break;
      }
    }
    
    if (nodeIndex != -1) {
      nodeData[nodeIndex].boardNum = nodeIndex + 101;
      nodeData[nodeIndex].temp = data->temp;
      nodeData[nodeIndex].humd = data->humd;
      nodeData[nodeIndex].active = true;
      memcpy(nodeData[nodeIndex].macAddress, data->macAddress, 6);
      
      updateDataPoints(nodeIndex);
      
      Serial.printf("Received data from board %d: Humidity=%.2f, Temperature=%.2f\n", 
                    nodeData[nodeIndex].boardNum, nodeData[nodeIndex].humd, nodeData[nodeIndex].temp);
    } else {
      Serial.println("No available slots for new boards");
    }
  }
}

String processor(const String& var) {
  if (var == "TEMPERATURE") {
    String tempString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].active) {
        float tempF = (nodeData[i].temp * 9.0 / 5.0) + 32.0;
        tempString += "Node " + String(nodeData[i].boardNum) + ": " + String(tempF, 2) + "°F ";
      }
    }
    return tempString;
  } else if (var == "HUMIDITY") {
    String humdString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].active) {
        humdString += "Node " + String(nodeData[i].boardNum) + ": " + String(nodeData[i].humd) + "% ";
      }
    }
    return humdString;
  }
  return String();
}

void setupWiFi() {
  WiFi.softAP(ssid, password);
  WiFi.setHostname(hostname);
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupESPNOW() {
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/main.html", "text/html", false, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    String tempString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].active) {
        float tempF = (nodeData[i].temp * 9.0 / 5.0) + 32.0;
        tempString += "Node " + String(nodeData[i].boardNum) + ": " + String(tempF, 2) + "°F\n";
      }
    }
    request->send(200, "text/plain", tempString);
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    String humdString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].active) {
        humdString += "Node " + String(nodeData[i].boardNum) + ": " + String(nodeData[i].humd) + "%\n";
      }
    }
    request->send(200, "text/plain", humdString);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<4096> jsonDoc;
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].active) {
        JsonObject node = jsonDoc.createNestedObject("Node " + String(nodeData[i].boardNum - 100));
        JsonArray humidityArray = node.createNestedArray("humidity");
        JsonArray temperatureArray = node.createNestedArray("temperature");
        for (int j = 0; j < maxDataPoints; j++) {
          humidityArray.add(humidityData[i][(dataIndex[i] + j) % maxDataPoints]);
          temperatureArray.add((temperatureData[i][(dataIndex[i] + j) % maxDataPoints] * 9.0 / 5.0) + 32.0);
        }
      }
    }
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    request->send(200, "application/json", jsonString);
  });

  server.serveStatic("/", LittleFS, "/").setDefaultFile("main.html");

  server.begin();
}
