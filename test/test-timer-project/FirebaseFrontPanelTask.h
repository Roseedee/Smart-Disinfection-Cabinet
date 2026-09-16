#ifndef FIREBASE_FRONT_PANEL_TASK_H
#define FIREBASE_FRONT_PANEL_TASK_H

#include <Arduino.h>

#include "Task.h"

class Buttons;
class Buzzer;
class TaskManager;
class Timer;
class TimerDisplay;

class FirebaseFrontPanelTask
{
public:

    FirebaseFrontPanelTask(
        Buttons& buttons,
        Buzzer& buzzer,
        TaskManager& taskManager,
        Timer& timer,
        TimerDisplay& display
    );


    void begin();

    void update(unsigned long now);


    // -------------------------------------------------
    // Firebase Task
    // -------------------------------------------------

    bool receiveTask(const Task& task);

    void start(unsigned long now);

    void stop();

    void cancel();


    // -------------------------------------------------
    // State
    // -------------------------------------------------

    bool hasTask() const;

    bool isRunning() const;

    uint32_t getDuration() const;

    uint32_t getRemaining() const;

    uint8_t getProgress() const;


    // -------------------------------------------------
    // Events
    // -------------------------------------------------

    bool consumeLocalStateChanged();

    bool consumeFinishedEvent();

    bool consumeCancelledEvent();

    void clearTask();


private:

    void updateTimer(unsigned long now);

    void updateDisplay();


private:

    Buttons& _buttons;
    Buzzer& _buzzer;
    TaskManager& _taskManager;

    Timer& _timer;
    TimerDisplay& _display;


    Task _task;


    bool _hasTask;
    bool _running;


    uint32_t _duration;
    uint32_t _remaining;

    uint8_t _progress;


    unsigned long _lastSecond;


    bool _localStateChanged;

    bool _finishedEvent;
    bool _cancelledEvent;


    static constexpr unsigned long SECOND_INTERVAL = 1000;
};

#endif