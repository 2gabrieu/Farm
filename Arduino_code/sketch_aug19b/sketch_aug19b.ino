#include <Wire.h>
#include <ds3231.h>
#include <dht11.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>


#define DHT11PIN 9
#define ONE_WIRE_BUS 8
const int chipSelect = 10;
void(* resetFunc) (void) = 0;


uint8_t sensor1[8] = { 0x28, 0xFF, 0xC0, 0xFE, 0x71, 0x16, 0x05, 0x33 };
uint8_t sensor2[8] = { 0x28, 0xFF, 0xDD, 0xFE, 0x71, 0x16, 0x05, 0xA3 };

struct  ts t; 

float PPMconversion=0.7;
float TemperatureCoef = 0.019;
float K = 0.025;
float light_L;
float light_H;
float nutri_1;
float nutri_2;
float Temperature=10;
float EC1=0;
float EC2=0;
int ppm =0;
 
 
float raw1= 0;
float raw2= 0;
float Vin= 5;
float Vdrop= 0;
float Rc= 0;
float buffer=0;

bool bomba = true;

String dataString = "";

int hora;
int minuto;
int test = 7;
int R1 = 470;
int Ra = 25; //Resistance of powering Pins
int Pin_1 = A1;
int Pin_2 = A2;
int ECPower = 2;
int i = 0;

dht11 DHT11;

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);





void setup() {
  sensors.begin();
  Serial.begin(9600);
  Wire.begin();
  pinMode(6, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(Pin_1,INPUT);
  pinMode(Pin_2,INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  R1=(R1+Ra);// Taking into acount Powering Pin Resitance
  
  
 /* DS3231_init(DS3231_CONTROL_INTCN);
  ----------------------------------------------------------------------------
 In order to synchronise your clock module, insert timetable values below !
  ----------------------------------------------------------------------------
  t.hour=22; 
  t.min=27;
  t.sec=30;
  t.mday=9;
  t.mon=10;
  t.year=2020;
 
  DS3231_set(t);*/
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
      digitalWrite(4, LOW);
      digitalWrite(6, HIGH);  
      delay(500);
    // don't do anything more:
    }
    else{Serial.println("card initialized.");}
  
  i = 0;
}
 
void loop() {

  Serial.println(i);

   
  dataString = "";

  light_L = analogRead(A5);
  light_H = analogRead(A3);

  int chk = DHT11.read(DHT11PIN);
  
  // Display temperature from each sensor
  sensors.requestTemperatures();
  GetEC(); 
    
  DS3231_get(&t);
  hora = t.hour;
  minuto = t.min;
  
  status_bomba();
    
  dataString += String(t.mday);
  dataString += "/";
  dataString += String(t.mon);
  dataString += "/";
  dataString += String(t.year); 
  dataString += ",";
  dataString += String(t.hour);
  dataString += String(":");
  dataString += String(t.min);
  dataString += String(":");
  dataString += String(t.sec);
  dataString += ",";
  dataString += String(sensors.getTempC(sensor1));
  dataString += ",";
  dataString += String(sensors.getTempC(sensor2));
  dataString += ",";
  dataString += String(light_L);
  dataString += ",";
  dataString += String(light_H);
  dataString += ",";
  dataString += String((float)DHT11.humidity, 2);
  dataString += ",";
  dataString += String((float)DHT11.temperature, 2);
  dataString += ",";
  dataString += String(nutri_1);
  dataString += String("uS");
  dataString += ",";
  dataString += String(nutri_2);
  dataString += String("uS");
  dataString += ",";
  dataString += String(bomba);
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
    digitalWrite(4, HIGH);
    digitalWrite(6, LOW); 
    delay(500);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
      digitalWrite(4, LOW);
      digitalWrite(6, HIGH);  
      delay(500);
      i= i + 1;
  }
  digitalWrite(4, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, bomba);  
  delay(4500);
}

void status_bomba(){
  if (hora >= 6 && hora <= 10) {
    if(minuto%15 == 0 ){
      if(test != minuto){
        test = minuto;
        bomba = !bomba;
      }
}}

if (hora >= 11 && hora < 14) {
    bomba = true;
    
}
 if (hora >= 14 && hora <= 20) {
    if(minuto%15 == 0 ){
      if(test != minuto){
        test = minuto;
        bomba = !bomba;
      }
     }
    }

    if (hora >= 21 || hora <= 6) {
      bomba = false; 
      if(minuto <= 15){
        bomba = true;
        }
      
    }
     
      
  if (i >= 50 && bomba == true){
    resetFunc();
    }
    
  }

  //************ This Loop Is called From Main Loop************************//
void GetEC(){
 
 
//*********Reading Temperature Of Solution *******************//
Temperature = sensors.getTempC(sensor1);
 
 
 
//************Estimates Resistance of Liquid ****************//
digitalWrite(2,HIGH);
raw1= analogRead(Pin_1);
raw2= analogRead(Pin_2);// This is not a mistake, First reading will be low beause if charged a capacitor
raw1= analogRead(Pin_1);
raw2= analogRead(Pin_2);// This is not a mistake, First reading will be low beause if charged a capacitor
digitalWrite(2,LOW);
 
 
 
 
//***************** Converts to EC **************************//
Vdrop= (Vin*raw1)/1024.0;
Rc=(Vdrop*R1)/(Vin-Vdrop);
Rc=Rc-Ra; //acounting for Digital Pin Resitance
EC1 = 1000/(Rc*K);
 
 
//*************Compensating For Temperaure********************//
nutri_1  =  EC1/ (1+ TemperatureCoef*(Temperature-25.0));
    
   
 

//************************** End OF EC Function ***************************//
//***************** Converts to EC **************************//
Vdrop = (Vin*raw2)/1024.0;
Rc=(Vdrop*R1)/(Vin-Vdrop);
Rc=Rc-Ra; //acounting for Digital Pin Resitance
EC2 = 1000/(Rc*K);
 
 
//*************Compensating For Temperaure********************//
nutri_2  =  EC2/ (1+ TemperatureCoef*(Temperature-25.0));
 
 
;}
//************************** End OF EC Function ***************************//
 
 
 
//***This Loop Is called From Main Loop- Prints to serial usefull info ***//
