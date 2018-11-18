#include <ESP8266WiFi.h>
#include "DHTesp.h"

#define dhtPin D1
#define pirPin D2
#define myPeriodic 15

const char* server = "api.thingspeak.com";
const char* MY_SSID = "PLDTMyDSL";
const char* MY_PWD = "pldtwifiD6346";
String apiKey = "19FK2MVUXKY4UR3N";
int sent = 0;

DHTesp dht;

void setup() {
  Serial.begin(115200);
  Serial.println();
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);
  dht.setup(dhtPin, DHTesp::DHT11); // Connect DHT sensor to GPIO 5
  pinMode(pirPin, INPUT);
}

void loop() {
  Serial.print("Heat Index:\t");
  float heatInd = getHeatIndex();
  Serial.println(heatInd, 2);
  if(heatInd>33){
    Serial.println("Heat index above threshold, detecting motion...");
    if(getMotion()){
      Serial.println("Motion detected, releasing mist");
    }else{
      Serial.print("No motion detected");
    }
  }
  delay(5000);
}

float getHeatIndex(){
  float hum = dht.getHumidity();
  float temp = dht.getTemperature();
  while(dht.getStatusString()!="OK"){
    hum = dht.getHumidity();
    temp = dht.getTemperature();
  }
  float heatInd = dht.computeHeatIndex(temp, hum, false);
  sendTemp(temp);
  int count = myPeriodic;
  while(count--){
    delay(1000);
  }
  sendRH(hum);
  count = myPeriodic;
  while(count--){
    delay(1000);
  }
  sendHeatIndex(heatInd);
  count = myPeriodic;
  while(count--){
    delay(1000);
  }
  return heatInd;
}

bool getMotion(){
  int lpCnt = 0;
  bool motTrue = false;
  bool motDet = true;
  while(lpCnt <= 30 && motDet){
    motDet = digitalRead(pirPin);
    lpCnt++;
    motTrue = motDet && (lpCnt>=30);
    delay(100);
  }
  return motTrue;
}

void connectWifi(){
  Serial.print("Connecting to "+*MY_SSID);
  WiFi.begin(MY_SSID, MY_PWD);
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected");
  Serial.println();
}

void sendTemp(float temp){
  WiFiClient client;
  if(client.connect(server, 80)){
//    Serial.println("Wifi Client connected");
    String postStr = apiKey;
    postStr += "&field2=";
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
  }
  sent++;
  client.stop();
}

void sendRH(float rh){
  WiFiClient client;
  if(client.connect(server, 80)){
//    Serial.println("Wifi Client connected rh");
    String postStr = apiKey;
    postStr += "&field3=";
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
  }
  sent++;
  client.stop();
}

void sendHeatIndex(float heatInd){
  WiFiClient client;
  if(client.connect(server, 80)){
//    Serial.println("Wifi Client connected heatInd");
    String postStr = apiKey;
    postStr += "&field4=";
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
  }
  sent++;
  client.stop();
}
