// Red ESP is a receiver

#include "esp_now.h"
#include "WiFi.h"

// Pin Setup
int OnOff_LED = 21;
int Delivery_LED = 23;

// Sensor Variable
int incomingReadings;
int incomingLightValue;

// Communication Setup
uint8_t selfAddress[] = {0x58, 0xBF, 0x25, 0x82, 0x7C, 0x88};
uint8_t broadcastAddress[] = {0x84, 0xCC, 0xA8, 0x6B, 0xE3, 0x48};
esp_now_peer_info_t peerInfo;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.print("\n==========\nLast Packet Sent Status:\n");
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);

  incomingLightValue = incomingReadings;
  Serial.println(incomingLightValue);
  Serial.print("\n==========\n");

  digitalWrite(Delivery_LED, HIGH);
  delay(1000);
  digitalWrite(Delivery_LED, LOW);
}

void setup()
{
  pinMode(OnOff_LED, OUTPUT);
  pinMode(Delivery_LED, OUTPUT);
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Delivery_LED, LOW);
  // Init Serial Monitor
  Serial.begin(115200);

  // Set WiFi Station Mode
  WiFi.mode(WIFI_STA);

  // Init ESP_NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: Initializing ESP-NOW Failed...");
    return;
  }

  // Register Peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add peer");
    return;
  }

  // Callback function when receive a message
  esp_now_register_recv_cb(OnDataRecv);
  
}

void loop()
{
  digitalWrite(OnOff_LED, HIGH);
  delay(1000);
  digitalWrite(OnOff_LED, LOW);
  delay(1000);
}
