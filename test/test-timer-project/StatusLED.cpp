#include "StatusLED.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

StatusLED::StatusLED(
    uint8_t readyPin,
    uint8_t onlinePin,
    uint8_t workingPin
)
{
    _readyPin = readyPin;

    _onlinePin = onlinePin;

    _workingPin = workingPin;


    _ready = false;

    _online = false;


    // -------------------------------------------------
    // Network activity
    // -------------------------------------------------

    _networkActivity = false;

    _networkActivityUntil = 0;


    // -------------------------------------------------
    // Working
    // -------------------------------------------------

    _working = false;

    _workingState = false;

    _lastWorkingBlink = 0;


    // -------------------------------------------------
    // Finish
    // -------------------------------------------------

    _finishing = false;

    _finishStart = 0;

    _lastAnimation = 0;


    _pattern = 0;

    _step = 0;
}


// =====================================================
// BEGIN
// =====================================================

void StatusLED::begin()
{
    pinMode(
        _readyPin,
        OUTPUT
    );

    pinMode(
        _onlinePin,
        OUTPUT
    );

    pinMode(
        _workingPin,
        OUTPUT
    );


    _networkActivity = false;

    _networkActivityUntil = 0;


    allOff();
}


// =====================================================
// UPDATE
// =====================================================

void StatusLED::update(
    unsigned long now
)
{
    // -------------------------------------------------
    // Finish Animation
    // -------------------------------------------------

    if (_finishing)
    {
        updateFinish(now);

        return;
    }


    // -------------------------------------------------
    // READY
    // -------------------------------------------------

    digitalWrite(
        _readyPin,
        _ready ? HIGH : LOW
    );


    // -------------------------------------------------
    // ONLINE
    // -------------------------------------------------

    updateNetworkActivity(now);


    // -------------------------------------------------
    // WORKING
    // -------------------------------------------------

    if (_working)
    {
        updateWorking(now);
    }
}


// =====================================================
// READY
// =====================================================

void StatusLED::setReady(
    bool state
)
{
    _ready = state;
}


// =====================================================
// ONLINE
// =====================================================

void StatusLED::setOnline(
    bool state
)
{
    _online = state;


    // ถ้าเปลี่ยนเป็น offline
    // ยกเลิก activity pulse
    if (!state)
    {
        _networkActivity = false;

        digitalWrite(
            _onlinePin,
            LOW
        );
    }
}


// =====================================================
// NETWORK ACTIVITY
// =====================================================
//
// ทุกครั้งที่มี TX/RX:
//
// ON
// ↓
// OFF 100ms
// ↓
// ON
//
// ถ้ามี traffic ใหม่ระหว่าง 100ms
// จะต่อเวลาออกไปอีก 100ms จาก traffic ล่าสุด
//
// =====================================================

void StatusLED::networkActivity(
    unsigned long now
)
{
    // ถ้า Offline ก็ไม่ pulse
    if (!_online)
    {
        return;
    }


    _networkActivity = true;

    _networkActivityUntil =
        now + NETWORK_ACTIVITY_DURATION;


    digitalWrite(
        _onlinePin,
        LOW
    );
}


// =====================================================
// UPDATE NETWORK ACTIVITY
// =====================================================

void StatusLED::updateNetworkActivity(
    unsigned long now
)
{
    if (!_online)
    {
        _networkActivity = false;

        digitalWrite(
            _onlinePin,
            LOW
        );

        return;
    }


    // -------------------------------------------------
    // Traffic pulse
    // -------------------------------------------------

    if (_networkActivity)
    {
        // ถ้ายังไม่ครบ 100ms
        if (now < _networkActivityUntil)
        {
            digitalWrite(
                _onlinePin,
                LOW
            );

            return;
        }


        // ครบ 100ms
        _networkActivity = false;


        digitalWrite(
            _onlinePin,
            HIGH
        );


        return;
    }


    // -------------------------------------------------
    // Normal ONLINE
    // -------------------------------------------------

    digitalWrite(
        _onlinePin,
        HIGH
    );
}


// =====================================================
// START WORKING
// =====================================================

void StatusLED::startWorking(
    unsigned long now
)
{
    _working = true;

    _finishing = false;

    _workingState = true;

    _lastWorkingBlink = now;


    digitalWrite(
        _workingPin,
        HIGH
    );


    Serial.println(
        "LED -> WORKING"
    );
}


// =====================================================
// UPDATE WORKING
// =====================================================

void StatusLED::updateWorking(
    unsigned long now
)
{
    if (
        now - _lastWorkingBlink >=
        WORKING_INTERVAL
    )
    {
        _lastWorkingBlink = now;


        _workingState =
            !_workingState;


        digitalWrite(
            _workingPin,
            _workingState
                ? HIGH
                : LOW
        );
    }
}


// =====================================================
// STOP WORKING
// =====================================================

void StatusLED::stopWorking()
{
    _working = false;

    _workingState = false;


    digitalWrite(
        _workingPin,
        LOW
    );


    Serial.println(
        "LED -> STOP"
    );
}


// =====================================================
// FINISH
// =====================================================

void StatusLED::finish(
    unsigned long now
)
{
    _working = false;

    _finishing = true;


    // Network activity ต้องหยุด
    // เพราะ Finish Animation คุม LED ทั้งหมด

    _networkActivity = false;


    _finishStart = now;

    _lastAnimation = now;


    _pattern = 0;

    _step = 0;


    allOff();


    Serial.println(
        "LED -> FINISH ANIMATION"
    );
}


// =====================================================
// UPDATE FINISH
// =====================================================

void StatusLED::updateFinish(
    unsigned long now
)
{
    // -------------------------------------------------
    // ครบ 10 วินาที
    // -------------------------------------------------

    if (
        now - _finishStart >=
        FINISH_DURATION
    )
    {
        _finishing = false;

        allOff();


        Serial.println(
            "LED -> FINISH END"
        );


        return;
    }


    // -------------------------------------------------
    // Animation speed
    // -------------------------------------------------

    if (
        now - _lastAnimation <
        ANIMATION_SPEED
    )
    {
        return;
    }


    _lastAnimation = now;


    // =================================================
    // PATTERN 1
    // 22 -> 21 -> 5
    // =================================================

    if (_pattern == 0)
    {
        allOff();


        if (_step == 0)
        {
            digitalWrite(
                _readyPin,
                HIGH
            );
        }


        if (_step == 1)
        {
            digitalWrite(
                _onlinePin,
                HIGH
            );
        }


        if (_step == 2)
        {
            digitalWrite(
                _workingPin,
                HIGH
            );
        }


        _step++;


        if (_step >= 3)
        {
            _step = 0;

            _pattern = 1;
        }
    }


    // =================================================
    // PATTERN 2
    // 22 -> 21 -> 5 -> 21
    // =================================================

    else if (_pattern == 1)
    {
        allOff();


        if (_step == 0)
        {
            digitalWrite(
                _readyPin,
                HIGH
            );
        }


        if (_step == 1)
        {
            digitalWrite(
                _onlinePin,
                HIGH
            );
        }


        if (_step == 2)
        {
            digitalWrite(
                _workingPin,
                HIGH
            );
        }


        if (_step == 3)
        {
            digitalWrite(
                _onlinePin,
                HIGH
            );
        }


        _step++;


        if (_step >= 4)
        {
            _step = 0;

            _pattern = 2;
        }
    }


    // =================================================
    // PATTERN 3
    // ALL BLINK
    // =================================================

    else if (_pattern == 2)
    {
        if (_step % 2 == 0)
        {
            allOn();
        }
        else
        {
            allOff();
        }


        _step++;


        if (_step >= 10)
        {
            _step = 0;

            _pattern = 3;
        }
    }


    // =================================================
    // PATTERN 4
    // 22 + 5 <-> 21
    // =================================================

    else if (_pattern == 3)
    {
        allOff();


        if (_step % 2 == 0)
        {
            digitalWrite(
                _readyPin,
                HIGH
            );

            digitalWrite(
                _workingPin,
                HIGH
            );
        }
        else
        {
            digitalWrite(
                _onlinePin,
                HIGH
            );
        }


        _step++;


        if (_step >= 8)
        {
            _step = 0;

            _pattern = 4;
        }
    }


    // =================================================
    // PATTERN 5
    // =================================================

    else if (_pattern == 4)
    {
        allOff();


        int led =
            _step % 3;


        if (led == 0)
        {
            digitalWrite(
                _readyPin,
                HIGH
            );
        }


        if (led == 1)
        {
            digitalWrite(
                _onlinePin,
                HIGH
            );
        }


        if (led == 2)
        {
            digitalWrite(
                _workingPin,
                HIGH
            );
        }


        _step++;


        if (_step >= 15)
        {
            _step = 0;

            _pattern = 0;
        }
    }
}


// =====================================================
// ALL OFF
// =====================================================

void StatusLED::allOff()
{
    digitalWrite(
        _readyPin,
        LOW
    );

    digitalWrite(
        _onlinePin,
        LOW
    );

    digitalWrite(
        _workingPin,
        LOW
    );
}


// =====================================================
// ALL ON
// =====================================================

void StatusLED::allOn()
{
    digitalWrite(
        _readyPin,
        HIGH
    );

    digitalWrite(
        _onlinePin,
        HIGH
    );

    digitalWrite(
        _workingPin,
        HIGH
    );
}