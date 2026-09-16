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
  DEVICE_SN);

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

// =====================================================
// LOOP
// =====================================================

void loop() {
  unsigned long now = millis();


  // =================================================
  // BOOT
  // =================================================

  if (!systemReady) {

    // ---------------------------------------------
    // 7SEG BOOT SPINNER
    // ---------------------------------------------

    display.updateLoading(now);


    // ---------------------------------------------
    // WIFI
    // ---------------------------------------------

    wifiTask.update(now);


    // ---------------------------------------------
    // START FIREBASE
    // ---------------------------------------------

    if (wifiTask.isConnected() && !firebaseStarted) {

      firebaseManager.begin();

      firebaseStarted = true;

      Serial.println("[SYSTEM] Firebase starting...");
    }


    // ---------------------------------------------
    // FIREBASE UPDATE
    // ---------------------------------------------

    if (firebaseStarted) {
      firebaseManager.update(now);
    }


    // ---------------------------------------------
    // ONLINE LED
    // WiFi + Firebase
    // ---------------------------------------------

    if (wifiTask.isConnecting()) {

      static unsigned long lastNetworkBlink = 0;
      static bool networkBlinkState = false;

      if (now - lastNetworkBlink >= 500) {

        lastNetworkBlink = now;

        networkBlinkState = !networkBlinkState;

        statusLED.setOnline(networkBlinkState);
      }

    } else if (wifiTask.isConnected()) {

      if (firebaseManager.isReady()) {

        // WiFi + Firebase พร้อม
        statusLED.setOnline(true);

      } else {

        // WiFi พร้อม แต่ Firebase ยังไม่พร้อม
        static unsigned long lastFirebaseBlink = 0;
        static bool firebaseBlinkState = false;

        if (now - lastFirebaseBlink >= 500) {

          lastFirebaseBlink = now;

          firebaseBlinkState = !firebaseBlinkState;

          statusLED.setOnline(firebaseBlinkState);
        }
      }

    } else if (wifiTask.isOffline()) {

      statusLED.setOnline(false);
    }


    // ---------------------------------------------
    // SYSTEM READY
    //
    // ต้องรอ:
    //
    // WiFi + Firebase
    //
    // หรือ
    //
    // WiFi Offline
    //
    // หรือ
    //
    // Firebase Timeout
    // ---------------------------------------------

    bool networkReady =
      wifiTask.isConnected() &&
      firebaseManager.isReady();

    bool networkOffline =
      wifiTask.isOffline() ||
      firebaseManager.isTimeout();


    if (networkReady || networkOffline) {

      systemReady = true;

      statusLED.setReady(true);


      Serial.println();
      Serial.println("==============================");
      Serial.println("       SYSTEM READY");
      Serial.println("==============================");


      if (networkReady) {

        Serial.println("Network : ONLINE");
        Serial.println("Firebase: READY");

      } else {

        Serial.println("Network : OFFLINE");

        if (wifiTask.isOffline()) {
          Serial.println("WiFi    : OFFLINE");
        }

        if (firebaseManager.isTimeout()) {
          Serial.println("Firebase: TIMEOUT");
        }
      }


      Serial.println();
    }


    // ---------------------------------------------
    // OUTPUT SYSTEMS
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
  // FIREBASE
  // =================================================

  if (firebaseStarted) {
    firebaseManager.update(now);
  }


  // =================================================
  // ONLINE LED
  // WiFi + Firebase
  // =================================================

  if (wifiTask.isConnecting()) {

    static unsigned long lastNetworkBlink = 0;
    static bool networkBlinkState = false;

    if (now - lastNetworkBlink >= 500) {

      lastNetworkBlink = now;

      networkBlinkState = !networkBlinkState;

      statusLED.setOnline(networkBlinkState);
    }

  } else if (wifiTask.isConnected()) {

    if (firebaseManager.isReady()) {

      statusLED.setOnline(true);

    } else {

      static unsigned long lastFirebaseBlink = 0;
      static bool firebaseBlinkState = false;

      if (now - lastFirebaseBlink >= 500) {

        lastFirebaseBlink = now;

        firebaseBlinkState = !firebaseBlinkState;

        statusLED.setOnline(firebaseBlinkState);
      }
    }

  } else {

    statusLED.setOnline(false);
  }


  // =================================================
  // FRONT PANEL TASK
  // =================================================

  frontPanel.update(now);


  // =================================================
  // TASK MANAGER
  // =================================================

  taskManager.update(now);


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