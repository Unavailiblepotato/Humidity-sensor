#include <esp_now.h>
#include <WiFi.h>
#include "Queue.h"

typedef struct board{
  int board;
  uint8_t macs[6];
}board;


board node;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    int boardNum;
    float humd;
    float temp;

} struct_message;
// Create a struct_message called Data
struct_message Data;

Queue<board> nodeQ(20);

int getBoardNum(const uint8_t *mac){
  
  int finNum = 0;

  for(int i = nodeQ.count(); i > 0; i--)
  {
    board t = nodeQ.peek();
    nodeQ.pop();
    if (memcmp(t.macs, mac, 6) == 0) {  // Compare MAC addresses
      finNum = t.board;
    }
    nodeQ.push(t);
  }
  if(finNum == 0){
    node.board = 100 + (nodeQ.count()+1);
    memcpy(node.macs, mac, 6);
    nodeQ.push(node);
    return(node.board);
  }

  return finNum;

}


// callback function that will be executed when Data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&Data, incomingData, sizeof(Data));

  
  Data.boardNum = getBoardNum(mac);
  Serial2.print("n:");Serial2.print(Data.boardNum);Serial2.print("h:");Serial2.print(Data.humd);Serial2.print("t:");Serial2.print(Data.temp);
  
/*
  if(Data.board == 1){
      Serial.print("temp1:");Serial.print(((Data.temp * 9) + 3) / 5 + 32);Serial.print(",");
      Serial.print("humd1:");Serial.print(Data.humd);Serial.print("\n");\
      
    }
  if(Data.board == 2){
      Serial.print("temp2:");Serial.print(((Data.temp * 9) + 3) / 5 + 32);Serial.print(",");
      Serial.print("humd2:");Serial.print(Data.humd);Serial.print("\n");
    }
    */
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Using GPIO16 as RX and GPIO17 as TX
  Serial2.print("model, humidity, temp");
}
 
void loop() {


}