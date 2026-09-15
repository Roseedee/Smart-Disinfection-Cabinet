#include <Arduino.h>

#include "WiFiManager.h"
#include "SegmentDisplay.h"


// =====================================================
// WIFI CONFIG
// =====================================================

const char* WIFI_SSID =
    "Dee";

const char* WIFI_PASSWORD =
    "20022002";


// =====================================================
// LED
// =====================================================

#define LED_CONNECTING  21
#define LED_ONLINE      19
#define LED_OFFLINE      5


// =====================================================
// RELAY
// =====================================================

#define RELAY_PIN       15

#define RELAY_OFF       LOW
#define RELAY_ON        HIGH


// =====================================================
// TM1637
// =====================================================

#define DISPLAY_CLK     14
#define DISPLAY_DIO     13


// =====================================================
// OBJECT
// =====================================================

WiFiManager wifi(
    WIFI_SSID,
    WIFI_PASSWORD,
    15000,
    30000
);


SegmentDisplay display(
    DISPLAY_CLK,
    DISPLAY_DIO
);


// =====================================================
// LED TIMER
// =====================================================

unsigned long ledTimer = 0;

bool ledBlinkState = false;


// =====================================================
// SYSTEM TIMER
// =====================================================

unsigned long systemTimer = 0;


// =====================================================
// FUNCTION
// =====================================================

void setupRelay();

void updateNetworkLED();

void handleSystem();


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(100);


    Serial.println();
    Serial.println("================================");
    Serial.println("SMART DISINFECTION CABINET");
    Serial.println("================================");


    // =================================================
    // RELAY
    // =================================================

    setupRelay();


    // =================================================
    // LED
    // =================================================

    pinMode(
        LED_CONNECTING,
        OUTPUT
    );

    pinMode(
        LED_ONLINE,
        OUTPUT
    );

    pinMode(
        LED_OFFLINE,
        OUTPUT
    );


    digitalWrite(
        LED_CONNECTING,
        LOW
    );

    digitalWrite(
        LED_ONLINE,
        LOW
    );

    digitalWrite(
        LED_OFFLINE,
        LOW
    );


    // =================================================
    // DISPLAY
    // =================================================

    display.begin();


    // =================================================
    // WIFI
    // =================================================

    wifi.begin();


    Serial.println("[SYSTEM] READY");
}


// =====================================================
// LOOP
// =====================================================

unsigned long countTimer = 0;
int numberCount = 0;
void loop()
{
    // =================================================
    // WIFI
    // =================================================

    wifi.update();


    // =================================================
    // DISPLAY
    // =================================================

    if (wifi.isConnecting())
    {
        display.startSpinner();
    }
    else
    {
        display.stopSpinner();
    }


    display.update();


    // =================================================
    // NETWORK LED
    // =================================================

    updateNetworkLED();


    // =================================================
    // MAIN SYSTEM
    // =================================================

    handleSystem();
}


// =====================================================
// RELAY SETUP
// =====================================================

void setupRelay()
{
    pinMode(
        RELAY_PIN,
        OUTPUT
    );


    digitalWrite(
        RELAY_PIN,
        RELAY_OFF
    );


    Serial.println("[RELAY] OFF");
}


// =====================================================
// NETWORK LED
// =====================================================

void updateNetworkLED()
{
    // =================================================
    // CONNECTING
    // =================================================

    if (wifi.isConnecting())
    {
        digitalWrite(
            LED_ONLINE,
            LOW
        );

        digitalWrite(
            LED_OFFLINE,
            LOW
        );


        if (
            millis() - ledTimer >= 300
        )
        {
            ledTimer = millis();

            ledBlinkState =
                !ledBlinkState;


            digitalWrite(
                LED_CONNECTING,
                ledBlinkState
            );
        }

        return;
    }


    // =================================================
    // ONLINE
    // =================================================

    if (wifi.isOnline())
    {
        digitalWrite(
            LED_CONNECTING,
            LOW
        );

        digitalWrite(
            LED_ONLINE,
            HIGH
        );

        digitalWrite(
            LED_OFFLINE,
            LOW
        );

        return;
    }


    // =================================================
    // OFFLINE
    // =================================================

    if (wifi.isOffline())
    {
        digitalWrite(
            LED_CONNECTING,
            LOW
        );

        digitalWrite(
            LED_ONLINE,
            LOW
        );

        digitalWrite(
            LED_OFFLINE,
            HIGH
        );

        return;
    }
}


// =====================================================
// MAIN SYSTEM
// =====================================================

void handleSystem()
{
    // =================================================
    // ทดสอบว่า LOOP ยังทำงาน
    // =================================================

    if (
        millis() - systemTimer >= 1000
    )
    {
        systemTimer = millis();
        display.showNumber(numberCount++);

        Serial.println(
            "[SYSTEM] RUNNING"
        );
    }


    // =================================================
    // ระบบจริงจะใส่ตรงนี้
    // =================================================

    // button.update();

    // timer.update();

    // relay.update();

    // door.update();

    // task.update();
}