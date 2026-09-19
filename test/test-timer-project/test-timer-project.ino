#include <Arduino.h>
#include <WiFi.h>

#include "Timer.h"
#include "Buttons.h"
#include "TimerDisplay.h"
#include "Buzzer.h"
#include "StatusLED.h"
#include "WiFiTask.h"
#include "SensorManager.h"

#include "DisinfectionController.h"
#include "Task.h"
#include "FrontPanelTask.h"
#include "TaskManager.h"
#include "FirebaseManager.h"
#include "FirebaseConfig.h"
#include "FirebaseFrontPanelTask.h"


// =====================================================
// DEVICE
// =====================================================

#define DEVICE_SN "AWE416E1W61"


// =====================================================
// WIFI
// =====================================================

#define WIFI_SSID "SDC"
#define WIFI_PASSWORD "00000000"


// =====================================================
// PIN
// =====================================================

#define TM1637_CLK 14
#define TM1637_DIO 13

#define BUTTON_START_STOP 27
#define BUTTON_UP         26
#define BUTTON_DOWN       25
#define BUTTON_SET        33

#define READY_LED   22
#define ONLINE_LED  21
#define WORKING_LED 5

#define BUZZER_PIN 18

#define LAMP1_PIN 15
#define LAMP2_PIN 4
#define LAMP3_PIN 16
#define LAMP4_PIN 17

#define MOTOR_PIN 19


// -----------------------------------------------------
// SENSOR
// -----------------------------------------------------

#define UV_SENSOR_PIN 32
#define DHT11_PIN     23
#define DOOR_SW_PIN   34


// =====================================================
// COMPONENTS
// =====================================================

StatusLED statusLED(
    READY_LED,
    ONLINE_LED,
    WORKING_LED
);

Timer timer;

Buttons buttons(
    BUTTON_START_STOP,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_SET
);

TimerDisplay display(
    TM1637_CLK,
    TM1637_DIO
);

Buzzer buzzer(
    BUZZER_PIN
);

WiFiTask wifiTask(
    WIFI_SSID,
    WIFI_PASSWORD
);

DisinfectionController disinfection(
    LAMP1_PIN,
    LAMP2_PIN,
    LAMP3_PIN,
    LAMP4_PIN,
    MOTOR_PIN
);

TaskManager taskManager(
    disinfection,
    statusLED
);

FrontPanelTask frontPanel(
    buttons,
    timer,
    buzzer,
    taskManager
);

FirebaseFrontPanelTask firebaseFrontPanel(
    buttons,
    buzzer,
    taskManager,
    timer,
    display
);

SensorManager sensors(
    UV_SENSOR_PIN,
    DHT11_PIN,
    DOOR_SW_PIN
);

FirebaseManager firebaseManager(
    FIREBASE_API_KEY,
    FIREBASE_DATABASE_URL,
    FIREBASE_EMAIL,
    FIREBASE_PASSWORD,
    DEVICE_SN,
    statusLED
);


// =====================================================
// SYSTEM STATE
// =====================================================

bool systemReady = false;
bool firebaseStarted = false;


// =====================================================
// DOOR SAFETY
// =====================================================

void updateDoorSafety(
    bool doorOpen,
    unsigned long now
)
{
    // Timer ต้องรู้ก่อนรับคำสั่ง START
    timer.setSafetyPause(doorOpen);

    // TaskManager คือด่านสุดท้ายของ Hardware
    taskManager.setDoorOpen(
        doorOpen,
        now
    );
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(100);


    Serial.println();
    Serial.println(
        "=============================="
    );
    Serial.println(
        " Smart Disinfection Cabinet"
    );
    Serial.println(
        "=============================="
    );


    // =================================================
    // HARDWARE INIT
    // =================================================

    buttons.begin();
    display.begin();
    buzzer.begin();
    timer.begin();
    statusLED.begin();
    disinfection.begin();
    taskManager.begin();
    firebaseFrontPanel.begin();
    sensors.begin();


    // =================================================
    // INITIAL DOOR SAFETY
    // =================================================

    bool initialDoorOpen =
        sensors.isDoorOpen();

    updateDoorSafety(
        initialDoorOpen,
        millis()
    );

    Serial.print("[SYSTEM] Door: ");
    Serial.println(
        initialDoorOpen ? "OPEN" : "CLOSED"
    );


    // =================================================
    // SYSTEM NOT READY
    // =================================================

    systemReady = false;

    statusLED.setReady(false);
    statusLED.setOnline(false);


    // =================================================
    // WIFI
    // =================================================

    wifiTask.begin();


    Serial.println(
        "[SYSTEM] Booting..."
    );

    Serial.println(
        "[SYSTEM] Waiting for WiFi result..."
    );
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    unsigned long now = millis();


    // =================================================
    // SENSOR + DOOR SAFETY
    // =================================================

    sensors.update(now);

    bool doorOpen =
        sensors.isDoorOpen();

    // ต้องทำทุก loop และต้องทำก่อน
    // FirebaseFrontPanelTask / FrontPanelTask
    updateDoorSafety(
        doorOpen,
        now
    );


    // =================================================
    // BOOT
    // =================================================

    if (!systemReady)
    {
        display.updateLoading(now);

        wifiTask.update(now);


        // ---------------------------------------------
        // START FIREBASE
        // ---------------------------------------------

        if (
            wifiTask.isConnected() &&
            !firebaseStarted
        )
        {
            firebaseManager.begin();

            firebaseStarted = true;

            Serial.println(
                "[SYSTEM] Firebase starting..."
            );
        }


        // ---------------------------------------------
        // FIREBASE
        // ---------------------------------------------

        if (firebaseStarted)
        {
            firebaseManager.update(
                now,
                firebaseFrontPanel,
                disinfection,
                taskManager,
                sensors
            );
        }


        // ---------------------------------------------
        // ONLINE LED
        // ---------------------------------------------

        if (wifiTask.isConnecting())
        {
            static unsigned long lastNetworkBlink = 0;
            static bool networkBlinkState = false;

            if (now - lastNetworkBlink >= 500)
            {
                lastNetworkBlink = now;
                networkBlinkState = !networkBlinkState;

                statusLED.setOnline(
                    networkBlinkState
                );
            }
        }
        else if (wifiTask.isConnected())
        {
            statusLED.setOnline(true);
        }
        else
        {
            statusLED.setOnline(false);
        }


        // ---------------------------------------------
        // SYSTEM READY
        // ---------------------------------------------

        bool networkReady =
            wifiTask.isConnected() &&
            firebaseManager.isReady();

        bool networkOffline =
            wifiTask.isOffline() ||
            firebaseManager.isTimeout();

        if (networkReady || networkOffline)
        {
            systemReady = true;

            statusLED.setReady(true);

            Serial.println();
            Serial.println(
                "=============================="
            );
            Serial.println(
                "       SYSTEM READY"
            );
            Serial.println(
                "=============================="
            );

            if (networkReady)
            {
                Serial.println(
                    "Network : ONLINE"
                );

                Serial.println(
                    "Firebase: READY"
                );
            }
            else
            {
                Serial.println(
                    "Network : OFFLINE"
                );

                Serial.println(
                    "Firebase: NOT AVAILABLE"
                );
            }

            Serial.println();
        }


        buzzer.update(now);
        statusLED.update(now);

        return;
    }


    // =================================================
    // WIFI
    // =================================================

    wifiTask.update(now);


    // =================================================
    // ONLINE LED
    // =================================================

    if (wifiTask.isConnecting())
    {
        static unsigned long lastWiFiBlink = 0;
        static bool wifiBlinkState = false;

        if (now - lastWiFiBlink >= 500)
        {
            lastWiFiBlink = now;
            wifiBlinkState = !wifiBlinkState;

            statusLED.setOnline(
                wifiBlinkState
            );
        }
    }
    else if (wifiTask.isConnected())
    {
        statusLED.setOnline(true);
    }
    else
    {
        statusLED.setOnline(false);
    }


    // =================================================
    // FIREBASE FRONT PANEL
    // =================================================
    // Door safety ถูก set แล้วก่อนถึงจุดนี้
    // =================================================

    firebaseFrontPanel.update(now);


    // =================================================
    // NORMAL FRONT PANEL
    // =================================================

    if (!firebaseFrontPanel.hasTask())
    {
        frontPanel.update(now);
    }


    // =================================================
    // TASK MANAGER
    // =================================================

    taskManager.update(now);


    // =================================================
    // FIREBASE
    // =================================================

    if (firebaseStarted)
    {
        firebaseManager.update(
            now,
            firebaseFrontPanel,
            disinfection,
            taskManager,
            sensors
        );
    }


    // =================================================
    // DISPLAY
    // =================================================

    if (!firebaseFrontPanel.hasTask())
    {
        display.update(timer);
    }


    // =================================================
    // OUTPUT
    // =================================================

    buzzer.update(now);
    statusLED.update(now);
}
