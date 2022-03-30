// Blue ESP is a sender

#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Ethernet.h>

// Pin Setup
int OnOff_LED = 21;
int Sent_LED = 22;
int Delivery_LED = 23;

// Sensor Variable
int lightValue;
int tempValue;
//float ADC_VREF_mV = 3300;
//float ADC_RESOLUTION = 4096;

// Communication Setup
uint8_t selfAddress[] = {0x84, 0xCC, 0xA8, 0x6B, 0xE3, 0x48};
uint8_t broadcastAddress[] = {0x58, 0xBF, 0x25, 0x82, 0x7C, 0x88};
esp_now_peer_info_t peerInfo;

// Can only connect to 2.4G WiFi
const char *WiFiSSID = "Panya Home5";
const char *WiFiPWD = "home137731";

// MQTT Setup
char *mqttServer = "broker.emqx.io";
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void connectToWiFi() {
  Serial.print("Connecting to ");
  WiFi.begin(WiFiSSID, WiFiPWD);
  Serial.print(WiFiSSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnection Successful!");
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\n==========\nLast Packet Sent Status:\n");
  if (status == ESP_NOW_SEND_SUCCESS){
    Serial.println("Delivery Success");
    digitalWrite(Delivery_LED, HIGH);
  } else {
    Serial.println("Delivery Failed");
    digitalWrite(Delivery_LED, LOW);
  }
  Serial.print("\n==========\n");
}

void setup()
{
  // Set GPIO
  pinMode(OnOff_LED, OUTPUT);
  pinMode(Sent_LED, OUTPUT);
  pinMode(Delivery_LED, OUTPUT);
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Sent_LED, LOW);
  digitalWrite(Delivery_LED, LOW);
  
  // Init Serial Monitor
  Serial.begin(115200);

  // Set WiFi Station Mode
  connectToWiFi();
  WiFi.mode(WIFI_STA);

  // Init ESP_NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: Initializing ESP-NOW Failed...");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // Register Peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add peer");
    return;
  }

  // Init MQTT Client
  mqttClient.setServer(mqttServer, 1883);
  
  if (mqttClient.connect("clientID", "emqx", "public")) 
  {
    Serial.println("Connection to MQTT Completed...");
  } 
  else 
  {
    Serial.println("Connection to MQTT Failed...");
    return;
  }
}

void loop()
{  
  digitalWrite(OnOff_LED, HIGH);
  // Read Sensors
  lightValue = analogRead(A0);
  tempValue = analogRead(A3);
  char lightString[8];
  char tempString[8];
  dtostrf(lightValue, 1, 2, lightString);
  dtostrf(tempValue, 1, 2, tempString);
  
  // tempValue = (tempValue * (ADC_VREF_mV / ADC_RESOLUTION)) / 10;
  Serial.print("\n==========\n");
  Serial.print("Local light read: ");
  Serial.println(lightString);
  Serial.print("Local temp read: ");
  Serial.println(tempString);
  
  Serial.print("\n==========\n");

  // ESP Now Communication
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &lightValue, sizeof(lightValue));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("ERROR: failed sending the data...");
  }

  // MQTT Communication
  
  mqttClient.loop();
  if(mqttClient.publish("WindowLight", lightString) && mqttClient.publish("WindowTemp", tempString))
  {
    Serial.println("Successfully publish a message...");
    digitalWrite(Sent_LED, HIGH);
  }
  else
  {
    Serial.println("ERROR: failed to publish a message");
  }
  delay(2000);

  // Reset the Test LED
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Sent_LED, LOW);
  digitalWrite(Delivery_LED, LOW);
  delay(1000);
}
