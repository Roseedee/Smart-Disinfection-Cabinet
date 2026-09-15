#include "DisinfectionController.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

DisinfectionController::DisinfectionController(
    uint8_t lamp1Pin,
    uint8_t lamp2Pin,
    uint8_t lamp3Pin,
    uint8_t lamp4Pin,
    uint8_t motorPin
)
{
    _lampPins[0] = lamp1Pin;
    _lampPins[1] = lamp2Pin;
    _lampPins[2] = lamp3Pin;
    _lampPins[3] = lamp4Pin;

    _motorPin = motorPin;

    for (int i = 0; i < 4; i++)
    {
        _lampState[i] = false;
    }

    _motorState = false;
    _running = false;
}


// =====================================================
// BEGIN
// =====================================================

void DisinfectionController::begin()
{
    for (int i = 0; i < 4; i++)
    {
        pinMode(_lampPins[i], OUTPUT);

        _lampState[i] = false;

        digitalWrite(_lampPins[i], LOW);
    }


    pinMode(_motorPin, OUTPUT);

    _motorState = false;

    digitalWrite(_motorPin, LOW);

    _running = false;
}


// =====================================================
// LAMP
// =====================================================

void DisinfectionController::setLamp(
    uint8_t lamp,
    bool state
)
{
    if (lamp < 1 || lamp > 4)
        return;

    uint8_t index = lamp - 1;

    _lampState[index] = state;

    digitalWrite(
        _lampPins[index],
        state ? HIGH : LOW
    );
}


// =====================================================
// SET ALL LAMPS
// =====================================================

void DisinfectionController::setLamps(
    bool lamp1,
    bool lamp2,
    bool lamp3,
    bool lamp4
)
{
    setLamp(1, lamp1);
    setLamp(2, lamp2);
    setLamp(3, lamp3);
    setLamp(4, lamp4);
}


// =====================================================
// ALL LAMPS ON
// =====================================================

void DisinfectionController::allLampsOn()
{
    setLamps(
        true,
        true,
        true,
        true
    );
}


// =====================================================
// ALL LAMPS OFF
// =====================================================

void DisinfectionController::allLampsOff()
{
    setLamps(
        false,
        false,
        false,
        false
    );
}


// =====================================================
// GET LAMP
// =====================================================

bool DisinfectionController::getLamp(uint8_t lamp) const
{
    if (lamp < 1 || lamp > 4)
        return false;

    return _lampState[lamp - 1];
}


// =====================================================
// MOTOR ON
// =====================================================

void DisinfectionController::motorOn()
{
    _motorState = true;

    digitalWrite(
        _motorPin,
        HIGH
    );
}


// =====================================================
// MOTOR OFF
// =====================================================

void DisinfectionController::motorOff()
{
    _motorState = false;

    digitalWrite(
        _motorPin,
        LOW
    );
}


// =====================================================
// MOTOR STATE
// =====================================================

bool DisinfectionController::isMotorOn() const
{
    return _motorState;
}


// =====================================================
// START DISINFECTION
// =====================================================

void DisinfectionController::start(
    bool lamp1,
    bool lamp2,
    bool lamp3,
    bool lamp4
)
{
    setLamps(
        lamp1,
        lamp2,
        lamp3,
        lamp4
    );

    // Motor ทำงานตลอดระหว่างกระบวนการ
    motorOn();

    _running = true;
}


// =====================================================
// STOP DISINFECTION
// =====================================================

void DisinfectionController::stop()
{
    allLampsOff();

    motorOff();

    _running = false;
}


// =====================================================
// RUNNING STATE
// =====================================================

bool DisinfectionController::isRunning() const
{
    return _running;
}