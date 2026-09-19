#include "FrontPanelTask.h"
#include "Buttons.h"
#include "Timer.h"
#include "Buzzer.h"
#include "TaskManager.h"


FrontPanelTask::FrontPanelTask(
    Buttons& buttons,
    Timer& timer,
    Buzzer& buzzer,
    TaskManager& taskManager)
    : _buttons(buttons),
      _timer(timer),
      _buzzer(buzzer),
      _taskManager(taskManager),
      _hasTask(false),
      _previousRunning(false)
{
}


void FrontPanelTask::update(unsigned long now)
{
    // =================================================
    // BUTTONS
    // =================================================

    _buttons.update(now);


    if (
        _buttons.startPressed() ||
        _buttons.upPressed() ||
        _buttons.downPressed() ||
        _buttons.setPressed()
    )
    {
        _buzzer.buttonBeep(now);
    }


    // =================================================
    // TIMER
    // =================================================

    _timer.update(now, _buttons);

    bool running =
        _timer.isRunning();


    // =================================================
    // START / RESUME
    // =================================================

    if (
        _timer.consumeStartedEvent()
    )
    {
        // -------------------------------------------------
        // มี Task เดิมอยู่
        // ใช้สำหรับ Resume งานที่ STOPPED
        // -------------------------------------------------

        if (_hasTask)
        {
            if (
                _taskManager.resumeTask(now)
            )
            {
                Serial.println(
                    "[FRONT PANEL] Task resumed"
                );
            }
            else
            {
                Serial.println(
                    "[FRONT PANEL] Resume failed"
                );
            }
        }

        // -------------------------------------------------
        // ไม่มี Task เดิม
        // สร้าง Task ใหม่
        // -------------------------------------------------

        else
        {
            _task.create(
                TaskSource::FRONT_PANEL,
                _timer.remainingSeconds(),
                true,
                true,
                true,
                true
            );


            if (
                _taskManager.submit(_task)
            )
            {
                _hasTask = true;

                Serial.println(
                    "[FRONT PANEL] Task submitted"
                );
            }
            else
            {
                Serial.println(
                    "[FRONT PANEL] Submit failed"
                );
            }
        }


        _buzzer.startBeep(now);
    }


    // =================================================
    // TIME UP
    // =================================================

    if (
        _timer.consumeTimeUpEvent()
    )
    {
        Serial.println(
            "[FRONT PANEL] TIME UP"
        );


        _buzzer.timeUp(now);


        // -------------------------------------------------
        // 1. แจ้งว่า Task เสร็จ
        // 2. หยุด Relay / Motor
        // 3. ปล่อย TaskManager
        // -------------------------------------------------

        _taskManager.finishTask();

        _taskManager.clearTask();


        // -------------------------------------------------
        // Front Panel พร้อมรับงานใหม่
        // -------------------------------------------------

        _hasTask = false;
        _previousRunning = false;


        return;
    }


    // =================================================
    // MANUAL STOP
    // =================================================

    if (
        _previousRunning &&
        !running &&
        _taskManager.getStatus() == TaskStatus::RUNNING
    )
    {
        Serial.println(
            "[FRONT PANEL] Manual STOP"
        );


        // STOPPED ยังเก็บ Task ไว้
        // เพื่อให้ START ครั้งต่อไป Resume ได้

        _taskManager.stopTask();
    }


    _previousRunning = running;
}


// =====================================================
// HAS TASK
// =====================================================

bool FrontPanelTask::hasTask() const
{
    return _hasTask;
}


// =====================================================
// GET TASK
// =====================================================

const Task& FrontPanelTask::getTask() const
{
    return _task;
}


// =====================================================
// CLEAR TASK
// =====================================================

void FrontPanelTask::clearTask()
{
    _task.clear();

    _hasTask = false;

    _previousRunning = false;
}