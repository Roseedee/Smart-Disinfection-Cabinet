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
#include "FirebaseManager.h"
#include "FirebaseConfig.h"
#include "FirebaseFrontPanelTask.h"



#define DEVICE_SN "AWE416E1W61"

// =====================================================
// WIFI
// =====================================================

#define WIFI_SSID "Dee"
#define WIFI_PASSWORD "20022002"


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
#define BUTTON_UP 26
#define BUTTON_DOWN 25
#define BUTTON_SET 33


// -----------------------------------------------------
// Status LED
// -----------------------------------------------------

#define READY_LED 22
#define ONLINE_LED 21
#define WORKING_LED 5


// -----------------------------------------------------
// Buzzer
// -----------------------------------------------------

#define BUZZER_PIN 18


// -----------------------------------------------------
// Disinfection Relay
// Active HIGH
//
// HIGH = ON
// LOW  = OFF
// -----------------------------------------------------

#define LAMP1_PIN 15
#define LAMP2_PIN 4
#define LAMP3_PIN 16
#define LAMP4_PIN 17

#define MOTOR_PIN 19


// =====================================================
// COMPONENTS
// =====================================================

// -----------------------------------------------------
// Status LED
// -----------------------------------------------------

StatusLED statusLED(
  READY_LED,
  ONLINE_LED,
  WORKING_LED);


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
  BUTTON_SET);


// -----------------------------------------------------
// Display
// -----------------------------------------------------

TimerDisplay display(
  TM1637_CLK,
  TM1637_DIO);


// -----------------------------------------------------
// Buzzer
// -----------------------------------------------------

Buzzer buzzer(
  BUZZER_PIN);


// -----------------------------------------------------
// WiFi
// -----------------------------------------------------

WiFiTask wifiTask(
  WIFI_SSID,
  WIFI_PASSWORD);

FirebaseManager firebaseManager(
  FIREBASE_API_KEY,
  FIREBASE_DATABASE_URL,
  FIREBASE_EMAIL,
  FIREBASE_PASSWORD,
  DEVICE_SN,
  statusLED);

// =====================================================
// DISINFECTION CONTROLLER
// =====================================================

DisinfectionController disinfection(
  LAMP1_PIN,
  LAMP2_PIN,
  LAMP3_PIN,
  LAMP4_PIN,
  MOTOR_PIN);


// =====================================================
// TASK MANAGER
// =====================================================

TaskManager taskManager(
  disinfection,
  statusLED);


// =====================================================
// FRONT PANEL TASK
// =====================================================

FrontPanelTask frontPanel(
  buttons,
  timer,
  buzzer,
  taskManager);



FirebaseFrontPanelTask firebaseFrontPanel(
    buttons,
    buzzer,
    taskManager,
    timer,
    display
);

// =====================================================
// SYSTEM STATE
// =====================================================

bool systemReady = false;
bool firebaseStarted = false;

// =====================================================
// SETUP
// =====================================================

void setup() {
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

  taskManager.begin();

  firebaseFrontPanel.begin();


  // =================================================
  // SYSTEM NOT READY
  // =================================================

  systemReady = false;

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


    // =====================================================
    // BOOT
    // =====================================================

    if (!systemReady)
    {
        display.updateLoading(now);

        wifiTask.update(now);


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


        if (firebaseStarted)
        {
            firebaseManager.update(
                now,
                firebaseFrontPanel
            );
        }


        // -------------------------------------------------
        // ONLINE LED
        // -------------------------------------------------

        if (wifiTask.isConnecting())
        {
            static unsigned long lastNetworkBlink = 0;
            static bool networkBlinkState = false;

            if (
                now - lastNetworkBlink >= 500
            )
            {
                lastNetworkBlink = now;

                networkBlinkState =
                    !networkBlinkState;

                statusLED.setOnline(
                    networkBlinkState
                );
            }
        }
        else if (
            wifiTask.isConnected()
        )
        {
            statusLED.setOnline(true);
        }
        else if (
            wifiTask.isOffline()
        )
        {
            statusLED.setOnline(false);
        }


        // -------------------------------------------------
        // SYSTEM READY
        // -------------------------------------------------

        bool networkReady =
            wifiTask.isConnected() &&
            firebaseManager.isReady();

        bool networkOffline =
            wifiTask.isOffline() ||
            firebaseManager.isTimeout();


        if (
            networkReady ||
            networkOffline
        )
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
            }
            else
            {
                Serial.println(
                    "Network : OFFLINE"
                );
            }

            Serial.println();
        }


        buzzer.update(now);
        statusLED.update(now);

        return;
    }


    // =====================================================
    // WIFI
    // =====================================================

    wifiTask.update(now);


    // =====================================================
    // FIREBASE
    // =====================================================

    if (firebaseStarted)
    {
        firebaseManager.update(
            now,
            firebaseFrontPanel
        );
    }


    // =====================================================
    // ONLINE LED
    // =====================================================

    if (wifiTask.isConnecting())
    {
        static unsigned long lastWiFiBlink = 0;
        static bool wifiBlinkState = false;

        if (
            now - lastWiFiBlink >= 500
        )
        {
            lastWiFiBlink = now;

            wifiBlinkState =
                !wifiBlinkState;

            statusLED.setOnline(
                wifiBlinkState
            );
        }
    }
    else if (
        wifiTask.isConnected()
    )
    {
        statusLED.setOnline(true);
    }
    else
    {
        statusLED.setOnline(false);
    }


    // =====================================================
    // FIREBASE FRONT PANEL
    // =====================================================

    firebaseFrontPanel.update(now);


    // =====================================================
    // NORMAL FRONT PANEL
    // =====================================================

    if (!firebaseFrontPanel.hasTask())
    {
        frontPanel.update(now);
    }


    // =====================================================
    // TASK MANAGER
    // =====================================================

    taskManager.update(now);


    // =====================================================
    // DISPLAY
    // =====================================================

    if (firebaseFrontPanel.hasTask())
    {
        // Firebase Task
        firebaseFrontPanel.update(now);
    }
    else
    {
        // Front Panel Task
        display.update(timer);
    }


    // =====================================================
    // BUZZER
    // =====================================================

    buzzer.update(now);


    // =====================================================
    // STATUS LED
    // =====================================================

    statusLED.update(now);
}