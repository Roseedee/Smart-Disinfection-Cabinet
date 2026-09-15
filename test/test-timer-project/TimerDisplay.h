#ifndef TIMER_DISPLAY_H
#define TIMER_DISPLAY_H

#include <Arduino.h>
#include <TM1637Display.h>

#include "Timer.h"


class TimerDisplay
{
public:

    TimerDisplay(
        uint8_t clkPin,
        uint8_t dioPin
    );

    void begin();

    void update(const Timer& timer);

    void updateLoading(unsigned long now);

private:

    TM1637Display _display;

    unsigned long _lastLoadingUpdate = 0;
    uint8_t _loadingStep = 0;

    static constexpr unsigned long LOADING_INTERVAL = 120;
};

#endif