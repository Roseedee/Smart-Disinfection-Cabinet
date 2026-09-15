#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>

#include "Task.h"
#include "DisinfectionController.h"
#include "StatusLED.h"


// =====================================================
// TASK MANAGER
// =====================================================

class TaskManager
{
public:

    TaskManager(
        DisinfectionController& disinfection,
        StatusLED& statusLED
    );


    // -------------------------------------------------
    // Begin
    // -------------------------------------------------

    void begin();


    // -------------------------------------------------
    // Update
    // -------------------------------------------------

    void update(unsigned long now);


    // -------------------------------------------------
    // Submit Task
    // -------------------------------------------------

    bool submit(const Task& task);


    // -------------------------------------------------
    // State
    // -------------------------------------------------

    bool hasTask() const;

    TaskStatus getStatus() const;


private:

    // -------------------------------------------------
    // Hardware
    // -------------------------------------------------

    DisinfectionController& _disinfection;

    StatusLED& _statusLED;


    // -------------------------------------------------
    // Current Task
    // -------------------------------------------------

    Task _task;

    bool _hasTask;


    // -------------------------------------------------
    // Finish protection
    // -------------------------------------------------

    bool _finishHandled;
};


#endif