// Fill-in information from your Blynk Template here
#define BLYNK_TEMPLATE_ID           "TMPLXMz5HKUd"
#define BLYNK_DEVICE_NAME           "Tank Monitoring System"

#define BLYNK_FIRMWARE_VERSION        "0.1.0"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#define APP_DEBUG
#define TdsSensorPin 32
#define VREF 3.3    
#define SCOUNT  30

// Uncomment your board, or configure a custom board in Settings.h
//#define USE_WROVER_BOARD
//#define USE_TTGO_T7
//#define USE_ESP32C3_DEV_MODULE
//#define USE_ESP32S2_DEV_KIT

#include "BlynkEdgent.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SOUND_SPEED 0.034
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int analogBuffer[SCOUNT]; 
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;

int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

const int trigPin = 18;
const int echoPin = 19;
const int redled = 2;
const int blueled = 4;
const int min_value = 10;
const int max_value = 57;
const int oneWireBus = 33;
long duration;
float distanceCm;
float percentage;
int inverse_percentage;
int full;
int empty;
int Auto1;
int manual1;
int motoron;
int motoroff;
const int buzzer = 13;
const int relay = 25;
const int Auto = 27;
const int manual = 26;
const int buzzer_mode = 23;
int buttonstate1 = 0;
int buttonstate2 = 0;
int buttonstate3 = 0;
String operation_mode;
String buzzer_status;
String motor_status;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void setup()
{
  Serial.begin(115200);
  sensors.begin();
  pinMode(TdsSensorPin,INPUT);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(13, 0);
  display.println("Smart Water Level");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(45, 10);
  display.println("Monitor");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 30);
  display.println("Connecting to Wi-Fi");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 40);
  display.println("and Server... Please wait");
  display.display(); 

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redled, OUTPUT);
  pinMode(blueled, OUTPUT);
  pinMode(Auto, INPUT);
  pinMode(manual, INPUT);
  pinMode(manual, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  motor_status = "Off";
  motoroff = 1;
  motoron = 0;
  delay(5000);

  BlynkEdgent.begin();
}

void loop() {

  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);

static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperatureC-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
    }
  }
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED/2;
  Serial.print("Distance(cm): ");
  Serial.println(distanceCm);
  if (distanceCm < min_value)
  {
    percentage = 0;
    } 
  else if (distanceCm > max_value)
  {
    percentage = 100; 
    }
   else
   {
    percentage = ((distanceCm - min_value)/(max_value - min_value))*100;
    }   
  Serial.print("Percentage: ");
  Serial.println(percentage);
  inverse_percentage = 100 - percentage;
  Blynk.virtualWrite(V0, inverse_percentage);
  Serial.print(" Inverse Percentage: ");
  Serial.println(inverse_percentage);
  
buttonstate3 = digitalRead(buzzer_mode);
buttonstate1 = digitalRead(Auto);
buttonstate2 = digitalRead(manual);
  if (buttonstate1 == HIGH)
  {
    Serial.println("Auto mode enabled");
    operation_mode = "Auto";
    Auto1 = 1;
    manual1 = 0;
    }
    
   if(buttonstate2 == HIGH)
    {
      Serial.println("Manual mode enabled");
      operation_mode = "Manual";
      manual1 = 1;
      Auto1 = 0;
      }
  
  if (inverse_percentage == 100)
  {
    full = 1;
    empty = 0;
    digitalWrite(redled, HIGH);
    digitalWrite(blueled, LOW);
    if (buttonstate3 == HIGH)
    {
    digitalWrite(buzzer, HIGH);
    }
    else
    {
      digitalWrite(buzzer, LOW);
      }
    if (operation_mode =="Auto")
    {
      digitalWrite(relay, LOW);
      motor_status = "Off";
      motoroff = 1;
      motoron = 0;
      }  
  }
  else if (inverse_percentage == 0)
  {
    full = 0;
    empty = 1;
    digitalWrite(redled, LOW);
    digitalWrite(blueled, HIGH);
    if (buttonstate3 == HIGH)
    {    
    digitalWrite(buzzer, HIGH);
    delay(1000);
    digitalWrite(buzzer, LOW);
    }
    else
    {
      digitalWrite(buzzer, LOW);
      }
    if (operation_mode =="Auto")
    {
      digitalWrite(relay, HIGH);
      motor_status = "On";
      motoron = 1;
      motoroff = 0;
      } 
  }
  else
  {
    full = 0;
    empty = 0;
    digitalWrite(buzzer, LOW);
    }
      
if (buttonstate3 == HIGH)
{
  buzzer_status = "On";
  }
else
{
  buzzer_status = "Off";
  }
   
if (operation_mode == "Manual")
{
  motor_status = "NA";
  }
                   
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 0);
display.println("Water Level: "+String(inverse_percentage)+"%");
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 10);
display.println("Operation Mode:"+operation_mode);
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 20);
display.println("Buzzer Status: "+buzzer_status);
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 30);
display.println("Motor Status: "+motor_status);
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 40);
display.println("Temperature: "+String(temperatureC)+" C");
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 50);
display.println("TDS: "+String(tdsValue)+" PPM");
display.display(); 

Blynk.virtualWrite(V1, full);
Blynk.virtualWrite(V2, empty);
Blynk.virtualWrite(V3, temperatureC);
Blynk.virtualWrite(V4, tdsValue);
Blynk.virtualWrite(V5, Auto1);
Blynk.virtualWrite(V6, manual1);
Blynk.virtualWrite(V7, motoron);
Blynk.virtualWrite(V8, motoroff);  
BlynkEdgent.run();
digitalWrite(redled, LOW);
digitalWrite(blueled, LOW);
delay(1000);
}
