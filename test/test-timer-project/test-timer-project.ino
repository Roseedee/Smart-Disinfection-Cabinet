#include <Arduino.h>

#include "Timer.h"
#include "Buttons.h"
#include "TimerDisplay.h"
#include "Buzzer.h"
#include "StatusLED.h"
// =====================================================
// PIN
// =====================================================

// TM1637
#define TM1637_CLK 14
#define TM1637_DIO 13

// Buttons
#define BUTTON_START_STOP 27
#define BUTTON_UP 26
#define BUTTON_DOWN 25
#define BUTTON_SET 33


// =====================================================
// COMPONENTS
// =====================================================

StatusLED statusLED(
  22,  // READY
  21,  // ONLINE
  5    // WORKING
);

Timer timer;

Buttons buttons(
  BUTTON_START_STOP,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_SET);

TimerDisplay display(
  TM1637_CLK,
  TM1637_DIO);


Buzzer buzzer(18);

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);

  buttons.begin();
  display.begin();
  buzzer.begin();
  timer.begin();

  display.update(timer);

  statusLED.begin();

  // ตอนนี้ระบบพร้อม
  statusLED.setReady(true);

  // ตอนนี้ถือว่า Online แล้ว
  statusLED.setOnline(true);

  Serial.println();
  Serial.println("======================");
  Serial.println("    MM:SS TIMER");
  Serial.println("======================");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    unsigned long now = millis();

    buttons.update(now);

    // ปุ่ม
    if (buttons.startPressed() ||
        buttons.upPressed() ||
        buttons.downPressed() ||
        buttons.setPressed()) {

        buzzer.buttonBeep(now);
    }

    // Timer
    timer.update(now, buttons);


    // ================================
    // TIMER START
    // ================================

    if (timer.consumeStartedEvent()) {

        buzzer.startBeep(now);

        // GPIO5 เริ่มกระพริบ
        statusLED.startWorking(now);
    }


    // ================================
    // TIMER TIME UP
    // ================================

    if (timer.consumeTimeUpEvent()) {

        buzzer.timeUp(now);

        // เล่น LED Finish Animation 10 วิ
        statusLED.finish(now);
    }


    // ================================
    // UPDATE
    // ================================

    display.update(timer);

    buzzer.update(now);

    statusLED.update(now);
}