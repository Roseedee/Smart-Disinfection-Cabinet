#include "Buttons.h"


Buttons::Buttons(
    uint8_t startStopPin,
    uint8_t upPin,
    uint8_t downPin,
    uint8_t setPin
)
{
    _startStopPin = startStopPin;
    _upPin = upPin;
    _downPin = downPin;
    _setPin = setPin;


    _lastStartState = HIGH;
    _lastUpState = HIGH;
    _lastDownState = HIGH;
    _lastSetState = HIGH;


    _startEvent = false;
    _upEvent = false;
    _downEvent = false;
    _setEvent = false;


    _lastStartPress = 0;
    _lastUpPress = 0;
    _lastDownPress = 0;
    _lastSetPress = 0;
}


// =====================================================
// BEGIN
// =====================================================

void Buttons::begin()
{
    pinMode(_startStopPin, INPUT_PULLUP);
    pinMode(_upPin, INPUT_PULLUP);
    pinMode(_downPin, INPUT_PULLUP);
    pinMode(_setPin, INPUT_PULLUP);
}


// =====================================================
// UPDATE
// =====================================================

void Buttons::update(unsigned long now)
{
    _startEvent = false;
    _upEvent = false;
    _downEvent = false;
    _setEvent = false;


    // -------------------------------------------------
    // START / STOP
    // -------------------------------------------------

    bool startState = digitalRead(_startStopPin);

    if (
        startState == LOW &&
        _lastStartState == HIGH &&
        now - _lastStartPress >= DEBOUNCE_TIME
    )
    {
        _startEvent = true;
        _lastStartPress = now;
    }

    _lastStartState = startState;


    // -------------------------------------------------
    // UP
    // -------------------------------------------------

    bool upState = digitalRead(_upPin);

    if (
        upState == LOW &&
        _lastUpState == HIGH &&
        now - _lastUpPress >= DEBOUNCE_TIME
    )
    {
        _upEvent = true;
        _lastUpPress = now;
    }

    _lastUpState = upState;


    // -------------------------------------------------
    // DOWN
    // -------------------------------------------------

    bool downState = digitalRead(_downPin);

    if (
        downState == LOW &&
        _lastDownState == HIGH &&
        now - _lastDownPress >= DEBOUNCE_TIME
    )
    {
        _downEvent = true;
        _lastDownPress = now;
    }

    _lastDownState = downState;


    // -------------------------------------------------
    // SET
    // -------------------------------------------------

    bool setState = digitalRead(_setPin);

    if (
        setState == LOW &&
        _lastSetState == HIGH &&
        now - _lastSetPress >= DEBOUNCE_TIME
    )
    {
        _setEvent = true;
        _lastSetPress = now;
    }

    _lastSetState = setState;
}


// =====================================================
// EVENTS
// =====================================================

bool Buttons::startPressed()
{
    return _startEvent;
}


bool Buttons::upPressed()
{
    return _upEvent;
}


bool Buttons::downPressed()
{
    return _downEvent;
}


bool Buttons::setPressed()
{
    return _setEvent;
}