#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

// =====================================================
// CONFIG
// =====================================================

#define FIREBASE_CONNECT_TIMEOUT 10000UL
#define FIREBASE_SERVER_TIMEOUT  5000U
#define FIREBASE_SOCKET_TIMEOUT  5000U

#define WORKER_STACK_SIZE        8192
#define SENSOR_INTERVAL          5000UL
#define HARDWARE_RETRY_INTERVAL  1000UL
#define TASK_PROGRESS_INTERVAL   1000UL
#define TASK_POLL_INTERVAL       1000UL
#define FIREBASE_POLL_FAIL_RETRY 200UL
#define FIREBASE_WORKER_IDLE     5UL

class FirebaseFrontPanelTask;
class StatusLED;
class DisinfectionController;
class TaskManager;
class SensorManager;

class FirebaseManager
{
public:

    FirebaseManager(
        const char* apiKey,
        const char* databaseUrl,
        const char* email,
        const char* password,
        const char* deviceSN,
        StatusLED& statusLED
    );

    void begin();

    void update(
        unsigned long now,
        FirebaseFrontPanelTask& firebaseTask,
        DisinfectionController& disinfection,
        TaskManager& taskManager,
        SensorManager& sensors
    );

    bool isReady() const;
    bool isTimeout() const;

private:

    FirebaseData _fbdo;
    FirebaseAuth _auth;
    FirebaseConfig _config;

    StatusLED& _statusLED;

    const char* _apiKey;
    const char* _databaseUrl;
    const char* _email;
    const char* _password;
    const char* _deviceSN;

    TaskHandle_t _workerTask;
    bool _workerStarted;

    static void workerEntry(void* arg);
    void workerLoop();

    volatile bool _ready;
    volatile bool _timeout;
    volatile bool _everReady;
    unsigned long _startTime;

    unsigned long _lastSensorPublish;
    unsigned long _lastDoorPublish;
    unsigned long _lastHardwarePublish;
    unsigned long _lastTaskProgressPublish;

    enum TxFlag : uint8_t
    {
        TX_NONE        = 0,
        TX_DELETE      = 1 << 0,
        TX_COMMAND_ACK = 1 << 1,
        TX_DOOR        = 1 << 2,
        TX_HW          = 1 << 3,
        TX_TASK        = 1 << 4,
        TX_SENSOR      = 1 << 5
    };

    volatile uint8_t _pendingFlags;

    struct SensorSnapshot
    {
        float uvRaw;
        float uvVoltage;
        float temperature;
        float humidity;
        bool dhtValid;
        bool doorOpen;
    };

    struct HardwareSnapshot
    {
        bool lamps[4];
        bool motor;
        bool busy;
    };

    // Status/progress writes NEVER contain command.
    // command is handled by a separate one-shot ACK operation.
    struct TaskWriteSnapshot
    {
        char status[16];
        bool isRunning;
        uint32_t remaining;
        uint8_t progress;
    };

    struct CommandAckSnapshot
    {
        char expectedCommand[16];
    };

    struct RemoteTaskSnapshot
    {
        bool valid;
        char userid[64];
        char taskId[64];
        char status[16];
        char command[16];
        uint32_t duration;
        uint32_t remaining;
        bool lamps[4];
    };

    SensorSnapshot _sensorSnapshot;
    SensorSnapshot _doorSnapshot;
    HardwareSnapshot _hardwareSnapshot;
    TaskWriteSnapshot _taskSnapshot;
    CommandAckSnapshot _commandAckSnapshot;
    RemoteTaskSnapshot _remoteSnapshot;

    volatile uint32_t _sensorSeq;
    volatile uint32_t _doorSeq;
    volatile uint32_t _hardwareSeq;
    volatile uint32_t _taskSeq;
    volatile uint32_t _commandAckSeq;
    volatile uint32_t _deleteSeq;

    volatile uint32_t _doorAckSeq;
    volatile uint32_t _hardwareAckSeq;
    SensorSnapshot _doorAckSnapshot;
    HardwareSnapshot _hardwareAckSnapshot;

    uint32_t _doorAckAppliedSeq;
    uint32_t _hardwareAckAppliedSeq;

    volatile uint32_t _remoteVersion;
    volatile uint32_t _processedRemoteVersion;

    unsigned long _lastRemotePoll;
    uint8_t _remoteReadFailCount;

    volatile bool _deletePending;
    bool _deleteSuperseded;

    char _activeUserId[64];
    char _activeTaskId[64];
    char _deleteTaskId[64];
    uint32_t _activeDuration;
    bool _activeLamps[4];

    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

    volatile bool _networkActivityPending;

    void queueNetworkActivity();
    void consumeNetworkActivity();

    void publishSensor(
        unsigned long now,
        SensorManager& sensors
    );

    void publishDoor(
        unsigned long now,
        SensorManager& sensors
    );

    void publishHardware(
        unsigned long now,
        DisinfectionController& disinfection,
        TaskManager& taskManager
    );

    void publishTaskStateFromPanel(
        FirebaseFrontPanelTask& firebaseTask
    );

    void publishTaskState(
        const char* status,
        bool isRunning,
        uint32_t remaining,
        uint8_t progress
    );

    void requestCommandAck(
        const char* command
    );

    void requestDeleteTask();

    void resetRemoteTaskState();

    void processRemoteTask(
        unsigned long now,
        FirebaseFrontPanelTask& firebaseTask,
        TaskManager& taskManager
    );

    bool readRemoteTask(
        RemoteTaskSnapshot& out
    );

    void pollRemoteTask(
        unsigned long now
    );

    bool sameRemoteTask(
        const RemoteTaskSnapshot& a,
        const RemoteTaskSnapshot& b
    ) const;

    void markRemoteProcessed(
        uint32_t version
    );

    bool copySensorSnapshot(
        SensorSnapshot& out,
        uint32_t& sequence
    );

    bool copyDoorSnapshot(
        SensorSnapshot& out,
        uint32_t& sequence
    );

    bool copyHardwareSnapshot(
        HardwareSnapshot& out,
        uint32_t& sequence
    );

    bool copyTaskSnapshot(
        TaskWriteSnapshot& out,
        uint32_t& sequence
    );

    bool copyCommandAckSnapshot(
        CommandAckSnapshot& out,
        uint32_t& sequence
    );

    uint32_t copyDeleteSequence();

    bool copyRemoteSnapshot(
        RemoteTaskSnapshot& out,
        uint32_t& version
    );

    bool takeTxFlag(
        uint8_t flag
    );

    void clearTxFlagIfSequenceUnchanged(
        uint8_t flag,
        uint32_t sequence
    );

    bool sendSensor(
        const SensorSnapshot& snapshot
    );

    bool sendDoor(
        const SensorSnapshot& snapshot
    );

    bool sendHardware(
        const HardwareSnapshot& snapshot
    );

    bool sendTask(
        const TaskWriteSnapshot& snapshot
    );

    bool sendCommandAck(
        const CommandAckSnapshot& snapshot
    );

    bool saveTaskHistory();

    bool deleteTask(
        const char* expectedTaskId
    );

    String taskPath() const;
    String commandPath() const;
    String hardwarePath() const;
    String sensorPath() const;
    String doorPath() const;
    String lastSeenPath() const;

    String historyPath(
        const char* userid,
        const char* taskId
    ) const;

    void setErrorStateFromWorker();
};

#endif
