#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xd0, 0xef, 0x76, 0x13, 0xc1, 0x9c};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    uint8_t macaddrs[6];
    float humidity;
    float temperature;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
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

    // Register for Send CB to get the status of Transmitted packet
    esp_now_register_send_cb(OnDataSent);

    // Register peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    // Initialize random seed
    randomSeed(analogRead(0));
}

void readMacAddress() {
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, myData.macaddrs);
    if (ret == ESP_OK) {
        Serial.printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                      myData.macaddrs[0], myData.macaddrs[1], myData.macaddrs[2],
                      myData.macaddrs[3], myData.macaddrs[4], myData.macaddrs[5]);
    } else {
        Serial.println("Failed to read MAC address");
    }
}

void loop() {
    // Generate random humidity between 33.00 and 37.00
    myData.humidity = random(3400, 3421) / 100.0;

    // Generate random temperature between 24.00 and 24.50
    myData.temperature = random(2400, 2411) / 100.0;

    Serial.printf("Humidity: %.2f%%, Temperature: %.2f°C\n", myData.humidity, myData.temperature);

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
        Serial.println("Sent with success");
    } else {
        Serial.println("Error sending the data");
    }

    delay(1000);
}
