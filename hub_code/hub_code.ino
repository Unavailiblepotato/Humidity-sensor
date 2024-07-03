#include <esp_now.h>
#include <WiFi.h>


// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    int board;
    float humd;
    float temp;

} struct_message;
// Create a struct_message called Data
struct_message Data;



// callback function that will be executed when Data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&Data, incomingData, sizeof(Data));
  Serial2.print("n:");Serial2.print(Data.board);Serial2.print("h:");Serial2.print(Data.humd);Serial2.print("t:");Serial2.print(Data.temp);

  if(Data.board == 1){
      Serial.print("temp1:");Serial.print(((Data.temp * 9) + 3) / 5 + 32);Serial.print(",");
      Serial.print("humd1:");Serial.print(Data.humd);Serial.print("\n");\
      
    }
  if(Data.board == 2){
      Serial.print("temp2:");Serial.print(((Data.temp * 9) + 3) / 5 + 32);Serial.print(",");
      Serial.print("humd2:");Serial.print(Data.humd);Serial.print("\n");
    }
    
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