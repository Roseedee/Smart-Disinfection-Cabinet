#ifndef FRONT_PANEL_TASK_H
#define FRONT_PANEL_TASK_H

#include <Arduino.h>
#include "Task.h"
#include "WiFiTask.h"

class Buttons;
class Timer;
class Buzzer;
class TaskManager;

class FrontPanelTask {
public:

  FrontPanelTask(
    Buttons& buttons,
    Timer& timer,
    Buzzer& buzzer,
    TaskManager& taskManager);

  void update(unsigned long now);



  bool hasTask() const;

  const Task& getTask() const;

  void clearTask();

private:
  Buttons& _buttons;
  Timer& _timer;
  Buzzer& _buzzer;
  TaskManager& _taskManager;

  Task _task;

  bool _hasTask;

  bool _previousRunning;
};

#endif