#include "TaskManager.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

TaskManager::TaskManager(
    DisinfectionController& disinfection,
    StatusLED& statusLED
)
    : _disinfection(disinfection),
      _statusLED(statusLED),
      _hasTask(false),
      _finishHandled(false)
{
}


// =====================================================
// BEGIN
// =====================================================

void TaskManager::begin()
{
    // Clear Task
    _task.clear();

    // ไม่มี Task
    _hasTask = false;

    // ยังไม่มีการ Finish
    _finishHandled = false;
}


// =====================================================
// SUBMIT TASK
// =====================================================

bool TaskManager::submit(const Task& task)
{
    // -------------------------------------------------
    // Task ไม่ valid
    // -------------------------------------------------

    if (!task.isValid())
    {
        return false;
    }


    // -------------------------------------------------
    // ไม่รับ Task ใหม่
    // ถ้ายังมีงานกำลังทำงาน
    // -------------------------------------------------

    if (_hasTask)
    {
        if (_task.getStatus() == TaskStatus::RUNNING ||
            _task.getStatus() == TaskStatus::PAUSED)
        {
            return false;
        }
    }


    // -------------------------------------------------
    // รับ Task
    // -------------------------------------------------

    _task = task;

    _hasTask = true;

    _finishHandled = false;


    Serial.println("[TASK MANAGER] Task submitted");


    return true;
}


// =====================================================
// UPDATE
// =====================================================

void TaskManager::update(unsigned long now)
{
    // -------------------------------------------------
    // ไม่มี Task
    // -------------------------------------------------

    if (!_hasTask)
    {
        return;
    }


    // =================================================
    // PENDING
    // =================================================

    if (_task.getStatus() == TaskStatus::PENDING)
    {
        Serial.println("[TASK MANAGER] Starting task");


        // ---------------------------------------------
        // เปิด Lamp ตาม Task
        // ---------------------------------------------

        _disinfection.start(
            _task.getLamp(1),
            _task.getLamp(2),
            _task.getLamp(3),
            _task.getLamp(4)
        );


        // ---------------------------------------------
        // Task -> RUNNING
        // ---------------------------------------------

        _task.setStatus(
            TaskStatus::RUNNING
        );


        // ---------------------------------------------
        // Working LED
        // ---------------------------------------------

        _statusLED.startWorking(now);


        Serial.println("[TASK MANAGER] Task started");
    }


    // =================================================
    // RUNNING
    // =================================================

    else if (_task.getStatus() == TaskStatus::RUNNING)
    {
        // ---------------------------------------------
        // ไม่ต้องทำอะไร
        //
        // Hardware กำลังทำงาน
        // Timer ถูกจัดการโดย FrontPanelTask
        // ---------------------------------------------
    }


    // =================================================
    // PAUSED
    // =================================================

    else if (_task.getStatus() == TaskStatus::PAUSED)
    {
        // ---------------------------------------------
        // ตอนนี้ยังไม่มี Pause logic
        // ---------------------------------------------
    }


    // =================================================
    // STOPPED
    //
    // ผู้ใช้กด START/STOP
    // =================================================

    else if (_task.getStatus() == TaskStatus::STOPPED)
    {
        Serial.println("[TASK MANAGER] Task stopped");


        // ---------------------------------------------
        // ปิด Lamp
        // ปิด Motor
        // ---------------------------------------------

        _disinfection.stop();


        // ---------------------------------------------
        // ปิด Working LED
        // ---------------------------------------------

        _statusLED.stopWorking();


        // ---------------------------------------------
        // จบการจัดการ Stop
        // ---------------------------------------------

        _finishHandled = true;
    }


    // =================================================
    // FINISHED
    //
    // Timer ถึง 00:00
    // =================================================

    else if (_task.getStatus() == TaskStatus::FINISHED)
    {
        // ---------------------------------------------
        // ทำเพียงครั้งเดียว
        // ---------------------------------------------

        if (!_finishHandled)
        {
            Serial.println("[TASK MANAGER] Task finished");


            // -----------------------------------------
            // ปิด Lamp ทั้งหมด
            // ปิด Motor
            //
            // Active HIGH:
            //
            // HIGH = ON
            // LOW  = OFF
            // -----------------------------------------

            _disinfection.stop();


            // -----------------------------------------
            // ปิด Working LED
            //
            // ไม่เรียก finish()
            //
            // เพราะ finish() คือ Animation
            // และจะทำให้ WORKING LED กลับมาติด
            // -----------------------------------------

            _statusLED.stopWorking();


            // -----------------------------------------
            // Finish handled
            // -----------------------------------------

            _finishHandled = true;


            Serial.println("[TASK MANAGER] Hardware stopped");
            Serial.println("[TASK MANAGER] Working LED OFF");
        }
    }
}


// =====================================================
// HAS TASK
// =====================================================

bool TaskManager::hasTask() const
{
    return _hasTask;
}


// =====================================================
// STATUS
// =====================================================

TaskStatus TaskManager::getStatus() const
{
    if (!_hasTask)
    {
        return TaskStatus::EMPTY;
    }


    return _task.getStatus();
}