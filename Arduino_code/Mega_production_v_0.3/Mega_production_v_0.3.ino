// data e hora
#include <TimeLib.h>

//sensores
#include <Wire.h>
#include <OneWire.h>
#include <dht11.h>
#include <DallasTemperature.h>

//display e datalogger
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
LiquidCrystal_I2C lcd(0x27,16,2);

//transmicao de dados serial
const byte numChars = 32;
char receivedChars[numChars];
//data e hora & controle de tempo
unsigned long epoch;
String dataNowstr = "";
String dataLUstr = "";
boolean newData = false;
unsigned long LCD_Time = millis();
boolean data_Tag = true;
boolean temp_Tag = true;
boolean amb_Tag = true;
boolean ec_Tag = true;
boolean light_Tag = true;

void setup() {
    Serial.begin(9600);   //inicia a comunicacao serial com o Computador
    Serial3.begin(9600);  //inicia a comunicacao serial com o ESP8266
    Serial.println("<Arduino is ready>");
    lcd.init();  //inicia o display LCD
    
}

void loop() {
    recvWithStartEndMarkers(); 
    screen_change();
    Serial.print(dataNowstr);
    UpdateTime();
}

void screen_change(){
  
  if(millis() - LCD_Time >= 0 && millis() - LCD_Time <= 100 && data_Tag){             //Imprime data e hora atual e ultima atualizacao de hora recebida
    dataNow();
    dataLU();
    LCD_Print(dataNowstr,dataLUstr);
    data_Tag = false;
  }
  
  else if(millis() - LCD_Time >= 5000 && millis() - LCD_Time <= 5100 && temp_Tag){    //Imprime temperatura da solucao nutritiva e da raiz das plantas
    LCD_Print("temperatura1","temperatura2");
    temp_Tag = false;
  }
  
  else if(millis() - LCD_Time >= 10000 && millis() - LCD_Time <= 10100 && amb_Tag){  //Imprime temperatura e umidade do ar
    LCD_Print("temperatura","umidade");
    amb_Tag = false;
  }
  else if(millis() - LCD_Time >= 15000 && millis() - LCD_Time <= 15100 && ec_Tag){  //Imprime condutividade eletrica recebida pelos 2 sensores
    LCD_Print("EC1 2000 uS","EC2 2210 uS");
    ec_Tag = false;
  }
  else if(millis() - LCD_Time >= 20000 && millis() - LCD_Time <= 20100 && light_Tag){  //Imprime quantidade de luz 
    LCD_Print("LuzA","luzB");
    light_Tag = false;    
  }else if(millis() - LCD_Time >= 25000){
    LCD_Time = millis();                                     // reinicia o "loop" do display
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
  dataLUstr = "LU";
  dataLUstr += String(day(epoch));
  dataLUstr += "/";
  dataLUstr += String(month(epoch));
  dataLUstr += " ";
  dataLUstr += String(hour(epoch));
  dataLUstr += String(":");
  dataLUstr += String(minute(epoch));
  dataLUstr += String(":");
  dataLUstr += String(second(epoch));
  }
  
void recvWithStartEndMarkers() {
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

void UpdateTime() {
  if (newData == true) {
    epoch = atol(receivedChars);
    setTime(epoch);
    newData = false;
  }
}
