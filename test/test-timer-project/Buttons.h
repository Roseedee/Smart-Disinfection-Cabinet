#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>


class Buttons
{
public:

    Buttons(
        uint8_t startStopPin,
        uint8_t upPin,
        uint8_t downPin,
        uint8_t setPin
    );

    void begin();
    void update(unsigned long now);


    bool startPressed();
    bool upPressed();
    bool downPressed();
    bool setPressed();


private:

    uint8_t _startStopPin;
    uint8_t _upPin;
    uint8_t _downPin;
    uint8_t _setPin;


    bool _lastStartState;
    bool _lastUpState;
    bool _lastDownState;
    bool _lastSetState;


    bool _startEvent;
    bool _upEvent;
    bool _downEvent;
    bool _setEvent;


    unsigned long _lastStartPress;
    unsigned long _lastUpPress;
    unsigned long _lastDownPress;
    unsigned long _lastSetPress;


    static const unsigned long DEBOUNCE_TIME = 150;
};

#endif