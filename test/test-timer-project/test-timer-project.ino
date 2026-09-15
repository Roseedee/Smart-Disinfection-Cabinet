#include <Arduino.h>
#include <WiFi.h>

#include "Timer.h"
#include "Buttons.h"
#include "TimerDisplay.h"
#include "Buzzer.h"
#include "StatusLED.h"
#include "WiFiTask.h"

#include "DisinfectionController.h"
#include "Task.h"
#include "FrontPanelTask.h"
#include "TaskManager.h"


// =====================================================
// WIFI
// =====================================================

#define WIFI_SSID       "Dee"
#define WIFI_PASSWORD   "20022002"


// =====================================================
// PIN
// =====================================================

// -----------------------------------------------------
// TM1637
// -----------------------------------------------------

#define TM1637_CLK 14
#define TM1637_DIO 13


// -----------------------------------------------------
// Buttons
// -----------------------------------------------------

#define BUTTON_START_STOP 27
#define BUTTON_UP         26
#define BUTTON_DOWN       25
#define BUTTON_SET        33


// -----------------------------------------------------
// Status LED
// -----------------------------------------------------

#define READY_LED         22
#define ONLINE_LED        21
#define WORKING_LED       5


// -----------------------------------------------------
// Buzzer
// -----------------------------------------------------

#define BUZZER_PIN        18


// -----------------------------------------------------
// Disinfection Relay
// Active HIGH
//
// HIGH = ON
// LOW  = OFF
// -----------------------------------------------------

#define LAMP1_PIN         15
#define LAMP2_PIN         4
#define LAMP3_PIN         16
#define LAMP4_PIN         17

#define MOTOR_PIN         19


// =====================================================
// COMPONENTS
// =====================================================

// -----------------------------------------------------
// Status LED
// -----------------------------------------------------

StatusLED statusLED(
    READY_LED,
    ONLINE_LED,
    WORKING_LED
);


// -----------------------------------------------------
// Timer
// -----------------------------------------------------

Timer timer;


// -----------------------------------------------------
// Buttons
// -----------------------------------------------------

Buttons buttons(
    BUTTON_START_STOP,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_SET
);


// -----------------------------------------------------
// Display
// -----------------------------------------------------

TimerDisplay display(
    TM1637_CLK,
    TM1637_DIO
);


// -----------------------------------------------------
// Buzzer
// -----------------------------------------------------

Buzzer buzzer(
    BUZZER_PIN
);


// -----------------------------------------------------
// WiFi
// -----------------------------------------------------

WiFiTask wifiTask(
    WIFI_SSID,
    WIFI_PASSWORD
);


// =====================================================
// DISINFECTION CONTROLLER
// =====================================================

DisinfectionController disinfection(
    LAMP1_PIN,
    LAMP2_PIN,
    LAMP3_PIN,
    LAMP4_PIN,
    MOTOR_PIN
);


// =====================================================
// FRONT PANEL TASK
// =====================================================

FrontPanelTask frontPanel(
    buttons,
    timer,
    buzzer
);


// =====================================================
// TASK MANAGER
// =====================================================

TaskManager taskManager(
    disinfection,
    statusLED
);


// =====================================================
// SYSTEM STATE
// =====================================================

bool systemReady = false;


// =====================================================
// FRONT PANEL TASK SUBMIT STATE
// =====================================================

bool frontPanelTaskSubmitted = false;


// =====================================================
// HARDWARE FORCE STOP STATE
// =====================================================
//
// ใช้สำหรับทดสอบก่อน
//
// เมื่อ FrontPanelTask เปลี่ยนเป็น
// FINISHED หรือ STOPPED
//
// Main จะสั่ง:
// - Relay OFF
// - Motor OFF
// - Working LED OFF
//
// =====================================================

bool hardwareForceStopped = false;


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(100);


    Serial.println();
    Serial.println("==============================");
    Serial.println(" Smart Disinfection Cabinet");
    Serial.println("==============================");


    // =================================================
    // HARDWARE INIT
    // =================================================

    buttons.begin();

    display.begin();

    buzzer.begin();

    timer.begin();

    statusLED.begin();

    disinfection.begin();

    frontPanel.begin();

    taskManager.begin();


    // =================================================
    // SYSTEM NOT READY
    // =================================================

    systemReady = false;

    frontPanelTaskSubmitted = false;

    hardwareForceStopped = false;

    statusLED.setReady(false);

    statusLED.setOnline(false);


    // =================================================
    // WIFI START
    // =================================================

    wifiTask.begin();


    Serial.println("[SYSTEM] Booting...");
    Serial.println("[SYSTEM] Waiting for WiFi result...");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    unsigned long now = millis();


    // =================================================
    // BOOT
    // =================================================

    if (!systemReady)
    {
        // ---------------------------------------------
        // 7SEG BOOT SPINNER
        // ---------------------------------------------

        display.updateLoading(now);


        // ---------------------------------------------
        // WIFI
        // ---------------------------------------------

        wifiTask.update(now);


        // ---------------------------------------------
        // ONLINE LED
        // ---------------------------------------------

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
        else if (wifiTask.isOffline())
        {
            statusLED.setOnline(false);
        }


        // ---------------------------------------------
        // WIFI RESULT
        // ---------------------------------------------

        if (wifiTask.isConnected() ||
            wifiTask.isOffline())
        {
            systemReady = true;

            statusLED.setReady(true);


            Serial.println();
            Serial.println("==============================");
            Serial.println("       SYSTEM READY");
            Serial.println("==============================");


            if (wifiTask.isConnected())
            {
                Serial.println("Network : ONLINE");
            }
            else
            {
                Serial.println("Network : OFFLINE");
            }


            Serial.println();
        }


        // ---------------------------------------------
        // UPDATE OUTPUT SYSTEMS
        // ---------------------------------------------

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
    else if (wifiTask.isOffline())
    {
        statusLED.setOnline(false);
    }


    // =================================================
    // FRONT PANEL TASK
    // =================================================

    frontPanel.update(now);


    // =================================================
    // FRONT PANEL -> TASK MANAGER
    // =================================================

    if (frontPanel.hasTask() &&
        !frontPanelTaskSubmitted)
    {
        if (taskManager.submit(
                frontPanel.getTask()
            ))
        {
            frontPanelTaskSubmitted = true;

            // Task ใหม่ยังไม่ถูก Force Stop
            hardwareForceStopped = false;


            Serial.println(
                "[MAIN] Front Panel Task submitted"
            );
        }
    }


    // =================================================
    // TASK MANAGER
    // =================================================

    taskManager.update(now);


    // =================================================
    // FORCE HARDWARE OFF
    // =================================================
    //
    // ใช้สถานะจาก FrontPanelTask โดยตรง
    //
    // เพราะตอนนี้ FrontPanelTask กับ TaskManager
    // เป็น Task คนละ object
    //
    // FrontPanelTask จะเปลี่ยนเป็น FINISHED/STOPPED
    // แต่ TaskManager อาจยังเห็น RUNNING
    //
    // ดังนั้นช่วงทดสอบ Main จะเป็นตัวสั่ง OFF
    //
    // =================================================

    if (frontPanelTaskSubmitted &&
        !hardwareForceStopped)
    {
        TaskStatus frontPanelStatus =
            frontPanel.getTask().getStatus();


        // ---------------------------------------------
        // TASK FINISHED
        // ---------------------------------------------

        if (frontPanelStatus == TaskStatus::FINISHED)
        {
            Serial.println(
                "[MAIN] Front Panel Task FINISHED"
            );


            // -----------------------------------------
            // FORCE HARDWARE OFF
            // -----------------------------------------

            disinfection.stop();

            statusLED.stopWorking();


            hardwareForceStopped = true;


            Serial.println(
                "[MAIN] FORCE OFF"
            );

            Serial.println(
                "[MAIN] Relay OFF"
            );

            Serial.println(
                "[MAIN] Motor OFF"
            );

            Serial.println(
                "[MAIN] Working LED OFF"
            );
        }


        // ---------------------------------------------
        // TASK STOPPED
        // ---------------------------------------------

        else if (frontPanelStatus == TaskStatus::STOPPED)
        {
            Serial.println(
                "[MAIN] Front Panel Task STOPPED"
            );


            // -----------------------------------------
            // FORCE HARDWARE OFF
            // -----------------------------------------

            disinfection.stop();

            statusLED.stopWorking();


            hardwareForceStopped = true;


            Serial.println(
                "[MAIN] FORCE OFF"
            );

            Serial.println(
                "[MAIN] Relay OFF"
            );

            Serial.println(
                "[MAIN] Motor OFF"
            );

            Serial.println(
                "[MAIN] Working LED OFF"
            );
        }
    }


    // =================================================
    // DISPLAY
    // =================================================

    display.update(timer);


    // =================================================
    // BUZZER
    // =================================================

    buzzer.update(now);


    // =================================================
    // STATUS LED
    // =================================================

    statusLED.update(now);
}