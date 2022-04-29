// Red ESP is a controller

#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Pin Setup
const int OnOff_LED = 21;
const int Sent_LED = 22;
const int Delivery_LED = 23;
const int Win1_Pin = 19;
const int Win2_Pin = 18;

// Window and Curtain Status
char *isOpenWindow = "close";
char *isOpenCurtain = "close";

// ESP Now message struct
typedef struct struct_message {
  float lightMsg;
  float dustMsg;
  float humidMsg;
  float tempCMsg;
} struct_message;

struct_message incomingSensorData;

// Communication Setup
uint8_t selfAddress[] = {0x58, 0xBF, 0x25, 0x82, 0x7C, 0x88};
uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0x53, 0xE0, 0x90};
esp_now_peer_info_t peerInfo;
int status = WL_IDLE_STATUS;

// Can only connect to 2.4G WiFi
const char *WiFiSSID = "PAMDESKTOP";
const char *WiFiPWD = "pampampam";

// MQTT Setup
const char *mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char *mqttUser = "RedESP";
const char *mqttPwd = "Window62";
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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Received a MQTT message");
  String topicS = String(topic);
  char rcMsg;
  rcMsg = (char)payload[0];
  
  if (topicS == "WindowCmd") {
    digitalWrite(Win1_Pin, HIGH);
    digitalWrite(Win2_Pin, HIGH);
    if (rcMsg == 'o') {
      isOpenWindow = "open";
//      if (mqttClient.publish("ConfirmWindowCmd", "open")) {
//        Serial.println("Sent the confirmation message!");
//      } else {
//        Serial.println("Failed the confirmation message!");
//      }
    } else {
      isOpenWindow = "close";
//      if (mqttClient.publish("ConfirmWindowCmd", "close")) {
//        Serial.println("Sent the confirmation message!");
//      } else {
//        Serial.println("Failed the confirmation message!");
//      }
    }
    Serial.print("From WindowCmd: ");
    Serial.println(isOpenWindow);
  }

  if (topicS == "CurtainCmd") {
    
    if (rcMsg == 'o') {
      isOpenCurtain = "open";
//      if (mqttClient.publish("ConfirmCurtainCmd", "open")) {
//        Serial.println("Sent the confirmation message!");
//      } else {
//        Serial.println("Failed the confirmation message!");
//      }
    } else {
      isOpenCurtain = "close";
//      if (mqttClient.publish("ConfirmCurtainCmd", "close")) {
//        Serial.println("Sent the confirmation message!");
//      } else {
//        Serial.println("Failed the confirmation message!");
//      }
    }
    Serial.print("From CurtainCmd: ");
    Serial.println(isOpenCurtain);
  }
  stopWindow();

  Serial.println("-----------------------");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.print("\n==========\nLast Packet Sent Status:\n");
  
  memcpy(&incomingSensorData, incomingData, len);
  Serial.print("Bytes received: ");
  Serial.println(len);
  // Received Sensor Value
  Serial.print("\n==========\n");

  Serial.print("Received light read: ");
  Serial.println(incomingSensorData.lightMsg);
  Serial.print("Received dust read: ");
  Serial.println(incomingSensorData.dustMsg);
  Serial.print("Received humidity read: ");
  Serial.print(incomingSensorData.humidMsg);
  Serial.println("%");
  Serial.print("Received temperature: ");
  Serial.print(incomingSensorData.tempCMsg);
  Serial.println("°C");
  Serial.print("\n==========\n");

  digitalWrite(Delivery_LED, HIGH);
  delay(1000);
  digitalWrite(Delivery_LED, LOW);
}

// Function for linear actuator control
void openWindow() {
  digitalWrite(Win1_Pin, HIGH);
  digitalWrite(Win2_Pin, LOW);
}

void closeWindow() {
  digitalWrite(Win1_Pin, LOW);
  digitalWrite(Win2_Pin, HIGH);
}

void stopWindow() {
  digitalWrite(Win1_Pin, HIGH);
  digitalWrite(Win2_Pin, HIGH);
  delay(2000);
}

void setup()
{
  // Setup GPIO
  pinMode(OnOff_LED, OUTPUT);
  pinMode(Sent_LED, OUTPUT);
  pinMode(Delivery_LED, OUTPUT);
  pinMode(Win1_Pin, OUTPUT);
  pinMode(Win2_Pin, OUTPUT);
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Sent_LED, LOW);
  digitalWrite(Delivery_LED, LOW);
  
  // Init Serial Monitor
  Serial.begin(115200);

  // Setup WiFi
  connectToWiFi();
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

  // Init MQTT Client
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
  while (!mqttClient.connected()) {
    if (mqttClient.connect("RedESPID", mqttUser, mqttPwd)) 
    {
      Serial.println("Connection to MQTT Completed...");
    } 
    else 
    {
      Serial.println("Connection to MQTT Failed...");
    }
  }

  mqttClient.subscribe("WindowCmd");
  mqttClient.subscribe("CurtainCmd");
  mqttClient.subscribe("Conditions");

  if (mqttClient.publish("Start", "hello")) {
    Serial.println("Sent the initial message!");
  }
}

void loop()
{
  digitalWrite(OnOff_LED, HIGH);
  delay(1000);

  // Window and Curtain Status
  Serial.print("Window Status: ");
  Serial.println(isOpenWindow);

  Serial.print("Curtain Status: ");
  Serial.println(isOpenCurtain);

  Serial.println("---------------");

  // MQTT Communication
  mqttClient.loop();
  delay(2000);

  // Open the window if status is open
  if (isOpenWindow == "open") {
    Serial.println("Opening Window");
    openWindow();
  } else {
    Serial.println("Closing Window");
    closeWindow();
  }
  
  // Reset the Test LED
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Sent_LED, LOW);
  delay(1000);

}
