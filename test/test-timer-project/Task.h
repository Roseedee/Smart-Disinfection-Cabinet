#ifndef TASK_H
#define TASK_H

#include <Arduino.h>


// =====================================================
// TASK SOURCE
// =====================================================

enum class TaskSource
{
    FRONT_PANEL,
    FIREBASE
};


// =====================================================
// TASK STATUS
// =====================================================

enum class TaskStatus
{
    EMPTY,
    PENDING,
    RUNNING,
    PAUSED,
    FINISHED,
    STOPPED
};


// =====================================================
// TASK
// =====================================================

class Task
{
public:

    Task();


    // -------------------------------------------------
    // Create task
    //
    // สำหรับ Front Panel
    // Lamp ทั้งหมด ON
    // -------------------------------------------------

    void create(
        TaskSource source,
        uint32_t durationSeconds
    );


    // -------------------------------------------------
    // Create task with lamp configuration
    //
    // สำหรับ Firebase ในอนาคต
    // -------------------------------------------------

    void create(
        TaskSource source,
        uint32_t durationSeconds,
        bool lamp1,
        bool lamp2,
        bool lamp3,
        bool lamp4
    );


    // -------------------------------------------------
    // Status
    // -------------------------------------------------

    void setStatus(TaskStatus status);

    TaskStatus getStatus() const;


    // -------------------------------------------------
    // Source
    // -------------------------------------------------

    TaskSource getSource() const;


    // -------------------------------------------------
    // Duration
    // -------------------------------------------------

    uint32_t getDuration() const;


    // -------------------------------------------------
    // Lamps
    // -------------------------------------------------

    bool getLamp(uint8_t lamp) const;


    // -------------------------------------------------
    // Task valid
    // -------------------------------------------------

    bool isValid() const;


    // -------------------------------------------------
    // Clear
    // -------------------------------------------------

    void clear();


private:

    TaskSource _source;
    TaskStatus _status;

    uint32_t _durationSeconds;

    bool _lamps[4];

    bool _valid;
};

#endif