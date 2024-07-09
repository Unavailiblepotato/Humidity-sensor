#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>

typedef struct board {
  int boardNum;
  uint8_t macs[6];
  bool init = false;
} board;

const int maxBoards = 20;
board node;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  uint8_t macaddrs[6];
  float humd;
  float temp;
} struct_message;

// Create a struct_message called Data
struct_message Data;

typedef struct BoardData {
  float humd;
  int boardNum;
  float temp;
  bool init;
} BoardData;

typedef struct sendboards {
  BoardData Datas[20];
} sendboards;

sendboards finData;
board boards[maxBoards];

int getBoardNum(const uint8_t *mac) {
  // Check if the first board is initialized
  if (!boards[0].init) {
    memcpy(boards[0].macs, mac, 6);
    boards[0].boardNum = 101;
    boards[0].init = true;
    return 101;
  }

  // Iterate through the boards array
  for (int i = 0; i < maxBoards; i++) {
    if (boards[i].init) {
      // Compare MAC addresses
      if (memcmp(boards[i].macs, mac, 6) == 0) {
        Serial.print("Existing board number: ");
        Serial.println(boards[i].boardNum);
        return boards[i].boardNum;
      }
    } else {
      // Initialize a new board
      memcpy(boards[i].macs, mac, 6);
      boards[i].boardNum = (i == 0) ? 101 : boards[i-1].boardNum + 1;
      boards[i].init = true;
      Serial.print("Creating new board number: ");
      Serial.println(boards[i].boardNum);
      return boards[i].boardNum;
    }
  }

  // If we reach this point, it means we have run out of space in the boards array
  Serial.println("No available slots for new boards");
  return -1;  // Indicate an error
}

// callback function that will be executed when Data is received
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(struct_message)) {
    memcpy(&Data, incomingData, sizeof(Data));
    int bn = (getBoardNum(Data.macaddrs) - 100);
    if (bn >= 0 && bn < 20) {
      finData.Datas[bn].humd = Data.humd;
      finData.Datas[bn].temp = Data.temp;
      finData.Datas[bn].boardNum = bn + 101;
      finData.Datas[bn].init = true;
      Serial.print("Received data from board: ");
      Serial.print(finData.Datas[bn].boardNum);
      Serial.print(" Humidity: ");
      Serial.print(finData.Datas[bn].humd);
      Serial.print(" Temperature: ");
      Serial.println(finData.Datas[bn].temp);
    }
  }
}

void sendData_i2c() {
  Wire.beginTransmission(9);  // Address of the I2C slave
  Wire.write((byte*)&finData, sizeof(finData));
  Wire.endTransmission();
  Serial.println("Data sent over I2C");
}

unsigned long prevUpdateTime = 0;
unsigned long updateInterval = 10000;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Wire.begin();
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Register callback function for receiving data
  esp_now_register_recv_cb(OnDataRecv);

  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Using GPIO16 as RX and GPIO17 as TX
  Serial2.print("model, humidity, temp");
}

void loop() {
  if (millis() - prevUpdateTime >= updateInterval) {
    prevUpdateTime = millis();
    sendData_i2c();
  }
}