#include "TaskManager.h"
#include "DisinfectionController.h"
#include "StatusLED.h"


TaskManager::TaskManager(
  DisinfectionController& disinfection,
  StatusLED& statusLED)
  : _disinfection(disinfection),
    _statusLED(statusLED),
    _hasTask(false) {
}


// =====================================================
// BEGIN
// =====================================================

void TaskManager::begin() {
  _task.clear();

  _hasTask = false;

  _disinfection.stop();
  _statusLED.stopWorking();

  Serial.println("[TASK MANAGER] Ready");
}


// =====================================================
// SUBMIT TASK
// =====================================================

bool TaskManager::submit(const Task& task) {
  // ไม่รับงานใหม่ถ้ากำลังทำงานอยู่
  if (_hasTask) {
    if (_task.getStatus() == TaskStatus::RUNNING || _task.getStatus() == TaskStatus::PAUSED) {
      Serial.println("[TASK MANAGER] Busy");
      return false;
    }
  }


  // copy task เข้ามาเป็น task หลัก
  _task = task;

  _task.setStatus(TaskStatus::PENDING);

  _hasTask = true;


  Serial.println("[TASK MANAGER] Task submitted");

  return true;
}


// =====================================================
// UPDATE
// =====================================================

void TaskManager::update(unsigned long now) {
  if (!_hasTask)
    return;

  // -------------------------------------------------
  // PENDING
  // -------------------------------------------------

  if (_task.getStatus() == TaskStatus::PENDING) {

    Serial.println("[TASK MANAGER] START TASK");

    _disinfection.start(
      _task.getLamp(1),
      _task.getLamp(2),
      _task.getLamp(3),
      _task.getLamp(4)
    );

    _task.setStatus(TaskStatus::RUNNING);

    _statusLED.startWorking(now);

    Serial.println("[TASK MANAGER] RUNNING");
  }

  // -------------------------------------------------
  // RUNNING
  // -------------------------------------------------

  else if (_task.getStatus() == TaskStatus::RUNNING) {
    // Timer ถูกควบคุมโดย FrontPanelTask
  }

  // -------------------------------------------------
  // PAUSED
  // -------------------------------------------------

  else if (_task.getStatus() == TaskStatus::PAUSED) {
    // รองรับภายหลัง
  }

  // -------------------------------------------------
  // FINISHED
  // -------------------------------------------------

  else if (_task.getStatus() == TaskStatus::FINISHED) {
    // จบแล้ว ไม่ต้องทำอะไร
    // Finish Animation ถูกเริ่มจาก finishTask()
  }

  // -------------------------------------------------
  // STOPPED
  // -------------------------------------------------

  else if (_task.getStatus() == TaskStatus::STOPPED) {
    // จบแล้ว ไม่ต้องทำอะไร
  }
}


// =====================================================
// FINISH TASK
// =====================================================

void TaskManager::finishTask() {
  if (!_hasTask)
    return;


  Serial.println("[TASK MANAGER] FINISH");


  // OFF ทุกอย่าง
  _disinfection.stop();


  // // Working LED OFF
  // _statusLED.stopWorking();
  _statusLED.finish(millis());


  // ไม่ใช้ finish()
  //
  // เพราะ finish() เดิมมี animation
  // และ user ไม่ต้องการให้ Working LED กลับมากระพริบ


  _task.setStatus(TaskStatus::FINISHED);


  Serial.println("[TASK MANAGER] Relay OFF");
  Serial.println("[TASK MANAGER] Motor OFF");
  Serial.println("[TASK MANAGER] Working LED OFF");
}


// =====================================================
// STOP TASK
// =====================================================

void TaskManager::stopTask() {
  if (!_hasTask)
    return;


  Serial.println("[TASK MANAGER] STOP");


  // OFF ทุกอย่าง
  _disinfection.stop();


  // Working LED OFF
  _statusLED.stopWorking();


  _task.setStatus(TaskStatus::STOPPED);


  Serial.println("[TASK MANAGER] Relay OFF");
  Serial.println("[TASK MANAGER] Motor OFF");
  Serial.println("[TASK MANAGER] Working LED OFF");
}


// =====================================================
// CLEAR TASK
// =====================================================

void TaskManager::clearTask() {
  _disinfection.stop();

  _statusLED.stopWorking();

  _task.clear();

  _hasTask = false;


  Serial.println("[TASK MANAGER] Task cleared");
}


// =====================================================
// STATUS
// =====================================================

bool TaskManager::hasTask() const {
  return _hasTask;
}


TaskStatus TaskManager::getStatus() const {
  if (!_hasTask)
    return TaskStatus::EMPTY;

  return _task.getStatus();
}


const Task& TaskManager::getTask() const {
  return _task;
}