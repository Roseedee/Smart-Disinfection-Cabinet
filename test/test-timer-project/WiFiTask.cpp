#include "WiFiTask.h"

// =====================================================
// CONSTRUCTOR
// =====================================================

WiFiTask::WiFiTask(
    const char* ssid,
    const char* password)
{
    _ssid = ssid;
    _password = password;

    _connected = false;
    _offline = false;

    _startTime = 0;
}


// =====================================================
// BEGIN
// =====================================================

void WiFiTask::begin()
{
    _connected = false;
    _offline = false;

    Serial.println();
    Serial.println("======================");
    Serial.println("      WIFI START");
    Serial.println("======================");

    WiFi.mode(WIFI_STA);

    WiFi.begin(
        _ssid,
        _password
    );

    _startTime = millis();

    Serial.print("[WiFi] Connecting to: ");
    Serial.println(_ssid);
}


// =====================================================
// UPDATE
// =====================================================

void WiFiTask::update(unsigned long now)
{
    // =================================================
    // OFFLINE
    // =================================================
    // ถ้า timeout แล้วจะไม่พยายามเชื่อมใหม่
    // =================================================

    if (_offline)
    {
        return;
    }


    // =================================================
    // CONNECTED
    // =================================================

    if (WiFi.status() == WL_CONNECTED)
    {
        if (!_connected)
        {
            _connected = true;

            Serial.println();
            Serial.println("[WiFi] CONNECTED");

            Serial.print("[WiFi] IP: ");
            Serial.println(WiFi.localIP());

            Serial.print("[WiFi] RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
        }

        return;
    }


    // =================================================
    // CONNECTING
    // =================================================

    if (!_connected)
    {
        // ---------------------------------------------
        // Timeout
        // ---------------------------------------------

        if (now - _startTime >= CONNECT_TIMEOUT)
        {
            _offline = true;

            Serial.println();
            Serial.println("[WiFi] CONNECTION TIMEOUT");
            Serial.println("[WiFi] OFFLINE MODE");


            // ปิด WiFi
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);

            return;
        }
    }
}


// =====================================================
// STATE
// =====================================================

bool WiFiTask::isConnecting() const
{
    return !_connected && !_offline;
}


bool WiFiTask::isConnected() const
{
    return _connected;
}


bool WiFiTask::isOffline() const
{
    return _offline;
}