// Wifi and time Server configurations 

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid     = "iHome - sn";
const char *password = "123456789";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
unsigned long MilisUpdateTime = millis();
unsigned long WiFiTime = millis();
bool WiFiTag = false;
bool res = true;

//============================================

// Example 5 - Receive with start- and end-markers combined with parsing

//============================================

//comm to Mega configurations

unsigned long MilisSendTime = 0;

//============================================

void setup(){
  Serial.begin(9600);
  res = true;
}

void loop() {
  StartUp();
  WiFiStatus();
  if(WiFiTag){
  GetTime();
  PrintTime();
  }
}

void GetTime(){
  if(millis() - MilisUpdateTime >= 3600000 || res){
    timeClient.update();
    MilisUpdateTime = millis();
  }
}

void PrintTime(){
  if(millis() - MilisSendTime >= 3600000 || res){
  Serial.print(WiFiTag);
  Serial.print("<");
  Serial.print(timeClient.getEpochTime());
  Serial.println(">");
  MilisSendTime = millis();
  }
}

void StartUp(){
  if(res){
  WiFiStatus();
  if(WiFiTag){ 
    timeClient.begin();
    GetTime();
    PrintTime();
    res = false;
      }
      else{
      WiFiStatus();
      
      }
  }
}

void WiFiStatus(){
    if(WiFi.status() != WL_CONNECTED ) {
    WiFiTag = false;
  }
  else{
   WiFiTag = true;
  }
}

void WiFiConnect(){
  WiFi.begin(ssid, password);
  }
