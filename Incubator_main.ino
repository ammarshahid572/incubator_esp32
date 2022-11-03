#include "sensors.h"
#include "actuators.h"
#include "display.h"
#include "webApis.h"
#include "soc/rtc_wdt.h"




#define time_for_roll 10000 //time for one roll in ms

float setTemp, setHumid;
long startTimeStamp; //holds time stamp for when incubation started.
bool isIncubating;
byte setRolling;
int value;
byte screenNo = 0; //screen nos: 0 sensor, 1 actuator, 2 WiFi status, 3 set Temp, 4 set Humid, 5 set rolling

void Task_HandleMain( void *pvParameters );
void Task_HandleButtons( void *pvParameters );

void IRAM_ATTR UpPressed();
void IRAM_ATTR DownPressed();
void IRAM_ATTR LeftPressed();
void IRAM_ATTR RightPressed();
void IRAM_ATTR OkPressed();


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  byte SensorError = sensorsBegin();
  
  //attachInterrupt(btn_up, UpPressed, FALLING);
  //attachInterrupt(btn_down, DownPressed, FALLING);
  //attachInterrupt(btn_left, LeftPressed, FALLING);
  //attachInterrupt(btn_right, RightPressed, FALLING);
  //attachInterrupt(btn_ok, OkPressed, FALLING);
  displayBegin();
  Serial.print("Chip ID is ");
  Serial.println(getChipID()); //print chip id

  if (SensorError > 0) {
    Serial.println("Error code" + String(SensorError));
  }
  else Serial.println("Sensors Initialized");
  actuatorsBegin();


  pref.begin("configs", false); //make space in SPI in FLASH to hold config settings

  setTemp = isnan(pref.getFloat("setTemp")) ? 0 : pref.getFloat("setTemp");
  setHumid = isnan(pref.getFloat("setHumid")) ? 0 : pref.getFloat("setHumid");
  setRolling =  pref.getInt("setRoll");


  isIncubating = pref.getBool("incubating");
  startTimeStamp = pref.getLong("timestamp");
  Serial.println("isIncubating: " + String(isIncubating));
  Serial.println("Start Time Stamp: " + String(startTimeStamp));

  connectToWifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);



  Serial.println("SetTemp: " + String(setTemp));

  xTaskCreatePinnedToCore(
    Task_HandleMain
    ,  "HandleMainLoop"
    ,  2000  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL
    ,  1); //core0


  xTaskCreatePinnedToCore(
    Task_HandleButtons
    ,  "HandleButtons"
    ,  3000 // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL
    ,  0); //core0

}

//check readings --takes 1-2 seconds- done once 10 seconds -core 1
//handleDisplay optimal takes -- 1.5 seconds -- core 1
//makeDecision takes --0.1 seconds  - core 1
//synchronize data takes -- 4 seconds- done once a minute -core 0
//buttons should be instant -core0


unsigned long readTimestamp = 0;
unsigned long displayTimestamp = 0;

int rollDuration;
unsigned long rollStartTime;
bool rollingStart = false , rollingComplete = false;
void loop() {}

void Task_HandleMain(void *pvParameters) {

  Serial.print("Main on:");
  Serial.println(xPortGetCoreID());

  while (true) {
    if (millis() - readTimestamp > 10000) {
      if (gotLocalTime()){
      setRTCTime(timeinfo->tm_year +1900, timeinfo->tm_mon + 1,  timeinfo->tm_mday, timeinfo->tm_hour , timeinfo->tm_min ,timeinfo->tm_sec); 
      }
      
      readTemperature();
      readHumidity();
      readWaterLevel();
      readTimestamp = millis();
      

      if (temp < setTemp - 0.5) startHeating();
      else if (temp > setTemp + 0.5) stopHeating();

      if (humidity < setHumid - 2) startHumidity();
      else if (humidity > setHumid + 2) stopHumidity();

    }
    else if (readTimestamp > millis()) readTimestamp = millis();

    if (millis() - displayTimestamp > 1000) { //1 sec refresh rate for display
      handleDisplay();
      displayTimestamp = millis();
    }
    else if (readTimestamp > millis()) readTimestamp = millis();




    //egg roll implementation
    //calculate rolling time and trigger moment
    bool trigger = false;
    if (setRolling > 0 && setRolling < 4) { //logic for every hour
      
      if (getTime().minute() == 0) {
        trigger = true;
        rollDuration = (int)((float)time_for_roll / (float)24) * setRolling;
      }
    }
    else if (setRolling > 3 && setRolling < 7) {
      DateTime now = getTime();
      if (now.minute() == 0 &&  now.hour() % 4 == 0) {
        trigger = true;
        rollDuration = (int)((float)time_for_roll / (float)6) * (setRolling -3);
      }
    }
    else if (setRolling > 6 && setRolling < 9) {
      DateTime now = getTime();
      if (now.minute() == 0 &&  now.hour() % 6 == 0) {
        trigger = true;
        rollDuration = (int)((float)time_for_roll / (float)4) * ((setRolling-7)*2);
      }
    }
    else if (setRolling > 8 && setRolling < 11) {
      DateTime now = getTime();
      if (now.minute() == 0 &&  now.hour() % 12 == 0) {
        trigger = true;
        rollDuration = (int)((float)time_for_roll / (float)2) * (setRolling-9);
      }
    }



    if (trigger && !rollingComplete) { //if current minute is 0 and rolling hasnt been done
      Serial.println("Rolling triggerred");
      if ( !rollingStart) { //if rolling is not started, start rolling
        Serial.println("Started rolling: " + String(getTime().minute()));
        rollStartTime = millis();  //note the time the rolling started
        Serial.println("Starting Roller for " + String(rollDuration) + "ms");
        startRolling();
        rollingStart = true;
      }
      else if (millis() - rollStartTime >= rollDuration) { //if rolling started and its duration is completed, stop rolling
        stopRolling();
        rollingComplete = true;
        rollingStart = false;
      }
    }
    else if (getTime().minute()!= 0) {
      rollingComplete = false;
    }
    vTaskDelay(2);
  }
}


unsigned long prev2;
void Task_HandleButtons(void *pvParameters) {
  (void) pvParameters;

  Serial.print("Handle on:");
  Serial.println(xPortGetCoreID());

  while (true) {

    int button = 0;
    for (int i = 0 ; i < 5 ; i++) {
      if (digitalRead(btn_pins[i]) == LOW) {
        button = btn_pins[i];
        break;
      }
    }
    switch (button) {
      case btn_up: UpPressed(); break;
      case btn_down: DownPressed(); break;
      case btn_left: LeftPressed(); break;
      case btn_right: RightPressed(); break;
      case btn_ok: OkPressed(); break;
    }
    if (button > 0) {
      vTaskDelay(250);
    }


    //attachInterrupt(btn_up, UpPressed, FALLING);
    //attachInterrupt(btn_down, DownPressed, FALLING);
    //attachInterrupt(btn_left, LeftPressed, FALLING);
    //attachInterrupt(btn_right, RightPressed, FALLING);
    //attachInterrupt(btn_ok, OkPressed, FALLING);
    //also handle syncing
    if (millis() - prev2 > 60000) {
      connectToWifi();
      int tmp = screenNo;
      // sync stuff
      screenNo = 7;
      Serial.println("Syncing Data");
      int dayNo = 0;
      if (isIncubating)dayNo = ((getTime().unixtime() - startTimeStamp) / 86400L) + 1;
      uploadSensor(temp, humidity, waterLevel, dayNo);
      getSettings(&setTemp, &setHumid, & setRolling);
      updateWiFiCredentials();
      prev2 = millis();
      screenNo = tmp;
    }
    vTaskDelay(10);
  }
}
void incrementValue() {
  switch (screenNo) {
    case 0:
    case 1:
    case 2:
      break;// do nothing
    case 3:
    case 4:
    case 5:
    case 6:
      value += 1;
  }
}
void decrementValue() {
  switch (screenNo) {
    case 0:
    case 1:
    case 2:
      break;// do nothing
    case 3:
    case 4:
    case 5:
    case 6:
      value -= 1;
  }
}
void saveSettings() {
  rtc_wdt_feed();
  switch (screenNo) {
    case 0:
    case 1:
    case 2:
      break;// do nothing
    case 3:
      setTemp = (float)value / 2.00;
      pref.putFloat("setTemp", setTemp);
      break;
    case 4:
      setHumid = (float)value;
      pref.putFloat("setHumid", setHumid);
      break;
    case 5:
      setRolling = value % 11;
      pref.putInt("setRoll", setRolling);
      break;
    case 6:
      isIncubating = (value % 2 == 1);
      Serial.println("Incubation setting: " + String(isIncubating));
      pref.putBool("incubating", isIncubating);
      if (isIncubating) {
        startTimeStamp = getTime().unixtime();
        pref.putLong("timestamp" , startTimeStamp);
      } break;
  }
  uploadSettings(setTemp, setHumid, setRolling);
}
void handleDisplay() {
  yield();
  int dayNow = 0 ;
  switch (screenNo) {
    case 0:

      if (isIncubating) dayNow = ((getTime().unixtime() - startTimeStamp) / 86400L) + 1;
      displaySensors(getTimeString(), temp, humidity, waterLevel, dayNow);
      break;
    case 1:
      displayActuators(isRolling, isHeating, isHumidifying);
      break;
    case 2:
      displayWifi(WiFi_connected, WiFi_IP);
      break;
    case 3:
      displaySetTemp(value);
      break;
    case 4:
      displaySetHumid(value);
      break;
    case 5:
      displaySetRolling(value);
      break;
    case 6:
      displayIncubatorConfig(value);
      break;
    case 7:
      displaySync();
      break;
  }
}
void IRAM_ATTR UpPressed() {
  Serial.println("Up Pressed");
  rtc_wdt_feed();
  switch (screenNo) {
    case 0:
      screenNo = 2; // move Cursor to Wifi
      break;
    case 1: // move to sensor
      screenNo = 0;
      break;
    case 2: //move to wifi
      screenNo = 1;
      break;
    case 3:
    case 4:
    case 5:
    case 6:
      incrementValue();  //if settings are on increase the value showed on Screen;
      break;
  }
  //detachInterrupt(btn_up);
  //handleDisplay();

}
void IRAM_ATTR DownPressed() {
  Serial.println("Down Pressed");
  rtc_wdt_feed();
  switch (screenNo) {
    case 0:
      screenNo = 1; // move to actuator
      break;
    case 1: //move to wifi
      screenNo = 2;
      break;
    case 2: //move to sensor
      screenNo = 0;
      break;
    case 3:
    case 4:
    case 5:
    case 6:
      decrementValue();  //if settings are on decrease the value showed on Screen;
      break;
  }
  //detachInterrupt(btn_down);
  //handleDisplay();
}
void IRAM_ATTR RightPressed() {
  Serial.println("Right Pressed");
  rtc_wdt_feed();
  switch (screenNo) {
    case 3:
      screenNo = 4; // move to set Humidity
      value = setHumid;
      break;
    case 4: //move to set Rolling
      screenNo = 5;
      value = setRolling;
      break;
    case 5: // move to sensors
      screenNo = 6;
      value = isIncubating;
      break;
    case 6:
      screenNo = 0;
      break;

    case 1:
    case 2:
    case 0:
      value = setTemp * 2;
      screenNo = 3; //move to settings;
      break;
  }
  //detachInterrupt(btn_right);
  //handleDisplay();
}
void IRAM_ATTR LeftPressed() {
  Serial.println("Left Pressed");
  rtc_wdt_feed();
  switch (screenNo) {
    case 3:
      screenNo = 0;
      break;
    case 4:
      value = setTemp * 2;
      screenNo = 3;
      break;
    case 5:
      value = setHumid;
      screenNo = 4;
      break;
    case 6:
      value = setRolling;
      screenNo = 5;
      break;
    case 1:
    case 2:
    case 0:
      screenNo = 6;
      value = isIncubating;
      break;
  }
  //detachInterrupt(btn_left);
  //handleDisplay();

}
void IRAM_ATTR OkPressed() {
  Serial.println("Ok Pressed");
  rtc_wdt_feed();
  switch (screenNo) {
    case 0:
    case 1:
    case 2:
      break;
    case 3:
    case 4:
    case 5:
    case 6:
      saveSettings();
      break;
  }
  //detachInterrupt(btn_ok);
  //handleDisplay();
}
