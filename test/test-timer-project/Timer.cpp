#include "Timer.h"


Timer::Timer()
{
    _remainingSeconds = 0;

    _running = false;

    _setMode = MINUTE;

    _blinkState = true;

    _lastSecond = 0;
    _lastBlink = 0;

    _startedEvent = false;
    _timeUpEvent = false;
}


// =====================================================
// BEGIN
// =====================================================

void Timer::begin()
{
    _remainingSeconds = 0;
    _running = false;

    _setMode = MINUTE;

    _blinkState = true;

    _lastSecond = millis();
    _lastBlink = millis();

    _startedEvent = false;
    _timeUpEvent = false;
}


// =====================================================
// UPDATE
// =====================================================

void Timer::update(
    unsigned long now,
    Buttons& buttons
)
{
    // =================================================
    // START / STOP
    // =================================================

    if (buttons.startPressed())
    {
        if (_remainingSeconds > 0)
        {
            _running = !_running;

            if (_running)
            {
                _startedEvent = true;

                _lastSecond = now;

                _blinkState = true;
                _lastBlink = now;

                Serial.println("START");
            }
            else
            {
                Serial.println("STOP");
            }
        }
    }


    // =================================================
    // SET
    // =================================================

    if (buttons.setPressed())
    {
        if (!_running)
        {
            changeSetMode();

            _blinkState = true;
            _lastBlink = now;

            if (_setMode == MINUTE)
            {
                Serial.println("SET MODE: MINUTE");
            }
            else
            {
                Serial.println("SET MODE: SECOND");
            }
        }
    }


    // =================================================
    // UP
    // =================================================

    if (buttons.upPressed())
    {
        if (!_running)
        {
            increase();

            _blinkState = true;
            _lastBlink = now;
        }
    }


    // =================================================
    // DOWN
    // =================================================

    if (buttons.downPressed())
    {
        if (!_running)
        {
            decrease();

            _blinkState = true;
            _lastBlink = now;
        }
    }


    // =================================================
    // COUNTDOWN
    // =================================================

    if (_running)
    {
        if (now - _lastSecond >= 1000)
        {
            _lastSecond += 1000;

            countdown();
        }
    }


    // =================================================
    // BLINK
    // =================================================

    if (now - _lastBlink >= 500)
    {
        _lastBlink += 500;

        _blinkState = !_blinkState;
    }
}


// =====================================================
// COUNTDOWN
// =====================================================

void Timer::countdown()
{
    if (_remainingSeconds > 0)
    {
        _remainingSeconds--;

        uint32_t minutes = _remainingSeconds / 60;
        uint32_t seconds = _remainingSeconds % 60;

        Serial.print("TIME = ");

        if (minutes < 10)
            Serial.print("0");

        Serial.print(minutes);

        Serial.print(":");

        if (seconds < 10)
            Serial.print("0");

        Serial.println(seconds);
    }


    if (_remainingSeconds == 0)
    {
        _running = false;

        _blinkState = true;

        _timeUpEvent = true;

        Serial.println("TIME UP");
    }
}


// =====================================================
// INCREASE
// =====================================================

void Timer::increase()
{
    uint32_t minutes = _remainingSeconds / 60;
    uint32_t seconds = _remainingSeconds % 60;


    if (_setMode == MINUTE)
    {
        minutes++;

        if (minutes > 99)
            minutes = 0;
    }
    else
    {
        seconds++;

        if (seconds > 59)
            seconds = 0;
    }


    _remainingSeconds = (minutes * 60) + seconds;
}


// =====================================================
// DECREASE
// =====================================================

void Timer::decrease()
{
    uint32_t minutes = _remainingSeconds / 60;
    uint32_t seconds = _remainingSeconds % 60;


    if (_setMode == MINUTE)
    {
        if (minutes == 0)
            minutes = 99;
        else
            minutes--;
    }
    else
    {
        if (seconds == 0)
            seconds = 59;
        else
            seconds--;
    }


    _remainingSeconds = (minutes * 60) + seconds;
}


// =====================================================
// CHANGE SET MODE
// =====================================================

void Timer::changeSetMode()
{
    if (_setMode == MINUTE)
        _setMode = SECOND;
    else
        _setMode = MINUTE;
}


// =====================================================
// GETTERS
// =====================================================

uint32_t Timer::remainingSeconds() const
{
    return _remainingSeconds;
}


bool Timer::isRunning() const
{
    return _running;
}


Timer::SetMode Timer::getSetMode() const
{
    return _setMode;
}


bool Timer::getBlinkState() const
{
    return _blinkState;
}

bool Timer::consumeStartedEvent()
{
    if (_startedEvent)
    {
        _startedEvent = false;
        return true;
    }

    return false;
}


bool Timer::consumeTimeUpEvent()
{
    if (_timeUpEvent)
    {
        _timeUpEvent = false;
        return true;
    }

    return false;
}