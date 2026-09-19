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
      _previousRunning(false) {}

void FrontPanelTask::update(unsigned long now) {
    _buttons.update(now);

    if (_buttons.startPressed() || _buttons.upPressed() ||
        _buttons.downPressed() || _buttons.setPressed()) {
        _buzzer.buttonBeep(now);
    }

    _timer.update(now, _buttons);
    bool running = _timer.isRunning();

    if (_timer.consumeStartedEvent()) {
        if (_hasTask) {
            if (_taskManager.resumeTask(now)) {
                Serial.println("[FRONT PANEL] Task resumed");
            } else {
                Serial.println("[FRONT PANEL] Resume failed");
            }
        } else {
            _task.create(
                TaskSource::FRONT_PANEL,
                _timer.remainingSeconds(),
                true, true, true, true);

            if (_taskManager.submit(_task)) {
                _hasTask = true;
                Serial.println("[FRONT PANEL] Task submitted");
            } else {
                Serial.println("[FRONT PANEL] Submit failed");
            }
        }

        _buzzer.startBeep(now);
    }

    if (_timer.consumeTimeUpEvent()) {
        Serial.println("[FRONT PANEL] TIME UP");
        _buzzer.timeUp(now);
        _taskManager.finishTask();
        _hasTask = false;
        _previousRunning = false;
        return;
    }

    if (_previousRunning && !running && _taskManager.getStatus() == TaskStatus::RUNNING) {
        Serial.println("[FRONT PANEL] Manual STOP");
        _taskManager.stopTask();
    }

    _previousRunning = running;
}

bool FrontPanelTask::hasTask() const { return _hasTask; }
const Task& FrontPanelTask::getTask() const { return _task; }
void FrontPanelTask::clearTask() {
    _task.clear();
    _hasTask = false;
    _previousRunning = false;
}
