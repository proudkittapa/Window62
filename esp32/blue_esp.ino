// Blue ESP is a sensor reader

#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTTYPE DHT11
#define DHTPIN 32

// Pin Setup
int Dust_Pin = 19;
int OnOff_LED = 21;
int Sent_LED = 22;
int Delivery_LED = 23;

// Average Sensor Setup
int sensorPointer = 0;
const int sensorSize = 10;

// Sensor Variable
float rawLightValue;
float rawDustValue;
float lightValueLog[sensorSize];
float tempValueLog[sensorSize];
float dustValueLog[sensorSize];
float humidValueLog[sensorSize];
float lightValue;
float dustValue;
float humidValue;
float tempValueC;
float avgLightValue;
float avgDustValue;
float avgHumidValue;
float avgTempValue;

// ESP Now message struct
typedef struct struct_message {
  float lightMsg;
  float dustMsg;
  float humidMsg;
  float tempCMsg;
} struct_message;

struct_message sensorData;

DHT dht(DHTPIN, DHTTYPE);

// Communication Setup
uint8_t selfAddress[] = {0x7C, 0x9E, 0xBD, 0x53, 0xE0, 0x90};
uint8_t broadcastAddress[] = {0x58, 0xBF, 0x25, 0x82, 0x7C, 0x88};
esp_now_peer_info_t peerInfo;

// Can only connect to 2.4G WiFi
const char *WiFiSSID = "Macaron";
const char *WiFiPWD = "AteSomeSoup";

// MQTT Setup
const char *mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char *mqttUser = "BlueESP";
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

float avg(float *ar, int len) {
    float sum = 0;
    int cnt = len;
    for (int i = 0; i < len; i++) {
        if (ar[i] < 0) {
            cnt -= 1;
        } else {
            sum += ar[i];
        }
    }
    
    if (cnt == 0) {
        return 0;
    } else {
        return sum / cnt;
    }
}

void resetValue(float *ar, int len) {
    for (int i = 0; i < len; i++) {
        ar[i] = -1;
    }
}

void printArr(float *ar, int len) {
  for (int i = 0; i < len; i++) {
    Serial.println(ar[i]);
  }
}

void setup()
{
  resetValue(lightValueLog, sensorSize);
  resetValue(dustValueLog, sensorSize);
  resetValue(tempValueLog, sensorSize);
  resetValue(humidValueLog, sensorSize);
  // Init Serial Monitor
  Serial.begin(115200);
  delay(2000);
  
  // Setup GPIO
  pinMode(OnOff_LED, OUTPUT);
  pinMode(Sent_LED, OUTPUT);
  pinMode(Delivery_LED, OUTPUT);
  pinMode(Dust_Pin, OUTPUT);
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Sent_LED, LOW);
  digitalWrite(Delivery_LED, LOW);
  dht.begin();

  // Setup WiFi
  connectToWiFi();
  WiFi.mode(WIFI_STA);

  // Init ESP_NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: Initializing ESP-NOW Failed...");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // Register Peer
  esp_now_peer_info_t peerInfo;
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add peer");
    return;
  }

  // Init MQTT Client
  mqttClient.setServer(mqttServer, mqttPort);
  while (!mqttClient.connected()) {
    if (mqttClient.connect("BlueESPID", mqttUser, mqttPwd)) 
    {
      Serial.println("Connection to MQTT Completed...");
    } 
    else 
    {
      Serial.println("Connection to MQTT Failed...");
    }
  }
}

void loop()
{
  digitalWrite(OnOff_LED, HIGH);
  
  // Read Sensors
  rawLightValue = analogRead(A0);
  humidValue  = dht.readHumidity();
  tempValueC = dht.readTemperature();
  rawDustValue = analogRead(A6);

  // Calculate the Analog Value
  lightValue = (rawLightValue / 4095.0) * 100;
  // lightValue = rawLightValue;
  digitalWrite(Dust_Pin, HIGH);
  dustValue = rawDustValue * (5 / 1024.0) * 0.17 - 0.1;
  if (dustValue < 0) {
    dustValue = 0;
  }
  digitalWrite(Dust_Pin, LOW);
  
  if (sensorPointer > sensorSize) {
    sensorPointer = 0;
  }
  lightValueLog[sensorPointer] = lightValue;
  dustValueLog[sensorPointer] = dustValue;
  humidValueLog[sensorPointer] = humidValue;
  tempValueLog[sensorPointer] = tempValueC;
  sensorPointer++;

  avgLightValue = avg(lightValueLog, sensorSize);
  avgDustValue = avg(dustValueLog, sensorSize);
  avgHumidValue = avg(humidValueLog, sensorSize);
  avgTempValue = avg(tempValueLog, sensorSize);
  
  // Prepare the value to be sent via ESP Now
  sensorData.lightMsg = avgLightValue;
  sensorData.dustMsg = avgDustValue;
  sensorData.humidMsg = avgHumidValue;
  sensorData.tempCMsg = avgTempValue;
  
  // Prepare the value to be sent via MQTT
  char lightString[8];
  char tempStringC[8];
  char humidString[8];
  char dustString[8];

  // Convert the value from sensors
  dtostrf(avgLightValue, 1, 2, lightString);
  dtostrf(avgTempValue, 1, 2, tempStringC);
  dtostrf(avgHumidValue, 1, 2, humidString);
  dtostrf(avgDustValue, 1, 2, dustString);
  
  Serial.print("\n==========\n");
  Serial.print("Local light read: ");
  Serial.println(lightString);
  Serial.print("Local dust read: ");
  Serial.println(dustString);
  Serial.print("Local humidity read: ");
  Serial.print(humidString);
  Serial.println("%");
  Serial.print("Local temperature: ");
  Serial.print(tempValueC);
  Serial.print("°C");
  Serial.print("\n==========\n");

  Serial.print("\n==========\n");
  Serial.print("Avg Local light read: ");
  Serial.println(avgLightValue);
  Serial.print("Avg Local dust read: ");
  Serial.println(avgDustValue);
  Serial.print("Avg Local humidity read: ");
  Serial.print(avgHumidValue);
  Serial.println("%");
  Serial.print("Avg Local temperature: ");
  Serial.print(avgTempValue);
  Serial.print("°C");
  Serial.print("\n==========\n");

  // ESP Now Communication
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sensorData, sizeof(sensorData));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("ERROR: failed sending the data...");
  }

  // MQTT Communication
  mqttClient.loop();
  mqttClient.publish("WindowLight", lightString);
  mqttClient.publish("WindowTemp", tempStringC);
  mqttClient.publish("WindowPM25", dustString);
  mqttClient.publish("WindowHumidity", humidString);

  delay(1000);

  // Reset the Test LED
  digitalWrite(OnOff_LED, LOW);
  digitalWrite(Sent_LED, LOW);
  digitalWrite(Delivery_LED, LOW);
  delay(1000);
}