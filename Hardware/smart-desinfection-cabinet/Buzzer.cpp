#include "Buzzer.h"


Buzzer::Buzzer(uint8_t pin)
{
    _pin = pin;

    _mode = OFF;

    _startTime = 0;
    _lastToggle = 0;

    _alarmState = false;
}


// =====================================================
// BEGIN
// =====================================================

void Buzzer::begin()
{
    pinMode(_pin, OUTPUT);

    // Active LOW
    // HIGH = OFF
    digitalWrite(_pin, HIGH);

    _mode = OFF;
}


// =====================================================
// UPDATE
// =====================================================

void Buzzer::update(unsigned long now)
{
    // =================================================
    // SHORT BUTTON BEEP
    // =================================================

    if (_mode == SHORT_BEEP)
    {
        if (now - _startTime >= 80)
        {
            off();
            _mode = OFF;
        }

        return;
    }


    // =================================================
    // START BEEP
    // =================================================

    if (_mode == START_BEEP)
    {
        if (now - _startTime >= 2000)
        {
            off();
            _mode = OFF;
        }

        return;
    }


    // =================================================
    // TIME UP ALARM
    // =================================================

    if (_mode == ALARM)
    {
        // รวมเวลา 10 วินาที
        if (now - _startTime >= 10000)
        {
            off();
            _mode = OFF;

            return;
        }


        // 800ms ON / 800ms OFF
        if (now - _lastToggle >= 800)
        {
            _lastToggle = now;

            _alarmState = !_alarmState;

            if (_alarmState)
                on();
            else
                off();
        }
    }
}


// =====================================================
// BUTTON BEEP
// =====================================================

void Buzzer::buttonBeep(unsigned long now)
{
    // ถ้า alarm กำลังทำงานอยู่ ไม่ขัด alarm
    if (_mode == ALARM)
        return;

    _mode = SHORT_BEEP;

    _startTime = now;

    on();
}


// =====================================================
// START BEEP
// =====================================================

void Buzzer::startBeep(unsigned long now)
{
    if (_mode == ALARM)
        return;

    _mode = START_BEEP;

    _startTime = now;

    on();
}


// =====================================================
// TIME UP
// =====================================================

void Buzzer::timeUp(unsigned long now)
{
    _mode = ALARM;

    _startTime = now;
    _lastToggle = now;

    // เริ่มด้วย ON
    _alarmState = true;

    on();
}


// =====================================================
// OUTPUT
// =====================================================

void Buzzer::on()
{
    // Active LOW
    digitalWrite(_pin, LOW);
}


void Buzzer::off()
{
    // Active LOW
    digitalWrite(_pin, HIGH);
}