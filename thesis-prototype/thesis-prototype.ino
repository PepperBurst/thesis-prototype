#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "DHTesp.h"

#define dhtPin D1
#define pirPin D2
#define outPin D3
#define myPeriodic 15
//APIKEY for Channel 1 = 459JH64HVXZMSZQJ
//APIKEY for Channel 2 = N4CZSPBXEJF4U16D
const char* server = "api.thingspeak.com";
String apiKey = "N4CZSPBXEJF4U16D"; // set Write API Key
int sent = 0;
bool delayForMist = false;
int countDelay = 0;
String fieldHI = "&field1=";
String fieldTemp = "&field2=";
String fieldRH = "&field3=";
String moduleName = "Module 2";     //set Module #
float hi = 0;
float temp = 0;
float rh = 0;

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
  Serial.println();
}

void loop() {
  while(wifiMulti.run() == WL_CONNECTED){
    getReadings();
    if(countDelay > 1){
      countDelay = 0;
      delayForMist = false;
    }
    if(!delayForMist){
      if(hi>32 && rh<90){
        Serial.println("Heat index is above threshold, Relative Humidity is in valid range");
        if(getMotion()){
          Serial.println("Motion detected, releasing mist...");
  //        digitalWrite(outPin, 0);
          releaseMist();
          delayForMist = true;
        }else{
          Serial.println("No motion detected");
        }
      }
    }else{
      Serial.println("Mist recently released, delaying mist");
      countDelay++;
    }
    delay(5000);
  }
  connectWifi();
}

void getReadings(){
  rh = dht.getHumidity();
  temp = dht.getTemperature();
  while(dht.getStatusString()!="OK"){
    rh = dht.getHumidity();
    temp = dht.getTemperature();
  }
  hi = dht.computeHeatIndex(temp, rh, false);
  sendHeatIndex(hi);
  sendTemp(temp);
  sendRH(rh);
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

void connectWifi(){
  Serial.print("Connecting...");
  wifiMulti.addAP("PASCUAPARASAMASA", "lalalalala");
  wifiMulti.addAP("PLDTHOMEDSL17A6F", "PLDTWIFI17A67");
  wifiMulti.addAP("PLDTMyDSL", "pldtwifiD6346");
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

void sendTemp(float temp){
  WiFiClient client;
  Serial.print("Temperature:\t");
  Serial.println(temp, 2);
  Serial.println("Uploading temperature reading...");
  if(client.connect(server, 80)){
    Serial.println("Wifi Client connected");
    String postStr = apiKey;
    postStr += fieldTemp;
    postStr += String(temp);
    postStr += "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("HOST: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(1000);
    Serial.println("Temperature uploaded");
    Serial.print("Wait 15 seconds for next upload");
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
    String postStr = apiKey;
    postStr += fieldRH;
    postStr += String(rh);
    postStr += "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("HOST: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(1000);
    Serial.println("Relative Humidity uploaded");
    Serial.print("Wait 15 seconds for next upload");
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
    String postStr = apiKey;
    postStr += fieldHI;
    postStr += String(heatInd);
    postStr += "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("HOST: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(1000);
    Serial.println("Heat index uploaded");
    Serial.print("Wait 15 seconds for next upload");
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

