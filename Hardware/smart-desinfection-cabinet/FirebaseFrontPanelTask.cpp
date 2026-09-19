#include "FirebaseFrontPanelTask.h"

#include "Buttons.h"
#include "Buzzer.h"
#include "TaskManager.h"
#include "Timer.h"
#include "TimerDisplay.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

FirebaseFrontPanelTask::FirebaseFrontPanelTask(
    Buttons& buttons,
    Buzzer& buzzer,
    TaskManager& taskManager,
    Timer& timer,
    TimerDisplay& display
)
    : _buttons(buttons),
      _buzzer(buzzer),
      _taskManager(taskManager),
      _timer(timer),
      _display(display),
      _hasTask(false),
      _running(false),
      _safetyPaused(false),
      _duration(0),
      _remaining(0),
      _progress(0),
      _lastSecond(0),
      _localStateChanged(false),
      _finishedEvent(false),
      _cancelledEvent(false)
{
}


// =====================================================
// BEGIN
// =====================================================

void FirebaseFrontPanelTask::begin()
{
    _task.clear();

    _hasTask = false;
    _running = false;
    _safetyPaused = false;

    _duration = 0;
    _remaining = 0;
    _progress = 0;

    _lastSecond = 0;

    _localStateChanged = false;

    _finishedEvent = false;
    _cancelledEvent = false;
}


// =====================================================
// UPDATE
// =====================================================

void FirebaseFrontPanelTask::update(
    unsigned long now
)
{
    if (!_hasTask)
        return;

    // Door safety: keep the task and remaining time, but do not accept
    // local START/STOP while the door is open.
    if (_safetyPaused) {
        updateDisplay();
        return;
    }


    // -------------------------------------------------
    // Buttons
    // -------------------------------------------------

    _buttons.update(now);


    // -------------------------------------------------
    // START / STOP
    //
    // ใช้เฉพาะปุ่ม START/STOP
    // UP / DOWN / SET ไม่มีผล
    // -------------------------------------------------

    if (_buttons.startPressed())
    {
        _buzzer.buttonBeep(now);


        if (_running)
        {
            stop();


            Serial.println(
                "[FIREBASE PANEL] STOP"
            );
        }
        else
        {
            start(now);


            if (_running)
            {
                _buzzer.startBeep(now);


                Serial.println(
                    "[FIREBASE PANEL] START"
                );
            }
        }
    }


    // -------------------------------------------------
    // Timer
    // -------------------------------------------------

    updateTimer(now);


    // -------------------------------------------------
    // Display
    // -------------------------------------------------

    updateDisplay();
}


void FirebaseFrontPanelTask::setSafetyPause(bool paused)
{
    if (_safetyPaused == paused)
        return;

    _safetyPaused = paused;
    _lastSecond = millis();
    _localStateChanged = true;

    if (paused)
    {
        Serial.println("[FIREBASE PANEL] SAFETY PAUSE - DOOR OPEN");
    }
    else
    {
        Serial.println("[FIREBASE PANEL] SAFETY RESUME - DOOR CLOSED");
    }
}

bool FirebaseFrontPanelTask::isSafetyPaused() const
{
    return _safetyPaused;
}


// =====================================================
// RECEIVE TASK
// =====================================================

bool FirebaseFrontPanelTask::receiveTask(
    const Task& task
)
{
    if (_hasTask)
    {
        Serial.println(
            "[FIREBASE PANEL] Already has task"
        );

        return false;
    }


    if (!task.isValid())
    {
        Serial.println(
            "[FIREBASE PANEL] Invalid task"
        );

        return false;
    }


    _task = task;


    _duration =
        task.getDuration();


    if (_duration == 0)
    {
        Serial.println(
            "[FIREBASE PANEL] Invalid duration"
        );

        _task.clear();

        return false;
    }


    _remaining =
        _duration;


    _progress = 0;


    _running = false;

    _hasTask = true;


    _lastSecond = millis();


    _localStateChanged = false;

    _finishedEvent = false;
    _cancelledEvent = false;


    Serial.println();
    Serial.println(
        "[FIREBASE PANEL] TASK ACCEPTED"
    );


    Serial.print(
        "[FIREBASE PANEL] Duration: "
    );

    Serial.println(
        _duration
    );


    return true;
}


// =====================================================
// START
// =====================================================

void FirebaseFrontPanelTask::start(
    unsigned long now
)
{
    if (!_hasTask)
        return;


    if (_running || _safetyPaused)
        return;


    if (_remaining == 0)
        return;


    // -------------------------------------------------
    // สร้าง Task ชุดเดียวกัน
    // ใช้เวลาที่เหลือ
    // -------------------------------------------------

    Task runTask;


    runTask.create(
        TaskSource::FIREBASE,
        _remaining,
        _task.getLamp(1),
        _task.getLamp(2),
        _task.getLamp(3),
        _task.getLamp(4)
    );


    bool started = false;

    // If the same Firebase task was stopped manually, resume it instead of
    // submitting a second task (which would be rejected as BUSY).
    if (_taskManager.hasTask() &&
        _taskManager.getStatus() == TaskStatus::STOPPED)
    {
        started = _taskManager.resumeTask(now);
    }
    else
    {
        started = _taskManager.submit(runTask);
        if (started) {
            _taskManager.update(now);
        }
    }

    if (!started)
    {
        Serial.println("[FIREBASE PANEL] Start failed");
        return;
    }

    _running = true;


    _lastSecond = now;


    _localStateChanged = true;


    Serial.println(
        "[FIREBASE PANEL] RUNNING"
    );
}


// =====================================================
// STOP
// =====================================================

void FirebaseFrontPanelTask::stop()
{
    if (!_hasTask)
        return;


    if (!_running)
        return;


    // -------------------------------------------------
    // TaskManager หยุด Relay / Motor
    // -------------------------------------------------

    _taskManager.stopTask();


    _running = false;


    _localStateChanged = true;


    Serial.print(
        "[FIREBASE PANEL] PAUSED: "
    );

    Serial.println(
        _remaining
    );
}


// =====================================================
// CANCEL
// =====================================================

void FirebaseFrontPanelTask::cancel()
{
    if (!_hasTask)
        return;


    Serial.println(
        "[FIREBASE PANEL] CANCEL"
    );


    // -------------------------------------------------
    // OFF Hardware
    // -------------------------------------------------

    if (_taskManager.hasTask())
    {
        _taskManager.stopTask();
    }


    _running = false;

    _hasTask = false;


    _cancelledEvent = true;


    _task.clear();


    _duration = 0;
    _remaining = 0;
    _progress = 0;
}


// =====================================================
// TIMER
// =====================================================

void FirebaseFrontPanelTask::updateTimer(
    unsigned long now
)
{
    if (!_running || _safetyPaused)
        return;


    if (
        now - _lastSecond <
        SECOND_INTERVAL
    )
    {
        return;
    }


    _lastSecond +=
        SECOND_INTERVAL;


    // -------------------------------------------------
    // Countdown
    // -------------------------------------------------

    if (_remaining > 0)
    {
        _remaining--;
    }


    // -------------------------------------------------
    // Progress
    // -------------------------------------------------

    if (_duration > 0)
    {
        uint32_t completed =
            _duration - _remaining;


        _progress =
            (completed * 100UL) /
            _duration;


        if (_progress > 100)
        {
            _progress = 100;
        }
    }


    Serial.print(
        "[FIREBASE PANEL] Remaining: "
    );

    Serial.print(
        _remaining
    );


    Serial.print(
        "  Progress: "
    );

    Serial.print(
        _progress
    );

    Serial.println("%");


    // -------------------------------------------------
    // TIME UP
    // -------------------------------------------------

    if (_remaining == 0)
    {
        _running = false;

        _progress = 100;


        _taskManager.finishTask();


        _finishedEvent = true;


        _buzzer.timeUp(now);


        Serial.println(
            "[FIREBASE PANEL] FINISHED"
        );
    }
}


// =====================================================
// DISPLAY
// =====================================================

void FirebaseFrontPanelTask::updateDisplay()
{
    if (!_hasTask)
        return;


    _display.updateSeconds(
        _remaining
    );
}

// =====================================================
// HAS TASK
// =====================================================

bool FirebaseFrontPanelTask::hasTask() const
{
    return _hasTask;
}


// =====================================================
// IS RUNNING
// =====================================================

bool FirebaseFrontPanelTask::isRunning() const
{
    return _running;
}


// =====================================================
// DURATION
// =====================================================

uint32_t FirebaseFrontPanelTask::getDuration() const
{
    return _duration;
}


// =====================================================
// REMAINING
// =====================================================

uint32_t FirebaseFrontPanelTask::getRemaining() const
{
    return _remaining;
}


// =====================================================
// PROGRESS
// =====================================================

uint8_t FirebaseFrontPanelTask::getProgress() const
{
    return _progress;
}


// =====================================================
// LOCAL STATE CHANGED
// =====================================================

bool FirebaseFrontPanelTask::consumeLocalStateChanged()
{
    if (_localStateChanged)
    {
        _localStateChanged = false;

        return true;
    }

    return false;
}


// =====================================================
// FINISHED EVENT
// =====================================================

bool FirebaseFrontPanelTask::consumeFinishedEvent()
{
    if (_finishedEvent)
    {
        _finishedEvent = false;

        return true;
    }

    return false;
}


// =====================================================
// CANCELLED EVENT
// =====================================================

bool FirebaseFrontPanelTask::consumeCancelledEvent()
{
    if (_cancelledEvent)
    {
        _cancelledEvent = false;

        return true;
    }

    return false;
}

void FirebaseFrontPanelTask::clearTask()
{
    _task.clear();

    _hasTask = false;
    _running = false;
    _safetyPaused = false;

    _duration = 0;
    _remaining = 0;
    _progress = 0;

    _localStateChanged = false;
}