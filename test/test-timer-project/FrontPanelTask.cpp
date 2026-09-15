#include "FrontPanelTask.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

FrontPanelTask::FrontPanelTask(
  Buttons& buttons,
  Timer& timer,
  Buzzer& buzzer)
  : _buttons(buttons),
    _timer(timer),
    _buzzer(buzzer),
    _previousRunning(false) {
}


// =====================================================
// BEGIN
// =====================================================

void FrontPanelTask::begin() {
  _task.clear();

  _previousRunning = _timer.isRunning();
}


// =====================================================
// UPDATE
// =====================================================

void FrontPanelTask::update(unsigned long now) {
  // =================================================
  // BUTTONS
  // =================================================

  _buttons.update(now);


  // =================================================
  // BUTTON BEEP
  // =================================================

  if (_buttons.startPressed() || _buttons.upPressed() || _buttons.downPressed() || _buttons.setPressed()) {
    _buzzer.buttonBeep(now);
  }


  // =================================================
  // TIMER
  //
  // ใช้ Timer เดิม
  // =================================================

  _timer.update(
    now,
    _buttons);


  bool running = _timer.isRunning();


  // =================================================
  // TIMER START
  // =================================================

  if (_timer.consumeStartedEvent()) {
    uint32_t duration = _timer.remainingSeconds();


    // ---------------------------------------------
    // สร้าง Task จาก Front Panel
    //
    // Front Panel:
    // Lamp ทั้ง 4 ดวง ON
    // ---------------------------------------------

    _task.create(
      TaskSource::FRONT_PANEL,
      duration);


    // ---------------------------------------------
    // เสียงเริ่มทำงาน
    // ---------------------------------------------

    _buzzer.startBeep(now);


    Serial.println("[FRONT PANEL] Task created");

    Serial.print("[TASK] Duration: ");
    Serial.print(duration);
    Serial.println(" seconds");

    Serial.println("[TASK] Source: FRONT_PANEL");

    Serial.println("[TASK] Lamps: ALL ON");
  }


  // =================================================
  // TIME UP
  //
  // ต้องตรวจ TIME UP ก่อน STOP
  // เพราะ Timer เมื่อถึง 0 จะ running = false
  // =================================================

  bool timeUp = _timer.consumeTimeUpEvent();


  if (timeUp) {
    if (_task.isValid()) {
      _task.setStatus(
        TaskStatus::FINISHED);
    }

    _buzzer.timeUp(now);

    Serial.println("[FRONT PANEL] Task finished");
  } else if (_previousRunning && !running && _task.isValid() && _task.getStatus() == TaskStatus::RUNNING) {
    _task.setStatus(
      TaskStatus::STOPPED);

    Serial.println("[FRONT PANEL] Task stopped");
  }


  // =================================================
  // MANUAL STOP
  //
  // RUNNING -> STOPPED
  //
  // จะทำงานเฉพาะกรณีที่ไม่ได้ TIME UP
  // =================================================

  else if (_previousRunning && !running && _task.isValid() && _task.getStatus() == TaskStatus::RUNNING) {
    _task.setStatus(
      TaskStatus::STOPPED);


    Serial.println("[FRONT PANEL] Task stopped");
  }


  // =================================================
  // SAVE RUNNING STATE
  // =================================================

  _previousRunning = running;
}


// =====================================================
// HAS TASK
// =====================================================

bool FrontPanelTask::hasTask() const {
  return _task.isValid();
}


// =====================================================
// GET TASK
// =====================================================

Task& FrontPanelTask::getTask() {
  return _task;
}


// =====================================================
// CLEAR TASK
// =====================================================

void FrontPanelTask::clearTask() {
  _task.clear();
}