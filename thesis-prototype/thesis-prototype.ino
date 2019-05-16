
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "DHTesp.h"
#include <ArduinoJson.h>

#define dhtPin D1
#define pirPin D2
#define solPin D3
#define pmpPin D4
#define comPin D5
#define myPeriodic 5

String readApiKey = "EX1DVX903H75LZCS";   //Channel 1;
String writeApiKey = "459JH64HVXZMSZQJ";  //Channel 1;
String tbID = "32023";                    //Channel 1;
String tbApiKey = "WGTNUEUDPAYLS3PN";     //Channel 1;
String moduleName = "Module 1";           //Channel 1;
int moduleNumber = 1;                     //Channel 1;
//String readApiKey = "SR26KL05EWR7E2YZ";   //Channel 2;
//String writeApiKey = "N4CZSPBXEJF4U16D";  //Channel 2;
//String tbID = "32823";                    //Channel 2;
//String tbApiKey = "KGUQTLB8771TD7CV";     //Channel 2;
//String moduleName = "Module 2";           //Channel 2;
//int moduleNumber = 2;                     //Channel 2;
//String readApiKey = "0YG3YFK75U80P940";   //Channel 3;
//String writeApiKey = "XOIG3YFOHR4OK8MW";  //Channel 3;
//String tbID = "32824";                    //Channel 3;
//String tbApiKey = "S11XC4U836TQ0261";     //Channel 3;
//String moduleName = "Module 3";           //Channel 3;
//int moduleNumber = 3;                     //Channel 3;
//String readApiKey = "KDIUJMWUGI9QPHPJ";   //Channel 4;
//String writeApiKey = "S2E20R36YWWDAMMC";  //Channel 4;
//String tbID = "32825";                    //Channel 4;
//String tbApiKey = "T7X7SYQN5K6Z9KK5";     //Channel 4;
//String moduleName = "Module 4";           //Channel 4;
//int moduleNumber = 4;                     //Channel 4;
const char* server = "api.thingspeak.com";
int sent = 0;
bool delayForMist = false;
int countDelay = 0;
String fieldHeatIndex = "&field1=";
String fieldTemperature = "&field2=";
String fieldRelativeHumidity = "&field3=";
String fieldHeatIndexTreshold = "&field4=";
String fieldRelativeHumidityTreshold = "&field5=";
String fieldSwarmSignal = "&field6=";
String fieldMotion = "&field7=";
const unsigned long postingInterval = 200000;
long lastUpdateTime = 0;
float heatIndex = 0;
float temperature = 0;
float relativeHumidity = 0;
float heatIndexTreshold = 0;
float relativeHumidityTreshold = 0;
int defaultMistDuration = 5000;
int lastMainReleaseCount = 10;
int mainReleaseMax = 10;
int dhtReadLoop = 5;
int maxReadDHT = 5;
int motionReadLoop = 10;
int maxReadPIR = 10;
bool recordedMotion = true;
String motionUpload = "FALSE";

DHTesp dht;
ESP8266WiFiMulti wifiMulti;

void setup() {
  Serial.begin(115200);
  Serial.println();
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);
  dht.setup(dhtPin, DHTesp::DHT11); // Connect DHT sensor to GPIO 5
  pinMode(pirPin, INPUT);
  pinMode(comPin, OUTPUT);
  pinMode(solPin, OUTPUT);
  pinMode(pmpPin, OUTPUT);
  digitalWrite(comPin, 1);
  digitalWrite(solPin, 1);
  digitalWrite(pmpPin, 1);
  connectWifi();
  Serial.println("Swarm of Automatic Misting Systems with Android Application");
  Serial.println(moduleName);
//  Serial.print("Posting Interval:\t");
//  Serial.println(postingInterval);
  getTresholds(); 
  getTalkbackCommand();
  doPOST();
}

void loop() {
  while(wifiMulti.run() == WL_CONNECTED){
//    DHT Reading loop
    if(dhtReadLoop >= maxReadDHT){
      Serial.println("Acquiring Readings...");
      getReadings();
      uploadReadings(heatIndex, temperature, relativeHumidity, "SWARM_OFF", motionUpload);
      dhtReadLoop = 0;
    }
    dhtReadLoop++;
//    PIR Reading Loop
//    If PIR has read motion in the past timeframe
//      Motion is recorded as true
    if(motionReadLoop >= maxReadPIR || !recordedMotion){
      recordedMotion = getMotion();
      motionReadLoop = 0;
    }
    motionReadLoop++;
//    Activation within thresholds
//    Will activate after a certain amount after
//    Releasing Mist
    if(heatIndex >= heatIndexTreshold && relativeHumidity <= relativeHumidityTreshold && lastMainReleaseCount >= mainReleaseMax){
      Serial.println("Checking for recorded motion...");
      if(recordedMotion){
        Serial.println("Recorded motion detected");
        releaseSequence(defaultMistDuration, true);
        sendSwarmSignal(moduleNumber);
        lastMainReleaseCount = 0;
      }else{
        Serial.println("No Recorded motion detected");
      }
    }
//    Update Thresholds and Commands
    getTresholds();
    getTalkbackCommand();
    lastMainReleaseCount++;
    delay(1000);
    Serial.print("DHT Loop Count:\t");
    Serial.println(dhtReadLoop);
    Serial.print("PIR Loop Count:\t");
    Serial.println(motionReadLoop);
    Serial.print("Mist Loop Count:\t");
    Serial.println(lastMainReleaseCount);
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
  while((trueCount + falseCount) <= 40){
    motDet = digitalRead(pirPin);
    if(motDet==1){
      trueCount++;
      Serial.print("O");
    }else{
      falseCount++;
      Serial.print("X");
    }
    delay(500);
  }
  Serial.println();
  motTrue = trueCount > 10;
  if(motTrue){
    Serial.println("Motion Detected");
  }else{
    Serial.println("No Motion Detected");
  }
  if(motTrue){
    motionUpload = "TRUE";
  }else{
    motionUpload = "FALSE";
  }
   
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
  client.stop();
}

void getTalkbackCommand(){
  WiFiClient client;
  String url = "/talkbacks/";
  url += tbID;
  url += "/commands/execute?api_key=";
  url += tbApiKey;
  if(client.connect(server, 80)){
    client.print(String("POST "));
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
    Serial.println("Getting talkback commands...");
    String all_data;
    while(client.available()){
      all_data += client.readStringUntil('\n');
    }
//    Serial.println(all_data);
    if(all_data=="MIST_RELEASE_1"){
      releaseMist(1);
    }else if(all_data=="MIST_RELEASE_2"){
      releaseMist(2);
    }else if(all_data=="MIST_RELEASE_3"){
      releaseMist(3);
    }else if(all_data=="MIST_RELEASE_4"){
      releaseMist(4);
    }else{
      Serial.println("Default command");
    }
    client.stop();
  }
}

void connectWifi(){
  Serial.print("Connecting...");
  wifiMulti.addAP("PASCUAPARASAMASA", "lalalalala");
  wifiMulti.addAP("HEI Classroom", "mikeymagone");
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

void uploadReadings(float hi, float temp, float rh, String ss, String mt){
  sendAllReadings(hi, temp, rh, ss, mt);
}

void sendSwarmSignal(int moduleNumber){
  Serial.print("Swarm Signal Source:\t");
  Serial.println(moduleNumber);
  uploadReadings(heatIndex, temperature, relativeHumidity, "SWARM_ON", motionUpload);
}

void releaseMist(int module){
  //Module 3
  switch(module){
    case 1:
      Serial.println("Source activation module 1");
      releaseSequence(5000 / 4, false);
      break;
    case 2:
      Serial.println("Source activation module 2");
      releaseSequence(5000 / 4, false);
      break;
    case 3:
      Serial.println("Source activation module 3");
      releaseSequence(5000 / 4, false);
      break;
    case 4:
      Serial.println("Source activation module 4");
      releaseSequence(5000 / 4, false);
      break;
    default:
      Serial.println("Default flow");
      break;
  }
  delay(5000);
  Serial.println("Mist release sequence has ended");
}

void releaseSequence(int duration, bool mainSequence){
  //Main release is 5 seconds
  if(mainSequence){
    Serial.println("Starting main mist sequence...");    
  }else{
    Serial.println("Starting mist assist sequence...");
  }
  digitalWrite(solPin, 0);
  digitalWrite(pmpPin, 0);
  delay(10000);
  digitalWrite(solPin, 1);
  delay(5000);
  digitalWrite(solPin, 0);
  delay(duration);
  digitalWrite(pmpPin, 1);
  digitalWrite(solPin, 1);
}

void testOutput(){
  Serial.println("Starting sequence...");
  Serial.println("Expected LED Seequence");
  //pump - solenoid
  Serial.println("O -");
  Serial.println("- O");
  Serial.println("O O");
  digitalWrite(solPin, 0);
  digitalWrite(pmpPin, 1);
  delay(5000);
  digitalWrite(solPin, 1);
  digitalWrite(pmpPin, 0);
  delay(5000);
  digitalWrite(solPin, 0);
  digitalWrite(pmpPin, 0);
  delay(5000);
  digitalWrite(solPin, 1);
  digitalWrite(pmpPin, 1);
}

void sendAllReadings(float local_heatIndex, float local_temperature, float local_relativeHumidity, String local_swarmTrigger, String local_motion){
  WiFiClient client;
  Serial.print("Heat Index:\t");
  Serial.println(local_heatIndex, 2);
  Serial.print("Temperature:\t");
  Serial.println(local_temperature, 2);
  Serial.print("Relative Humidity:\t");
  Serial.println(local_relativeHumidity, 2);
  Serial.println("Uploading all readings...");
  if(client.connect(server, 80)){
    Serial.println("Wifi Client connected");
    String postStr = writeApiKey;
    postStr += fieldHeatIndex;
    postStr += String(local_heatIndex);
    postStr += fieldTemperature;
    postStr += String(local_temperature);
    postStr += fieldRelativeHumidity;
    postStr += String(local_relativeHumidity);
    postStr += fieldHeatIndexTreshold;
    postStr += String(heatIndexTreshold);
    postStr += fieldRelativeHumidityTreshold;
    postStr += String(relativeHumidityTreshold);
    postStr += fieldSwarmSignal;
    postStr += String(local_swarmTrigger);
    postStr += fieldMotion;
    postStr += String(local_motion);
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
    Serial.println("Readings Uploaded");
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

void doPOST(){
  Serial.println("Initializing Power On Self Test");
  digitalWrite(solPin, 1);
  digitalWrite(pmpPin, 1);
  delay(1000);
  digitalWrite(solPin, 0);
  digitalWrite(pmpPin, 0);
  delay(10000);
  Serial.println("POST done");
}
