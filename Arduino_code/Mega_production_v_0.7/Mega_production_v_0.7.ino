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

// data e hora
#include <TimeLib.h>

//sensores
#include <Wire.h>
#include <OneWire.h>
#include <dht11.h>
#include <DallasTemperature.h>
#define DHT11PIN 30
#define ONE_WIRE_BUS 31

uint8_t sensor1[8] = { 0x28, 0xFF, 0xC0, 0xFE, 0x71, 0x16, 0x05, 0x33 };
uint8_t sensor2[8] = { 0x28, 0xFF, 0xDD, 0xFE, 0x71, 0x16, 0x05, 0xA3 };

dht11 DHT11;

OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

//variaveis para iluminacao
int intensidade = 0;

//variaveis para bomba
bool Status_bomba = true;
unsigned long bomba_time = millis();

//Variaveis para temperatura
int Temp_Raiz;
int Temp_Agua;
int Temp_Ambiente;
int Umidade_Ambiente;

//Variaveis para EC
int R1 = 1000;
int Ra = 25; //Resistance of powering Pins
int ECPin1 = A0;
int ECPin2 = A1;
int ECPower1 = 40;
int ECPower2 = 41;

float PPMconversion=0.7;
float TemperatureCoef = 0.019;
float K=0.88;

float EC=0;
float EC25 =0;
int ppm =0;
 
 
float raw= 0;
float Vin= 5;
float Vdrop= 0;
float Rc= 0;
float buffer=0;


//display e datalogger
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
LiquidCrystal_I2C lcd(0x27,16,2);
unsigned long datalog_time = millis();
String datalogstr = "";
const int chipSelect = 53;

//recepcao de dados serial
const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;
unsigned long envia_time = millis();

//data e hora & controle de tempo
unsigned long epoch;
String dataNowstr = "";
String dataLUstr = "";
unsigned long LCD_Time = millis();
boolean data_Tag = true;
boolean temp_Tag = true;
boolean amb_Tag = true;
boolean ec_Tag = true;
boolean light_Tag = true;

void setup() {
    sensors.begin();
    Wire.begin();
    Serial.begin(9600);   //inicia a comunicacao serial com o Computador
    Serial3.begin(9600);  //inicia a comunicacao serial com o ESP8266
    Serial.println("<Arduino is ready>");
    lcd.begin();  //inicia o display LCD

    pinMode(ECPower1, OUTPUT);
    pinMode(ECPower2,OUTPUT);
    pinMode(ECPin1,INPUT);
    pinMode(ECPin2,INPUT);
    digitalWrite(ECPower1,LOW);
    digitalWrite(ECPower2,LOW);
    pinMode(22,OUTPUT);
    pinMode(23,OUTPUT);
    pinMode(24,OUTPUT);
    pinMode(25,OUTPUT);
    digitalWrite(22,HIGH);
    digitalWrite(23,HIGH);
    digitalWrite(24,HIGH);
    digitalWrite(25,HIGH);

    
    if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
      delay(500);
    // don't do anything more:
    }
    else{Serial.println("card initialized.");}

}

void loop() {
    recvWithStartEndMarkers();
    screen_change();
    UpdateTime();
    bomba();
    Datalog();
    }

void screen_change(){           // muda a informacao em exibicao no display
  
  if(millis() - LCD_Time >= 0 && millis() - LCD_Time <= 4999 && data_Tag){             //Imprime data e hora atual e ultima atualizacao de hora recebida
    dataNow();
    dataLU();
    LCD_Print(dataNowstr,dataLUstr);
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
    
    LCD_Print(String(String(condutividade(ECPower1)) + " uS"), String(String(condutividade(ECPower2)) + " uS"));
    ec_Tag = false;
  }
  else if(millis() - LCD_Time >= 20000 && millis() - LCD_Time <= 24999 && light_Tag){  //Imprime quantidade de luz 
    LCD_Print(String( "Luz " + String(iluminacao()) + "%"),String("Bomba" + String("Bomba " + String(Status_bomba))));
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
  
void dataNow(){                  //data e hora atual
  time_t t = now();
  dataNowstr = "";
  dataNowstr += String(day(t));
  dataNowstr += "/";
  dataNowstr += String(month(t));
  dataNowstr += " ";
  dataNowstr += String(hour(t));
  dataNowstr += String(":");
  dataNowstr += String(minute(t));
  dataNowstr += String(":");
  dataNowstr += String(second(t));
  }

void dataLU(){                  //data e hora Last Update (shows last received epoch time)
  dataLUstr = "Att ";
  dataLUstr += String(day(epoch));
  dataLUstr += "/";
  dataLUstr += String(month(epoch));
  dataLUstr += " ";
  dataLUstr += String(hour(epoch));
  dataLUstr += String(":");
  dataLUstr += String(minute(epoch));
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
  Temp_Ambiente = DHT11.temperature;
  Umidade_Ambiente = DHT11.humidity;
} 

int condutividade(int ECPower){

  //*********Reading Temperature Of Solution *******************
  sensors.requestTemperatures();
  Temp_Agua = int(sensors.getTempC(sensor1)); //Stores Value in Variable
    
  //************Estimates Resistance of Liquid ****************//
  digitalWrite(ECPower,HIGH);
 
  if(ECPower == ECPower1){
    raw = analogRead(ECPin1);
    raw = analogRead(ECPin1);
    digitalWrite(ECPower,LOW);

  }
  else
  {
    raw = analogRead(ECPin2);
    raw = analogRead(ECPin2);
    digitalWrite(ECPower,LOW);
  }
   
  //***************** Converts to EC **************************//
  Vdrop = (Vin * raw) / 1024.0;
  Rc = (Vdrop * R1) / (Vin - Vdrop);
  Rc = Rc-Ra; //acounting for Digital Pin Resitance
  EC = 1000/(Rc * K);
  
  
  //*************Compensating For Temperaure********************//
  EC25  =  EC / ( 1 + TemperatureCoef * (Temp_Agua - 25));
  
  return(int(EC25));
  
}

int iluminacao(){
  intensidade = map(analogRead(A6), 0, 750, 0, 100);
  return(intensidade);
}

void bomba(){
  if(Temp_Raiz >= Temp_Agua + 5 || intensidade >= 4){
     if(millis() - bomba_time > 900000){
      bomba_time = millis();
      Status_bomba = !Status_bomba;
      }
  }else{
    if(millis() - bomba_time > 3600000){
      bomba_time = millis();
      Status_bomba = !Status_bomba;

    }
  }
  digitalWrite(22, Status_bomba);

}
void Envia_ESP(String envia){
envia = "<" + envia + ">";
Serial3.print(envia);
}

void Datalog(){
if(millis() - datalog_time > 600000){
  dataNow();
  datalogstr = dataNowstr + "," + String(Temp_Agua) + "," + String(Temp_Raiz) + "," + String(Temp_Ambiente)
   + "," + String(Umidade_Ambiente) + "," + String(intensidade) + "," + String(condutividade(ECPower1)) + "," 
   + String(condutividade(ECPower2));
   File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(datalogstr);
    dataFile.close();
   datalog_time = millis();

   Envia_ESP(datalogstr);

   
   }
}
}
