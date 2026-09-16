#include "FirebaseManager.h"
#include "StatusLED.h"

#include <ESP32Time.h>
#include <time.h>

#include "FirebaseFrontPanelTask.h"

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
      _lastSeenUpdate(0),
      _startTime(0),
      _timeout(false),
      _lastTaskCheck(0)
{
}


// =====================================================
// BEGIN
// =====================================================

void FirebaseManager::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("      FIREBASE START");
    Serial.println("==============================");


    _config.api_key =
        _apiKey;

    _config.database_url =
        _databaseUrl;


    _auth.user.email =
        _email;

    _auth.user.password =
        _password;


    Firebase.begin(
        &_config,
        &_auth
    );


    Firebase.reconnectWiFi(true);


    _startTime =
        millis();

    _timeout = false;


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
    FirebaseFrontPanelTask& firebaseTask
)
{
    if (
        WiFi.status() != WL_CONNECTED
    )
    {
        _ready = false;
        return;
    }


    if (!Firebase.ready())
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
    // LAST SEEN
    // =================================================

    if (
        now - _lastSeenUpdate >=
        LASTSEEN_INTERVAL
    )
    {
        struct tm timeinfo;


        if (
            getLocalTime(
                &timeinfo,
                1000
            )
        )
        {
            time_t timestamp =
                time(nullptr);


            Serial.printf(
                "[NTP] %04d-%02d-%02d %02d:%02d:%02d\n",

                timeinfo.tm_year + 1900,

                timeinfo.tm_mon + 1,

                timeinfo.tm_mday,

                timeinfo.tm_hour,

                timeinfo.tm_min,

                timeinfo.tm_sec
            );


            updateLastSeen(
                timestamp
            );


            _lastSeenUpdate = now;
        }
        else
        {
            Serial.println(
                "[NTP] Time not ready"
            );
        }
    }


    // =================================================
    // TASK
    // =================================================

    updateTask(
        now,
        firebaseTask
    );
}


// =====================================================
// UPDATE TASK
// =====================================================

void FirebaseManager::updateTask(
    unsigned long now,
    FirebaseFrontPanelTask& firebaseTask
)
{
    // -------------------------------------------------
    // Check every 1 second
    // -------------------------------------------------

    if (
        now - _lastTaskCheck <
        TASK_CHECK_INTERVAL
    )
    {
        return;
    }

    _lastTaskCheck = now;


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
        _statusLED.networkActivity(now);
        return;
    }


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

    if (data.success)
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

    if (data.success)
    {
        command =
            data.to<String>();
    }


    // =================================================
    // COMMAND: CANCEL
    // =================================================
    //
    // cancel เป็น command แบบ one-shot
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


        deleteTask(path);

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


            firebaseTask.start(now);


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


            deleteTask(path);


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


            deleteTask(path);


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
            lastProgressWrite = now;


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
    // =================================================
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

    if (data.success)
    {
        duration =
            data.to<uint32_t>();
    }


    if (duration == 0)
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

    if (data.success)
    {
        lamp1 =
            data.to<bool>();
    }


    json.get(
        data,
        "lamps/L2"
    );

    if (data.success)
    {
        lamp2 =
            data.to<bool>();
    }


    json.get(
        data,
        "lamps/L3"
    );

    if (data.success)
    {
        lamp3 =
            data.to<bool>();
    }


    json.get(
        data,
        "lamps/L4"
    );

    if (data.success)
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
        !firebaseTask.receiveTask(task)
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


        firebaseTask.start(now);


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
        _statusLED.networkActivity(millis());
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
        _statusLED.networkActivity(millis());
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


// =====================================================
// LAST SEEN
// =====================================================

void FirebaseManager::updateLastSeen(
    time_t timestamp
)
{
    String path =
        "/devices/";

    path += _deviceSN;
    path += "/lastseen";


    if (
        Firebase.RTDB.setInt(
            &_fbdo,
            path.c_str(),
            (int)timestamp
        )
    )
    {
        _statusLED.networkActivity(millis());
        Serial.print(
            "[Firebase] lastseen: "
        );

        Serial.println(
            (long)timestamp
        );
    }
    else
    {
        Serial.print(
            "[Firebase] lastseen FAILED: "
        );

        Serial.println(
            _fbdo.errorReason()
        );
    }
}