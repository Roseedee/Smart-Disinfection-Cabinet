#include "SegmentDisplay.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

SegmentDisplay::SegmentDisplay(
    uint8_t clkPin,
    uint8_t dioPin
)
    : _display(clkPin, dioPin)
{
}


// =====================================================
// BEGIN
// =====================================================

void SegmentDisplay::begin()
{
    _display.setBrightness(7);

    _display.clear();

    _spinnerRunning = false;

    _spinnerPosition = 0;

    _spinnerTimer = millis();
}


// =====================================================
// UPDATE
// =====================================================

void SegmentDisplay::update()
{
    if (!_spinnerRunning)
    {
        return;
    }


    if (
        millis() - _spinnerTimer
        < SPINNER_INTERVAL
    )
    {
        return;
    }


    _spinnerTimer = millis();


    // =================================================
    // เลข 0
    // =================================================
    //
    // TM1637Display.h มี SEG_A ถึง SEG_F
    // อยู่แล้ว ไม่ประกาศซ้ำ
    //

    const uint8_t ZERO =
        SEG_A |
        SEG_B |
        SEG_C |
        SEG_D |
        SEG_E |
        SEG_F;


    // =================================================
    // ทั้ง 4 หลักเป็น 0
    // =================================================

    uint8_t segments[4] =
    {
        ZERO,
        ZERO,
        ZERO,
        ZERO
    };


    // =================================================
    // เลือก segment ที่จะดับ
    // =================================================

    uint8_t offSegment = SEG_A;


    switch (_spinnerPosition)
    {
        case 0:
            offSegment = SEG_A;
            break;

        case 1:
            offSegment = SEG_B;
            break;

        case 2:
            offSegment = SEG_C;
            break;

        case 3:
            offSegment = SEG_D;
            break;

        case 4:
            offSegment = SEG_E;
            break;

        case 5:
            offSegment = SEG_F;
            break;
    }


    // =================================================
    // ดับ segment เดียวกันทั้ง 4 digit
    // =================================================

    for (uint8_t i = 0; i < 4; i++)
    {
        segments[i] &= ~offSegment;
    }


    // =================================================
    // แสดง
    // =================================================

    _display.setSegments(segments);


    // =================================================
    // หมุน
    // =================================================

    _spinnerPosition++;

    if (_spinnerPosition >= 6)
    {
        _spinnerPosition = 0;
    }
}


// =====================================================
// CLEAR
// =====================================================

void SegmentDisplay::clear()
{
    _display.clear();
}


// =====================================================
// BRIGHTNESS
// =====================================================

void SegmentDisplay::setBrightness(
    uint8_t brightness
)
{
    if (brightness > 7)
    {
        brightness = 7;
    }

    _display.setBrightness(brightness);
}


// =====================================================
// SHOW NUMBER
// =====================================================

void SegmentDisplay::showNumber(
    int number,
    bool leadingZero
)
{
    _display.showNumberDec(
        number,
        leadingZero
    );
}


// =====================================================
// START SPINNER
// =====================================================

void SegmentDisplay::startSpinner()
{
    if (_spinnerRunning)
    {
        return;
    }


    _spinnerRunning = true;

    _spinnerPosition = 0;

    _spinnerTimer = millis();
}


// =====================================================
// STOP SPINNER
// =====================================================

void SegmentDisplay::stopSpinner()
{
    _spinnerRunning = false;
}


// =====================================================
// SPINNER STATUS
// =====================================================

bool SegmentDisplay::isSpinnerRunning() const
{
    return _spinnerRunning;
}