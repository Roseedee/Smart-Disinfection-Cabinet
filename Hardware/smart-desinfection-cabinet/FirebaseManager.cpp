#include "FirebaseManager.h"

#include "FirebaseFrontPanelTask.h"
#include "DisinfectionController.h"
#include "SensorManager.h"
#include "StatusLED.h"
#include "TaskManager.h"

#include <WiFi.h>
#include <time.h>
#include <cstring>


// =====================================================
// CONSTRUCTOR
// =====================================================

FirebaseManager::FirebaseManager(
    const char* apiKey,
    const char* databaseUrl,
    const char* email,
    const char* password,
    const char* deviceSN,
    StatusLED& statusLED
)
    : _statusLED(statusLED),

      _apiKey(apiKey),
      _databaseUrl(databaseUrl),
      _email(email),
      _password(password),
      _deviceSN(deviceSN),

      _workerTask(nullptr),
      _workerStarted(false),

      _ready(false),
      _timeout(false),
      _everReady(false),

      _startTime(0),

      _lastSensorPublish(0),
      _lastDoorPublish(0),
      _lastHardwarePublish(0),
      _lastTaskProgressPublish(0),

      _pendingFlags(TX_NONE),

      _sensorSeq(0),
      _doorSeq(0),
      _hardwareSeq(0),
      _taskSeq(0),
      _commandAckSeq(0),
      _deleteSeq(0),

      _doorAckSeq(0),
      _hardwareAckSeq(0),
      _doorAckAppliedSeq(0),
      _hardwareAckAppliedSeq(0),

      _remoteVersion(0),
      _processedRemoteVersion(0),

      _lastRemotePoll(0),
      _remoteReadFailCount(0),

      _deletePending(false),
      _deleteSuperseded(false),

      _activeDuration(0),

      _networkActivityPending(false)
{
    memset(&_sensorSnapshot, 0, sizeof(_sensorSnapshot));
    memset(&_doorSnapshot, 0, sizeof(_doorSnapshot));
    memset(&_hardwareSnapshot, 0, sizeof(_hardwareSnapshot));
    memset(&_taskSnapshot, 0, sizeof(_taskSnapshot));
    memset(&_commandAckSnapshot, 0, sizeof(_commandAckSnapshot));
    memset(&_remoteSnapshot, 0, sizeof(_remoteSnapshot));
    memset(&_doorAckSnapshot, 0, sizeof(_doorAckSnapshot));
    memset(&_hardwareAckSnapshot, 0, sizeof(_hardwareAckSnapshot));

    memset(_activeUserId, 0, sizeof(_activeUserId));
    memset(_activeTaskId, 0, sizeof(_activeTaskId));
    memset(_deleteTaskId, 0, sizeof(_deleteTaskId));
    memset(_activeLamps, 0, sizeof(_activeLamps));
}


// =====================================================
// BEGIN
// =====================================================

void FirebaseManager::begin()
{
    if (_workerStarted) {
        return;
    }

    // =================================================
    // FIREBASE CONFIG
    // =================================================

    _config.api_key =
        _apiKey;

    _config.database_url =
        _databaseUrl;

    _auth.user.email =
        _email;

    _auth.user.password =
        _password;


    // =================================================
    // FIREBASE TIMEOUT
    // =================================================

    _config.timeout.serverResponse =
        5000;

    _config.timeout.socketConnection =
        5000;


    // =================================================
    // STATE
    // =================================================

    _startTime =
        millis();

    _ready = false;

    _timeout = false;

    _everReady = false;

    _lastSensorPublish = 0;

    _lastDoorPublish = 0;

    _lastHardwarePublish = 0;

    _lastTaskProgressPublish = 0;


    // =================================================
    // NTP
    // =================================================

    configTime(
        7 * 3600,
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );


    // =================================================
    // CREATE WORKER
    //
    // IMPORTANT:
    // CPU 1
    // =================================================

    BaseType_t result =
        xTaskCreatePinnedToCore(
            workerEntry,
            "FirebaseWorker",
            WORKER_STACK_SIZE,
            this,
            1,
            &_workerTask,
            1
        );


    if (result != pdPASS)
    {
        _timeout = true;

        Serial.println(
            "[Firebase] Failed to create background task"
        );

        return;
    }


    _workerStarted = true;

    Serial.println(
        "[Firebase] Background worker started on CPU 1"
    );
}


// =====================================================
// UPDATE
//
// IMPORTANT:
// ไม่มี Firebase API call ที่นี่
// =====================================================

void FirebaseManager::update(
    unsigned long now,
    FirebaseFrontPanelTask& firebaseTask,
    DisinfectionController& disinfection,
    TaskManager& taskManager,
    SensorManager& sensors
)
{
    consumeNetworkActivity();

    // Clear source state only after the worker successfully sent the
    // exact snapshot and no newer state change occurred meanwhile.
    uint32_t doorAck = 0;
    uint32_t hardwareAck = 0;

    portENTER_CRITICAL(&_mux);
    doorAck = _doorAckSeq;
    hardwareAck = _hardwareAckSeq;
    portEXIT_CRITICAL(&_mux);

    if (doorAck != 0 &&
        doorAck != _doorAckAppliedSeq)
    {
        SensorSnapshot ackSnapshot{};

        portENTER_CRITICAL(&_mux);
        ackSnapshot = _doorAckSnapshot;
        portEXIT_CRITICAL(&_mux);

        // Only clear the source flag when the live state still matches
        // the exact state Firebase confirmed. A newer state change that
        // happened after the ACK must remain pending.
        if (sensors.isDoorOpen() == ackSnapshot.doorOpen)
        {
            sensors.clearDoorStateChanged();
            _doorAckAppliedSeq = doorAck;
        }
    }

    if (hardwareAck != 0 &&
        hardwareAck != _hardwareAckAppliedSeq)
    {
        HardwareSnapshot ackSnapshot{};

        portENTER_CRITICAL(&_mux);
        ackSnapshot = _hardwareAckSnapshot;
        portEXIT_CRITICAL(&_mux);

        bool currentMatches = true;

        for (uint8_t i = 0; i < 4; ++i)
        {
            if (disinfection.getLamp(i + 1) != ackSnapshot.lamps[i])
            {
                currentMatches = false;
                break;
            }
        }

        if (currentMatches &&
            disinfection.isMotorOn() != ackSnapshot.motor)
        {
            currentMatches = false;
        }

        if (currentMatches &&
            taskManager.isBusy() != ackSnapshot.busy)
        {
            currentMatches = false;
        }

        if (currentMatches)
        {
            disinfection.clearStateChanged();
            taskManager.clearBusyStateChanged();
            _hardwareAckAppliedSeq = hardwareAck;
        }
    }


    if (!_workerStarted) {
        return;
    }


    if (WiFi.status() != WL_CONNECTED) {
        return;
    }


    // =================================================
    // DOOR
    // =================================================

    if (sensors.hasDoorStateChanged())
    {
        publishDoor(
            now,
            sensors
        );
    }


    // =================================================
    // SENSOR
    // =================================================

    if (
        now - _lastSensorPublish >=
        SENSOR_INTERVAL
    )
    {
        publishSensor(
            now,
            sensors
        );
    }


    // =================================================
    // HARDWARE
    // =================================================

    if (
        disinfection.hasStateChanged() ||
        taskManager.hasBusyStateChanged()
    )
    {
        if (
            now - _lastHardwarePublish >=
            HARDWARE_RETRY_INTERVAL
        )
        {
            publishHardware(
                now,
                disinfection,
                taskManager
            );
        }
    }


    // =================================================
    // LOCAL TASK STATE
    // =================================================

    if (
        firebaseTask.consumeLocalStateChanged()
    )
    {
        publishTaskStateFromPanel(
            firebaseTask
        );
    }


    // =================================================
    // FINISHED
    // =================================================

    if (
        firebaseTask.consumeFinishedEvent()
    )
    {
        /*
         * สำคัญ:
         *
         * History จะถูกจัดการก่อน DELETE
         * โดย worker
         *
         * แต่ถึง History fail
         * DELETE ก็ยังต้องเกิด
         */

        /*
         * IMPORTANT:
         * Do not write a final "completed" snapshot into /task here.
         *
         * The task node is an input/command queue. When finish is detected,
         * the authoritative completed record is History, then /task is
         * deleted. Writing TX_TASK here can race with DELETE and recreate
         * the task node again.
         */
        requestDeleteTask();

        // FirebaseFrontPanelTask::finishTask() stops the hardware and sets
        // TaskManager -> FINISHED. Clear it now so the next Firebase task
        // is accepted without a reboot.
        if (taskManager.hasTask())
        {
            taskManager.clearTask();
        }

        firebaseTask.clearTask();
    }


    // =================================================
    // CANCELLED
    // =================================================

    if (
        firebaseTask.consumeCancelledEvent()
    )
    {
        requestDeleteTask();

        if (
            taskManager.hasTask()
        )
        {
            taskManager.clearTask();
        }

        firebaseTask.clearTask();
    }


    // =================================================
    // TASK PROGRESS
    // =================================================

    if (
        firebaseTask.hasTask() &&
        firebaseTask.isRunning() &&
        now - _lastTaskProgressPublish >=
        TASK_PROGRESS_INTERVAL
    )
    {
        _lastTaskProgressPublish =
            now;

        publishTaskStateFromPanel(
            firebaseTask
        );
    }


    // =================================================
    // REMOTE TASK
    // =================================================

    processRemoteTask(
        now,
        firebaseTask,
        taskManager
    );
}


// =====================================================
// WORKER ENTRY
// =====================================================

void FirebaseManager::workerEntry(
    void* arg
)
{
    FirebaseManager* manager =
        static_cast<FirebaseManager*>(arg);

    manager->workerLoop();

    vTaskDelete(nullptr);
}


// =====================================================
// WORKER LOOP
// =====================================================

void FirebaseManager::workerLoop()
{
    Serial.println(
        "[Firebase] Worker initializing client"
    );


    // =================================================
    // FIREBASE START
    // =================================================

    Firebase.begin(
        &_config,
        &_auth
    );

    Firebase.reconnectWiFi(
        true
    );


    unsigned long lastTaskPoll =
        0;


    // =================================================
    // LOOP
    // =================================================

    for (;;)
    {
        unsigned long now =
            millis();


        // =================================================
        // WIFI
        // =================================================

        if (
            WiFi.status() != WL_CONNECTED
        )
        {
            _ready = false;

            vTaskDelay(
                pdMS_TO_TICKS(100)
            );

            continue;
        }


        // =================================================
        // FIREBASE READY
        // =================================================

        if (
            !Firebase.ready()
        )
        {
            _ready = false;


            if (
                !_everReady &&
                !_timeout &&
                now - _startTime >=
                FIREBASE_CONNECT_TIMEOUT
            )
            {
                _timeout = true;

                Serial.println(
                    "[Firebase] CONNECTION TIMEOUT / OFFLINE MODE"
                );
            }


            vTaskDelay(
                pdMS_TO_TICKS(50)
            );

            continue;
        }


        // =================================================
        // READY
        // =================================================

        if (!_ready)
        {
            _ready = true;

            _timeout = false;

            _everReady = true;


            Serial.print(
                "[Firebase] READY - Device SN: "
            );

            Serial.println(
                _deviceSN
            );
        }


        bool didWork =
            false;

        // Poll remote task first so a newly-created task is discovered
        // before lower-priority telemetry work.
        pollRemoteTask(now);


        // =================================================
        // 1. DELETE
        //
        // Highest priority
        // =================================================

        if (
            takeTxFlag(TX_DELETE)
        )
        {
            uint32_t seq =
                copyDeleteSequence();

            char expectedTaskId[64] = {0};

            portENTER_CRITICAL(&_mux);
            strncpy(
                expectedTaskId,
                _deleteTaskId,
                sizeof(expectedTaskId) - 1
            );
            expectedTaskId[sizeof(expectedTaskId) - 1] = '\0';
            portEXIT_CRITICAL(&_mux);

            /*
             * Save history first. History failure must never block DELETE.
             */

            /*
             * Do NOT write /task again here.
             * Finish already marked the task complete locally.
             * History is the completed record; /task is now a command/input
             * node that must be removed.
             */

            bool historyResult = saveTaskHistory();

            if (!historyResult)
            {
                Serial.println(
                    "[Firebase History] FAILED - continue DELETE"
                );
            }

            /*
             * IMPORTANT:
             * Before deleting, verify that /task still belongs to the
             * task that just finished. This prevents a new Task from
             * being accidentally deleted while the old DELETE request
             * is waiting in the worker queue.
             */
            if (deleteTask(expectedTaskId))
            {
                bool superseded = false;

                portENTER_CRITICAL(&_mux);
                superseded = _deleteSuperseded;
                _deleteSuperseded = false;
                _deletePending = false;

                memset(_activeUserId, 0, sizeof(_activeUserId));
                memset(_activeTaskId, 0, sizeof(_activeTaskId));
                memset(_deleteTaskId, 0, sizeof(_deleteTaskId));
                _activeDuration = 0;
                memset(_activeLamps, 0, sizeof(_activeLamps));

                // /task was deleted, so any queued command ACK is obsolete.
                _pendingFlags &= ~TX_COMMAND_ACK;

                portEXIT_CRITICAL(&_mux);

                clearTxFlagIfSequenceUnchanged(
                    TX_DELETE,
                    seq
                );

                // Only reset the remote snapshot when the old task was
                // actually deleted. If a newer task appeared first,
                // preserve its remote snapshot for immediate processing.
                if (!superseded)
                {
                    resetRemoteTaskState();
                }

                didWork = true;
            }
            else
            {
                /* Delete/network check failed - retry. */
                vTaskDelay(
                    pdMS_TO_TICKS(100)
                );
            }
        }


        // =================================================
        // 2. COMMAND ACK
        //
        // command is a one-shot Firebase input. ACK it separately from
        // status/progress so normal task updates never overwrite command.
        // =================================================

        if (
            !didWork &&
            takeTxFlag(TX_COMMAND_ACK)
        )
        {
            CommandAckSnapshot snapshot;

            uint32_t seq =
                0;

            if (
                copyCommandAckSnapshot(
                    snapshot,
                    seq
                ) &&
                sendCommandAck(
                    snapshot
                )
            )
            {
                clearTxFlagIfSequenceUnchanged(
                    TX_COMMAND_ACK,
                    seq
                );

                didWork = true;
            }
            else
            {
                vTaskDelay(
                    pdMS_TO_TICKS(100)
                );
            }
        }


        // =================================================
        // 3. HARDWARE
        //
        // Relay / motor / busy state.
        // =================================================

        if (
            !didWork &&
            takeTxFlag(TX_HW)
        )
        {
            HardwareSnapshot snapshot;

            uint32_t seq =
                0;

            if (
                copyHardwareSnapshot(
                    snapshot,
                    seq
                ) &&
                sendHardware(
                    snapshot
                )
            )
            {
                portENTER_CRITICAL(&_mux);
                _hardwareAckSeq = seq;
                _hardwareAckSnapshot = snapshot;
                portEXIT_CRITICAL(&_mux);

                clearTxFlagIfSequenceUnchanged(
                    TX_HW,
                    seq
                );

                didWork = true;
            }
            else
            {
                vTaskDelay(
                    pdMS_TO_TICKS(100)
                );
            }
        }


        // =================================================
        // 4. DOOR
        // =================================================

        if (
            !didWork &&
            takeTxFlag(TX_DOOR)
        )
        {
            SensorSnapshot snapshot;

            uint32_t seq =
                0;


            if (
                copyDoorSnapshot(
                    snapshot,
                    seq
                ) &&
                sendDoor(
                    snapshot
                )
            )
            {
                portENTER_CRITICAL(&_mux);
                _doorAckSeq = seq;
                _doorAckSnapshot = snapshot;
                portEXIT_CRITICAL(&_mux);

                clearTxFlagIfSequenceUnchanged(
                    TX_DOOR,
                    seq
                );

                didWork = true;
            }
            else
            {
                vTaskDelay(
                    pdMS_TO_TICKS(100)
                );
            }
        }


        // =================================================
        // 5. TASK
        //
        // Periodic task/progress state.
        // Suppressed while an old task is being deleted.
        // =================================================

        bool deletePendingForTaskWrite = false;

        portENTER_CRITICAL(&_mux);
        deletePendingForTaskWrite = _deletePending;
        portEXIT_CRITICAL(&_mux);

        if (
            !didWork &&
            !deletePendingForTaskWrite &&
            takeTxFlag(TX_TASK)
        )
        {
            TaskWriteSnapshot snapshot;

            uint32_t seq =
                0;

            if (
                copyTaskSnapshot(
                    snapshot,
                    seq
                ) &&
                sendTask(
                    snapshot
                )
            )
            {
                clearTxFlagIfSequenceUnchanged(
                    TX_TASK,
                    seq
                );

                didWork = true;
            }
            else
            {
                vTaskDelay(
                    pdMS_TO_TICKS(100)
                );
            }
        }


        // =================================================
        // 6. SENSOR
        // =================================================

        if (
            !didWork &&
            takeTxFlag(TX_SENSOR)
        )
        {
            SensorSnapshot snapshot;

            uint32_t seq =
                0;


            if (
                copySensorSnapshot(
                    snapshot,
                    seq
                ) &&
                sendSensor(
                    snapshot
                )
            )
            {
                clearTxFlagIfSequenceUnchanged(
                    TX_SENSOR,
                    seq
                );

                didWork = true;
            }
            else
            {
                vTaskDelay(
                    pdMS_TO_TICKS(100)
                );
            }
        }


        // =================================================
        // GIVE CPU
        // =================================================

        if (!didWork)
        {
            vTaskDelay(
                pdMS_TO_TICKS(FIREBASE_WORKER_IDLE)
            );
        }
        else
        {
            /*
             * ถึงจะทำงานสำเร็จ
             * ก็ต้อง yield
             *
             * ป้องกัน Worker กิน CPU
             */

            vTaskDelay(
                pdMS_TO_TICKS(2)
            );
        }
    }
}


// =====================================================
// PUBLISH SENSOR
// =====================================================

void FirebaseManager::publishSensor(
    unsigned long now,
    SensorManager& sensors
)
{
    SensorSnapshot snapshot{};


    snapshot.uvRaw =
        sensors.getUVRaw();

    snapshot.uvVoltage =
        sensors.getUVVoltage();

    snapshot.temperature =
        sensors.getTemperature();

    snapshot.humidity =
        sensors.getHumidity();

    snapshot.dhtValid =
        sensors.hasDHTData();

    snapshot.doorOpen =
        sensors.isDoorOpen();


    portENTER_CRITICAL(
        &_mux
    );

    _sensorSnapshot =
        snapshot;

    ++_sensorSeq;

    _pendingFlags |=
        TX_SENSOR;

    portEXIT_CRITICAL(
        &_mux
    );


    _lastSensorPublish =
        now;
}


// =====================================================
// PUBLISH DOOR
// =====================================================

void FirebaseManager::publishDoor(
    unsigned long now,
    SensorManager& sensors
)
{
    SensorSnapshot snapshot{};

    snapshot.doorOpen =
        sensors.isDoorOpen();


    portENTER_CRITICAL(
        &_mux
    );

    _doorSnapshot =
        snapshot;

    ++_doorSeq;

    _pendingFlags |=
        TX_DOOR;

    portEXIT_CRITICAL(
        &_mux
    );


    _lastDoorPublish =
        now;


    /*
     * สำคัญมาก:
     *
     * ไม่ clearDoorStateChanged() ที่นี่
     *
     * เพราะ Firebase ยังไม่ได้ส่ง
     */
}


// =====================================================
// PUBLISH HARDWARE
// =====================================================

void FirebaseManager::publishHardware(
    unsigned long now,
    DisinfectionController& disinfection,
    TaskManager& taskManager
)
{
    HardwareSnapshot snapshot{};


    for (
        uint8_t i = 0;
        i < 4;
        ++i
    )
    {
        snapshot.lamps[i] =
            disinfection.getLamp(
                i + 1
            );
    }


    snapshot.motor =
        disinfection.isMotorOn();

    snapshot.busy =
        taskManager.isBusy();


    portENTER_CRITICAL(
        &_mux
    );

    _hardwareSnapshot =
        snapshot;

    ++_hardwareSeq;

    _pendingFlags |=
        TX_HW;

    portEXIT_CRITICAL(
        &_mux
    );


    _lastHardwarePublish =
        now;


    /*
     * ห้าม clear state ที่นี่
     *
     * จะ clear หลัง sendHardware()
     * สำเร็จเท่านั้น
     */
}


// =====================================================
// PUBLISH TASK STATE
// =====================================================

void FirebaseManager::publishTaskStateFromPanel(
    FirebaseFrontPanelTask& firebaseTask
)
{
    if (
        !firebaseTask.hasTask()
    )
    {
        return;
    }


    const bool running =
        firebaseTask.isRunning() &&
        !firebaseTask.isSafetyPaused();


    const char* status =
        running
            ? "running"
            : "paused";


    publishTaskState(
        status,
        running,
        firebaseTask.getRemaining(),
        firebaseTask.getProgress()
    );
}


// =====================================================
// PUBLISH TASK STATE
// =====================================================

void FirebaseManager::publishTaskState(
    const char* status,
    bool isRunning,
    uint32_t remaining,
    uint8_t progress
)
{
    bool deletePending = false;

    portENTER_CRITICAL(&_mux);
    deletePending = _deletePending;
    portEXIT_CRITICAL(&_mux);

    if (deletePending)
    {
        return;
    }

    TaskWriteSnapshot snapshot{};


    strncpy(
        snapshot.status,
        status,
        sizeof(snapshot.status) - 1
    );


    snapshot.isRunning =
        isRunning;

    snapshot.remaining =
        remaining;

    snapshot.progress =
        progress;


    portENTER_CRITICAL(
        &_mux
    );

    _taskSnapshot =
        snapshot;

    ++_taskSeq;

    _pendingFlags |=
        TX_TASK;

    portEXIT_CRITICAL(
        &_mux
    );
}


// =====================================================
// REQUEST COMMAND ACK
// =====================================================

void FirebaseManager::requestCommandAck(
    const char* command
)
{
    if (command == nullptr || command[0] == '\0')
        return;

    portENTER_CRITICAL(&_mux);

    strncpy(
        _commandAckSnapshot.expectedCommand,
        command,
        sizeof(_commandAckSnapshot.expectedCommand) - 1
    );

    _commandAckSnapshot.expectedCommand[
        sizeof(_commandAckSnapshot.expectedCommand) - 1
    ] = '\0';

    ++_commandAckSeq;
    _pendingFlags |= TX_COMMAND_ACK;

    portEXIT_CRITICAL(&_mux);
}


// =====================================================
// REQUEST DELETE
// =====================================================

void FirebaseManager::requestDeleteTask()
{
    portENTER_CRITICAL(&_mux);

    // Only Firebase-origin tasks have an active task_id.
    if (_activeTaskId[0] == '\0')
    {
        portEXIT_CRITICAL(&_mux);
        return;
    }

    strncpy(
        _deleteTaskId,
        _activeTaskId,
        sizeof(_deleteTaskId) - 1
    );
    _deleteTaskId[sizeof(_deleteTaskId) - 1] = '\0';

    // From this point until DELETE succeeds:
    // - no task status/progress write may be sent
    // - no remote task command may be processed
    _deletePending = true;
    _deleteSuperseded = false;

    // Drop any stale progress/status write queued before finish.
    _pendingFlags &= ~TX_TASK;

    ++_deleteSeq;
    _pendingFlags |= TX_DELETE;

    portEXIT_CRITICAL(&_mux);
}


// =====================================================
// RESET REMOTE TASK STATE

// =====================================================

void FirebaseManager::resetRemoteTaskState()
{
    portENTER_CRITICAL(&_mux);

    RemoteTaskSnapshot empty{};
    _remoteSnapshot = empty;

    // The old task has been fully handled. New Firebase task must
    // produce a fresh version transition from this clean state.
    _processedRemoteVersion = _remoteVersion;
    _remoteReadFailCount = 0;

    portEXIT_CRITICAL(&_mux);

    Serial.println("[Firebase Task] Remote state reset - READY FOR NEXT TASK");
}


// =====================================================
// PROCESS REMOTE TASK
// =====================================================

void FirebaseManager::processRemoteTask(
    unsigned long now,
    FirebaseFrontPanelTask& firebaseTask,
    TaskManager& taskManager
)
{
    // Once finish/cancel requested DELETE, the old task is no longer
    // allowed to drive the machine. Wait until DELETE completes.
    bool deletePending = false;

    portENTER_CRITICAL(&_mux);
    deletePending = _deletePending;
    portEXIT_CRITICAL(&_mux);

    if (deletePending)
    {
        return;
    }

    RemoteTaskSnapshot remote{};

    uint32_t version =
        0;


    if (
        !copyRemoteSnapshot(
            remote,
            version
        )
    )
    {
        return;
    }


    if (
        version == 0 ||
        version ==
        _processedRemoteVersion
    )
    {
        return;
    }


    // =================================================
    // START + DOOR OPEN
    // =================================================

    if (
        remote.valid &&
        strcmp(
            remote.command,
            "start"
        ) == 0 &&
        taskManager.isDoorOpen()
    )
    {
        /*
         * ยังไม่ mark processed
         *
         * ให้รอ door close
         */

        return;
    }


    // =================================================
    // CANCEL
    // =================================================

    if (
        remote.valid &&
        strcmp(
            remote.command,
            "cancel"
        ) == 0
    )
    {
        Serial.println(
            "[Firebase Task] COMMAND: CANCEL"
        );


        if (
            firebaseTask.hasTask()
        )
        {
            firebaseTask.cancel();
        }


        if (
            taskManager.hasTask()
        )
        {
            taskManager.clearTask();
        }


        bool hasFirebaseIdentity = false;

        portENTER_CRITICAL(&_mux);
        hasFirebaseIdentity = (_activeTaskId[0] != '\0');
        portEXIT_CRITICAL(&_mux);

        requestDeleteTask();

        // If there is no Firebase task identity to delete, still consume
        // the one-shot CANCEL command so it cannot remain latched forever.
        if (!hasFirebaseIdentity)
        {
            requestCommandAck("cancel");
        }

        markRemoteProcessed(
            version
        );


        return;
    }


    // =================================================
    // EXISTING FIREBASE TASK
    // =================================================

    if (
        firebaseTask.hasTask()
    )
    {
        // ---------------------------------------------
        // START
        // ---------------------------------------------

        if (
            remote.valid &&
            strcmp(
                remote.command,
                "start"
            ) == 0 &&
            !firebaseTask.isRunning()
        )
        {
            Serial.println(
                "[Firebase Task] COMMAND: START"
            );


            firebaseTask.start(
                now
            );


            if (
                firebaseTask.isRunning()
            )
            {
                publishTaskStateFromPanel(
                    firebaseTask
                );
            }

            // command is one-shot: acknowledge it regardless of whether
            // start succeeded. A rejected start must not remain latched.
            requestCommandAck("start");

            markRemoteProcessed(
                version
            );


            return;
        }


        // ---------------------------------------------
        // STOP
        // ---------------------------------------------

        if (
            remote.valid &&
            strcmp(
                remote.command,
                "stop"
            ) == 0 &&
            firebaseTask.isRunning()
        )
        {
            Serial.println(
                "[Firebase Task] COMMAND: STOP"
            );


            firebaseTask.stop();


            publishTaskState(
                "paused",
                false,
                firebaseTask.getRemaining(),
                firebaseTask.getProgress()
            );

            requestCommandAck("stop");

            markRemoteProcessed(
                version
            );


            return;
        }


        // ---------------------------------------------
        // SAME TASK
        // ---------------------------------------------

        markRemoteProcessed(
            version
        );

        return;
    }


    // =================================================
    // PHYSICAL TASK OWNS MACHINE
    // =================================================

    if (
        taskManager.hasTask()
    )
    {
        TaskStatus localStatus =
            taskManager.getStatus();

        if (localStatus == TaskStatus::FINISHED)
        {
            Serial.println(
                "[Firebase Task] Clearing finished front-panel task"
            );

            taskManager.clearTask();
        }
        else
        {
            Serial.println(
                "[Firebase Task] Machine BUSY - command rejected"
            );

            if (
                remote.valid &&
                strcmp(remote.command, "none") != 0
            )
            {
                requestCommandAck(remote.command);
            }

            markRemoteProcessed(
                version
            );

            return;
        }
    }


    // =================================================
    // INVALID REMOTE
    // =================================================

    if (
        !remote.valid ||
        strcmp(
            remote.status,
            "pending"
        ) != 0
    )
    {
        if (
            remote.valid &&
            strcmp(remote.command, "none") != 0
        )
        {
            requestCommandAck(remote.command);
        }

        markRemoteProcessed(
            version
        );

        return;
    }


    // =================================================
    // INVALID DURATION
    // =================================================

    if (
        remote.duration == 0
    )
    {
        Serial.println(
            "[Firebase Task] Invalid duration"
        );

        if (
            remote.valid &&
            strcmp(remote.command, "none") != 0
        )
        {
            requestCommandAck(remote.command);
        }

        markRemoteProcessed(
            version
        );

        return;
    }


    // =================================================
    // CREATE TASK
    // =================================================

    Task task;


    task.create(
        TaskSource::FIREBASE,

        remote.duration,

        remote.lamps[0],
        remote.lamps[1],
        remote.lamps[2],
        remote.lamps[3]
    );


    // =================================================
    // RECEIVE
    // =================================================

    if (
        !firebaseTask.receiveTask(
            task
        )
    )
    {
        return;
    }


    // Save task identity locally. History must remain possible even if
    // the remote task is changed/deleted before the finish event is handled.
    portENTER_CRITICAL(&_mux);

    strncpy(_activeUserId, remote.userid, sizeof(_activeUserId) - 1);
    _activeUserId[sizeof(_activeUserId) - 1] = '\0';

    strncpy(_activeTaskId, remote.taskId, sizeof(_activeTaskId) - 1);
    _activeTaskId[sizeof(_activeTaskId) - 1] = '\0';

    _activeDuration = remote.duration;

    for (uint8_t i = 0; i < 4; ++i) {
        _activeLamps[i] = remote.lamps[i];
    }

    // A newly accepted task supersedes any old delete target.
    memset(_deleteTaskId, 0, sizeof(_deleteTaskId));

    portEXIT_CRITICAL(&_mux);

    Serial.println(
        "[Firebase Task] NEW TASK RECEIVED"
    );


    Serial.print(
        "[Firebase Task] task_id = "
    );

    Serial.println(
        remote.taskId
    );


    Serial.print(
        "[Firebase Task] userid = "
    );

    Serial.println(
        remote.userid
    );


    // =================================================
    // START
    // =================================================

    if (
        strcmp(
            remote.command,
            "start"
        ) == 0
    )
    {
        firebaseTask.start(
            now
        );


        if (
            firebaseTask.isRunning()
        )
        {
            publishTaskStateFromPanel(
                firebaseTask
            );
        }
        else
        {
            publishTaskState(
                "accepted",
                false,
                firebaseTask.getRemaining(),
                firebaseTask.getProgress()
            );
        }
    }
    else
    {
        publishTaskState(
            "accepted",
            false,
            firebaseTask.getRemaining(),
            firebaseTask.getProgress()
        );
    }

    // One-shot command: consume any command that arrived with the task.
    if (
        strcmp(remote.command, "none") != 0
    )
    {
        requestCommandAck(remote.command);
    }


    markRemoteProcessed(
        version
    );
}


// =====================================================
// NETWORK ACTIVITY
// =====================================================

void FirebaseManager::queueNetworkActivity()
{
    portENTER_CRITICAL(
        &_mux
    );

    _networkActivityPending =
        true;

    portEXIT_CRITICAL(
        &_mux
    );
}


// =====================================================
// NETWORK ACTIVITY
// =====================================================

void FirebaseManager::consumeNetworkActivity()
{
    bool pending =
        false;


    portENTER_CRITICAL(
        &_mux
    );

    pending =
        _networkActivityPending;

    _networkActivityPending =
        false;

    portEXIT_CRITICAL(
        &_mux
    );


    if (pending)
    {
        _statusLED.networkActivity(
            millis()
        );
    }
}


// =====================================================
// TAKE TX FLAG
// =====================================================

bool FirebaseManager::takeTxFlag(
    uint8_t flag
)
{
    portENTER_CRITICAL(
        &_mux
    );

    bool result =
        (_pendingFlags & flag) != 0;

    portEXIT_CRITICAL(
        &_mux
    );


    return result;
}


// =====================================================
// CLEAR TX FLAG
// =====================================================

void FirebaseManager::clearTxFlagIfSequenceUnchanged(
    uint8_t flag,
    uint32_t sequence
)
{
    portENTER_CRITICAL(
        &_mux
    );


    bool unchanged =
        false;


    switch (flag)
    {
        case TX_SENSOR:

            unchanged =
                (_sensorSeq == sequence);

            break;


        case TX_DOOR:

            unchanged =
                (_doorSeq == sequence);

            break;


        case TX_HW:

            unchanged =
                (_hardwareSeq == sequence);

            break;


        case TX_TASK:

            unchanged =
                (_taskSeq == sequence);

            break;


        case TX_COMMAND_ACK:

            unchanged =
                (_commandAckSeq == sequence);

            break;


        case TX_DELETE:

            unchanged =
                (_deleteSeq == sequence);

            break;
    }


    if (unchanged)
    {
        _pendingFlags &=
            ~flag;
    }


    portEXIT_CRITICAL(
        &_mux
    );
}


// =====================================================
// COPY SENSOR
// =====================================================

bool FirebaseManager::copySensorSnapshot(
    SensorSnapshot& out,
    uint32_t& sequence
)
{
    portENTER_CRITICAL(
        &_mux
    );

    out =
        _sensorSnapshot;

    sequence =
        _sensorSeq;

    portEXIT_CRITICAL(
        &_mux
    );


    return sequence != 0;
}


// =====================================================
// COPY DOOR
// =====================================================

bool FirebaseManager::copyDoorSnapshot(
    SensorSnapshot& out,
    uint32_t& sequence
)
{
    portENTER_CRITICAL(
        &_mux
    );

    out =
        _doorSnapshot;

    sequence =
        _doorSeq;

    portEXIT_CRITICAL(
        &_mux
    );


    return sequence != 0;
}


// =====================================================
// COPY HARDWARE
// =====================================================

bool FirebaseManager::copyHardwareSnapshot(
    HardwareSnapshot& out,
    uint32_t& sequence
)
{
    portENTER_CRITICAL(
        &_mux
    );

    out =
        _hardwareSnapshot;

    sequence =
        _hardwareSeq;

    portEXIT_CRITICAL(
        &_mux
    );


    return sequence != 0;
}


// =====================================================
// COPY TASK
// =====================================================

bool FirebaseManager::copyTaskSnapshot(
    TaskWriteSnapshot& out,
    uint32_t& sequence
)
{
    portENTER_CRITICAL(
        &_mux
    );

    out =
        _taskSnapshot;

    sequence =
        _taskSeq;

    portEXIT_CRITICAL(
        &_mux
    );


    return sequence != 0;
}


// =====================================================
// COPY COMMAND ACK
// =====================================================

bool FirebaseManager::copyCommandAckSnapshot(
    CommandAckSnapshot& out,
    uint32_t& sequence
)
{
    portENTER_CRITICAL(&_mux);

    out = _commandAckSnapshot;
    sequence = _commandAckSeq;

    portEXIT_CRITICAL(&_mux);

    return sequence != 0;
}


// =====================================================
// COPY DELETE SEQUENCE
// =====================================================

uint32_t FirebaseManager::copyDeleteSequence()
{
    portENTER_CRITICAL(
        &_mux
    );

    uint32_t seq =
        _deleteSeq;

    portEXIT_CRITICAL(
        &_mux
    );


    return seq;
}


// =====================================================
// COPY REMOTE
// =====================================================

bool FirebaseManager::copyRemoteSnapshot(
    RemoteTaskSnapshot& out,
    uint32_t& version
)
{
    portENTER_CRITICAL(
        &_mux
    );

    out =
        _remoteSnapshot;

    version =
        _remoteVersion;

    portEXIT_CRITICAL(
        &_mux
    );


    return version != 0;
}


// =====================================================
// MARK REMOTE PROCESSED
// =====================================================

void FirebaseManager::markRemoteProcessed(
    uint32_t version
)
{
    portENTER_CRITICAL(
        &_mux
    );


    if (
        version >
        _processedRemoteVersion
    )
    {
        _processedRemoteVersion =
            version;
    }


    portEXIT_CRITICAL(
        &_mux
    );
}


// =====================================================
// POLL REMOTE TASK
// =====================================================

void FirebaseManager::pollRemoteTask(
    unsigned long now
)
{
    unsigned long interval =
        (_remoteReadFailCount > 0)
            ? FIREBASE_POLL_FAIL_RETRY
            : TASK_POLL_INTERVAL;

    if (_lastRemotePoll != 0 &&
        now - _lastRemotePoll < interval)
    {
        return;
    }

    _lastRemotePoll = now;

    RemoteTaskSnapshot remote{};

    if (!readRemoteTask(remote))
    {
        if (_remoteReadFailCount < 255) {
            ++_remoteReadFailCount;
        }

        if (_remoteReadFailCount == 1 ||
            (_remoteReadFailCount % 10) == 0)
        {
            Serial.print("[Firebase Task] Poll failed, retry count = ");
            Serial.println(_remoteReadFailCount);
        }

        return;
    }

    if (_remoteReadFailCount > 0)
    {
        Serial.println("[Firebase Task] Poll recovered");
        _remoteReadFailCount = 0;
    }

    bool changed = false;

    portENTER_CRITICAL(&_mux);

    changed = !sameRemoteTask(
        remote,
        _remoteSnapshot
    );

    if (changed)
    {
        _remoteSnapshot = remote;
        ++_remoteVersion;
    }

    portEXIT_CRITICAL(&_mux);

    if (changed)
    {
        Serial.print("[Firebase Task] REMOTE TASK CHANGED: ");
        if (remote.valid)
        {
            Serial.print(remote.taskId);
            Serial.print(" status=");
            Serial.print(remote.status);
            Serial.print(" command=");
            Serial.println(remote.command);
        }
        else
        {
            Serial.println("EMPTY");
        }

        queueNetworkActivity();
    }
}


// =====================================================
// READ REMOTE TASK
// =====================================================

bool FirebaseManager::readRemoteTask(
    RemoteTaskSnapshot& out
)
{
    out = {};


    String path =
        taskPath();


    if (
        !Firebase.RTDB.getJSON(
            &_fbdo,
            path.c_str()
        )
    )
    {
        Serial.print(
            "[Firebase Task] Read FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }


    FirebaseJson json =
        _fbdo.jsonObject();


    FirebaseJsonData data;


    // =================================================
    // STATUS
    // =================================================

    json.get(
        data,
        "status"
    );


    if (!data.success)
    {
        out.valid =
            false;

        return true;
    }


    String status =
        data.to<String>();


    strncpy(
        out.status,
        status.c_str(),
        sizeof(out.status) - 1
    );


    // =================================================
    // USER ID
    // =================================================

    json.get(
        data,
        "userid"
    );


    if (data.success)
    {
        String userid =
            data.to<String>();


        strncpy(
            out.userid,
            userid.c_str(),
            sizeof(out.userid) - 1
        );
    }


    // =================================================
    // TASK ID
    // =================================================

    json.get(
        data,
        "task_id"
    );


    if (data.success)
    {
        String taskId =
            data.to<String>();


        strncpy(
            out.taskId,
            taskId.c_str(),
            sizeof(out.taskId) - 1
        );
    }


    // =================================================
    // COMMAND
    // =================================================

    json.get(
        data,
        "command"
    );


    if (data.success)
    {
        String command =
            data.to<String>();


        strncpy(
            out.command,
            command.c_str(),
            sizeof(out.command) - 1
        );
    }
    else
    {
        strncpy(
            out.command,
            "none",
            sizeof(out.command) - 1
        );
    }


    // =================================================
    // DURATION
    // =================================================

    json.get(
        data,
        "duration"
    );


    if (data.success)
    {
        out.duration =
            data.to<uint32_t>();
    }


    // =================================================
    // REMAINING
    // =================================================

    json.get(
        data,
        "remaining"
    );


    if (data.success)
    {
        out.remaining =
            data.to<uint32_t>();
    }


    // =================================================
    // LAMPS
    // =================================================

    const char* keys[4] =
    {
        "L1",
        "L2",
        "L3",
        "L4"
    };


    for (
        uint8_t i = 0;
        i < 4;
        ++i
    )
    {
        String key =
            String("lamps/") +
            keys[i];


        json.get(
            data,
            key.c_str()
        );


        if (data.success)
        {
            out.lamps[i] =
                data.to<bool>();
        }
    }


    out.valid =
        true;


    return true;
}


// =====================================================
// SAME REMOTE TASK
// =====================================================

bool FirebaseManager::sameRemoteTask(
    const RemoteTaskSnapshot& a,
    const RemoteTaskSnapshot& b
) const
{
    if (
        a.valid != b.valid
    )
    {
        return false;
    }


    if (!a.valid)
    {
        return true;
    }


    if (
        strcmp(
            a.userid,
            b.userid
        ) != 0
    )
    {
        return false;
    }


    if (
        strcmp(
            a.taskId,
            b.taskId
        ) != 0
    )
    {
        return false;
    }


    if (
        strcmp(
            a.status,
            b.status
        ) != 0
    )
    {
        return false;
    }


    if (
        strcmp(
            a.command,
            b.command
        ) != 0
    )
    {
        return false;
    }


    if (
        a.duration != b.duration ||
        a.remaining != b.remaining
    )
    {
        return false;
    }


    for (
        uint8_t i = 0;
        i < 4;
        ++i
    )
    {
        if (
            a.lamps[i] !=
            b.lamps[i]
        )
        {
            return false;
        }
    }


    return true;
}


// =====================================================
// SEND SENSOR
//
// IMPORTANT:
//
// ห้ามเขียน /devices/{SN}
// โดยตรง
//
// Sensor:
// /devices/{SN}/hw_status/sensors
//
// Lastseen:
// /devices/{SN}/lastseen
// =====================================================

bool FirebaseManager::sendSensor(
    const SensorSnapshot& snapshot
)
{
    FirebaseJson json;


    // =================================================
    // SENSOR
    // =================================================

    json.set(
        "uv_raw",
        snapshot.uvRaw
    );

    json.set(
        "uv_voltage",
        snapshot.uvVoltage
    );

    json.set(
        "door_open",
        snapshot.doorOpen
    );


    if (
        snapshot.dhtValid
    )
    {
        json.set(
            "temperature",
            snapshot.temperature
        );

        json.set(
            "humidity",
            snapshot.humidity
        );
    }


    if (
        !Firebase.RTDB.updateNode(
            &_fbdo,
            sensorPath().c_str(),
            &json
        )
    )
    {
        Serial.print(
            "[Firebase Sensor] Update FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }


    // =================================================
    // LASTSEEN
    // =================================================

    time_t timestamp =
        time(nullptr);


    if (
        timestamp > 100000
    )
    {
        if (
            !Firebase.RTDB.setInt(
                &_fbdo,
                lastSeenPath().c_str(),
                (int)timestamp
            )
        )
        {
            Serial.print(
                "[Firebase Lastseen] Update FAILED: "
            );

            Serial.println(
                _fbdo.errorReason()
            );
        }
        else
        {
            Serial.println(
                "[Firebase Lastseen] Updated"
            );
        }
    }


    Serial.println(
        "[Firebase Sensor] Updated"
    );


    queueNetworkActivity();


    return true;
}


// =====================================================
// SEND DOOR
// =====================================================

bool FirebaseManager::sendDoor(
    const SensorSnapshot& snapshot
)
{
    if (
        !Firebase.RTDB.setBool(
            &_fbdo,
            doorPath().c_str(),
            snapshot.doorOpen
        )
    )
    {
        Serial.print(
            "[Firebase Door] Update FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }


    Serial.print(
        "[Firebase Door] door_open = "
    );

    Serial.println(
        snapshot.doorOpen
            ? "true"
            : "false"
    );


    queueNetworkActivity();


    return true;
}


// =====================================================
// SEND HARDWARE
// =====================================================

bool FirebaseManager::sendHardware(
    const HardwareSnapshot& snapshot
)
{
    FirebaseJson json;


    json.set(
        "lamps/1",
        snapshot.lamps[0]
    );

    json.set(
        "lamps/2",
        snapshot.lamps[1]
    );

    json.set(
        "lamps/3",
        snapshot.lamps[2]
    );

    json.set(
        "lamps/4",
        snapshot.lamps[3]
    );

    json.set(
        "motor",
        snapshot.motor
    );

    json.set(
        "busy",
        snapshot.busy
    );


    if (
        !Firebase.RTDB.updateNode(
            &_fbdo,
            hardwarePath().c_str(),
            &json
        )
    )
    {
        Serial.print(
            "[Firebase HW] Update FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }


    Serial.print("[Firebase HW] Status updated: lamps=");
    Serial.print(snapshot.lamps[0]);
    Serial.print(snapshot.lamps[1]);
    Serial.print(snapshot.lamps[2]);
    Serial.print(snapshot.lamps[3]);
    Serial.print(" motor=");
    Serial.print(snapshot.motor);
    Serial.print(" busy=");
    Serial.println(snapshot.busy);


    queueNetworkActivity();


    return true;
}


// =====================================================
// SEND TASK
// =====================================================

bool FirebaseManager::sendTask(
    const TaskWriteSnapshot& snapshot
)
{
    FirebaseJson json;


    json.set(
        "status",
        snapshot.status
    );

    json.set(
        "isRunning",
        snapshot.isRunning
    );

    json.set(
        "remaining",
        (int)snapshot.remaining
    );

    json.set(
        "progress",
        (int)snapshot.progress
    );


    if (
        !Firebase.RTDB.updateNode(
            &_fbdo,
            taskPath().c_str(),
            &json
        )
    )
    {
        Serial.print(
            "[Firebase Task] Update FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }


    Serial.print(
        "[Firebase Task] Updated: "
    );

    Serial.println(
        snapshot.status
    );


    queueNetworkActivity();


    return true;
}


// =====================================================
// SEND COMMAND ACK
// =====================================================

bool FirebaseManager::sendCommandAck(
    const CommandAckSnapshot& snapshot
)
{
    String path = commandPath();

    if (
        !Firebase.RTDB.getString(
            &_fbdo,
            path.c_str()
        )
    )
    {
        Serial.print(
            "[Firebase Command] Read FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }

    String currentCommand =
        _fbdo.stringData();

    // Do not clear a newer command that arrived after the original command.
    if (
        currentCommand !=
        String(snapshot.expectedCommand)
    )
    {
        Serial.print(
            "[Firebase Command] ACK skipped, current="
        );

        Serial.println(
            currentCommand
        );

        return true;
    }

    if (
        !Firebase.RTDB.setString(
            &_fbdo,
            path.c_str(),
            "none"
        )
    )
    {
        Serial.print(
            "[Firebase Command] ACK FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }

    Serial.print(
        "[Firebase Command] ACK: "
    );

    Serial.print(
        snapshot.expectedCommand
    );

    Serial.println(
        " -> none"
    );

    queueNetworkActivity();

    return true;
}


// =====================================================
// SAVE HISTORY
//
// /users/{userid}/history/{task_id}
//
// ไม่ overwrite user data อื่น
// =====================================================

bool FirebaseManager::saveTaskHistory()
{
    char userid[64] = {0};
    char taskId[64] = {0};
    uint32_t duration = 0;
    bool lamps[4] = {false, false, false, false};

    portENTER_CRITICAL(&_mux);

    strncpy(userid, _activeUserId, sizeof(userid) - 1);
    userid[sizeof(userid) - 1] = '\0';

    strncpy(taskId, _activeTaskId, sizeof(taskId) - 1);
    taskId[sizeof(taskId) - 1] = '\0';

    duration = _activeDuration;

    for (uint8_t i = 0; i < 4; ++i) {
        lamps[i] = _activeLamps[i];
    }

    portEXIT_CRITICAL(&_mux);

    if (strlen(userid) == 0)
    {
        Serial.println("[Firebase History] userid missing");
        return false;
    }

    if (strlen(taskId) == 0)
    {
        Serial.println("[Firebase History] task_id missing");
        return false;
    }

    String path = historyPath(userid, taskId);

    FirebaseJson json;

    json.set("task_id", taskId);
    json.set("userid", userid);
    json.set("duration", (int)duration);
    json.set("remaining", 0);
    json.set("progress", 100);
    json.set("status", "completed");

    json.set("lamps/L1", lamps[0]);
    json.set("lamps/L2", lamps[1]);
    json.set("lamps/L3", lamps[2]);
    json.set("lamps/L4", lamps[3]);

    if (!Firebase.RTDB.updateNode(
            &_fbdo,
            path.c_str(),
            &json))
    {
        Serial.print("[Firebase History] Save FAILED: ");
        Serial.println(_fbdo.errorReason());
        return false;
    }

    Serial.print("[Firebase History] Saved: ");
    Serial.println(path);

    queueNetworkActivity();

    return true;
}


// =====================================================
// DELETE TASK
// =====================================================

bool FirebaseManager::deleteTask(
    const char* expectedTaskId
)
{
    /*
     * Guard against this race:
     *
     * Task A finishes
     *   -> DELETE is queued
     *   -> Android creates Task B
     *   -> worker blindly deletes /task
     *   -> Task B is lost
     *
     * We verify task_id immediately before deleting.
     */
    if (expectedTaskId != nullptr &&
        expectedTaskId[0] != '\0')
    {
        String taskIdPath =
            taskPath() + "/task_id";

        if (Firebase.RTDB.getString(
                &_fbdo,
                taskIdPath.c_str()))
        {
            String currentTaskId =
                _fbdo.stringData();

            if (currentTaskId.length() > 0 &&
                currentTaskId != expectedTaskId)
            {
                Serial.print(
                    "[Firebase Task] DELETE SKIPPED - newer task detected: "
                );
                Serial.println(currentTaskId);

                // The old delete request is obsolete. Keep the new task
                // and do not reset _remoteSnapshot in the caller.
                portENTER_CRITICAL(&_mux);
                _deleteSuperseded = true;
                portEXIT_CRITICAL(&_mux);

                return true;
            }
        }
        else
        {
            /*
             * If the task_id read itself failed, do not blindly delete.
             * Retry so a transient Firebase/network failure cannot remove
             * a newer task.
             */
            Serial.print(
                "[Firebase Task] Delete guard read FAILED: "
            );
            Serial.println(
                _fbdo.errorReason()
            );
            return false;
        }
    }

    if (
        !Firebase.RTDB.deleteNode(
            &_fbdo,
            taskPath().c_str()
        )
    )
    {
        Serial.print(
            "[Firebase Task] Delete FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return false;
    }

    Serial.println(
        "[Firebase Task] Task deleted"
    );

    queueNetworkActivity();

    return true;
}


// =====================================================
// PATH: TASK
// =====================================================

String FirebaseManager::taskPath() const
{
    return
        String("/devices/") +
        _deviceSN +
        "/task";
}


// =====================================================
// PATH: COMMAND
// =====================================================

String FirebaseManager::commandPath() const
{
    return
        taskPath() +
        "/command";
}


// =====================================================
// PATH: HARDWARE
// =====================================================

String FirebaseManager::hardwarePath() const
{
    return
        String("/devices/") +
        _deviceSN +
        "/hw_status";
}


// =====================================================
// PATH: SENSOR
// =====================================================

String FirebaseManager::sensorPath() const
{
    return
        hardwarePath() +
        "/sensors";
}


// =====================================================
// PATH: DOOR
// =====================================================

String FirebaseManager::doorPath() const
{
    return
        sensorPath() +
        "/door_open";
}


// =====================================================
// PATH: LASTSEEN
// =====================================================

String FirebaseManager::lastSeenPath() const
{
    return
        String("/devices/") +
        _deviceSN +
        "/lastseen";
}


// =====================================================
// PATH: HISTORY
// =====================================================

String FirebaseManager::historyPath(
    const char* userid,
    const char* taskId
) const
{
    return
        String("/users/") +
        userid +
        "/history/" +
        taskId;
}


// =====================================================
// ERROR STATE
// =====================================================

void FirebaseManager::setErrorStateFromWorker()
{
    _ready = false;
}


// =====================================================
// READY
// =====================================================

bool FirebaseManager::isReady() const
{
    return _ready;
}


// =====================================================
// TIMEOUT
// =====================================================

bool FirebaseManager::isTimeout() const
{
    return _timeout;
}