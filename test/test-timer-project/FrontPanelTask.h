#ifndef FRONT_PANEL_TASK_H
#define FRONT_PANEL_TASK_H

#include <Arduino.h>

#include "Buttons.h"
#include "Timer.h"
#include "Buzzer.h"
#include "Task.h"


class FrontPanelTask
{
public:

    FrontPanelTask(
        Buttons& buttons,
        Timer& timer,
        Buzzer& buzzer
    );


    void begin();

    void update(unsigned long now);


    // -------------------------------------------------
    // Task
    // -------------------------------------------------

    bool hasTask() const;

    Task& getTask();

    void clearTask();


private:

    Buttons& _buttons;
    Timer& _timer;
    Buzzer& _buzzer;

    Task _task;

    bool _previousRunning;
};

#endif