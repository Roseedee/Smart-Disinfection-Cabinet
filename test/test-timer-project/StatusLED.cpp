#include "StatusLED.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

StatusLED::StatusLED(uint8_t readyPin,
                     uint8_t onlinePin,
                     uint8_t workingPin)
{
    _readyPin = readyPin;
    _onlinePin = onlinePin;
    _workingPin = workingPin;

    _ready = false;
    _online = false;

    _working = false;
    _workingState = false;
    _lastWorkingBlink = 0;

    _finishing = false;
    _finishStart = 0;
    _lastAnimation = 0;

    _pattern = 0;
    _step = 0;
}


// =====================================================
// BEGIN
// =====================================================

void StatusLED::begin()
{
    pinMode(_readyPin, OUTPUT);
    pinMode(_onlinePin, OUTPUT);
    pinMode(_workingPin, OUTPUT);

    allOff();
}


// =====================================================
// UPDATE
// =====================================================

void StatusLED::update(unsigned long now)
{
    // ถ้ากำลัง Finish Animation
    // ให้ Animation เป็นคนควบคุม LED ทั้งหมด
    if (_finishing) {
        updateFinish(now);
        return;
    }

    // READY
    digitalWrite(
        _readyPin,
        _ready ? HIGH : LOW
    );

    // ONLINE
    digitalWrite(
        _onlinePin,
        _online ? HIGH : LOW
    );

    // WORKING
    if (_working) {
        updateWorking(now);
    }
}


// =====================================================
// READY
// =====================================================

void StatusLED::setReady(bool state)
{
    _ready = state;
}


// =====================================================
// ONLINE
// =====================================================

void StatusLED::setOnline(bool state)
{
    _online = state;
}


// =====================================================
// START WORKING
// =====================================================

void StatusLED::startWorking(unsigned long now)
{
    _working = true;

    _finishing = false;

    _workingState = true;

    _lastWorkingBlink = now;

    digitalWrite(_workingPin, HIGH);

    Serial.println("LED -> WORKING");
}


// =====================================================
// WORKING
// กระพริบทุก 1 วินาที
// =====================================================

void StatusLED::updateWorking(unsigned long now)
{
    if (now - _lastWorkingBlink >= WORKING_INTERVAL) {

        _lastWorkingBlink = now;

        _workingState = !_workingState;

        digitalWrite(
            _workingPin,
            _workingState ? HIGH : LOW
        );
    }
}


// =====================================================
// STOP WORKING
// =====================================================

void StatusLED::stopWorking()
{
    _working = false;

    _workingState = false;

    digitalWrite(_workingPin, LOW);

    Serial.println("LED -> STOP");
}


// =====================================================
// FINISH
// เริ่ม Animation 10 วินาที
// =====================================================

void StatusLED::finish(unsigned long now)
{
    _working = false;

    _finishing = true;

    _finishStart = now;
    _lastAnimation = now;

    _pattern = 0;
    _step = 0;

    allOff();

    Serial.println("LED -> FINISH ANIMATION");
}


// =====================================================
// FINISH UPDATE
// =====================================================

void StatusLED::updateFinish(unsigned long now)
{
    // ครบ 10 วินาที
    if (now - _finishStart >= FINISH_DURATION) {

        _finishing = false;

        allOff();

        Serial.println("LED -> FINISH END");

        return;
    }


    // เปลี่ยน animation ทุก 100ms
    if (now - _lastAnimation < ANIMATION_SPEED) {
        return;
    }

    _lastAnimation = now;


    // =================================================
    // PATTERN 1
    // 22 -> 21 -> 5
    // =================================================

    if (_pattern == 0) {

        allOff();

        if (_step == 0)
            digitalWrite(_readyPin, HIGH);

        if (_step == 1)
            digitalWrite(_onlinePin, HIGH);

        if (_step == 2)
            digitalWrite(_workingPin, HIGH);

        _step++;

        if (_step >= 3) {
            _step = 0;
            _pattern = 1;
        }
    }


    // =================================================
    // PATTERN 2
    // ไปกลับ
    // 22 -> 21 -> 5 -> 21
    // =================================================

    else if (_pattern == 1) {

        allOff();

        if (_step == 0)
            digitalWrite(_readyPin, HIGH);

        if (_step == 1)
            digitalWrite(_onlinePin, HIGH);

        if (_step == 2)
            digitalWrite(_workingPin, HIGH);

        if (_step == 3)
            digitalWrite(_onlinePin, HIGH);

        _step++;

        if (_step >= 4) {
            _step = 0;
            _pattern = 2;
        }
    }


    // =================================================
    // PATTERN 3
    // กระพริบพร้อมกัน
    // =================================================

    else if (_pattern == 2) {

        if (_step % 2 == 0)
            allOn();
        else
            allOff();

        _step++;

        if (_step >= 10) {
            _step = 0;
            _pattern = 3;
        }
    }


    // =================================================
    // PATTERN 4
    // 22 + 5 <-> 21
    // =================================================

    else if (_pattern == 3) {

        allOff();

        if (_step % 2 == 0) {

            digitalWrite(_readyPin, HIGH);
            digitalWrite(_workingPin, HIGH);

        } else {

            digitalWrite(_onlinePin, HIGH);
        }

        _step++;

        if (_step >= 8) {
            _step = 0;
            _pattern = 4;
        }
    }


    // =================================================
    // PATTERN 5
    // กระพริบทีละดวงเร็ว ๆ
    // =================================================

    else if (_pattern == 4) {

        allOff();

        int led = _step % 3;

        if (led == 0)
            digitalWrite(_readyPin, HIGH);

        if (led == 1)
            digitalWrite(_onlinePin, HIGH);

        if (led == 2)
            digitalWrite(_workingPin, HIGH);

        _step++;

        if (_step >= 15) {

            _step = 0;

            // วนกลับไป Pattern 1
            _pattern = 0;
        }
    }
}


// =====================================================
// ALL OFF
// =====================================================

void StatusLED::allOff()
{
    digitalWrite(_readyPin, LOW);
    digitalWrite(_onlinePin, LOW);
    digitalWrite(_workingPin, LOW);
}


// =====================================================
// ALL ON
// =====================================================

void StatusLED::allOn()
{
    digitalWrite(_readyPin, HIGH);
    digitalWrite(_onlinePin, HIGH);
    digitalWrite(_workingPin, HIGH);
}