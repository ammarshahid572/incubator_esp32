#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <HTTPClient.h>

#include<time.h>

Preferences pref;

const char* server = "http://192.168.1.45:8080/";
const char* ep_UpdateSensor = "device/sensorData";
const char* ep_UpdateSettings = "device/settings";
const char* ep_getSettings = "device/settings/";
const char* ep_getWifi = "device/wifi-creds/";


String ChipID;
HTTPClient client;

WiFiMulti wifiMulti;
bool WiFi_connected;
IPAddress WiFi_IP;


const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 5*3600, daylightOffset_sec=0;
struct tm* timeinfo;

bool gotLocalTime()
{ time_t now = time(nullptr);
  timeinfo=localtime(&now);
  return(timeinfo->tm_year>100);
}

bool uploadSensor(float temp, float humid, float wtrlvl, int dayNo) {
  String host = String(server) + String(ep_UpdateSensor);

  client.begin(host);
  client.addHeader("Content-Type", "application/json");
  Serial.println(host);
  String body = "{\"devID\":\"" + ChipID + "\",\"temp\":\"" + String(temp, 2) + "\",\"humid\":\"" + String(humid, 2) + "\",\"wtrlvl\":\"" + String(wtrlvl, 4) + "\",\"dayNo\":\"" + String(dayNo) + "\"}";
  Serial.println(body);
  int httpcode = client.POST(body);
  return (httpcode == 200);
}

String getChipID() {
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: "); Serial.println(chipId);
  ChipID = String(chipId);
  return ChipID;
}

void getSettings( float *setTemp, float *setHumid, byte *setRoll) {
  client.begin(String(server) + String(ep_getSettings) + ChipID);
  client.addHeader("Content-Type", "application/json");
  int httpcode = client.GET();
  if (httpcode == 200) {
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, client.getString());

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    *setTemp = (float)doc["SETTEMP"];
    *setHumid = (float)doc["SETHUMID"];
    *setRoll = (int) doc["SETROLL"] % 11;

    pref.putInt("setRoll", (int) doc["SETROLL"] % 5);
    pref.putFloat("setHumid", (float)doc["SETHUMID"]);
    pref.putFloat("setTemp", (float)doc["SETTEMP"]);
  }
}

bool uploadSettings(float setTemp, float setHumid, int setRoll) {
  client.begin(String(server) + String(ep_UpdateSettings));
  client.addHeader("Content-Type", "application/json");
  String body="{\"devID\":\"" + ChipID + "\",\"setTemp\":\"" + String(setTemp) + "\",\"setHumid\":\"" + String(setHumid) + "\",\"setRoll\":\"" + String(setRoll) + "\"}"; 
  Serial.println(body);
  int httpcode = client.POST(body);
  
  return (httpcode == 200);
}


void registerChip() {

}

void updateWiFiCredentials() {
  client.begin(String(server) + String(ep_getWifi) + ChipID);
  int httpcode = client.GET();
  if (httpcode == 200) {
    StaticJsonDocument<256> doc;

    DeserializationError error = deserializeJson(doc, client.getString());

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    if (doc["SSID1"] != NULL && doc["PASS1"] != NULL) {
      const char* ssid = doc["SSID1"];
      const char* pass = doc["PASS1"];
      pref.putString("SSID1", String(ssid));
      pref.putString("PASS1", String(pass));
    }
    if (doc["SSID2"] != NULL && doc["PASS2"] != NULL) {
      const char* ssid = doc["SSID2"];
      const char* pass = doc["PASS2"];
      pref.putString("SSID2", String(ssid));
      pref.putString("PASS2", String(pass));
    }
    if (doc["SSID3"] != NULL && doc["PASS3"] != NULL) {
      const char* ssid = doc["SSID3"];
      const char* pass = doc["PASS3"];
      pref.putString("SSID3", String(ssid));
      pref.putString("PASS3", String(pass));
    }
    if (doc["SSID4"] != NULL && doc["PASS4"] != NULL) {
      const char* ssid = doc["SSID4"];
      const char* pass = doc["PASS4"];
      pref.putString("SSID4", String(ssid));
      pref.putString("PASS4", String(pass));
    }
  }
}

bool connectToWifi() {

  wifiMulti.addAP("Incubator", "incubator123");

  if (pref.getString("SSID1") != NULL && pref.getString("PASS1") != NULL)
    wifiMulti.addAP(pref.getString("SSID1").c_str(), pref.getString("PASS1").c_str());

  if (pref.getString("SSID2") != NULL && pref.getString("PASS2") != NULL)
    wifiMulti.addAP(pref.getString("SSID2").c_str(), pref.getString("PASS2").c_str());

  if (pref.getString("SSID3") != NULL && pref.getString("PASS3") != NULL)
    wifiMulti.addAP(pref.getString("SSID3").c_str(), pref.getString("PASS3").c_str());

  if (pref.getString("SSID4") != NULL && pref.getString("PASS4") != NULL)
    wifiMulti.addAP(pref.getString("SSID4").c_str(), pref.getString("PASS4").c_str());


  bool connected = wifiMulti.run() == WL_CONNECTED;
  if (connected) WiFi_connected = true;
  else WiFi_connected = false;
  WiFi_IP = WiFi.localIP();
  return connected;
}
