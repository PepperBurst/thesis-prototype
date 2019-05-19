
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "DHTesp.h"
#include <ArduinoJson.h>

//#define dhtPin D1
//#define pirPin D2
//#define solPin D3
//#define pmpPin D4
//#define comPin D5
#define myPeriodic 5

const int dhtPin = 5;
const int pirPin = 4;
const int solPin = 0;
const int pmpPin = 2;

String readApiKey = "EX1DVX903H75LZCS";   //Channel 1;
String writeApiKey = "459JH64HVXZMSZQJ";  //Channel 1;
String tbID = "32023";                    //Channel 1;
String tbApiKey = "WGTNUEUDPAYLS3PN";     //Channel 1;
String moduleName = "Module 1";           //Channel 1;
int moduleNumber = 1;                     //Channel 1;
int channelID = 630835;                   //Channel 1;
//String readApiKey = "SR26KL05EWR7E2YZ";   //Channel 2;
//String writeApiKey = "N4CZSPBXEJF4U16D";  //Channel 2;
//String tbID = "32823";                    //Channel 2;
//String tbApiKey = "KGUQTLB8771TD7CV";     //Channel 2;
//String moduleName = "Module 2";           //Channel 2;
//int moduleNumber = 2;                     //Channel 2;
//int channelID = 649321;                   //Channel 2;
//String readApiKey = "0YG3YFK75U80P940";   //Channel 3;
//String writeApiKey = "XOIG3YFOHR4OK8MW";  //Channel 3;
//String tbID = "32824";                    //Channel 3;
//String tbApiKey = "S11XC4U836TQ0261";     //Channel 3;
//String moduleName = "Module 3";           //Channel 3;
//int moduleNumber = 3;                     //Channel 3;
//int channelID = 743475;                   //Channel 3;
//String readApiKey = "KDIUJMWUGI9QPHPJ";   //Channel 4;
//String writeApiKey = "S2E20R36YWWDAMMC";  //Channel 4;
//String tbID = "32825";                    //Channel 4;
//String tbApiKey = "T7X7SYQN5K6Z9KK5";     //Channel 4;
//String moduleName = "Module 4";           //Channel 4;
//int moduleNumber = 4;                     //Channel 4;
//int channelID = 749656;                   //Channel 4;
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
String fieldLog = "&field8=";
const unsigned long postingInterval = 200000;
long lastUpdateTime = 0;
float global_heatIndex = 0;
float global_temperature = 0;
float global_relativeHumidity = 0;
float global_heatIndexTreshold = 0;
float global_relativeHumidityTreshold = 0;
int defaultMistDuration = 5000;
int lastMainReleaseCount = 10;
int mainReleaseMax = 10;
int dhtReadLoop = 5;
int maxReadDHT = 5;
int motionReadLoop = 10;
int maxReadPIR = 10;
bool recordedMotion = true;
String global_motionUpload = "FALSE";

DHTesp dht;
ESP8266WiFiMulti wifiMulti;

void setup() {
  Serial.begin(115200);
  Serial.println();
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);
  dht.setup(dhtPin, DHTesp::DHT11); // Connect DHT sensor to GPIO 5
  pinMode(pirPin, INPUT);
//  pinMode(comPin, OUTPUT);
  pinMode(solPin, OUTPUT);
  pinMode(pmpPin, OUTPUT);
//  digitalWrite(comPin, 1);
  digitalWrite(solPin, 1);
  digitalWrite(pmpPin, 1);
  connectWifi();
  // logToThingspeak("Module Powered On");
  Serial.println("Swarm of Automatic Misting Systems with Android Application");
  Serial.println(moduleName);
//  Serial.print("Posting Interval:\t");
//  Serial.println(postingInterval);
  doPOST();
  getPastReadings();
  getTalkbackCommand();
  getTresholds(); 
  
}

void loop() {
  while(wifiMulti.run() == WL_CONNECTED){
//    DHT Reading loop
    if(dhtReadLoop >= maxReadDHT){
      Serial.println("Acquiring Readings...");
      getReadings();
      sendAllReadings(global_heatIndex, global_temperature, global_relativeHumidity, "SWARM_OFF", global_motionUpload);
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
    if(global_heatIndex >= global_heatIndexTreshold && global_relativeHumidity <= global_relativeHumidityTreshold && lastMainReleaseCount >= mainReleaseMax){
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
  global_relativeHumidity = dht.getHumidity();
  global_temperature = dht.getTemperature();
  while(dht.getStatusString()!="OK"){
    global_relativeHumidity = dht.getHumidity();
    global_temperature = dht.getTemperature();
  }
  global_heatIndex = dht.computeHeatIndex(global_temperature, global_relativeHumidity, false);
//  Serial.println("Heat Index:\t" + String(global_heatIndex));
//  Serial.println("Temperature:\t" + String(global_temperature));
//  Serial.println("Relative Humidity:\t" + String(global_relativeHumidity));
  // logToThingspeak("Readings acquired");
}

bool getMotion(){
  Serial.println("Detecting motion...");
  // logToThingspeak("Detecting motion");
  bool motTrue = false;
  int motDet = 0;
  int falseCount = 0;
  int trueCount = 0;
  Serial.print("Detect count:\t");
  while((trueCount + falseCount) <= 60){
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
  motTrue = trueCount > 20;
  if(motTrue){
    Serial.println("Motion Detected");
    // logToThingspeak("Motion detected");
  }else{
    Serial.println("No Motion Detected");
    // logToThingspeak("No motion deteced");
  }
  if(motTrue){
    global_motionUpload = "TRUE";
  }else{
    global_motionUpload = "FALSE";
  }
   
  return motTrue;
}

void getTresholds(){
  getHITreshold();
  getRHTreshold();
  // logToThingspeak("Thresholds acquired");
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
    // Serial.println(all_data);
    const char* json = all_data.c_str();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    global_heatIndexTreshold = doc["field4"].as<float>();
    Serial.print("Heat Index Treshold:\t");
    Serial.println(global_heatIndexTreshold, 2);
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
    // Serial.println(all_data);
    const char* json = all_data.c_str();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    global_relativeHumidityTreshold = doc["field5"].as<float>();
    Serial.print("Relative Humidity Treshold:\t");
    Serial.println(global_relativeHumidityTreshold, 2);
  }
  client.stop();
}

void getPastReadings(){
  getPastHI();
  getPastTemp();
  getPastRH();
//  logToThingspeak("Past readings acquired");
}

void getPastHI(){
  WiFiClient client;
  String url = "/channels/";
  url += String(channelID);
  url += "/fields/1/last.json?api_key=";
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
    Serial.println("Acquiring Previous Heat Index Value...");
    String all_data;
    while(client.available()){
      all_data += client.readStringUntil('}');
    }
    all_data.substring('{', '}');
    all_data.trim();
    all_data.remove(all_data.length()-2, 2);
    all_data += "}";
    // Serial.println(all_data);
    const char* json = all_data.c_str();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    global_heatIndex = doc["field1"].as<float>();
    Serial.print("Heat Index:\t");
    Serial.println(global_heatIndex, 2);
  }
}

void getPastTemp(){
  WiFiClient client;
  String url = "/channels/";
  url += String(channelID);
  url += "/fields/2/last.json?api_key=";
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
    Serial.println("Acquiring Previous Temperature Value...");
    String all_data;
    while(client.available()){
      all_data += client.readStringUntil('}');
    }
    all_data.substring('{', '}');
    all_data.trim();
    all_data.remove(all_data.length()-2, 2);
    all_data += "}";
    // Serial.println(all_data);
    const char* json = all_data.c_str();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    global_temperature = doc["field2"].as<float>();
    Serial.print("Temperature:\t");
    Serial.println(global_temperature, 2);
  }
}

void getPastRH(){
  WiFiClient client;
  String url = "/channels/";
  url += String(channelID);
  url += "/fields/3/last.json?api_key=";
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
    Serial.println("Acquiring Previous Relative Humidity Value...");
    String all_data;
    while(client.available()){
      all_data += client.readStringUntil('}');
    }
    all_data.substring('{', '}');
    all_data.trim();
    all_data.remove(all_data.length()-2, 2);
    all_data += "}";
    // Serial.println(all_data);
    const char* json = all_data.c_str();
    const size_t capacity = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    global_relativeHumidity = doc["field3"].as<float>();
    Serial.print("Relative Humidity:\t");
    Serial.println(global_relativeHumidity, 2);
  }
}


void getTalkbackCommand(){
  // logToThingspeak("Getting talback commands");
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

// void uploadReadings(float hi, float temp, float rh, String ss, String mt){
//   sendAllReadings(hi, temp, rh, ss, mt);
// }

void sendSwarmSignal(int moduleNumber){
  Serial.print("Swarm Signal Source:\t");
  Serial.println(moduleNumber);
  sendAllReadings(global_heatIndex, global_temperature, global_relativeHumidity, "SWARM_ON", global_motionUpload);
}

void releaseMist(int module){
  // logToThingspeak("Assisting mist sequence source " + String(module));
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
}

void releaseSequence(int duration, bool mainSequence){
  //Main release is 5 seconds
  if(mainSequence){
    Serial.println("Starting main mist sequence...");  
    // logToThingspeak("Starting main mist release sequence");  
  }else{
    Serial.println("Starting mist assist sequence...");
  }
  digitalWrite(solPin, 0);
  digitalWrite(pmpPin, 0);
  delay(duration);
  digitalWrite(pmpPin, 1);
  digitalWrite(solPin, 1);
  delay(1000);
  Serial.println("Mist sequence ended");
  // logToThingspeak("Mist sequence ended");
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
    postStr += String(global_heatIndexTreshold);
    postStr += fieldRelativeHumidityTreshold;
    postStr += String(global_relativeHumidityTreshold);
    postStr += fieldSwarmSignal;
    postStr += String(local_swarmTrigger);
    postStr += fieldMotion;
    postStr += String(local_motion);
    // postStr += fieldLog;
    // postStr += String(local_message);
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
  // logToThingspeak("Initializing POST");
  digitalWrite(solPin, 1);
  digitalWrite(pmpPin, 1);
  delay(1000);
  digitalWrite(solPin, 0);
  digitalWrite(pmpPin, 0);
  delay(20000);
  digitalWrite(solPin, 1);
  digitalWrite(pmpPin, 1);
  Serial.println("POST done");
  // logToThingspeak("POST done");
}

// void logToThingspeak(String local_message){
//   sendAllReadings(global_heatIndex, global_temperature, global_relativeHumidity, "SWARM_OFF", global_motionUpload, local_message);
// }
