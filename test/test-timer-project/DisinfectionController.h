#ifndef DISINFECTION_CONTROLLER_H
#define DISINFECTION_CONTROLLER_H

#include <Arduino.h>

class DisinfectionController
{
public:

    DisinfectionController(
        uint8_t lamp1Pin,
        uint8_t lamp2Pin,
        uint8_t lamp3Pin,
        uint8_t lamp4Pin,
        uint8_t motorPin
    );

    void begin();

    // -----------------------------
    // Lamps
    // -----------------------------

    void setLamp(uint8_t lamp, bool state);

    void setLamps(
        bool lamp1,
        bool lamp2,
        bool lamp3,
        bool lamp4
    );

    void allLampsOn();
    void allLampsOff();

    bool getLamp(uint8_t lamp) const;


    // -----------------------------
    // Motor
    // -----------------------------

    void motorOn();
    void motorOff();

    bool isMotorOn() const;


    // -----------------------------
    // Disinfection
    // -----------------------------

    void start(
        bool lamp1,
        bool lamp2,
        bool lamp3,
        bool lamp4
    );

    void stop();

    bool isRunning() const;


private:

    uint8_t _lampPins[4];
    uint8_t _motorPin;

    bool _lampState[4];
    bool _motorState;

    bool _running;
};

#endif