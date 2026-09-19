#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>


class Buzzer
{
public:

    Buzzer(uint8_t pin);

    void begin();

    void update(unsigned long now);

    // เสียงตอนกดปุ่ม
    void buttonBeep(unsigned long now);

    // เสียงตอนเริ่มทำงาน
    void startBeep(unsigned long now);

    // เสียง Timer หมดเวลา
    void timeUp(unsigned long now);


private:

    uint8_t _pin;

    enum Mode
    {
        OFF,
        SHORT_BEEP,
        START_BEEP,
        ALARM
    };

    Mode _mode;

    unsigned long _startTime;
    unsigned long _lastToggle;

    bool _alarmState;


    void on();
    void off();
};

#endif