#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// =====================================================
// WiFi
// =====================================================

#define WIFI_SSID     "ssid"
#define WIFI_PASSWORD "password"

// =====================================================
// Firebase
// =====================================================

#define API_KEY       "apikey"
#define DATABASE_URL  "dashboardurl/"

// =====================================================
// Device
// =====================================================

#define DEVICE_PATH "/devices/esp_001"

// =====================================================
// GPIO
// =====================================================

const int lamp_pin[4] = {
    15,   // L1
    2,   // L2
    4,   // L3
    16    // L4
};

// =====================================================
// Firebase objects
// =====================================================

FirebaseData fbdo;
FirebaseData stream;

FirebaseAuth auth;
FirebaseConfig config;

// =====================================================
// Stream
// =====================================================

bool streamStarted = false;

// =====================================================
// Timer Task
// =====================================================

bool timerActive = false;

unsigned long timerStart = 0;
unsigned long timerDuration = 0;

String currentTaskId = "";

bool timerLamp[4] = {
    false,
    false,
    false,
    false
};

// =====================================================
// Function declarations
// =====================================================

void updateCurrentLamps();
void checkTaskStream();
void startTimerTask();
void processTimerTask();
void finishTask();


// =====================================================
// Setup
// =====================================================

void setup()
{
    Serial.begin(115200);

    // -------------------------
    // GPIO
    // -------------------------

    for (int i = 0; i < 4; i++)
    {
        pinMode(lamp_pin[i], OUTPUT);

        // OFF ตอนเริ่มต้น
        digitalWrite(lamp_pin[i], LOW);
    }

    // -------------------------
    // WiFi
    // -------------------------

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("WiFi connected");

    // -------------------------
    // Firebase config
    // -------------------------

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // ถ้าใช้ Anonymous Authentication
    auth.user.email = "email";
    auth.user.password = "password";

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

    Serial.println("Firebase initialized");
}


// =====================================================
// Loop
// =====================================================

void loop()
{
    if (Firebase.ready())
    {
        // ---------------------------------
        // เริ่ม Stream
        // ---------------------------------

        if (!streamStarted)
        {
            // อ่านสถานะ Lamp ปัจจุบันก่อน
            updateCurrentLamps();

            // เปิด Stream สำหรับ Task
            if (Firebase.RTDB.beginStream(
                    &stream,
                    String(DEVICE_PATH).c_str()))
            {
                streamStarted = true;

                Serial.println("Task stream started");
            }
            else
            {
                Serial.println(stream.errorReason());
            }
        }
        else
        {
            // ตรวจ Task ใหม่
            checkTaskStream();
        }
    }

    // ---------------------------------
    // ประมวลผล Timer
    // ---------------------------------

    processTimerTask();
}


// =====================================================
// อ่านสถานะ Lamp ปัจจุบัน
// =====================================================

void updateCurrentLamps()
{
    String path = String(DEVICE_PATH) + "/lamps";

    if (!Firebase.RTDB.getJSON(&fbdo, path.c_str()))
    {
        Serial.print("Get lamp failed: ");
        Serial.println(fbdo.errorReason());

        return;
    }

    FirebaseJson &json = fbdo.jsonObject();

    FirebaseJsonData data;

    // -------------------------
    // L1
    // -------------------------

    if (json.get(data, "L1"))
    {
        if (data.typeNum == FirebaseJson::JSON_BOOL)
        {
            digitalWrite(
                lamp_pin[0],
                data.boolValue ? HIGH : LOW
            );
        }
    }

    // -------------------------
    // L2
    // -------------------------

    if (json.get(data, "L2"))
    {
        if (data.typeNum == FirebaseJson::JSON_BOOL)
        {
            digitalWrite(
                lamp_pin[1],
                data.boolValue ? HIGH : LOW
            );
        }
    }

    // -------------------------
    // L3
    // -------------------------

    if (json.get(data, "L3"))
    {
        if (data.typeNum == FirebaseJson::JSON_BOOL)
        {
            digitalWrite(
                lamp_pin[2],
                data.boolValue ? HIGH : LOW
            );
        }
    }

    // -------------------------
    // L4
    // -------------------------

    if (json.get(data, "L4"))
    {
        if (data.typeNum == FirebaseJson::JSON_BOOL)
        {
            digitalWrite(
                lamp_pin[3],
                data.boolValue ? HIGH : LOW
            );
        }
    }

    Serial.println("Current lamp status loaded");
}


// =====================================================
// ตรวจ Task จาก Firebase Stream
// =====================================================

void checkTaskStream()
{
    if (!Firebase.RTDB.readStream(&stream))
    {
        Serial.println(stream.errorReason());

        return;
    }

    if (!stream.streamAvailable())
        return;


    // =================================================
    // Path
    // =================================================

    String path = stream.dataPath();

    Serial.print("Task Path: ");
    Serial.println(path);

    Serial.print("Task Type: ");
    Serial.println(stream.dataType());


    // =================================================
    // Task ใหม่
    //
    // เช่น
    // /tasks/task_001
    // =================================================

    if (stream.dataType() == "json")
    {
        FirebaseJson &json = stream.jsonObject();

        FirebaseJsonData data;


        // ---------------------------------------------
        // อ่าน status
        // ---------------------------------------------

        if (!json.get(data, "status"))
            return;

        String status = data.stringValue;

        Serial.print("Task status: ");
        Serial.println(status);


        // ---------------------------------------------
        // สนใจเฉพาะ pending
        // ---------------------------------------------

        if (status != "pending")
            return;


        // ---------------------------------------------
        // ป้องกันรับ Task ใหม่ขณะ Task เดิมกำลังทำ
        // ---------------------------------------------

        if (timerActive)
        {
            Serial.println("Task already running");

            return;
        }


        // ---------------------------------------------
        // เอา Task ID
        //
        // /tasks/task_001
        //
        // จะได้
        //
        // task_001
        // ---------------------------------------------

        if (path.startsWith("/tasks/"))
        {
            currentTaskId =
                path.substring(
                    String("/tasks/").length()
                );
        }
        else
        {
            return;
        }


        Serial.print("Task ID: ");
        Serial.println(currentTaskId);


        // =================================================
        // Duration
        // =================================================

        if (!json.get(data, "duration"))
        {
            Serial.println("Task has no duration");

            return;
        }

        timerDuration = data.intValue;


        // =================================================
        // Lamps
        // =================================================

        timerLamp[0] = false;
        timerLamp[1] = false;
        timerLamp[2] = false;
        timerLamp[3] = false;


        if (json.get(data, "lamps/L1"))
        {
            timerLamp[0] = data.boolValue;
        }

        if (json.get(data, "lamps/L2"))
        {
            timerLamp[1] = data.boolValue;
        }

        if (json.get(data, "lamps/L3"))
        {
            timerLamp[2] = data.boolValue;
        }

        if (json.get(data, "lamps/L4"))
        {
            timerLamp[3] = data.boolValue;
        }


        // =================================================
        // เริ่ม Task
        // =================================================

        startTimerTask();
    }
}


// =====================================================
// เริ่ม Timer Task
// =====================================================

void startTimerTask()
{
    timerStart = millis();

    timerActive = true;


    // =================================================
    // เปิด Lamp ตาม Task
    // =================================================

    for (int i = 0; i < 4; i++)
    {
        digitalWrite(
            lamp_pin[i],
            timerLamp[i] ? HIGH : LOW
        );
    }


    // =================================================
    // เปลี่ยนสถานะ Task
    //
    // pending → running
    // =================================================

    String path =
        String(DEVICE_PATH) +
        "/tasks/" +
        currentTaskId +
        "/status";


    if (!Firebase.RTDB.setString(
            &fbdo,
            path.c_str(),
            "running"))
    {
        Serial.print("Set running failed: ");
        Serial.println(fbdo.errorReason());
    }


    // =================================================
    // startedAt
    // =================================================

    // ตรงนี้ยังไม่ใช้เวลา Unix จริง
    // เพราะต้องมี NTP/RTC ก่อน
    //
    // ถ้าต้องการเวลาใน History จริง
    // เราจะเพิ่มภายหลัง
    // =================================================


    Serial.println("-------------------------");
    Serial.println("Task started");

    Serial.print("Task: ");
    Serial.println(currentTaskId);

    Serial.print("Duration: ");
    Serial.print(timerDuration);
    Serial.println(" seconds");

    Serial.print("Lamps: ");

    for (int i = 0; i < 4; i++)
    {
        if (timerLamp[i])
        {
            Serial.print("L");
            Serial.print(i + 1);
            Serial.print(" ");
        }
    }

    Serial.println();
    Serial.println("-------------------------");
}


// =====================================================
// ตรวจ Timer
// =====================================================

void processTimerTask()
{
    if (!timerActive)
        return;


    unsigned long elapsed =
        millis() - timerStart;


    // =================================================
    // ครบเวลา
    // =================================================

    if (elapsed >= timerDuration * 1000UL)
    {
        finishTask();
    }
}


// =====================================================
// Task เสร็จ
// =====================================================

void finishTask()
{
    // =================================================
    // ปิดเฉพาะ Lamp ที่ Task เปิด
    // =================================================

    for (int i = 0; i < 4; i++)
    {
        if (timerLamp[i])
        {
            digitalWrite(
                lamp_pin[i],
                LOW
            );
        }
    }


    timerActive = false;


    // =================================================
    // Task Path
    // =================================================

    String taskPath =
        String(DEVICE_PATH) +
        "/tasks/" +
        currentTaskId;


    // =================================================
    // เปลี่ยน status
    //
    // running → completed
    // =================================================

    if (!Firebase.RTDB.setString(
            &fbdo,
            (taskPath + "/status").c_str(),
            "completed"))
    {
        Serial.print("Set completed failed: ");
        Serial.println(fbdo.errorReason());
    }


    // =================================================
    // สร้าง History
    // =================================================

    FirebaseJson history;


    history.set(
        "taskId",
        currentTaskId
    );

    history.set(
        "duration",
        (int)timerDuration
    );

    history.set(
        "status",
        "completed"
    );


    history.set(
        "lamps/L1",
        timerLamp[0]
    );

    history.set(
        "lamps/L2",
        timerLamp[1]
    );

    history.set(
        "lamps/L3",
        timerLamp[2]
    );

    history.set(
        "lamps/L4",
        timerLamp[3]
    );


    // =================================================
    // Firebase Push ID
    // =================================================

    String historyPath =
        String(DEVICE_PATH) +
        "/history";


    if (!Firebase.RTDB.pushJSON(
            &fbdo,
            historyPath.c_str(),
            &history))
    {
        Serial.print("History failed: ");
        Serial.println(fbdo.errorReason());
    }
    else
    {
        Serial.println("History saved");
    }


    // =================================================
    // Clear Task
    // =================================================

    currentTaskId = "";

    timerDuration = 0;

    for (int i = 0; i < 4; i++)
    {
        timerLamp[i] = false;
    }


    Serial.println("-------------------------");
    Serial.println("Task completed");
    Serial.println("-------------------------");
}