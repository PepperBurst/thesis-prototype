
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "DHTesp.h"
#include <ArduinoJson.h>

#define dhtPin D1
#define pirPin D2
#define outPin D3
#define myPeriodic 5

//Read APIKEY for Channel 1 = EX1DVX903H75LZCS
//Write APIKEY for Channel 1 = 459JH64HVXZMSZQJ
//TalkBack ID for Channel 1 = 32023;
//TalkBack APIKEY for Channel 1 = WGTNUEUDPAYLS3PN
//Read APIKEY for Channel 2 = SR26KL05EWR7E2YZ
//Write APIKEY for Channel 2 = N4CZSPBXEJF4U16D
//Read APIKEY for Channel 3 = 0YG3YFK75U80P940
//Write APIKEY for Channel 3 = XOIG3YFOHR4OK8MW
//Read APIKEY for Channel 4 = KDIUJMWUGI9QPHPJ
//Write APIKEY for Channel 4 = S2E20R36YWWDAMMC


const char* server = "api.thingspeak.com";
String writeApiKey = "XOIG3YFOHR4OK8MW"; // set Write API Key
String readApiKey = "0YG3YFK75U80P940";
int sent = 0;
bool delayForMist = false;
int countDelay = 0;
String fieldHeatIndex = "&field1=";
String fieldTemperature = "&field2=";
String fieldRelativeHumidity = "&field3=";
String fieldHeatIndexTreshold = "&field=4";
String moduleName = "Module 3";     //set Module #
const unsigned long postingInterval = 50000;
long lastUpdateTime = 0;
float heatIndex = 0;
float temperature = 0;
float relativeHumidity = 0;
float heatIndexTreshold = 0;
float relativeHumidityTreshold = 0;


DHTesp dht;
ESP8266WiFiMulti wifiMulti;

void setup() {
  Serial.begin(115200);
  Serial.println();
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);
  dht.setup(dhtPin, DHTesp::DHT11); // Connect DHT sensor to GPIO 5
  pinMode(pirPin, INPUT);
  pinMode(outPin, OUTPUT);
  digitalWrite(outPin, 1);
  connectWifi();
  Serial.println("Swarm of Automatic Misting Systems with Android Application");
  Serial.println(moduleName);
  Serial.print("Posting Interval:\t");
  Serial.println(postingInterval);
  getTresholds();
}

void loop() {
  while(wifiMulti.run() == WL_CONNECTED){
    unsigned long currentMillis = millis();
    Serial.print("Current Time:\t");
    Serial.println(currentMillis);
    Serial.print("Last Update Time:\t");
    Serial.println(lastUpdateTime);
    if (currentMillis - lastUpdateTime >=  postingInterval) {
      lastUpdateTime = currentMillis;
      getReadings();
      uploadReadings(heatIndex, temperature, relativeHumidity);
      getTresholds();
    }
    delay(1000);
//    getReadings();
//    if(countDelay > 1){
//      countDelay = 0;
//      delayForMist = false;
//    }
//    if(!delayForMist){
//      if(hi>32 && rh<90){
//        Serial.println("Heat index is above threshold, Relative Humidity is in valid range");
//        if(getMotion()){
//          Serial.println("Motion detected, releasing mist...");
//  //        digitalWrite(outPin, 0);
//          releaseMist();
//          delayForMist = true;
//        }else{
//          Serial.println("No motion detected");
//        }
//      }
//    }else{
//      Serial.println("Mist recently released, delaying mist");
//      countDelay++;
//    }
//    delay(5000);
  }
  connectWifi();
}

void getReadings(){
  relativeHumidity = dht.getHumidity();
  temperature = dht.getTemperature();
  while(dht.getStatusString()!="OK"){
    relativeHumidity = dht.getHumidity();
    temperature = dht.getTemperature();
  }
  heatIndex = dht.computeHeatIndex(temperature, relativeHumidity, false);
}

bool getMotion(){
  Serial.println("Detecting motion...");  
  bool motTrue = false;
  int motDet = 0;
  int falseCount = 0;
  int trueCount = 0;
  Serial.print("Detect count:\t");
  while((trueCount + falseCount) <= 300){
    motDet = digitalRead(pirPin);
    if(motDet==1){
      trueCount++;
      Serial.print("O");
    }else{
      falseCount++;
      Serial.print("X");
    }
    delay(100);
  }
  Serial.println();
  motTrue = trueCount > 50;
  return motTrue;
}

void getTresholds(){
  getHITreshold();
  getRHTreshold();
}

void getHITreshold(){
  WiFiClient client;
  String url = "/channels/743475/fields/4/last.json?api_key=";
  url += readApiKey;
  if(client.connect(server, 80)){
    client.print(String("GET "));
    client.print(url + " HTTP/1.1\r\n");
    client.print("HOST: api.thingspeak.com\r\n");
    client.print("Connection: close\r\n\r\n");
    delay(1000);
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      return;
    }
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
    }
    client.find("\r\n");
    Serial.println("Acquiring Treshold Value...");
    String all_data;
    while(client.available()){
      all_data += client.readStringUntil('}');
    }
    all_data.substring('{', '}');
    all_data.trim();
    all_data.remove(all_data.length()-2, 2);
    all_data += "}";
    Serial.println(all_data);
    const char* json = all_data.c_str();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    heatIndexTreshold = doc["field4"].as<float>();
    Serial.print("Heat Index Treshold:\t");
    Serial.println(heatIndexTreshold, 2);
  }
}

void getRHTreshold(){
  WiFiClient client;
  String url = "/channels/743475/fields/5/last.json?api_key=";
  url += readApiKey;
  if(client.connect(server, 80)){
    client.print(String("GET "));
    client.print(url + " HTTP/1.1\r\n");
    client.print("HOST: api.thingspeak.com\r\n");
    client.print("Connection: close\r\n\r\n");
    delay(1000);
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      return;
    }
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
    }
    client.find("\r\n");
    Serial.println("Acquiring Treshold Value...");
    String all_data;
    while(client.available()){
      all_data += client.readStringUntil('}');
    }
    all_data.substring('{', '}');
    all_data.trim();
    all_data.remove(all_data.length()-2, 2);
    all_data += "}";
    Serial.println(all_data);
    const char* json = all_data.c_str();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    relativeHumidityTreshold = doc["field5"].as<float>();
    Serial.print("Relative Humidity Treshold:\t");
    Serial.println(relativeHumidityTreshold, 2);
  }
}

void connectWifi(){
  Serial.print("Connecting...");
  wifiMulti.addAP("PASCUAPARASAMASA", "lalalalala");
  wifiMulti.addAP("PLDTHOMEDSL17A6F", "PLDTWIFI17A67");
  wifiMulti.addAP("PLDT2GRamos", "2243taradale");
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(1000);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void uploadReadings(float hi, float temp, float rh){
  sendHeatIndex(hi);
  sendTemp(temp);
  sendRH(rh);
}

void sendTemp(float temp){
  WiFiClient client;
  Serial.print("Temperature:\t");
  Serial.println(temp, 2);
  Serial.println("Uploading temperature reading...");
  if(client.connect(server, 80)){
    Serial.println("Wifi Client connected");
    String postStr = writeApiKey;
    postStr += fieldTemperature;
    postStr += String(temp);
//    postStr += "\r\n\r\n";
    postStr += "";
    client.print("POST /update HTTP/1.1\n");
    client.print("HOST: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeApiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(1000);
    Serial.println("Temperature uploaded");
    Serial.print("Wait 5 seconds for next upload");
    int count = myPeriodic;
    while(count--){
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
  }else{
    Serial.println("Upload failed");
  }
  sent++;
  client.stop();
}

void sendRH(float rh){
  WiFiClient client;
  Serial.print("Relative Humidity:\t");
  Serial.println(rh, 2);
  Serial.println("Uploading Relative Humidity...");
  if(client.connect(server, 80)){
    Serial.println("Wifi Client connected");
    String postStr = writeApiKey;
    postStr += fieldRelativeHumidity;
    postStr += String(rh);
//    postStr += "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("HOST: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeApiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(1000);
    Serial.println("Relative Humidity uploaded");
    Serial.print("Wait 5 seconds for next upload");
    int count = myPeriodic;
    while(count--){
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
  }else{
    Serial.println("Upload failed");
  }
  sent++;
  client.stop();
}

void sendHeatIndex(float heatInd){
  WiFiClient client;
  Serial.print("Heat Index:\t");
  Serial.println(heatInd, 2);
  Serial.println("Uploading Heat index...");
  if(client.connect(server, 80)){
    Serial.println("Wifi Client connected");
    String postStr = writeApiKey;
    postStr += fieldHeatIndex;
    postStr += String(heatInd);
//    postStr += "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("HOST: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeApiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(1000);
    Serial.println("Heat index uploaded");
    Serial.print("Wait 5 seconds for next upload");
    int count = myPeriodic;
    while(count--){
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
  }else{
    Serial.println("Upload failed");
  }
  sent++;
  client.stop();
}

void releaseMist(){
  Serial.println("Starting mist release sequence...");
  int endCount = 0;
  while(endCount < 3){
    digitalWrite(outPin, 0);
    delay(5000);
    digitalWrite(outPin, 1);
    delay(10000);
    endCount++;
  }
  Serial.println("Mist release sequence has ended");
}
