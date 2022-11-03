#include <Arduino.h>
#include <Wire.h>
#include <BMx280I2C.h>

#include "RTClib.h"

RTC_DS1307 rtc;


#include <DHT.h>

#define btn_up 27
#define btn_down 14
#define btn_right 12
#define btn_left 13
#define btn_ok 16

byte btn_pins[] = {27, 14, 12, 13, 16}; // up, down, right, left, ok
#define DHTPIN 4
#define trigPin 5
#define echoPin 17
#define waterPin 32

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


#define I2C_ADDRESS 0x76
BMx280I2C bmx280(I2C_ADDRESS);

//-----------------ERRORS ----------------
#define SENSORS_OKAY 0
#define DHT_ERROR 1
#define TEMP_ERROR 2
#define WTRLVL_ERROR 3
#define RTC_ERROR 4


//----------------- VALUES ------------------

float temp;
float humidity;
float waterLevel;


float readTemperature() {
  if (!bmx280.measure()) return 0;
  do
  {
    delay(100);
  } while (!bmx280.hasValue());
  
  float t= bmx280.getTemperature();
  temp = isnan(t)?temp:t;
  return temp;
}

float readHumidity() {
  float h= dht.readHumidity();
  humidity= isnan(h)?humidity:h;
  
  return humidity;
}

float readWaterLevel(){
int lvl= analogRead(waterPin);

waterLevel= (float)lvl/(float)4095 * 100;

return waterLevel;
}

/*
int readWaterLevel() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 26000);

  double distance = duration /58;
  Serial.println(distance);
  if (distance>0)
  waterLevel=(float) (distance/(float)24.000);
  else {}
  return waterLevel;
}
*/

String getTimeString() {
  DateTime now =  rtc.now();
  String dateString = String(now.day(), DEC) + String("/") + String(now.month(), DEC) + String("/") + String(now.year(), DEC) ;
  String timeString = String(now.hour(), DEC) + String(":") + String(now.minute(), DEC);
  return dateString + " " + timeString;
}

DateTime getTime() {
  return rtc.now();
}



void setRTCTime(int year, int month, int day, int hour, int minute, int second){
rtc.adjust(DateTime(year, month, day, hour, minute, second));
}

byte sensorsBegin() {
  if (!rtc.begin()) return RTC_ERROR;
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  analogReadResolution(12);
  Wire.begin(21,22);
  if (!bmx280.begin()) return TEMP_ERROR;
  bmx280.resetToDefaults();
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  int wtr_lvl = readWaterLevel();
  if (wtr_lvl > 200) return WTRLVL_ERROR;
  for (int i = 0; i < 5; i++){
    pinMode(btn_pins[i], INPUT_PULLUP); 
  }
  dht.begin();

  float h = dht.readHumidity();
  if (isnan(h)) return DHT_ERROR;


  return SENSORS_OKAY;
}
