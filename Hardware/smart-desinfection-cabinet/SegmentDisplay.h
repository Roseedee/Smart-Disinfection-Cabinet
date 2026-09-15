#pragma once

#include <Arduino.h>
#include <TM1637Display.h>

class SegmentDisplay
{
public:

    SegmentDisplay(
        uint8_t clkPin,
        uint8_t dioPin
    );

    void begin();

    void update();

    void clear();

    void setBrightness(
        uint8_t brightness
    );

    void showNumber(
        int number,
        bool leadingZero = true
    );

    void startSpinner();

    void stopSpinner();

    bool isSpinnerRunning() const;


private:

    TM1637Display _display;

    bool _spinnerRunning = false;

    uint8_t _spinnerPosition = 0;

    unsigned long _spinnerTimer = 0;

    static constexpr unsigned long SPINNER_INTERVAL = 120;
};