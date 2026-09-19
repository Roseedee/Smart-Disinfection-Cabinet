#include "TaskManager.h"
#include "DisinfectionController.h"
#include "StatusLED.h"


TaskManager::TaskManager(
    DisinfectionController& disinfection,
    StatusLED& statusLED
)
    : _disinfection(disinfection),
      _statusLED(statusLED),
      _hasTask(false),
      _busy(false),
      _busyChanged(false),
      _doorOpen(false),
      _doorPaused(false)
{
}


// =====================================================
// BEGIN
// =====================================================

void TaskManager::begin()
{
    _task.clear();

    _hasTask = false;

    _busy = false;
    _busyChanged = false;

    _doorOpen = false;
    _doorPaused = false;

    _disinfection.stop();
    _statusLED.stopWorking();

    Serial.println("[TASK MANAGER] Ready");
}


// =====================================================
// BUSY
// =====================================================

void TaskManager::setBusy(bool busy)
{
    if (_busy == busy)
        return;

    _busy = busy;
    _busyChanged = true;
}


bool TaskManager::isBusy() const
{
    return _busy;
}


bool TaskManager::hasBusyStateChanged() const
{
    return _busyChanged;
}


void TaskManager::clearBusyStateChanged()
{
    _busyChanged = false;
}


// =====================================================
// DOOR
// =====================================================

void TaskManager::setDoorOpen(
    bool open,
    unsigned long now
)
{
    if (_doorOpen == open)
        return;

    _doorOpen = open;

    if (_doorOpen)
    {
        Serial.println("[TASK MANAGER] DOOR OPEN");

        if (_hasTask)
        {
            if (_task.getStatus() == TaskStatus::RUNNING ||
                _task.getStatus() == TaskStatus::PENDING)
            {
                pauseForDoor(now);
            }
        }
    }
    else
    {
        Serial.println("[TASK MANAGER] DOOR CLOSED");

        if (_doorPaused)
        {
            resumeAfterDoor(now);
        }
    }
}


bool TaskManager::isDoorOpen() const
{
    return _doorOpen;
}


void TaskManager::pauseForDoor(unsigned long now)
{
    (void)now;

    if (!_hasTask)
        return;

    TaskStatus status =
        _task.getStatus();

    if (status != TaskStatus::RUNNING &&
        status != TaskStatus::PENDING)
    {
        return;
    }

    // ปิด Hardware ทันที
    _disinfection.stop();
    _statusLED.stopWorking();

    _task.setStatus(TaskStatus::PAUSED);

    _doorPaused = true;

    // Busy ต้องยังเป็น true
    setBusy(true);

    Serial.println(
        "[TASK MANAGER] PAUSED BY DOOR"
    );
}


void TaskManager::resumeAfterDoor(unsigned long now)
{
    if (!_doorPaused)
        return;

    if (!_hasTask)
    {
        _doorPaused = false;
        return;
    }

    if (_task.getStatus() != TaskStatus::PAUSED)
    {
        _doorPaused = false;
        return;
    }

    // เปิด Hardware กลับตาม Task เดิม
    _disinfection.start(
        _task.getLamp(1),
        _task.getLamp(2),
        _task.getLamp(3),
        _task.getLamp(4)
    );

    _task.setStatus(TaskStatus::RUNNING);

    _statusLED.startWorking(now);

    _doorPaused = false;

    setBusy(true);

    Serial.println(
        "[TASK MANAGER] RESUME AFTER DOOR CLOSED"
    );
}


// =====================================================
// SUBMIT TASK
// =====================================================

bool TaskManager::submit(const Task& task)
{
    if (!task.isValid())
        return false;


    // =================================================
    // FINAL SAFETY GATE
    // =================================================
    // ไม่ว่า Task จะมาจากมือถือหรือหน้าเครื่อง
    // ถ้าประตูเปิด ห้ามรับ Start ใหม่
    // =================================================

    if (_doorOpen)
    {
        Serial.println(
            "[TASK MANAGER] START BLOCKED - DOOR OPEN"
        );

        return false;
    }


    // มีงานอยู่และยัง Busy = รับงานใหม่ไม่ได้
    if (_busy)
    {
        Serial.println(
            "[TASK MANAGER] BUSY"
        );

        return false;
    }


    _task = task;

    _task.setStatus(
        TaskStatus::PENDING
    );

    _hasTask = true;

    setBusy(true);


    Serial.println(
        "[TASK MANAGER] Task submitted"
    );

    return true;
}


// =====================================================
// UPDATE
// =====================================================

void TaskManager::update(unsigned long now)
{
    if (!_hasTask)
        return;


    // =================================================
    // PENDING
    // =================================================

    if (_task.getStatus() == TaskStatus::PENDING)
    {
        // Safety กันอีกชั้น
        if (_doorOpen)
        {
            pauseForDoor(now);
            return;
        }


        Serial.println(
            "[TASK MANAGER] START TASK"
        );

        _disinfection.start(
            _task.getLamp(1),
            _task.getLamp(2),
            _task.getLamp(3),
            _task.getLamp(4)
        );

        _task.setStatus(
            TaskStatus::RUNNING
        );

        _statusLED.startWorking(now);

        setBusy(true);

        Serial.println(
            "[TASK MANAGER] RUNNING"
        );
    }


    // =================================================
    // RUNNING
    // =================================================

    else if (_task.getStatus() == TaskStatus::RUNNING)
    {
        // Timer ถูกควบคุมโดย FrontPanelTask / FirebaseFrontPanelTask
    }


    // =================================================
    // PAUSED
    // =================================================

    else if (_task.getStatus() == TaskStatus::PAUSED)
    {
        // ถ้าเป็น pause จากประตู จะ resume จาก setDoorOpen(false)
    }


    // =================================================
    // FINISHED
    // =================================================

    else if (_task.getStatus() == TaskStatus::FINISHED)
    {
        // จบแล้ว
    }


    // =================================================
    // STOPPED
    // =================================================

    else if (_task.getStatus() == TaskStatus::STOPPED)
    {
        // หยุดแล้ว
    }
}


// =====================================================
// FINISH TASK
// =====================================================

void TaskManager::finishTask()
{
    if (!_hasTask)
        return;

    Serial.println(
        "[TASK MANAGER] FINISH"
    );

    _disinfection.stop();

    _statusLED.finish(millis());

    _task.setStatus(
        TaskStatus::FINISHED
    );

    _doorPaused = false;

    // FINISHED = ไม่ Busy
    setBusy(false);

    Serial.println(
        "[TASK MANAGER] Relay OFF"
    );

    Serial.println(
        "[TASK MANAGER] Motor OFF"
    );

    Serial.println(
        "[TASK MANAGER] Working LED OFF"
    );
}


// =====================================================
// STOP TASK
// =====================================================

void TaskManager::stopTask()
{
    if (!_hasTask)
        return;

    Serial.println(
        "[TASK MANAGER] STOP"
    );

    _disinfection.stop();

    _statusLED.stopWorking();

    _task.setStatus(
        TaskStatus::STOPPED
    );

    // STOPPED = ยัง Busy ตาม logic เดิม
    setBusy(true);

    // สำคัญ: ห้าม door close แล้ว auto resume งานที่ถูก STOP
    _doorPaused = false;

    Serial.println(
        "[TASK MANAGER] Relay OFF"
    );

    Serial.println(
        "[TASK MANAGER] Motor OFF"
    );

    Serial.println(
        "[TASK MANAGER] Working LED OFF"
    );
}


// =====================================================
// CLEAR TASK
// =====================================================

void TaskManager::clearTask()
{
    _disinfection.stop();

    _statusLED.stopWorking();

    _task.clear();

    _hasTask = false;

    _doorPaused = false;

    // CLEAR = ไม่ Busy
    setBusy(false);

    Serial.println(
        "[TASK MANAGER] Task cleared"
    );
}


// =====================================================
// STATUS
// =====================================================

bool TaskManager::hasTask() const
{
    return _hasTask;
}


TaskStatus TaskManager::getStatus() const
{
    if (!_hasTask)
        return TaskStatus::EMPTY;

    return _task.getStatus();
}


const Task& TaskManager::getTask() const
{
    return _task;
}
