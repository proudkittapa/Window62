// Red ESP is a controller

#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Stepper.h>

#define STEPS 2048

Stepper stepperBind(STEPS, 26, 14, 27, 12);
Stepper stepperCur(STEPS, 25, 32, 33, 4);

// Condition Variable
String conditionStr;
String condition[5];
typedef struct condition_struct {
  String id;
  String type;
  String denote;
  float value;
  String state;
} condition_struct;
// Light / Dust / Humid / Temp | More / Less
condition_struct mainCondStruct[8];
bool trigArr[8] = {true, true, true, true, true, true, true, true};

// Pin Setup
const int OnOff_LED = 21;
const int Sent_LED = 22;
const int Delivery_LED = 23;
const int Win1_Pin = 19;
const int Win2_Pin = 18;

// Window and Curtain Status
String isOpenWindow = "close";
String isOpenCurtain = "close";
String isOpenBind = "close";

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
const char *WiFiSSID = "Macaron";
const char *WiFiPWD = "AteSomeSoup";

// MQTT Setup
const char *mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char *mqttUser = "RedESP";
const char *mqttPwd = "Window62";
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void updateCondition(condition_struct newCon, condition_struct *conArr) {
  int conIndex = 0;
  if (newCon.type == "Cancel") {
    for (int i = 0; i < 8; i++) {
      // Cancel existing condition
      if (conArr[i].id == newCon.id) {
        conArr[i] = newCon;
        return;
      }
    }
    return;
  }
  else if (newCon.type == "Light") {
    conIndex = 0;
  }
  else if (newCon.type == "Dust") {
    conIndex = 1;
  }
  else if (newCon.type == "Humidity") {
    conIndex = 2;
  }
  else if (newCon.type == "Temperature") {
    conIndex = 3;
  }

  if (newCon.denote == "more") {
    conIndex = conIndex * 2;
  } else if (newCon.denote == "less") {
    conIndex = conIndex * 2 + 1;
  }

  conArr[conIndex] = newCon;
  trigArr[conIndex] = false;
}

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

void callback(char* topic, byte* payload, unsigned int len) {
  Serial.println("Received a MQTT message");
  String topicS = String(topic);
  String rcMsg;
  Serial.println("Topic: " + topicS);
  for (int i=0;i < len;i++) {
    rcMsg += (char)payload[i];
  }
  Serial.println("Message: " + rcMsg);
  
  // Window Command Topic
  if (topicS == "WindowCmd") {
    digitalWrite(Win1_Pin, HIGH);
    digitalWrite(Win2_Pin, HIGH);
    if (rcMsg == "open") {
      isOpenWindow = "open";
    } else {
      isOpenWindow = "close";
    }
    Serial.print("From WindowCmd: ");
    Serial.println(isOpenWindow);
  }
  // Curtain Command Topic
  if (topicS == "CurtainCmd") {
    Serial.print("From CurtainCmd: ");
    Serial.println(rcMsg);
    if (rcMsg == "open" && isOpenCurtain == "close") {
      openCurtain();
      isOpenCurtain = "open";
    } else if (rcMsg == "close" && isOpenCurtain == "open") {
      closeCurtain();
      isOpenCurtain = "close";
    }
  }
  // Curtain Bind Command Topic'
  if (topicS == "CurtainBindCmd") {
    Serial.print("From CurtainBindCmd: ");
    Serial.println(rcMsg);
    if (rcMsg == "open") {
      openBind();
      isOpenBind = "open";
    } else if (rcMsg == "close") {
      closeBind();
      isOpenBind = "close";
    }
  }
  // User Conditions Topic
  if (topicS == "Conditions") {
    splitStr(rcMsg, condition);
    Serial.print("Array: [ ");
    for (int i = 0; i < 5; i++) {
      Serial.print(condition[i] + " ");
    }
    Serial.println("]");
    condition_struct tempStruct = {condition[0], condition[1], condition[2], condition[3].toInt(), condition[4]};
    updateCondition(tempStruct, mainCondStruct);
  }

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

bool CurtainBusy = false;

// Function for stepper actuator control
void openCurtain() {
  if (!CurtainBusy) {
    for(int s=0; s<(3*2048); s++)
          {
            if (s % 200 == 0) {
              mqttClient.loop();
            }
            stepperCur.step(1);
          }
          CurtainBusy = false;
  }
}

void closeCurtain() {
  if (!CurtainBusy) {
    for(int s=0; s<(3*2048); s++)
        {
          if (s % 200 == 0) {
            mqttClient.loop();
          }
          stepperCur.step(-1);
        }
        CurtainBusy = false;
  }

}

bool BindBusy = false;

void openBind() {
  //  val = 10*2048;
  if (!BindBusy) {
    for(int s=0; s<(10*2048); s++)
      {
        if (s % 200 == 0) {
            mqttClient.loop();
          }
        stepperBind.step(1);
      }
      BindBusy = false;
  }
}

void closeBind() {
  //  val = 10*2048;
  if (!BindBusy) {
    for(int s=0; s<(10*2048); s++)
        {
          if (s % 200 == 0) {
              mqttClient.loop();
            }
          stepperBind.step(-1);
        }
        BindBusy = false;
  }

}

void stopWindow() {
  digitalWrite(Win1_Pin, HIGH);
  digitalWrite(Win2_Pin, HIGH);
  delay(2000);
}

void splitStr(String s, String *splitArr) {
  int cur = 0;
  int index = 0;
  for (int i = 0; i < s.length(); i++) {
    if (s[i] == ',') {
      int next = i;
      splitArr[index] = s.substring(cur, next);
      index++;
      cur = next+1;
    }
  }
  splitArr[index] = s.substring(cur, s.length());
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

  // Init Motor
  stepperBind.setSpeed(15);
  stepperCur.setSpeed(5);

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
  mqttClient.subscribe("CurtainBindCmd");
  mqttClient.subscribe("Conditions");

  if (mqttClient.publish("Start", "hello")) {
    Serial.println("Sent the initial message!");
  } else {
    Serial.println("Failed to sent the initial message");
  }

  // Prepare the condition
  for (int i = 0; i < 8; i++) {
    condition_struct tempStruct = {"0", "None", "None", 0, "None"};
    mainCondStruct[i] = tempStruct;
  }
//  for (int i = 0; i < 8; i++) {
//    Serial.println(mainCondStruct[i].id);
//    Serial.println(mainCondStruct[i].type);
//    Serial.println(mainCondStruct[i].denote);
//    Serial.println(mainCondStruct[i].value);
//    Serial.println(mainCondStruct[i].state);
//  }
}

void loop()
{
  digitalWrite(OnOff_LED, HIGH);

//  for (int i = 0; i < 8; i++) {
//    Serial.print(mainCondStruct[i].id);
//    Serial.print(" ");
//    Serial.print(mainCondStruct[i].type);
//    Serial.print(" ");
//    Serial.print(mainCondStruct[i].denote);
//    Serial.print(" ");
//    Serial.println(mainCondStruct[i].value);
//    Serial.print(" ");
//    Serial.println(mainCondStruct[i].state);
//  }
  
  // Window and Curtain Status
  Serial.print("Window Status: ");
  Serial.println(isOpenWindow);

  Serial.print("Curtain Status: ");
  Serial.println(isOpenCurtain);

  Serial.print("Curtain Bind Status: ");
  Serial.println(isOpenBind);

  Serial.println("---------------");

  // MQTT Communication
  mqttClient.loop();
  delay(2000);

  // Condition
  // Light more than condition
  if (incomingSensorData.lightMsg > mainCondStruct[0].value && not trigArr[0]) {
    if (mainCondStruct[0].state == "open") {
      isOpenBind = "open";
      openBind();
    } else if (mainCondStruct[0].state == "close") {
      isOpenBind = "close";
      closeBind();
    }
    trigArr[0] = true;
  } else if (incomingSensorData.lightMsg < mainCondStruct[0].value) {
    trigArr[0] = false;
  }
  // Light less than condition
  if (incomingSensorData.lightMsg < mainCondStruct[1].value && not trigArr[1]) {
    if (mainCondStruct[1].state == "open") {
      isOpenBind = "open";
      openBind();
    } else if (mainCondStruct[1].state == "close") {
      isOpenBind = "close";
      closeBind();
    }
    trigArr[1] = true;
  } else if (incomingSensorData.lightMsg > mainCondStruct[1].value) {
    trigArr[1] = false;
  }
  // Dust more than condition
  if (incomingSensorData.dustMsg > mainCondStruct[2].value && not trigArr[2]) {
    isOpenWindow = mainCondStruct[2].state;
    trigArr[2] = true;
  } else if (incomingSensorData.dustMsg < mainCondStruct[2].value) {
    trigArr[2] = false;
  }
  // Dust less than condition
  if (incomingSensorData.dustMsg < mainCondStruct[3].value && not trigArr[3]) {
    isOpenWindow = mainCondStruct[3].state;
    trigArr[3] = true;
  } else if (incomingSensorData.dustMsg > mainCondStruct[3].value) {
    trigArr[3] = false;
  }
  // Humid more than condition
  if (incomingSensorData.humidMsg > mainCondStruct[4].value && not trigArr[4]) {
    isOpenWindow = mainCondStruct[4].state;
    trigArr[4] = true;
  } else if (incomingSensorData.humidMsg < mainCondStruct[4].value) {
    trigArr[4] = false;
  }
  // Humid less than condition
  if (incomingSensorData.humidMsg < mainCondStruct[5].value && not trigArr[5]) {
    isOpenWindow = mainCondStruct[5].state;
    trigArr[5] = true;
  } else if (incomingSensorData.humidMsg > mainCondStruct[5].value) {
    trigArr[5] = false;
  }
  // Temperature more than condition
  if (incomingSensorData.tempCMsg > mainCondStruct[6].value && not trigArr[6]) {
    isOpenWindow = mainCondStruct[6].state;
    trigArr[6] = true;
  } else if (incomingSensorData.tempCMsg < mainCondStruct[6].value) {
    trigArr[6] = false;
  }
  // Temperature less than condition
  if (incomingSensorData.tempCMsg < mainCondStruct[7].value && not trigArr[7]) {
    if (mainCondStruct[7].state == "open") {
      isOpenCurtain = "open";
      openCurtain();
    } else if (mainCondStruct[7].state == "close") {
      isOpenCurtain = "close";
      closeCurtain();
    }
    trigArr[7] = true;
  } else if (incomingSensorData.humidMsg > mainCondStruct[7].value) {
    trigArr[7] = false;
  }

  // Open the window if status is open
  if (isOpenWindow == "open") {
//    Serial.println("Opening Window");
    openWindow();
  } else {
//    Serial.println("Closing Window");
    closeWindow();
  }
  
  // Reset the Test LED
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Sent_LED, LOW);
  delay(1000);
}