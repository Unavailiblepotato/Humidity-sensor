#include <esp_now.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <Adafruit_AHTX0.h>
#include <esp_wifi.h>

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xd0, 0xef, 0x76, 0x13, 0xc1, 0x9c};
DHTesp dht;
// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
uint8_t macaddrs[6];
float a;
float b;
};
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


Adafruit_AHTX0 aht;
 
void setup() {

  Serial.begin(115200);


  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 or AHT20 found");
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
      readMacAddress();
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;


  // Set a shared LMK

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
void readMacAddress(){
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, myData.macaddrs);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  myData.macaddrs[0], myData.macaddrs[1], myData.macaddrs[2],
                  myData.macaddrs[3], myData.macaddrs[4], myData.macaddrs[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  // Set values to send
  myData.b = temp.temperature;
  myData.a = humidity.relative_humidity;
  Serial.println(myData.a);
  // Send message via ESP-NOW

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  delay(1000);
}