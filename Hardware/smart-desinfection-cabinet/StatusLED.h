#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>

class StatusLED
{
public:

    StatusLED(
        uint8_t readyPin,
        uint8_t onlinePin,
        uint8_t workingPin
    );


    void begin();

    void update(
        unsigned long now
    );


    // -------------------------------------------------
    // Basic status
    // -------------------------------------------------

    void setReady(bool state);

    void setOnline(bool state);


    // -------------------------------------------------
    // Network traffic
    // -------------------------------------------------

    void networkActivity(
        unsigned long now
    );


    // -------------------------------------------------
    // Working
    // -------------------------------------------------

    void startWorking(
        unsigned long now
    );

    void stopWorking();


    // -------------------------------------------------
    // Finish animation
    // -------------------------------------------------

    void finish(
        unsigned long now
    );


private:

    uint8_t _readyPin;

    uint8_t _onlinePin;

    uint8_t _workingPin;


    bool _ready;

    bool _online;


    // -------------------------------------------------
    // Network activity
    // -------------------------------------------------

    bool _networkActivity;

    unsigned long _networkActivityUntil;


    // -------------------------------------------------
    // Working
    // -------------------------------------------------

    bool _working;

    bool _workingState;

    unsigned long _lastWorkingBlink;


    // -------------------------------------------------
    // Finish animation
    // -------------------------------------------------

    bool _finishing;

    unsigned long _finishStart;

    unsigned long _lastAnimation;


    int _pattern;

    int _step;


    static const unsigned long WORKING_INTERVAL = 500;

    static const unsigned long FINISH_DURATION = 10000;

    static const unsigned long ANIMATION_SPEED = 100;

    static const unsigned long NETWORK_ACTIVITY_DURATION = 50;


    void updateWorking(
        unsigned long now
    );

    void updateFinish(
        unsigned long now
    );


    void updateNetworkActivity(
        unsigned long now
    );


    void allOff();

    void allOn();
};

#endif