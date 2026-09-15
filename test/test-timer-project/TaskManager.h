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

    // ยกเลิก/ล้าง Task
    void clearTask();

    bool hasTask() const;

    TaskStatus getStatus() const;

    const Task& getTask() const;

private:

    DisinfectionController& _disinfection;
    StatusLED& _statusLED;

    Task _task;

    bool _hasTask;
};

#endif