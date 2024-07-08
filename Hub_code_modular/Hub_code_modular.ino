#include <ArduinoJson.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h"

const char* hostname = "report";

IPAddress local_IP(192,168,1,1);
IPAddress gateway(192,168,1,2);
IPAddress subnet(255,255,255,0);

const char* ssid = "Humidity Sensor";
const char* password = "12345670";

AsyncWebServer server(80);

const int maxNodes = 20;
const int maxDataPoints = 100;

typedef struct data_form {
  int board;
  float temp;
  float humd;
} data_form;

data_form nodeData[maxNodes];
float humidityData[maxNodes][maxDataPoints];
float temperatureData[maxNodes][maxDataPoints];
int dataIndex[maxNodes];

void initDataPoints() {
  for (int i = 0; i < maxNodes; i++) {
    nodeData[i].board = 0; // Initialize board ID to 0 (non-existing)
    for (int j = 0; j < maxDataPoints; j++) {
      humidityData[i][j] = 0;
      temperatureData[i][j] = 0;
    }
    dataIndex[i] = 0;
  }
}

void updateDataPoints(int node) {
  float humidity = nodeData[node].humd;
  float temperature = nodeData[node].temp;
  if (!isnan(humidity) && !isnan(temperature)) {
    humidityData[node][dataIndex[node]] = humidity;
    temperatureData[node][dataIndex[node]] = temperature;
    dataIndex[node] = (dataIndex[node] + 1) % maxDataPoints;
  }
}

void readDHT() {
  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    int boardNum = message.substring(message.indexOf("n:") + 2, message.indexOf("n:") + 5).toInt();
    float temp = message.substring(message.indexOf("t:") + 2, message.indexOf("t:") + 7).toFloat();
    float humd = message.substring(message.indexOf("h:") + 2, message.indexOf("h:") + 7).toFloat();

    if (boardNum >= 100 && boardNum < 100 + maxNodes) {
      int node = boardNum - 100;
      nodeData[node].board = boardNum;
      nodeData[node].temp = temp; // Keep the temperature in Celsius
      nodeData[node].humd = humd;
    }
  }
}

String processor(const String& var) {
  if (var == "TEMPERATURE") {
    String tempString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].board != 0) {
        float tempF = (nodeData[i].temp * 9.0 / 5.0) + 32.0; // Convert to Fahrenheit
        tempString += "Node " + String(nodeData[i].board) + ": " + String(tempF, 2) + "°F ";
      }
    }
    return tempString;
  } else if (var == "HUMIDITY") {
    String humdString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].board != 0) {
        humdString += "Node " + String(nodeData[i].board) + ": " + String(nodeData[i].humd) + "% ";
      }
    }
    return humdString;
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Using GPIO16 as RX and GPIO17 as TX

  WiFi.softAP(ssid, password);

  Serial.println(WiFi.softAPIP());

  WiFi.setHostname(hostname);

  initDataPoints();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/main.html", "text/html", false, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    String tempString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].board != 0) {
        float tempF = (nodeData[i].temp * 9.0 / 5.0) + 32.0; // Convert to Fahrenheit
        tempString += "Node " + String(nodeData[i].board) + ": " + String(tempF, 2) + "°F\n";
      }
    }
    request->send(200, "text/plain", tempString);
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    String humdString = "";
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].board != 0) {
        humdString += "Node " + String(nodeData[i].board) + ": " + String(nodeData[i].humd) + "%\n";
      }
    }
    request->send(200, "text/plain", humdString);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<4096> jsonDoc;
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].board != 0) {
        JsonObject node = jsonDoc.createNestedObject("Node" + String(nodeData[i].board));
        JsonArray humidityArray = node.createNestedArray("humidity");
        JsonArray temperatureArray = node.createNestedArray("temperature");
        for (int j = 0; j < maxDataPoints; j++) {
          humidityArray.add(humidityData[i][(dataIndex[i] + j) % maxDataPoints]);
          temperatureArray.add((temperatureData[i][(dataIndex[i] + j) % maxDataPoints] * 9.0 / 5.0) + 32.0); // Convert to Fahrenheit
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

void loop() {
  readDHT();
  static unsigned long lastUpdateTime = 0;
  if (millis() - lastUpdateTime > 10000) {
    for (int i = 0; i < maxNodes; i++) {
      if (nodeData[i].board != 0) {
        updateDataPoints(i);
      }
    }
    lastUpdateTime = millis();
  }
  delay(500);
}