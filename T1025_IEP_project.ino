/*
Name:Nyan Pyae Sone                Tao Xu
Adm:P2635846                       P2530558
Class:DCPE/FT/1B/22
Work:rest of the functions.         updateSerialPlotter() and readSensors()
*/


//buzzer
#include "RichShieldPassiveBuzzer.h"
#define PassiveBuzzerPin 3
PassiveBuzzer buzzer(PassiveBuzzerPin);
  
//display
#include "ModifiedTM1637.h"
#define CLK 10
#define DIO 11
ModifiedTM1637 disp(CLK,DIO);

//tempreature and huminidty sensor
#include "RichShieldNTC.h"
#define NTC_PIN A1
NTC temper(NTC_PIN);
#include "RichShieldDHT.h"
DHT dht(12, DHT11);

//IR remote
#include "RichShieldIRremote.h"
#define RECV_PIN 2  
IRrecv IR(RECV_PIN);
#define KEY_POWER 0x45
#define KEY_MENU 0x47
#define KEY_TEST 0x44
#define KEY_PLUS 0x40
#define KEY_BACK 0x43
#define KEY_PREV 0x07
#define KEY_PLAY 0x15
#define KEY_NEXT 0x09
#define KEY_ZERO 0x16
#define KEY_MINUS 0x19
#define KEY_C 0x0D
#define KEY_ONE 0x0C
#define KEY_TWO 0x18
#define KEY_THREE 0x5E
#define KEY_FOUR 0x08
#define KEY_FIVE 0x1C
#define KEY_SIX 0x5A
#define KEY_SEVEN 0x42
#define KEY_EIGHT 0x52
#define KEY_NINE 0x4A

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

void firstdis(int ldrVal,float ptemp,bool &menu);
void ALL_LEDOFF();
void control_type(bool &menu,bool &con_chanMode,char &control,bool &remote_Enable);
void Mkeyboard(bool &ready,bool &menu,float temp,float hum,int ldrVal,bool &monitorMode);
void IR_remote(float temp,float hum,bool ready,int ldrVal,bool &remote_Enable,int &brightness,char &control,unsigned long &previousLED,bool &powerFlash);
void fire_alarm(float ptemp);
void passive_temp(float ptemp);
void light_reminder(int ldrVal,float ptemp);
void DHT_sensor(float &temp,float &hum,bool &ready);
void fireSiren();
void health_Check(int ldrVal,float ptemp,int knobVal,bool &menu,bool &healthMode);
void updateSerialPlotter(bool &fluxMode,bool &menu);
void readSensors(
    const int sensorPins[3],
    float sensorValues[3],
    const char* sensorLabels[3]
);



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
  IR.enableIRIn();
  disp.init();
}

void loop()
{ 
  float ptemp = temper.getTemperature();
  static unsigned long bothStart=0;
  static bool bothHeld=false;
  static bool waitRelease=false;
  static bool deviceOFF=true;
  static bool menu=true;


  if(digitalRead(BUTTON_K1)==LOW && digitalRead(BUTTON_K2)==LOW)
  {
    if(!waitRelease)
    {
      if(!bothHeld)
      {
        bothStart=millis();
        bothHeld=true;
      }

      if(millis()-bothStart>=3000)
      {
        deviceOFF=!deviceOFF;
        bothHeld=false;
        waitRelease=true;

        if(deviceOFF)
        {
          ALL_LEDOFF();
          disp.clearDisplay();
          Serial.println("Device OFF");
        }
        else
        {
          Serial.println("Device ON");
          menu=true;
        }
      }
    }
  }
  else
  {
    bothHeld=false;
    waitRelease=false;
  }

  fire_alarm(ptemp);

  if(deviceOFF)
  {
    return;
  }

  int ldrVal = analogRead(LDR_PIN);

  passive_temp(ptemp);
  light_reminder(ldrVal,ptemp);
  firstdis(ldrVal,ptemp,menu);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void firstdis(int ldrVal,float ptemp,bool &menu)
{ 
  int knobVal = analogRead(KNOB_PIN);
  static float temp=0.0;
  static float hum=0.0;
  static bool ready=false;
  static char control='k';
  static bool remote_Enable=false;
  static bool monitorMode=false;
  static bool con_chanMode=false;
  static int brightness=7;
  static bool healthMode=false;
  static bool fluxMode=false;
  static unsigned long previousLED=0;
  static bool powerFlash=false;

  if(menu)
  {
    Serial.println(F(""));
    Serial.println(F("<=Smart Indoor Environment & Safety Monitoring System control panel=>"));
    Serial.println(F("1.Monitor"));
    Serial.println(F("2.Sensor graph"));
    Serial.println(F("3.Components health check"));
    Serial.println(F("4.Control"));
    Serial.println(F(""));
    menu=false;
  }

  if(!remote_Enable)
  {
    if(IR.decode())
    {
      if(IR.isReleased() && IR.keycode==KEY_POWER)
      {
        remote_Enable=true;
        digitalWrite(LED_RED,HIGH);
        previousLED=millis();
        powerFlash=true;
      }
      IR.resume();
    }
  }
  
  if(remote_Enable)
  { 
    control='r';
    IR_remote(temp,hum,ready,ldrVal,remote_Enable,brightness,control,previousLED,powerFlash);
  }

  if(powerFlash && millis()-previousLED>=1500)
  {
    digitalWrite(LED_RED,LOW);
    powerFlash=false;
  }
  
  DHT_sensor(temp,hum,ready);   // must run every loop - it accumulates samples over several seconds

  if(Serial.available() && !con_chanMode && !monitorMode && !healthMode && !fluxMode)
  {
    char input=Serial.read();
    
    if(input=='1' && (control=='k' || control=='K') && !menu)
    {
      monitorMode=true;
    }
    else if(input=='2' && (control=='k' || control=='K') && !menu)
    {
      fluxMode=true;
    }
    else if(input=='3' && (control=='k' || control=='K') && !menu)
    {
      healthMode=true;
    }
    else if(input=='4' && (control=='k' || control=='K') && !menu)
    {
      con_chanMode=true;
    }
  }

  if(monitorMode)
  {
    Mkeyboard(ready,menu,temp,hum,ldrVal,monitorMode);
  }

  if(con_chanMode)
  {
    control_type(menu,con_chanMode,control,remote_Enable);
  }

  if(healthMode)
  {
    health_Check(ldrVal,ptemp,knobVal,menu,healthMode);
  }
  
  if(fluxMode)
  {
    updateSerialPlotter(fluxMode,menu);
  }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ALL_LEDOFF()
{
  digitalWrite(LED_RED,LOW);
  digitalWrite(LED_YELLOW,LOW);
  digitalWrite(LED_BLUE,LOW);
  digitalWrite(LED_GREEN,LOW);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void control_type(bool &menu,bool &con_chanMode,char &control,bool &remote_Enable)
{
  static bool started=false;

  if(!started)
  {
    started=true;
    Serial.println(F("Enter 'k' for keyborad and 'r' for remote."));
    Serial.println(F("Type 'e' after finish changing control type."));
    Serial.print(F("Current control type:"));

    if(control=='k' || control=='K')
    {
      Serial.println(F("keyboard"));
    }
    else
    {
      Serial.println(F("remote"));
    }
  }

  if(Serial.available())
  {
    char c=Serial.read();
    if(c=='k' || c=='K')
    {
      control=c;
      remote_Enable=false;
      Serial.println(F("New control type:keyboard"));
    }
    else if(c=='r' || c=='R')
    {
      control=c;
      remote_Enable=true;
      Serial.println(F("New control type:remote"));
    }
    else if(c=='e' || c=='E')
    {
      Serial.println(F("Exited."));
      menu=true;
      con_chanMode=false;
      started=false;
      return;
    }
    else
    {
      Serial.println(F("Error!"));
      Serial.println(F("Enter again."));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Mkeyboard(bool &ready,bool &menu,float temp,float hum,int ldrVal,bool &monitorMode)
{ 
  static bool started_menu=false;
  static bool lightView=false;
  static unsigned long lightTimer=0;

  if(!started_menu)
  {
    Serial.println(F("<=======menu=======>"));
    Serial.println(F("1.Temperature"));
    Serial.println(F("2.Humidity"));
    Serial.println(F("3.light intensity"));
    Serial.println(F("4.return to menu"));
    Serial.println(F("<==================>"));
    Serial.println(F(""));
    started_menu=true;
  }

  if(lightView && millis()-lightTimer>=5000)
  {
    lightTimer=millis();
    Serial.print(F("Light intensity is "));
    Serial.print(ldrVal);
    Serial.println(F("."));
  }

  if(Serial.available())
  {
    char Minput=Serial.read();

    Serial.println("");

    if(Minput=='1')
    {
      lightView=false;
      if(ready)
      {
        Serial.print(F("Temperature is "));
        Serial.print(temp);
        Serial.println(F("C."));
        ready=false;
      }
      else
      {
        Serial.println(F("Temperature value not ready yet,wait for a few second."));
      }
    }
    else if(Minput=='2')
    {
      lightView=false;
      if(ready)
      { 
        Serial.print(F("Humidity is "));
        Serial.print(hum);
        Serial.println(F("%."));
        ready=false;
      }
      else
      {
        Serial.println(F("Humidity value not ready yet,wait for a few second."));
      }
    }
    else if(Minput=='3')
    {
      Serial.print(F("Light intensity is "));
      Serial.print(ldrVal);
      Serial.println(F("."));
      lightView=true;
      lightTimer=millis();
    }
    else if(Minput=='4' || Minput=='e' || Minput=='E')
    {
      Serial.println(F("Exited."));
      lightView=false;
      monitorMode=false;
      menu=true;
      started_menu=false;
      return;
    }  
    else
    {
      Serial.println(F("ERROR!"));
      Serial.println(F("Enter again."));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void IR_remote(float temp,float hum,bool ready,int ldrVal,bool &remote_Enable,int &brightness,char &control,unsigned long &previousLED,bool &powerFlash)
{ 
  static int8_t TEMP[]={20,14,21,25};
  static int8_t HUMI[]={18,19,21,22};
  static int8_t LIGH[]={23,22,24,18};

  if (IR.decode())
  {
    if (IR.isReleased())
    {
      switch (IR.keycode)
      {
        case KEY_ONE:
          {
            disp.display(TEMP);
            if(temp>20 && temp<30)
            {
              ALL_LEDOFF();
              digitalWrite(LED_GREEN,HIGH);
            }
            else if(temp>30)
            {
              ALL_LEDOFF();
              digitalWrite(LED_YELLOW,HIGH);
            }
            else if(temp<20 )
            {
              ALL_LEDOFF();
              digitalWrite(LED_BLUE,HIGH);
            }
          }
          break;

          case KEY_TWO:
          {
            disp.display(HUMI);
            if(hum>40 && hum<60)
            {
              ALL_LEDOFF();
              digitalWrite(LED_GREEN,HIGH);
            }
            else if(hum>60 && hum<70)
            {
              ALL_LEDOFF();
              digitalWrite(LED_YELLOW,HIGH);
            }
            else if(hum<40 && hum!=0.0)
            {
              ALL_LEDOFF();
              digitalWrite(LED_BLUE,HIGH);
            }
          }
          break;

          case KEY_THREE:
          {
            disp.display(LIGH);
            if(ldrVal>500 && ldrVal<1000)
            {
              ALL_LEDOFF();
              digitalWrite(LED_GREEN,HIGH);
            }
            else if(ldrVal>1000)
            {
              ALL_LEDOFF();
              digitalWrite(LED_YELLOW,HIGH);
            }
            else if(ldrVal<500)
            {
              ALL_LEDOFF();
              digitalWrite(LED_BLUE,HIGH);
            }
          }
          break;

          case KEY_POWER:
          {
            remote_Enable=false;
            if(!remote_Enable) control='k';
            ALL_LEDOFF();
            disp.clearDisplay();
            digitalWrite(LED_RED,HIGH);
            previousLED=millis();
            powerFlash=true;
          }
          break;
          
          case KEY_PLUS:
          {
            if(brightness<7)
            {
              brightness++;
            }
            disp.set(brightness);
          }
          break;

          case KEY_MINUS:
          {
            if(brightness>0)
            {
              brightness--;
            }
            disp.set(brightness);
          }
          break;
      }
    }
    IR.resume();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void fire_alarm(float ptemp)
{
  static bool started=false;
  static bool fireLED=false;
  static unsigned long previousFire=0;
  static unsigned long previousFirePrint=0;

  if(!started)
  {
    started=true;
    ALL_LEDOFF();
  }

  if(ptemp>50) // turn on the sprinklers and transmits a digital, cellular, or internet signal to a 24/7 central monitoring station
  { 
    fireSiren();
    if(millis()-previousFire>=500)
    {
      previousFire=millis();
      fireLED=!fireLED;
      digitalWrite(LED_RED,fireLED); 
    }
    
    if (millis()-previousFirePrint>=5000)
    {
      previousFirePrint=millis();
      Serial.println(F("Fire!!!"));
    }
  }
  else
  {
    fireLED = false;
    digitalWrite(LED_RED,LOW);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void passive_temp(float ptemp)
{ 
  static unsigned long previousWarning=0;
  static bool warningLED=false;
  static bool templow=true;
  static bool temphigh=true;
  static bool warning=true;
  static bool started=false;

  if(!started)
  {
    started=true;
    ALL_LEDOFF();
  }
  
 if(ptemp<10)
  {
    if(templow)
    { 
      Serial.println(F(""));
      Serial.println(F("Temperature is low, adjuct the air conditioning to increase the temperature.")); //relay the signal to a heater
      templow=false;
    }
  }
  else 
  {
    templow=true;
  }

  if(ptemp>=30 && ptemp<=40)
  {
    if(temphigh)
    {
      Serial.println(F(""));
      Serial.println(F("Temperature is high, adjuct the air conditioning to lower the temperature."));//relay a signal to turn up the air conditioning
      temphigh=false;
    }
  }
  else
  {
    temphigh=true;
  } 

  if(ptemp>40 && ptemp<=50)
  {
    if (warning)
    {
      Serial.println(F(""));
      Serial.println(F("High temperature detected, possibility of a fire!"));
      ALL_LEDOFF();
      warning = false;
    }
    if(millis()-previousWarning>=500)
    {
      previousWarning=millis();
      warningLED=!warningLED;
      digitalWrite(LED_YELLOW,warningLED);
    }
  }
  else
  {
    warningLED=false;
    warning = true;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void light_reminder(int ldrVal,float ptemp)
{
  static unsigned long previousBright=0;
  static unsigned long previousDark=0;
  static bool bright=true;
  static bool dark=true;

  if(ldrVal>900 && ptemp<=50) //adjust the number of light that should be kept on
  {
    if(bright)
    { 
      ALL_LEDOFF();
      bright=false;
    }
    if(millis()-previousBright>=1000)
    {
      previousBright=millis();
      digitalWrite(LED_BLUE,LOW);
    } 
  }
  else if(ldrVal<850)   // must drop meaningfully below the 900 cutoff before re-arming
  {
    bright=true;
  }

  if(ldrVal<150 && ptemp<=50) //automatically turn on the lights
  {
    if(dark)
    {
      ALL_LEDOFF();
      Serial.println(F(""));
      Serial.println(F("Switch on some light the room is dark."));
      dark=false;
    }
    if(millis()-previousDark>=1000)
    {
      previousDark=millis();
      digitalWrite(LED_BLUE,HIGH);
    }
    
  }
  else if(ldrVal>200)   // must rise meaningfully above the 150 cutoff before re-arming
  {
    dark=true;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////

void DHT_sensor(float &temp,float &hum,bool &ready)
{
  float Tsum=0.0;
  float Hsum=0.0;
  static float avetemp[3];
  static float avehum[3];
  static unsigned long previousDHT=0;
  static int8_t n=0;
  
  if(millis()-previousDHT>=2000)
  { 
    previousDHT=millis();
    float t=dht.readTemperature();
    float h=dht.readHumidity();
    if(!isnan(t) && !isnan(h))
    {
      avetemp[n] = t;
      avehum[n] = h;
      n++;
    }
  }  
  if(n>=3)
  {
    int i;
    for(i=0;i<3;i++)
    {
      Tsum += avetemp[i];
      Hsum += avehum[i];
    }
    temp=Tsum/3.0;
    hum=Hsum/3.0;
    n=0;
    ready=true;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////

void fireSiren()
{
    static unsigned long previousSiren = 0;
    static int note = 0;

    const int fireAlarm[] =
    {
        2500,2500,2500,2500,
        1800,1800,1800,1800
    };

    if (millis() - previousSiren >= 60)
    {
        previousSiren = millis();

        buzzer.playTone(fireAlarm[note], 60);

        note++;

        if (note >= sizeof(fireAlarm) / sizeof(fireAlarm[0]))
        {
            note = 0;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void health_Check(int ldrVal,float ptemp,int knobVal,bool &menu,bool &healthMode)
{
  static bool started=false;
  static bool begin=false;
  static int stage=0;
  static int startKnob=0;
  static unsigned long dis_timer=0;
  static unsigned long ir_timer=0;
  static bool irOK[20]={false};
  static bool knob=false;
  static bool ldr=false;
  static bool k1=false;
  static bool k2=false;
  static bool dht_s=false;
  static bool ntc=false;

  if (Serial.available())
  {
    char input_HC = Serial.read();

    if(input_HC == 'e' || input_HC == 'E')
    {
      ALL_LEDOFF();
      disp.clearDisplay();
      started = false;
      stage = 0;
      healthMode = false;
      menu = true;
      begin=false;

      for (int i = 0; i < 20; i++)
      {
        irOK[i] = false;
      }
      Serial.println(F(""));
      Serial.println(F("Health check exited."));
      return;
    }
    else if(input_HC=='.' || input_HC=='>')
    {
      if(stage<3)
      {
        started=false;
        stage++;   
      }
      
    }
    else if(input_HC==',' || input_HC=='<')
    {
      if(stage>0)
      {
        started=false;
        stage--;
      }
      
      
    }
    else if(input_HC=='r' || input_HC=='R')
    {
      started=false;
    }
  }

  if (!begin)
  {
    begin = true;
    stage = 0;

    for (int i = 0; i < 20; i++)
    {
      irOK[i] = false;
    }
    Serial.println(F(""));
    Serial.println(F("'<' for previous, '>' for next, 'r' for refreshin and 'e' for exiting."));
    Serial.println(F("<======HEALTH CHECK======>"));
  }

  switch (stage)
  {
    case 0:
    {
      float t = dht.readTemperature();
      float h = dht.readHumidity();

      if(!started)
      {
        Serial.println(F(""));
        Serial.println(F("<SENSOR TEST>"));
        started=true;

        if (!isnan(t) && !isnan(h))
        {
          Serial.println(F("DHT: OK"));
          dht_s=true;
        }
        else
        Serial.println(F("DHT: FAIL"));

        if (ldrVal >= 0 && ldrVal <= 1023)
        {
          Serial.println(F("LDR: OK"));
          ldr=true;
        }
        else
        Serial.println(F("LDR: FAIL"));

        if (ptemp > -40 && ptemp < 125)
        {
          Serial.println(F("NTC: OK"));
          ntc=true;
        }
        else
        Serial.println(F("NTC: FAIL"));

        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, HIGH);
        digitalWrite(LED_YELLOW, HIGH);
        Serial.println(F("LEDs: OK"));
        disp.set(7);
        disp.display(8888);

        if(millis()-dis_timer>=1000)
        {
          dis_timer=millis();
          disp.clearDisplay();
          ALL_LEDOFF();
        }

        Serial.println(F("7-segment: OK"));
        buzzer.playTone(1000, 100);
        Serial.println(F("Buzzer: OK"));
      }
      break;
    }

    case 1:
    {
      static unsigned long knobPrintTimer=0;

      if(!started)
      {
        Serial.println(F(""));
        Serial.println(F("<KNOB TEST>"));
        startKnob = knobVal;
        started=true;
        knobPrintTimer=millis();
      }

      if(millis()-knobPrintTimer>=300)
      {
        knobPrintTimer=millis();
        Serial.print(F("KNOB: "));
        Serial.println(knobVal);
      }

      if (abs(knobVal - startKnob)>10)
      {
        knob=true;
      }
      break;
    }

    case 2:
    {
      if(!started)
      {
        Serial.println(F(""));
        started=true;
        Serial.println(F("<BUTTON TEST>"));
      }

      if (digitalRead(BUTTON_K1) == LOW && !k1)
      {
        Serial.println(F("K1: OK"));
        k1=true;
      }
      
      if (digitalRead(BUTTON_K2) == LOW && !k2)
      {
        Serial.println(F("K2: OK"));
        k2=true;
      }
      break;
    }

    case 3:
    {
      if(!started)
      {
        started=true;
        Serial.println(F(""));
        Serial.println(F("<IR REMOTE TEST>"));
        ir_timer=millis();
      }

      if (IR.decode())
      {
        if (IR.isReleased())
        {
          switch (IR.keycode)
          {
            case KEY_POWER:
             { 
               irOK[0] = true;
               Serial.println(F("POWER: OK"));
               break;
             }

            case KEY_MENU:
            {  
              irOK[1] = true;
              Serial.println(F("MENU: OK"));
              break;
            }

            case KEY_TEST:
            {
              irOK[2] = true;
              Serial.println(F("TEST: OK"));
              break;
            }

            case KEY_PLUS:
            {
              irOK[3] = true;
              Serial.println(F("PLUS: OK"));
              break;
            }

            case KEY_BACK:
            {
              irOK[4] = true;
              Serial.println(F("BACK: OK"));
              break;
            }

            case KEY_PREV:
            {
              irOK[5] = true;
              Serial.println(F("PREV: OK"));
              break;
            }

            case KEY_PLAY:
            {
              irOK[6] = true;
              Serial.println(F("PLAY: OK"));
              break;
            }  

            case KEY_NEXT:
            {
              irOK[7] = true;
              Serial.println(F("NEXT: OK"));
              break;
            }

            case KEY_ZERO:
            {
              irOK[8] = true;
              Serial.println(F("ZERO: OK"));
              break;
            }

            case KEY_MINUS:
            {
              irOK[9] = true;
              Serial.println(F("MINUS: OK"));
              break;
            }

            case KEY_C:
            {
              irOK[10] = true;
              Serial.println(F("C: OK"));
              break;
            }

            case KEY_ONE:
            {
              irOK[11] = true;
              Serial.println(F("ONE: OK"));
              break;
            }

            case KEY_TWO:
            {
              irOK[12] = true;
              Serial.println(F("TWO: OK"));
              break;
            }

            case KEY_THREE:
            {
              irOK[13] = true;
              Serial.println(F("THREE: OK"));
              break;
            }

            case KEY_FOUR:
            {
              irOK[14] = true;
              Serial.println(F("FOUR: OK"));
              break;
            }

            case KEY_FIVE:
            {
              irOK[15] = true;
              Serial.println(F("FIVE: OK"));
              break;
            }

            case KEY_SIX:
            {
              irOK[16] = true;
              Serial.println(F("SIX: OK"));
              break;
            }

            case KEY_SEVEN:
            {
              irOK[17] = true;
              Serial.println(F("SEVEN: OK"));
              break;
            }

            case KEY_EIGHT:
            {
              irOK[18] = true;
              Serial.println(F("EIGHT: OK"));
              break;
            }

            case KEY_NINE:
            {
              irOK[19] = true;
              Serial.println(F("NINE: OK"));
              break;
            }
          }
        }
        IR.resume();
      }

      bool allIR = true;

      for (int i = 0; i < 20; i++)
      {
        if (!irOK[i])
        {
          allIR=false;
          break;
        }
      }

      if (allIR || millis()-ir_timer>=600000)
      {
        if(!irOK[0]) Serial.println(F("POWER: FAIL"));
        if(!irOK[1]) Serial.println(F("MENU: FAIL"));
        if(!irOK[2]) Serial.println(F("TEST: FAIL"));
        if(!irOK[3]) Serial.println(F("PLUS: FAIL"));
        if(!irOK[4]) Serial.println(F("BACK: FAIL"));
        if(!irOK[5]) Serial.println(F("PREV: FAIL"));
        if(!irOK[6]) Serial.println(F("PLAY: FAIL"));
        if(!irOK[7]) Serial.println(F("NEXT: FAIL"));
        if(!irOK[8]) Serial.println(F("ZERO: FAIL"));
        if(!irOK[9]) Serial.println(F("MINUS: FAIL"));
        if(!irOK[10]) Serial.println(F("C: FAIL"));
        if(!irOK[11]) Serial.println(F("ONE: FAIL"));
        if(!irOK[12]) Serial.println(F("TWO: FAIL"));
        if(!irOK[13]) Serial.println(F("THREE: FAIL"));
        if(!irOK[14]) Serial.println(F("FOUR: FAIL"));
        if(!irOK[15]) Serial.println(F("FIVE: FAIL"));
        if(!irOK[16]) Serial.println(F("SIX: FAIL"));
        if(!irOK[17]) Serial.println(F("SEVEN: FAIL"));
        if(!irOK[18]) Serial.println(F("EIGHT: FAIL"));
        if(!irOK[19]) Serial.println(F("NINE: FAIL"));
        if(allIR) Serial.println(F("All buttons of the remote works remote."));

        if(ldr && k1 && k2 && knob && ntc && dht_s && allIR)
        {
          Serial.println(F(""));
          Serial.println(F("All components are working."));
          Serial.println(F(""));
        }
        Serial.println(F("<==HEALTH CHECK COMPLETE==>"));

        ALL_LEDOFF();

        started = false;
        stage = 0;
        healthMode = false;
        menu = true;
        begin=false;
        knob=false;
        ldr=false;
        k1=false;
        k2=false;
        dht_s=false;
        ntc=false;

        for (int i = 0; i < 20; i++)
        {
          irOK[i] = false;
        }
      }
      break;
    }
  }
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=*/

void readSensors(
    const int sensorPins[3],
    float sensorValues[3],
    const char* sensorLabels[3]
)
{
    sensorValues[0] = dht.readTemperature();
    sensorValues[1] = dht.readHumidity();
    sensorValues[2] = analogRead(sensorPins[2]);

    for (int i = 0; i < 3; i++)
    {
        if (i > 0)
            Serial.print(F("\t"));

        Serial.print(sensorLabels[i]);
        Serial.print(F(":"));
        Serial.print(sensorValues[i]);
    }

    Serial.println();
}

void updateSerialPlotter(bool &fluxMode,bool &menu)
{
  static const int sensorPins[3] = {12, NTC_PIN, LDR_PIN};
  static const char* sensorLabels[3] = {"Temperature","Humidity","LDR"};
  static const unsigned long PLOT_INTERVAL = 2000;
  static unsigned long lastRead = 0;
  static bool once=true;

  float sensorValues[3];

  if(once)
  {
    Serial.println(F("Open serial plotter."));
    Serial.println(F("Press e to exit"));
    once=false;
  }
  if(Serial.available())
  {
    char i=Serial.read();

    if(i=='e' || i=='E')
    {
      once=true;
      fluxMode=false;
      menu=true;
    }
  }
    if (millis() - lastRead >= PLOT_INTERVAL)
    {
        lastRead = millis();

        readSensors(
            sensorPins,
            sensorValues,
            sensorLabels
        );
    }
}