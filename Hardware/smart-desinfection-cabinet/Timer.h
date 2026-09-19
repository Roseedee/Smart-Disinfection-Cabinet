#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>
#include "Buttons.h"


class Timer
{
public:

    enum SetMode
    {
        MINUTE = 0,
        SECOND = 1
    };


    Timer();

    void begin();

    void update(
        unsigned long now,
        Buttons& buttons
    );

    // ประตูเปิด = หยุดการนับชั่วคราว
    // _running จะยังคงเป็น true เพื่อจำสถานะเดิม
    void setSafetyPause(bool paused);

    bool isSafetyPaused() const;

    uint32_t remainingSeconds() const;

    bool isRunning() const;

    SetMode getSetMode() const;

    bool getBlinkState() const;

    bool consumeStartedEvent();
    bool consumeTimeUpEvent();


private:

    uint32_t _remainingSeconds;

    bool _running;

    // Safety pause จาก door switch
    bool _safetyPaused;

    SetMode _setMode;

    bool _blinkState;

    unsigned long _lastSecond;
    unsigned long _lastBlink;


    void countdown();

    void increase();

    void decrease();

    void changeSetMode();

    bool _startedEvent;
    bool _timeUpEvent;
};

#endif
