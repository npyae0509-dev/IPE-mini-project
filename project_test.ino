//buzzer
#include "RichShieldPassiveBuzzer.h"
#define PassiveBuzzerPin 3
PassiveBuzzer buzzer(PassiveBuzzerPin);
  
//display
#include "RichShieldTM1637.h"
#define CLK 10
#define DIO 11
TM1637 disp(CLK,DIO);

//tempreature sensor
#include "RichShieldNTC.h"
#define NTC_PIN A1
NTC temper(NTC_PIN);
#include "RichShieldDHT.h"
DHT dht;

//IR remote
#include "RichShieldIRremote.h"
#define RECV_PIN 2  
IRrecv IR(RECV_PIN);

//LED
#define LED_RED 4 
#define LED_GREEN 5
#define LED_BLUE 6
#define LED_YELLOW 7

//push buttom
#define BUTTON_K1 8
#define BUTTON_K2 9

//knob
#define KNOB_PIN A0 

//light sensor
#define LDR_PIN A2 

void pressK2(float temp,float hum,int ldrVal,int knobVal);
void firealarm();



void setup() 
{
  Serial.begin(9600);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT); 
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(BUTTON_K1,INPUT_PULLUP);
  pinMode(BUTTON_K2,INPUT_PULLUP);
  dht.begin();


}

void loop() 
{ 
  float htemp = temper.getTemperature();
  int ldrVal;
  ldrVal=analogRead(LDR_PIN);
  int knobVal;
  knobVal=analogRead(KNOB_PIN);
  float temp= dht.readTemperature();
  float hum = dht.readHumidity();
  


  if (digitalRead(BUTTON_K2)==0)
  {
    buzzer.playTone(1000,500);
    pressK2(temp,hum,ldrVal,knobVal);
    while (digitalRead(BUTTON_K2)==0);
    delay(100);
  }
  
  if (htemp>50)
    firealarm();


}
void pressK2(float temp,float hum,int ldrVal,int knobVal)
{ 
  delay(100);
 if (knobVal>=15 && knobVal<350)   //LDR
  {
    buzzer.playTone(1000,200);
    delay(100);
    digitalWrite(LED_YELLOW,LOW);
    digitalWrite(LED_BLUE,LOW);
    digitalWrite(LED_GREEN,HIGH);
    Serial.print("Light intensity:");
    Serial.println(ldrVal);
    delay(3000);
  }
  else if(knobVal>=350 && knobVal<700)    //Temperature
  {
    buzzer.playTone(1000,200);
    delay(100);
    digitalWrite(LED_GREEN,LOW);
    digitalWrite(LED_YELLOW,LOW);
    digitalWrite(LED_BLUE,HIGH);
    if(isnan(temp))
    {
      Serial.println("Sensor Error");
      return;
    }
    else
    { 
      Serial.print("Temperature:");
      Serial.print(temp);
      Serial.println("C");
      delay(3000);
    }
  }
  else if(knobVal>=700 && knobVal<1000)     //Humindity
  {
    buzzer.playTone(1000,200);
    delay(100);
    digitalWrite(LED_GREEN,LOW);
    digitalWrite(LED_BLUE,LOW);
    digitalWrite(LED_YELLOW,HIGH);
    Serial.print("Humidity:");
    Serial.print(hum);
    Serial.println("%");
    delay(3000);
  }
  else
  {
    buzzer.playTone(1000,200);
    delay(100);
    digitalWrite(LED_YELLOW,LOW);
    digitalWrite(LED_BLUE,LOW);
    digitalWrite(LED_GREEN,LOW);
  }
 delay(200);
}

void firealarm()
{
  digitalWrite(LED_RED,LOW);
  digitalWrite(LED_YELLOW,LOW);
  digitalWrite(LED_BLUE,LOW);
  digitalWrite(LED_GREEN,LOW);
  Serial.println("High temperature detected possiblity of a fire!!!");
  Serial.println("Fire alarms are going off.");
  delay(100);
  digitalWrite(LED_RED,HIGH);
  buzzer.playTone(2000,500);
  delay(500);
  digitalWrite(LED_RED,LOW);
  delay(500);
}








