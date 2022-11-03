
#define trayPin  25
#define heaterPin 26
#define humidPin 33

bool isRolling = false;
bool isHeating = false;
bool isHumidifying = false;


void startRolling() {
  digitalWrite(trayPin, LOW);
  isRolling = true;
}

void stopRolling() {
  digitalWrite(trayPin, HIGH);
  isRolling = false;
}

void startHeating() {
  digitalWrite(heaterPin, LOW);
  isHeating = true;

}

void stopHeating() {
digitalWrite(heaterPin, HIGH);
  
  isHeating = false;
}

void startHumidity() {
  digitalWrite(humidPin, LOW);
  
  isHumidifying = true;
}

void stopHumidity() {
  digitalWrite(humidPin, HIGH);
  
  isHumidifying = false;
}

void actuatorsBegin() {
  pinMode(trayPin , OUTPUT);
  pinMode(heaterPin , OUTPUT);
  pinMode(humidPin , OUTPUT);

  stopRolling();
  stopHeating();
  stopHumidity();
}
