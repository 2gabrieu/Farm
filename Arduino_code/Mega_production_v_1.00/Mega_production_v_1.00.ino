/*   Pinagem
  
  DHT-11 pino digital 30
  Sensores de temperatura pino digital 31
  
  Sensor de condutividade eletrica:
  - Entrada de dados pinos analogicos A0 e A1
  -Saida de energia para EC pinos digitais 40 e 41
  
  display pinos SDA e SCL

  Sensor de Iluminacao pino A6

  pinos digitais relés 22 23 24 25

  pinos para sdcard 50 51 52 53

*/

// Bibliotecas
#include <TimeLib.h> //Biblioteca para formatar a hora recebida do ESP
#include <Wire.h>    //biblioteca para sensores Onewire
#include <OneWire.h> //biblioteca para sensores Onewire
#include <dht11.h>   //Biblioteca para sensor dht11
#include <DallasTemperature.h> //biblioteca para sensor de temperatura
#include <NewPing.h> //Biblioteca para Sensor de nivel
#include <SPI.h> //biblioteca comunicacao para SD
#include <LiquidCrystal_I2C.h> //biblioteca Display
#include <SD.h> //biblioteca cartao SD

//definições de sensores
#define DHT11PIN 30     //pino de dados DHT11
#define ONE_WIRE_BUS 31 //pino de dados sensores de temperatura
#define TRIGGER_PIN  2  //pino trigger do sensor de nivel
#define ECHO_PIN     3  //pino echo do sensor de nivel

uint8_t sensor1[8] = { 0x28, 0xFF, 0xC0, 0xFE, 0x71, 0x16, 0x05, 0x33 }; //endereço do sensor de temperatura
uint8_t sensor2[8] = { 0x28, 0xFF, 0xDD, 0xFE, 0x71, 0x16, 0x05, 0xA3 }; //endereço do sensor de temperatura

#define MAX_DISTANCE 85 //distancia maxima para o sensor de nivel

//funcoes dos sensores 
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); //sensor de nivel
dht11 DHT11; //sensor de temperatura ambiente e umidade
OneWire oneWire(ONE_WIRE_BUS); //sensores de temperatura 
DallasTemperature sensors(&oneWire); //sensores de temperatura
LiquidCrystal_I2C lcd(0x27,16,2); //inicializa display


//variaveis do tipo int
int intensidade = 0;
int Temp_Raiz = 0;
int Temp_Agua = 0;
int Temp_Ambiente = 0;
int Umidade_Ambiente = 0;
int nivel = 0;
int R1 = 470;
int Ra = 25; //Resistance of powering Pins
int ECPin1 = A0;
int ECPower1 = 40;
int ppm = 0;
const int chipSelect = 53;

//variaveis do tipo boolean
bool Status_bomba = true;
bool data_Tag = true;
bool temp_Tag = true;
bool amb_Tag = true;
bool ec_Tag = true;
bool light_Tag = true;
bool newData = false;

//variaveis de Tempo 
unsigned long bomba_time = millis();
unsigned long overheat_time = millis();
unsigned long waterlevel_time = millis();
unsigned long LCD_Time = millis();
unsigned long datalog_time = millis();
unsigned long envia_time = millis();
unsigned long epoch;

//variaveis string
String bomba_str = "";
String datalogstr = "";
String string_data = "";

//Variaveis float
float TemperatureCoef = 0.019;
float K = 0.01486;
float EC = 0;
float raw= 0;
float Vdrop= 0;
float Rc= 0;
float buffer=0;

//Outras variaveis
const byte numChars = 32;
char receivedChars[numChars];

void setup() {
    sensors.begin();
    Wire.begin();
    Serial.begin(9600);   //inicia a comunicacao serial com o Computador
    Serial3.begin(9600);  //inicia a comunicacao serial com o ESP8266
    Serial.println("<Arduino is ready>");
    lcd.begin();  //inicia o display LCD

    pinMode(ECPower1, OUTPUT);      //Define os pinos para calculo de condutividade elétrica
    pinMode(ECPin1,INPUT);
    digitalWrite(ECPower1,LOW);

    for(int i = 22; i <= 25; i = i++){    //Coloca os pinos para acionar os relés como saida e desligado
      pinMode(i,OUTPUT);
      digitalWrite(i,HIGH);
    }
       
    if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
      delay(50);
    }
    else{Serial.println("card initialized.");}

}

void loop() {
    recvWithStartEndMarkers();
    screen_change();
    UpdateTime();
    bomba();
    Datalog();
    waterlevel();
    coolers();
    }

void screen_change(){           // muda a informacao em exibicao no display
  
  if(millis() - LCD_Time >= 0 && millis() - LCD_Time <= 4999 && data_Tag){             //Imprime data e hora atual e a versao do software
    data();
    LCD_Print(string_data,"TeChem Agro v1.0");
    data_Tag = false;
  }
  
  else if(millis() - LCD_Time >= 5000 && millis() - LCD_Time <= 9999 && temp_Tag){    //Imprime temperatura da solucao nutritiva e da raiz das plantas
    temperatura();
    LCD_Print(String("Agua " + String(Temp_Agua) + " C"),String("Raiz " + String(Temp_Raiz) + " C"));
    temp_Tag = false;
  }
  
  else if(millis() - LCD_Time >= 10000 && millis() - LCD_Time <= 14999 && amb_Tag){  //Imprime temperatura e umidade do ar
    temperatura();
    LCD_Print(String("Temp " + String(Temp_Ambiente) + " C"),String("Umidade " + String(Umidade_Ambiente) + "%"));
    amb_Tag = false;
  }
  else if(millis() - LCD_Time >= 15000 && millis() - LCD_Time <= 19999 && ec_Tag){  //Imprime condutividade eletrica recebida pelos 2 sensores
    
    LCD_Print("EC " + String(condutividade(ECPower1)) + " uS", "Tanque" + String(nivel) + "%");
    ec_Tag = false;
  }
  else if(millis() - LCD_Time >= 20000 && millis() - LCD_Time <= 24999 && light_Tag){  //Imprime quantidade de luz 
    if(Status_bomba){
      bomba_str = "Ligado";
    }else{
      bomba_str = "Desligado";
    }
    LCD_Print(String( "Luz " + String(iluminacao()) + "%"),String("Bomba " + bomba_str));
    light_Tag = false;    
  }else if(millis() - LCD_Time >= 25000){                     // reinicia o "loop" do display
    LCD_Time = millis();                
    data_Tag = true;
    temp_Tag = true;
    amb_Tag = true;
    ec_Tag = true;
    light_Tag = true;
  }
}

void LCD_Print(String str_1,String str_2){
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0,0);
  lcd.print(str_1);
  lcd.setCursor(0,1);
  lcd.print(str_2);
}
  
void data(){                  //data e hora atual
  time_t t = now();
  string_data = "";
  string_data += String(day(t));
  string_data += "/";
  string_data += String(month(t));
  string_data += " ";
  string_data += String(hour(t));
  string_data += String(":");
  string_data += String(minute(t));
  }

void recvWithStartEndMarkers() {                //recebe dados do ESP
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial3.available() > 0 && newData == false) {
        rc = Serial3.read();

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

void UpdateTime() {               //atualiza a hora quando recebe um valor do ESP
  if (newData == true) {
    epoch = atol(receivedChars);
    setTime(epoch);
    newData = false;
  }
}

void temperatura(){
  sensors.requestTemperatures();
  Temp_Agua = int(sensors.getTempC(sensor1));
  Temp_Raiz = int(sensors.getTempC(sensor2));
  int chk = DHT11.read(DHT11PIN);
  Temp_Ambiente = DHT11.temperature;
  Umidade_Ambiente = DHT11.humidity;
} 

int condutividade(int ECPower){

  //*********Reading Temperature Of Solution *******************
  sensors.requestTemperatures();
  Temp_Agua = int(sensors.getTempC(sensor1)); //Stores Value in Variable
    
  //************Estimates Resistance of Liquid ****************//
  digitalWrite(ECPower,HIGH);
 
    raw = analogRead(ECPin1);
    raw = analogRead(ECPin1);
    digitalWrite(ECPower,LOW);
   
  //***************** Converts to EC **************************//
  Vdrop = (5 * raw) / 1024.0;
  Rc = (Vdrop * R1) / (5 - Vdrop);
  Rc = Rc-Ra; //acounting for Digital Pin Resitance
  EC = 1000/(Rc * K);
  
  
  //*************Compensating For Temperaure********************//
  EC  =  EC / ( 1 + TemperatureCoef * (Temp_Agua - 25));
  return(int(EC));
  
}

int iluminacao(){
  intensidade = map(analogRead(A6), 0, 600, 0, 100);
  return(intensidade);
}

void bomba(){
  if(Temp_Raiz >= Temp_Agua + 5 || intensidade >= 4){
    if (millis() > bomba_time) {
      if (Status_bomba) {
        Status_bomba = false;
        bomba_time = millis() + 900000;
      } else {
        Status_bomba = true;
        bomba_time = millis() + 1800000;
      }
    }
  }
  else{
    if (millis() > bomba_time) {
      if (Status_bomba) {
        Status_bomba = false;
        bomba_time = millis() + 2700000;
      } else {
        Status_bomba = true;
        bomba_time = millis() + 900000;
      }
    }
  }
  digitalWrite(22, !Status_bomba);
}
void Envia_ESP(String envia){
envia = "<" + envia + ">";
Serial3.print(envia);
Serial.print(envia);
}

void Datalog(){
if(millis() - datalog_time > 60000){
  datalogstr = String(Temp_Agua) + "," + String(Temp_Raiz) + "," + String(Temp_Ambiente)
   + "," + String(Umidade_Ambiente) + "," + String(intensidade) + "," + String(condutividade(ECPower1)) + "," 
   + String(nivel) + String(Status_bomba);
   File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(String(string_data + "," + datalogstr));
    dataFile.close();
  }
   Envia_ESP(datalogstr);
   datalog_time = millis();
  }
}

void waterlevel(){
  if( millis() > waterlevel_time){
    nivel = sonar.ping_cm();
    nivel = map((80 - nivel), 0, 60, 0, 100);
    waterlevel_time = millis() + 120000;
  }
}

void coolers(){
  bool Status_cooler = true;
  unsigned long cooler_time = millis();
 if(Temp_Raiz >= Temp_Agua + 5 || intensidade >= 4){
    Status_cooler = true;
      }
  else{
    if (millis() > cooler_time) {
      if (Status_cooler) {
        Status_cooler = false;
        cooler_time = millis() + 3600000;
      } else {
        Status_cooler = true;
        cooler_time = millis() + 2700000;
        waterlevel();
      }
    }
  }
  digitalWrite(23, !Status_cooler);
}
