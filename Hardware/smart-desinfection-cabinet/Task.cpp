#include "Task.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

Task::Task()
{
    clear();
}


// =====================================================
// CREATE TASK
// =====================================================

void Task::create(
    TaskSource source,
    uint32_t durationSeconds
)
{
    _source = source;

    _durationSeconds = durationSeconds;

    // Front Panel
    // ไม่มีการเลือก Lamp
    // เปิดทั้งหมด

    _lamps[0] = true;
    _lamps[1] = true;
    _lamps[2] = true;
    _lamps[3] = true;

    _status = TaskStatus::PENDING;

    _valid = true;
}


// =====================================================
// CREATE TASK WITH LAMPS
// =====================================================

void Task::create(
    TaskSource source,
    uint32_t durationSeconds,
    bool lamp1,
    bool lamp2,
    bool lamp3,
    bool lamp4
)
{
    _source = source;

    _durationSeconds = durationSeconds;

    _lamps[0] = lamp1;
    _lamps[1] = lamp2;
    _lamps[2] = lamp3;
    _lamps[3] = lamp4;

    _status = TaskStatus::PENDING;

    _valid = true;
}


// =====================================================
// STATUS
// =====================================================

void Task::setStatus(TaskStatus status)
{
    _status = status;
}


TaskStatus Task::getStatus() const
{
    return _status;
}


// =====================================================
// SOURCE
// =====================================================

TaskSource Task::getSource() const
{
    return _source;
}


// =====================================================
// DURATION
// =====================================================

uint32_t Task::getDuration() const
{
    return _durationSeconds;
}


// =====================================================
// LAMP
// =====================================================

bool Task::getLamp(uint8_t lamp) const
{
    if (lamp < 1 || lamp > 4)
        return false;

    return _lamps[lamp - 1];
}


// =====================================================
// VALID
// =====================================================

bool Task::isValid() const
{
    return _valid;
}


// =====================================================
// CLEAR
// =====================================================

void Task::clear()
{
    _source = TaskSource::FRONT_PANEL;

    _status = TaskStatus::EMPTY;

    _durationSeconds = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        _lamps[i] = false;
    }

    _valid = false;
}