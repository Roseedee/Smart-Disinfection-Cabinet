
// UV Lamp
const uint8_t lamp[4] = {15, 2, 14, 16};

// UV Sensor
const uint8_t uv_sensor[4] = {19, 21, 22, 23};

// Gate Sensor
const uint8_t gate_sensor[3] = {17, 5, 18};


void setPinMode();

void setLamp(int, bool);

void setup() {
  setPinMode();

}

void setPinMode() {
  for(byte i = 0; i < 4; i++) {
    pinMode(lamp[i], OUTPUT);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}

void setLamp(int index, bool state = false) {
  digitalWrite(lamp[index], state);
}
