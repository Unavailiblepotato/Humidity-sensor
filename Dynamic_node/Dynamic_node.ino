#include <esp_now.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <Adafruit_AHTX0.h>
#include <esp_wifi.h>


// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    uint8_t macaddrs[6];
    float a;
    float b;
} struct_message;

struct_message myData;
Adafruit_AHTX0 aht;
esp_now_peer_info_t peerInfo;

#define MAX_PEERS 1
esp_now_peer_info_t peers[MAX_PEERS];
int numPeers = 0;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  int i = 1;
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
void setup() {
    Serial.begin(115200);

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    readMacAddress();

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 or AHT20 found");
    esp_now_register_send_cb(OnDataSent);

    // Scan for peers
    int8_t numNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numNetworks && numPeers < MAX_PEERS; i++) {
        // Add peer information dynamically
        if (WiFi.BSSID(i) != NULL) {
            memcpy(peers[numPeers].peer_addr, WiFi.BSSID(i), 6);
            peers[numPeers].channel = 0;  
            peers[numPeers].encrypt = false;
            if (esp_now_add_peer(&peers[numPeers]) == ESP_OK) {
                numPeers++;
            }
        }
    }
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  // Set values to send
  myData.b = temp.temperature;
  myData.a = humidity.relative_humidity;
  Serial.println(myData.a);

    for (int i = 0; i < numPeers; i++) {
        esp_now_send(peers[i].peer_addr, (uint8_t *)&myData, sizeof(myData));
    }

    delay(2000); // Adjust the delay as needed
}