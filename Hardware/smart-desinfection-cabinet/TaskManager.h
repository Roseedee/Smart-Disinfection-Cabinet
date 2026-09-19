#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include "Task.h"

class DisinfectionController;
class StatusLED;


class TaskManager
{
public:

    TaskManager(
        DisinfectionController& disinfection,
        StatusLED& statusLED
    );

    void begin();

    // รับ Task ใหม่
    bool submit(const Task& task);

    // ประมวลผล Task
    void update(unsigned long now);

    // จบ Task
    void finishTask();

    // หยุด Task
    void stopTask();


bool resumeTask(unsigned long now);
    // ยกเลิก/ล้าง Task
    void clearTask();

    // =================================================
    // DOOR SAFETY
    // =================================================

    void setDoorOpen(bool open, unsigned long now);
    bool isDoorOpen() const;

    // =================================================
    // BUSY STATE
    // =================================================

    bool isBusy() const;
    bool hasBusyStateChanged() const;
    void clearBusyStateChanged();

    bool hasTask() const;

    TaskStatus getStatus() const;

    const Task& getTask() const;


private:

    DisinfectionController& _disinfection;
    StatusLED& _statusLED;

    Task _task;

    bool _hasTask;

    // -------------------------------------------------
    // Busy
    // PENDING/RUNNING/PAUSED/STOPPED = true
    // FINISHED/CLEAR/EMPTY = false
    // -------------------------------------------------

    bool _busy;
    bool _busyChanged;

    // -------------------------------------------------
    // Door safety
    // -------------------------------------------------

    bool _doorOpen;
    bool _doorPaused;

    void setBusy(bool busy);

    void pauseForDoor(unsigned long now);
    void resumeAfterDoor(unsigned long now);
};

#endif
