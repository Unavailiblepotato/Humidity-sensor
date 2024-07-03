#include <ArduinoJson.h>                          // needed for JSON encapsulation (send multiple variables with one string)
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

const int maxDataPoints = 100;
float humidityData[maxDataPoints];
float temperatureData[maxDataPoints];
int dataIndex = 0;

typedef struct data_form{
  int board;
  float temp;
  float humd;
}data_form;

data_form lastData;

data_form Data;

//Queue<data_form> timeData(100);
void initDataPoints() {
  for (int i = 0; i < maxDataPoints; i++) {
    humidityData[i] = 0;
    temperatureData[i] = 0;
  }
}

// Function to update data points
void updateDataPoints() {
  
  float humidity = readDHT().humd;
  float temperature = readDHT().temp;
  if (!isnan(humidity) && !isnan(temperature)) {
    humidityData[dataIndex] = humidity;
    temperatureData[dataIndex] = temperature;
    dataIndex = (dataIndex + 1) % maxDataPoints;
  }
}

data_form readDHT() {
    if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    Serial.print(message);
    //Serial.println(message);
    String board = message.substring(message.indexOf("n:") + 2, message.indexOf("t:") + 5);
    String temp = message.substring(message.indexOf("t:") + 2, message.indexOf("t:") + 6);
    String humd = message.substring(message.indexOf("h:") + 2, message.indexOf("h:") + 6);
    Data.temp = (temp.toFloat()*1.8)+32;
    Data.humd = humd.toFloat();
    //if(Data != lastData){
     // timeData.push(Data);
    //}
   // lastData = Data;
   
    return Data;
  }else{
    return Data;
  }

}
uint8_t retries=0;

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  Data = readDHT();
  if(var == "TEMPERATURE"){

    return String(Data.temp);
  }
  else if(var == "HUMIDITY"){
    return String(Data.humd);
  }
  return String();
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  // Initialize UART with baud rate 9600
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Using GPIO16 as RX and GPIO17 as TX

  WiFi.softAP(ssid, password);

  Serial.println(WiFi.softAPIP());

  WiFi.setHostname(hostname);


 /* WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED && retries<20)
  {
    Serial.print(".");
    retries++;
    delay(1000);
  }
  if(WiFi.status()==WL_CONNECTED)//WiFi has succesfully Connected
  {
    Serial.print("Successfully connected to ");
    Serial.println(ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }*/

  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/main.html", "text/html", false, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(readDHT().temp).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(readDHT().humd).c_str());
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<512> jsonDoc;
    JsonArray humidityArray = jsonDoc.createNestedArray("humidity");
    JsonArray temperatureArray = jsonDoc.createNestedArray("temperature");
    for (int i = 0; i < maxDataPoints; i++) {
      humidityArray.add(humidityData[(dataIndex + i) % maxDataPoints]);
      temperatureArray.add(temperatureData[(dataIndex + i) % maxDataPoints]);
    }
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    request->send(200, "application/json", jsonString);
  });
  server.serveStatic("/", LittleFS, "/").setDefaultFile("main.html");

  server.begin();

}

void loop() {
  Data = readDHT();
  static unsigned long lastUpdateTime = 0;
  if (millis() - lastUpdateTime > 10000) {
    updateDataPoints();
    lastUpdateTime = millis();
  }
  Serial.print(String(Data.temp));
  Serial.print("");
  Serial.println(String(Data.humd));
  delay(500);
}