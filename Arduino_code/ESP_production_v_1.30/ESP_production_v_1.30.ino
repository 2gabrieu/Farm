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


//============================================

// Example 5 - Receive with start- and end-markers combined with parsing

const byte numChars = 50;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

      // variables to hold the parsed data
char messageFromPC[numChars] = {0};
int Channel_1 = 0;
int Channel_2 = 0;
int Channel_3 = 0;
int Channel_4 = 0;
int Channel_5 = 0;
int Channel_6 = 0;
int Channel_7 = 0;
int Channel_8 = 0;

char endereco_api_thingspeak[] = "api.thingspeak.com";
String chave_escrita_thingspeak = "DPI6SR3L0O7Z7VGA";  /* Coloque aqui sua chave de escrita do seu canal */
unsigned long last_connection_time;
char fields_a_serem_enviados[100] = {0};
WiFiClient client;

boolean newData = false;
boolean powerup = true;

//============================================

//comm to Mega configurations

unsigned long MilisSendTime = 0;

//============================================

void setup(){
Serial.begin(9600);         // Start the Serial communication to send messages to the computer
  delay(10);
  WiFiConnect();
  
}


void loop() {
  WiFiStatus();
  if(WiFi.status() == WL_CONNECTED){
    GetTime();
    
    PrintTime();
  }
   
   recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
        newData = false;
    }
    envia_dados();
    powerup = false;
}

void GetTime(){
  if(millis() - MilisUpdateTime >= 600000 || powerup){
    timeClient.update();
    MilisUpdateTime = millis();
    delay(500);
  }
}

void PrintTime(){
  if(millis() - MilisSendTime >= 600000 || powerup){
  Serial.print(WiFi.status());
  Serial.print("<");
  Serial.print(timeClient.getEpochTime());
  Serial.println(">");
  MilisSendTime = millis();
  }
}

void WiFiStatus(){
    if(WiFi.status() != WL_CONNECTED ) {
    Serial.print("Reconectando...");
    WiFiConnect();
  }
}

void WiFiConnect(){
  Serial.println('\n');
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connected!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
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
    Channel_1 = atoi(strtokIndx);     // convert this part to an integer
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    Channel_2 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    Channel_3 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    Channel_4 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    Channel_5 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    Channel_6 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    Channel_7 = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    Channel_8 = atoi(strtokIndx);     // convert this part to an integer
    
}

void envia_dados() {
    if( millis() - last_connection_time > 120000 )
    {
        
        sprintf(fields_a_serem_enviados,"field1=%d&field2=%d&field3=%d&field4=%d&field5=%d&field6=%d&field7=%d&field8=%d", Channel_1, Channel_2, Channel_3,
        Channel_4, Channel_5, Channel_6, Channel_7, Channel_8);
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
