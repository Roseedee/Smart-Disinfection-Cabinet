#include <Arduino.h>

// ===============================
// BUTTON PIN
// ===============================
#define BTN_27 27
#define BTN_26 26
#define BTN_25 25
#define BTN_33 33


void setup()
{
    Serial.begin(115200);

    pinMode(BTN_27, INPUT_PULLUP);
    pinMode(BTN_26, INPUT_PULLUP);
    pinMode(BTN_25, INPUT_PULLUP);
    pinMode(BTN_33, INPUT_PULLUP);

    Serial.println("Button Test Start");
}


void loop()
{
    int b27 = digitalRead(BTN_27);
    int b26 = digitalRead(BTN_26);
    int b25 = digitalRead(BTN_25);
    int b33 = digitalRead(BTN_33);

    Serial.print("GPIO27=");
    Serial.print(b27);

    Serial.print(" | GPIO26=");
    Serial.print(b26);

    Serial.print(" | GPIO25=");
    Serial.print(b25);

    Serial.print(" | GPIO33=");
    Serial.println(b33);

    delay(200);
}