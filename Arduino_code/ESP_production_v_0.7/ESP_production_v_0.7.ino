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

const byte numChars = 50;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

      // variables to hold the parsed data
char messageFromPC[numChars] = {0};
int Temp_Agua = 0;
int Temp_Raiz = 0;
int Temp_Ambiente = 0;
int Umidade_Ambiente = 0;
int intensidade = 0;
int condutividade1 = 0;
int condutividade2 = 0;
int condutividade = 0;
int bomba = 0;

char endereco_api_thingspeak[] = "api.thingspeak.com";
String chave_escrita_thingspeak = "DPI6SR3L0O7Z7VGA";  /* Coloque aqui sua chave de escrita do seu canal */
unsigned long last_connection_time;
char fields_a_serem_enviados[100] = {0};
WiFiClient client;

boolean newData = false;

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
  envia_dados();
  }
  else{
    WiFiStatus();
    
  }


   recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
        newData = false;
    }
    
}

void GetTime(){
  if(millis() - MilisUpdateTime >= 1800000 || res){
    timeClient.update();
    MilisUpdateTime = millis();
  }
}

void PrintTime(){
  if(millis() - MilisSendTime >= 1800000 || res){
  Serial.print(WiFiTag);
  Serial.print("<");
  Serial.print(timeClient.getEpochTime());
  Serial.println(">");

  Serial.print("<");
  Serial.print(timeClient.getEpochTime());
  Serial.println(">");
  MilisSendTime = millis();
  }
}

void StartUp(){
  if(res && millis() - MilisUpdateTime>45000){
  WiFiStatus();
  }
  if(WiFiTag){ 
    timeClient.begin();
    GetTime();
    PrintTime();
    res = false;
      }

}

void WiFiStatus(){
    if(WiFi.status() != WL_CONNECTED ) {
    WiFiTag = false;
    WiFiConnect();
  }
  else{
   WiFiTag = true;

  }
}

void WiFiConnect(){
  WiFi.begin(ssid, password);
  }

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

//============

void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    Temp_Agua = atoi(strtokIndx);     // convert this part to an integer
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    Temp_Raiz = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    Temp_Ambiente = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    Umidade_Ambiente = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    intensidade = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    condutividade1 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    condutividade2 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    bomba = atoi(strtokIndx);     // convert this part to an integer
    
}

void envia_dados() {
    if( millis() - last_connection_time > 60000 )
    {
        condutividade = (condutividade1 + condutividade2)/2;
        sprintf(fields_a_serem_enviados,"field1=%d&field2=%d&field3=%d&field4=%d&field5=%d&field6=%d&field7=%d", Temp_Agua, Temp_Raiz, Temp_Ambiente,
        Umidade_Ambiente, condutividade, intensidade, bomba);
        envia_informacoes_thingspeak(fields_a_serem_enviados);
    }
}

void envia_informacoes_thingspeak(String string_dados){
    client.connect(endereco_api_thingspeak, 80);
    if (client.connect(endereco_api_thingspeak, 80))
    {
        /* faz a requisição HTTP ao ThingSpeak */
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+chave_escrita_thingspeak+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(string_dados.length());
        client.print("\n\n");
        client.print(string_dados);
         
        last_connection_time = millis();

    }
}
