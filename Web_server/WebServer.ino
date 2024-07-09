#include <ArduinoJson.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h"
#include <Wire.h>

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
  Serial.println("Initializing data points...");
  for (int i = 0; i < maxNodes; i++) {
    nodeData[i].board = 0; // Initialize board ID to 0 (non-existing)
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
    Serial.print("Updated data points for node ");
    Serial.print(node);
    Serial.print(": Humidity=");
    Serial.print(humidity);
    Serial.print(", Temperature=");
    Serial.println(temperature);
  } else {
    Serial.println("Received invalid data. Skipping update.");
  }
}

typedef struct Data {
  float humd;
  int boardNum;
  float temp;
  bool init;
} Data;

typedef struct sendboards {
  Data Datas[20];
} sendboards;

sendboards rxData;
bool newRxData = false;

void receiveData(int numBytes) {
  Serial.print("Receiving data: ");
  Serial.print(numBytes);
  Serial.println(" bytes");

    Wire.readBytes((char*)&rxData, sizeof(sendboards));
    newRxData = true;
    Serial.println("Data received successfully.");
}

void processReceivedData() {
  if (newRxData) {
    Serial.println("Processing received data...");
    for (int i = 0; i < 20; i++) {
      if(rxData.Datas[i].init){
      int boardNum = rxData.Datas[i].boardNum;
      Serial.print("Board number: ");
      Serial.println(boardNum);
      if (boardNum >= 100 && boardNum < 100 + maxNodes) {
        int node = boardNum - 100;
        nodeData[node].board = boardNum;
        nodeData[node].temp = rxData.Datas[i].temp;
        nodeData[node].humd = rxData.Datas[i].humd;
        Serial.print("Node ");
        Serial.print(node);
        Serial.print(": Temp=");
        Serial.print(nodeData[node].temp);
        Serial.print(", Humd=");
        Serial.println(nodeData[node].humd);
        updateDataPoints(node);}
       else {
        Serial.println("Board number out of range. Skipping...");
      }}
    }
    newRxData = false;
    Serial.println("Finished processing data.");
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
  Wire.begin(9);
  Wire.onReceive(receiveData);
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

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
        JsonObject node = jsonDoc.createNestedObject("Node "+ String(nodeData[i].board - 101));
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
  Serial.println("Setup complete.");
}

void loop() {
  processReceivedData();

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