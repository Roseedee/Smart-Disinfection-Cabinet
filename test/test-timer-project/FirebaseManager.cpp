#include "FirebaseManager.h"

#include "StatusLED.h"
#include "DisinfectionController.h"
#include "TaskManager.h"
#include "SensorManager.h"

#include <ESP32Time.h>
#include <time.h>

#include "FirebaseFrontPanelTask.h"


// =====================================================
// RTC
// =====================================================

ESP32Time rtc(0);


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
    : _apiKey(apiKey),
      _databaseUrl(databaseUrl),
      _email(email),
      _password(password),
      _deviceSN(deviceSN),

      _statusLED(statusLED),

      _ready(false),
      _startTime(0),
      _timeout(false),

      _lastSeenUpdate(0),
      _lastSensorUpdate(0),

      _lastTaskCheck(0),

      _lastHardwareStateAttempt(0)
{
}


// =====================================================
// BEGIN
// =====================================================

void FirebaseManager::begin()
{
    Serial.println();
    Serial.println(
        "=============================="
    );

    Serial.println(
        "      FIREBASE START"
    );

    Serial.println(
        "=============================="
    );


    // =================================================
    // CONFIG
    // =================================================

    _config.api_key =
        _apiKey;

    _config.database_url =
        _databaseUrl;


    // =================================================
    // AUTH
    // =================================================

    _auth.user.email =
        _email;

    _auth.user.password =
        _password;


    // =================================================
    // FIREBASE
    // =================================================

    Firebase.begin(
        &_config,
        &_auth
    );


    Firebase.reconnectWiFi(
        true
    );


    // =================================================
    // STATE
    // =================================================

    _startTime =
        millis();

    _timeout = false;

    _ready = false;

    _lastSeenUpdate = 0;
    _lastSensorUpdate = 0;

    _lastTaskCheck = 0;

    _lastHardwareStateAttempt = 0;


    // =================================================
    // NTP
    // =================================================

    configTime(
        7 * 3600,
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );


    Serial.println(
        "[NTP] Time sync started"
    );

    Serial.println(
        "[Firebase] Begin"
    );
}


// =====================================================
// UPDATE
// =====================================================

void FirebaseManager::update(
    unsigned long now,
    FirebaseFrontPanelTask& firebaseTask,
    DisinfectionController& disinfection,
    TaskManager& taskManager,
    SensorManager& sensors
)
{
    // =================================================
    // WIFI
    // =================================================

    if (
        WiFi.status() != WL_CONNECTED
    )
    {
        _ready = false;

        return;
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
            !_timeout &&
            now - _startTime >=
            FIREBASE_CONNECT_TIMEOUT
        )
        {
            _timeout = true;


            Serial.println();

            Serial.println(
                "[Firebase] CONNECTION TIMEOUT"
            );

            Serial.println(
                "[Firebase] OFFLINE MODE"
            );
        }


        return;
    }


    // =================================================
    // READY
    // =================================================

    if (!_ready)
    {
        _ready = true;

        _timeout = false;


        Serial.println();

        Serial.println(
            "[Firebase] READY"
        );


        Serial.print(
            "[Firebase] Device SN: "
        );

        Serial.println(
            _deviceSN
        );
    }


    // =================================================
    // HEARTBEAT + SENSOR
    //
    // ทุก 5 วินาที
    // lastseen + sensor ใน request เดียว
    // =================================================

    updateHeartbeat(
        now,
        sensors
    );


    // =====================================================
// FIREBASE TASK
    //
    // ทำก่อน Hardware State
    //
    // เพื่อให้ start/stop/cancel
    // เปลี่ยน busy/relay แล้วส่ง state
    // ในรอบเดียวกัน
    // =================================================

    updateTask(
        now,
        firebaseTask,
        taskManager
    );


    // =================================================
    // HARDWARE STATE
    //
    // ส่งเมื่อ:
    //
    // Lamp/Motor เปลี่ยน
    // หรือ
    // Busy เปลี่ยน
    // =================================================

    updateHardwareState(
        now,
        disinfection,
        taskManager
    );
}


// =====================================================
// UPDATE HARDWARE STATE
//
// Path:
//
// /devices/{SN}/hw_status
//
// ส่ง:
// lamps
// motor
// busy
//
// เฉพาะเมื่อมีการเปลี่ยนแปลง
// =====================================================

void FirebaseManager::updateHardwareState(
    unsigned long now,
    DisinfectionController& disinfection,
    TaskManager& taskManager
)
{
    // =================================================
    // CHECK CHANGE
    // =================================================

    if (
        !disinfection.hasStateChanged() &&
        !taskManager.hasBusyStateChanged()
    )
    {
        return;
    }


    // =================================================
    // RETRY
    // =================================================

    if (
        _lastHardwareStateAttempt != 0 &&
        now - _lastHardwareStateAttempt <
        HARDWARE_STATE_RETRY_INTERVAL
    )
    {
        return;
    }


    _lastHardwareStateAttempt =
        now;


    // =================================================
    // JSON
    // =================================================

    FirebaseJson json;


    // =================================================
    // LAMPS
    // =================================================

    json.set(
        "lamps/1",
        disinfection.getLamp(1)
    );

    json.set(
        "lamps/2",
        disinfection.getLamp(2)
    );

    json.set(
        "lamps/3",
        disinfection.getLamp(3)
    );

    json.set(
        "lamps/4",
        disinfection.getLamp(4)
    );


    // =================================================
    // MOTOR
    // =================================================

    json.set(
        "motor",
        disinfection.isMotorOn()
    );


    // =================================================
    // BUSY
    // =================================================

    json.set(
        "busy",
        taskManager.isBusy()
    );


    // =================================================
    // PATH
    // =================================================

    String path =
        "/devices/";

    path += _deviceSN;

    path += "/hw_status";


    // =================================================
    // SEND
    // =================================================

    if (
        Firebase.RTDB.updateNode(
            &_fbdo,
            path.c_str(),
            &json
        )
    )
    {
        // ---------------------------------------------
        // สำเร็จ
        // ---------------------------------------------

        disinfection.clearStateChanged();

        taskManager.clearBusyStateChanged();


        _statusLED.networkActivity(
            millis()
        );


        Serial.println(
            "[Firebase HW] Status updated"
        );
    }
    else
    {
        // ---------------------------------------------
        // FAIL
        //
        // ห้าม clear state
        // เพื่อให้ retry
        // ---------------------------------------------

        Serial.print(
            "[Firebase HW] Update FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );
    }
}


// =====================================================
// UPDATE TASK
// =====================================================

void FirebaseManager::updateTask(
    unsigned long now,
    FirebaseFrontPanelTask& firebaseTask,
    TaskManager& taskManager
)
{
    // =================================================
    // CHECK EVERY 1 SECOND
    // =================================================

    if (
        now - _lastTaskCheck <
        TASK_CHECK_INTERVAL
    )
    {
        return;
    }


    _lastTaskCheck =
        now;


    // =================================================
    // PATH
    // =================================================

    String path =
        "/devices/";

    path += _deviceSN;

    path += "/task";


    // =================================================
    // READ TASK
    // =================================================

    if (
        !Firebase.RTDB.getJSON(
            &_fbdo,
            path.c_str()
        )
    )
    {
        // ---------------------------------------------
        // อ่านไม่ได้
        //
        // ไม่เรียก networkActivity()
        // เพราะ request ไม่สำเร็จ
        // ---------------------------------------------

        Serial.print(
            "[Firebase Task] Read FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        return;
    }


    // =================================================
    // READ สำเร็จ
    // =================================================

    _statusLED.networkActivity(
        now
    );


    FirebaseJson json =
        _fbdo.jsonObject();

    FirebaseJsonData data;


    // =================================================
    // READ STATUS
    // =================================================

    String remoteStatus = "";


    json.get(
        data,
        "status"
    );


    if (
        data.success
    )
    {
        remoteStatus =
            data.to<String>();
    }


    // =================================================
    // READ COMMAND
    // =================================================

    String command = "none";


    json.get(
        data,
        "command"
    );


    if (
        data.success
    )
    {
        command =
            data.to<String>();
    }


    // =================================================
    // COMMAND: CANCEL
    // =================================================

    if (
        command == "cancel"
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


        // ---------------------------------------------
        // ให้ TaskManager กลับเป็นว่างแน่นอน
        // ---------------------------------------------

        if (
            taskManager.hasTask()
        )
        {
            taskManager.clearTask();
        }


        deleteTask(
            path
        );


        return;
    }


    // =================================================
    // EXISTING TASK
    // =================================================

    if (
        firebaseTask.hasTask()
    )
    {
        // =================================================
        // COMMAND: START
        // =================================================

        if (
            command == "start" &&
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
                updateTaskValues(
                    path,
                    "running",
                    true,
                    "none",
                    firebaseTask.getRemaining(),
                    firebaseTask.getProgress()
                );
            }


            return;
        }


        // =================================================
        // COMMAND: STOP
        // =================================================

        if (
            command == "stop" &&
            firebaseTask.isRunning()
        )
        {
            Serial.println(
                "[Firebase Task] COMMAND: STOP"
            );


            firebaseTask.stop();


            updateTaskValues(
                path,
                "paused",
                false,
                "none",
                firebaseTask.getRemaining(),
                firebaseTask.getProgress()
            );


            return;
        }


        // =================================================
        // LOCAL BUTTON START / STOP
        // =================================================

        if (
            firebaseTask.consumeLocalStateChanged()
        )
        {
            if (
                firebaseTask.isRunning()
            )
            {
                Serial.println(
                    "[Firebase Task] LOCAL START"
                );


                updateTaskValues(
                    path,
                    "running",
                    true,
                    "none",
                    firebaseTask.getRemaining(),
                    firebaseTask.getProgress()
                );
            }
            else
            {
                Serial.println(
                    "[Firebase Task] LOCAL STOP"
                );


                updateTaskValues(
                    path,
                    "paused",
                    false,
                    "none",
                    firebaseTask.getRemaining(),
                    firebaseTask.getProgress()
                );
            }


            return;
        }


        // =================================================
        // FINISHED
        // =================================================

        if (
            firebaseTask.consumeFinishedEvent()
        )
        {
            Serial.println(
                "[Firebase Task] FINISHED"
            );


            deleteTask(
                path
            );


            firebaseTask.clearTask();


            return;
        }


        // =================================================
        // CANCELLED
        // =================================================

        if (
            firebaseTask.consumeCancelledEvent()
        )
        {
            Serial.println(
                "[Firebase Task] CANCELLED"
            );


            if (
                taskManager.hasTask()
            )
            {
                taskManager.clearTask();
            }


            deleteTask(
                path
            );


            firebaseTask.clearTask();


            return;
        }


        // =================================================
        // PROGRESS
        // =================================================

        static unsigned long lastProgressWrite = 0;


        if (
            firebaseTask.isRunning() &&
            now - lastProgressWrite >= 1000
        )
        {
            lastProgressWrite =
                now;


            updateTaskValues(
                path,
                "running",
                true,
                "none",
                firebaseTask.getRemaining(),
                firebaseTask.getProgress()
            );
        }


        return;
    }


    // =================================================
    // NO LOCAL TASK
    //
    // รับเฉพาะ pending
    // =================================================

    if (
        remoteStatus != "pending"
    )
    {
        return;
    }


    // =================================================
    // DURATION
    // =================================================

    uint32_t duration = 0;


    json.get(
        data,
        "duration"
    );


    if (
        data.success
    )
    {
        duration =
            data.to<uint32_t>();
    }


    if (
        duration == 0
    )
    {
        Serial.println(
            "[Firebase Task] Invalid duration"
        );


        return;
    }


    // =================================================
    // LAMPS
    // =================================================

    bool lamp1 = false;

    bool lamp2 = false;

    bool lamp3 = false;

    bool lamp4 = false;


    json.get(
        data,
        "lamps/L1"
    );

    if (
        data.success
    )
    {
        lamp1 =
            data.to<bool>();
    }


    json.get(
        data,
        "lamps/L2"
    );

    if (
        data.success
    )
    {
        lamp2 =
            data.to<bool>();
    }


    json.get(
        data,
        "lamps/L3"
    );

    if (
        data.success
    )
    {
        lamp3 =
            data.to<bool>();
    }


    json.get(
        data,
        "lamps/L4"
    );

    if (
        data.success
    )
    {
        lamp4 =
            data.to<bool>();
    }


    // =================================================
    // CREATE TASK
    // =================================================

    Task task;


    task.create(
        TaskSource::FIREBASE,

        duration,

        lamp1,
        lamp2,
        lamp3,
        lamp4
    );


    // =================================================
    // RECEIVE TASK
    // =================================================

    if (
        !firebaseTask.receiveTask(
            task
        )
    )
    {
        return;
    }


    Serial.println();

    Serial.println(
        "[Firebase Task] NEW TASK RECEIVED"
    );


    // =================================================
    // COMMAND START IMMEDIATELY
    // =================================================

    if (
        command == "start"
    )
    {
        Serial.println(
            "[Firebase Task] START IMMEDIATELY"
        );


        firebaseTask.start(
            now
        );


        if (
            firebaseTask.isRunning()
        )
        {
            updateTaskValues(
                path,
                "running",
                true,
                "none",
                firebaseTask.getRemaining(),
                firebaseTask.getProgress()
            );


            return;
        }
    }


    // =================================================
    // ACCEPTED
    // =================================================

    updateTaskValues(
        path,
        "accepted",
        false,
        "none",
        firebaseTask.getRemaining(),
        firebaseTask.getProgress()
    );
}


// =====================================================
// UPDATE TASK VALUES
// =====================================================

void FirebaseManager::updateTaskValues(
    const String& path,
    const char* status,
    bool isRunning,
    const char* command,
    uint32_t remaining,
    uint8_t progress
)
{
    FirebaseJson json;


    json.set(
        "status",
        status
    );


    json.set(
        "isRunning",
        isRunning
    );


    json.set(
        "command",
        command
    );


    json.set(
        "remaining",
        (int)remaining
    );


    json.set(
        "progress",
        (int)progress
    );


    if (
        Firebase.RTDB.updateNode(
            &_fbdo,
            path.c_str(),
            &json
        )
    )
    {
        _statusLED.networkActivity(
            millis()
        );


        Serial.print(
            "[Firebase Task] Updated: "
        );


        Serial.println(
            status
        );
    }
    else
    {
        Serial.print(
            "[Firebase Task] Update FAILED: "
        );


        Serial.println(
            _fbdo.errorReason()
        );
    }
}


// =====================================================
// DELETE TASK
// =====================================================

void FirebaseManager::deleteTask(
    const String& path
)
{
    if (
        Firebase.RTDB.deleteNode(
            &_fbdo,
            path.c_str()
        )
    )
    {
        _statusLED.networkActivity(
            millis()
        );


        Serial.println(
            "[Firebase Task] Task deleted"
        );
    }
    else
    {
        Serial.print(
            "[Firebase Task] Delete failed: "
        );


        Serial.println(
            _fbdo.errorReason()
        );
    }
}


// =====================================================
// HEARTBEAT + SENSOR
// =====================================================

void FirebaseManager::updateHeartbeat(
    unsigned long now,
    SensorManager& sensors
)
{
    // -------------------------------------------------
    // DOOR CHANGE HAS PRIORITY
    // -------------------------------------------------
    // ส่งทันทีเมื่อ door state เปลี่ยน
    // ไม่ต้องรอ 5 วินาที
    // และไม่ผูกกับ NTP
    // -------------------------------------------------

    if (sensors.hasDoorStateChanged())
    {
        updateDoorState(now, sensors);
    }


    // -------------------------------------------------
    // PERIODIC UPDATE
    // -------------------------------------------------
    // lastseen + sensor ทุก 5 วินาที
    // -------------------------------------------------

    if (
        now - _lastSeenUpdate < LASTSEEN_INTERVAL &&
        now - _lastSensorUpdate < SENSOR_UPDATE_INTERVAL
    )
    {
        return;
    }


    // =================================================
    // TIME
    // =================================================

    time_t timestamp =
        time(nullptr);


    // =================================================
    // PATH
    // =================================================

    String devicePath =
        "/devices/";

    devicePath += _deviceSN;


    // =================================================
    // SENSOR PATH
    // =================================================

    String sensorPath =
        devicePath;

    sensorPath += "/hw_status/sensors";


    // =================================================
    // PERIODIC SENSOR JSON
    // =================================================

    bool periodicSensorDue =
        (now - _lastSensorUpdate >= SENSOR_UPDATE_INTERVAL);


    if (periodicSensorDue)
    {
        FirebaseJson sensorJson;

        sensorJson.set(
            "uv_raw",
            sensors.getUVRaw()
        );

        sensorJson.set(
            "uv_voltage",
            sensors.getUVVoltage()
        );

        sensorJson.set(
            "door_open",
            sensors.isDoorOpen()
        );

        if (sensors.hasDHTData())
        {
            sensorJson.set(
                "temperature",
                sensors.getTemperature()
            );

            sensorJson.set(
                "humidity",
                sensors.getHumidity()
            );
        }


        if (
            Firebase.RTDB.updateNode(
                &_fbdo,
                sensorPath.c_str(),
                &sensorJson
            )
        )
        {
            _lastSensorUpdate = now;

            _statusLED.networkActivity(now);

            Serial.println(
                "[Firebase Sensor] Updated"
            );

            // ถ้าการ update รอบนี้ส่ง door state ได้สำเร็จ
            // เคลียร์ event ได้เลย
            sensors.clearDoorStateChanged();
        }
        else
        {
            Serial.print(
                "[Firebase Sensor] Update FAILED: "
            );

            Serial.println(
                _fbdo.errorReason()
            );
        }
    }


    // =================================================
    // LAST SEEN
    // =================================================

    // if (
    //     now - _lastSeenUpdate >= LASTSEEN_INTERVAL
    // )
    // {
    //     // NTP ยังไม่พร้อม -> retry รอบหน้า
    //     if (timestamp <= 100000)
    //     {
    //         Serial.println(
    //             "[NTP] Time not ready"
    //         );
    //     }
    //     else
    //     {
    //         if (
    //             Firebase.RTDB.setInt(
    //                 &_fbdo,
    //                 (devicePath + "/lastseen").c_str(),
    //                 (int)timestamp
    //             )
    //         )
    //         {
    //             _lastSeenUpdate = now;

    //             _statusLED.networkActivity(now);

    //             Serial.println(
    //                 "[Firebase] Lastseen updated"
    //             );
    //         }
    //         else
    //         {
    //             Serial.print(
    //                 "[Firebase] Lastseen FAILED: "
    //             );

    //             Serial.println(
    //                 _fbdo.errorReason()
    //             );
    //         }
    //     }
    // }
}


// =====================================================
// UPDATE DOOR STATE
// =====================================================
// ส่ง door_open ทันทีเมื่อมีการเปลี่ยนสถานะ
// =====================================================

void FirebaseManager::updateDoorState(
    unsigned long now,
    SensorManager& sensors
)
{
    String path =
        "/devices/";

    path += _deviceSN;
    path += "/hw_status/sensors/door_open";


    if (
        Firebase.RTDB.setBool(
            &_fbdo,
            path.c_str(),
            sensors.isDoorOpen()
        )
    )
    {
        sensors.clearDoorStateChanged();

        _statusLED.networkActivity(now);

        Serial.print(
            "[Firebase Door] door_open = "
        );

        Serial.println(
            sensors.isDoorOpen() ? "true" : "false"
        );
    }
    else
    {
        Serial.print(
            "[Firebase Door] Update FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );

        // ไม่ clear event -> รอบต่อไปจะ retry
    }
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