#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's

String spinLevels[]= {"No Spinning", 
                      "1 rotation per day, 15 degrees every hour ",
                      "2 rotations per day, 30 degrees every hour",
                      "3 rotations per day, 60 degrees every hour",
                      "60 degree every 4 hours",
                      "120 degree every 4 hours",
                      "180 degree every 4 hours",
                      "180 degree every 6 hours",
                      "360 degree every 6 hours",
                      "180 degree every 12 hours",
                      "360 degree every 12 hours"
                      };


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void displayBegin() {
  Serial.println("Starting Display");
  Wire.begin(21, 22);
  delay(250);
  display.begin(i2c_Address, true);
  display.display();
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.println("Starting ... ");
  display.display();
}

void displaySensors( String timeString, float temp, float humid, float water, int day) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(timeString); // write something to the internal memory
  display.print("Temperature: ");
  display.println(String(String(temp, 3) + "C"));
  display.print("Humidity : ");
  display.println(String(String(humid, 2) + "%"));
  display.print("Water Level :");
  display.println(String(String(water, 2) + "%"));
  display.print("Day : ");
  display.println(day);
  display.display();


}

void displayIncubatorConfig(int value){
display.clearDisplay();
display.setCursor(0,0);
display.println("Incubatation Status");
display.print((value%2==1)?"Started":"Stopped");
display.display();
}

void displayActuators ( bool isRolling, bool isHeating, bool isHumidifying) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Rollers: ");
  display.println(isRolling ? "ON" : "OFF");

  display.print("Heater: ");
  display.println(isHeating ? "ON" : "OFF");

  display.print("Humidifier: ");
  display.println(isHumidifying ? "ON" : "OFF");
  display.display();
}

void displayWifi(bool wifiStatus, IPAddress WiFi_IP) {
  display.clearDisplay();
  display.setCursor(0, 0);
  if (wifiStatus == true)
    display.println("Wifi is connected");
  else
    display.println("Wifi is not connected");

  display.println(WiFi_IP);
  display.display();
}
void displaySetTemp(int value) {
  String valueString = String((float)value/(float)2);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Set Temperature");

  display.print(valueString);
  display.display();          // transfer internal memory to the display


}
void displaySetHumid(int value) {
  String valueString = String(value);
  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("Set Humidity");

  display.print(valueString);
  display.display();          // transfer internal memory to the display


}
void displaySetRolling(int value) {
 display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Set Rolling");
  display.print(spinLevels[value%11]);
  display.display();          // transfer internal memory to the display

  yield();
  Wire.beginTransmission(0x20);
  byte error = Wire.endTransmission(); //just try to clean up stuff before display curropts any readings

}

void displaySync(){
  
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Synchronizing... Please Wait.");
  display.display();
  Wire.beginTransmission(0x20);
  byte error = Wire.endTransmission(); //just try to clean up stuff before display curropts any readings
}
