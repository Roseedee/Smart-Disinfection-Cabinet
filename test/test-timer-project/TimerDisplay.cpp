#include "TimerDisplay.h"


TimerDisplay::TimerDisplay(
    uint8_t clkPin,
    uint8_t dioPin
)
    : _display(clkPin, dioPin)
{
}


// =====================================================
// BEGIN
// =====================================================

void TimerDisplay::begin()
{
    _display.setBrightness(7);

    _display.clear();
}


// =====================================================
// UPDATE
// =====================================================

void TimerDisplay::update(const Timer& timer)
{
    uint32_t totalSeconds = timer.remainingSeconds();

    uint32_t minutes = totalSeconds / 60;
    uint32_t seconds = totalSeconds % 60;


    uint16_t value =
        (minutes * 100) +
        seconds;


    // =================================================
    // RUNNING
    // =================================================

    if (timer.isRunning())
    {
        _display.showNumberDecEx(
            value,
            timer.getBlinkState()
                ? 0b01000000
                : 0,
            true
        );

        return;
    }


    // =================================================
    // SET MODE
    // =================================================

    uint8_t seg[4];


    // -------------------------------------------------
    // MINUTE
    // -------------------------------------------------

    if (timer.getSetMode() == Timer::MINUTE)
    {
        if (timer.getBlinkState())
        {
            seg[0] = _display.encodeDigit(minutes / 10);
            seg[1] = _display.encodeDigit(minutes % 10);
        }
        else
        {
            seg[0] = 0x00;
            seg[1] = 0x00;
        }

        seg[2] = _display.encodeDigit(seconds / 10);
        seg[3] = _display.encodeDigit(seconds % 10);
    }


    // -------------------------------------------------
    // SECOND
    // -------------------------------------------------

    else
    {
        seg[0] = _display.encodeDigit(minutes / 10);
        seg[1] = _display.encodeDigit(minutes % 10);

        if (timer.getBlinkState())
        {
            seg[2] = _display.encodeDigit(seconds / 10);
            seg[3] = _display.encodeDigit(seconds % 10);
        }
        else
        {
            seg[2] = 0x00;
            seg[3] = 0x00;
        }
    }


    _display.setSegments(seg);
}