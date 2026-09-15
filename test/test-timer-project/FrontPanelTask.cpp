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
    _previousRunning(false) {
}


// =====================================================
// UPDATE
// =====================================================

void FrontPanelTask::update(unsigned long now) {
  // -------------------------------------------------
  // Buttons
  // -------------------------------------------------

  _buttons.update(now);


  // -------------------------------------------------
  // Button beep
  // -------------------------------------------------

  if (_buttons.startPressed() || _buttons.upPressed() || _buttons.downPressed() || _buttons.setPressed()) {
    _buzzer.buttonBeep(now);
  }


  // -------------------------------------------------
  // Timer
  // -------------------------------------------------

  _timer.update(now, _buttons);


  bool running = _timer.isRunning();


  // =================================================
  // START
  // =================================================

  if (_timer.consumeStartedEvent()) {
    // ถ้ามี Task เก่าค้างอยู่
    if (_hasTask) {
      Serial.println("[FRONT PANEL] Old task exists");
      return;
    }


    // Front panel ใช้หลอดทั้ง 4
    _task.create(
      TaskSource::FRONT_PANEL,
      _timer.remainingSeconds(),

      true,
      true,
      true,
      true);


    Serial.println("[FRONT PANEL] Task created");


    // ส่ง Task เข้า TaskManager
    if (_taskManager.submit(_task)) {
      _hasTask = true;

      Serial.println("[FRONT PANEL] Task submitted");
    } else {
      Serial.println("[FRONT PANEL] Submit failed");
    }


    // Buzzer start
    _buzzer.startBeep(now);
  }


  // =================================================
  // TIME UP
  // =================================================

  bool timeUp = _timer.consumeTimeUpEvent();


  if (timeUp) {
    Serial.println("[FRONT PANEL] TIME UP");


    // Buzzer alarm
    _buzzer.timeUp(now);


    // ให้ TaskManager จัดการจบงาน
    _taskManager.finishTask();

    _hasTask = false;
    _previousRunning = false;

    return;
  }


  // =================================================
  // MANUAL STOP
  // =================================================

  if (_previousRunning && !running) {
    // ถ้าไม่ได้เป็น TIME UP
    // แปลว่า user กด START/STOP เพื่อหยุด

    if (_taskManager.getStatus() == TaskStatus::RUNNING) {
      Serial.println("[FRONT PANEL] Manual STOP");

      _taskManager.stopTask();

      _hasTask = false;
    }
  }


  _previousRunning = running;
}


// =====================================================
// HAS TASK
// =====================================================

bool FrontPanelTask::hasTask() const {
  return _hasTask;
}


// =====================================================
// GET TASK
// =====================================================

const Task& FrontPanelTask::getTask() const {
  return _task;
}


// =====================================================
// CLEAR
// =====================================================

void FrontPanelTask::clearTask() {
  _task.clear();

  _hasTask = false;

  _previousRunning = false;
}