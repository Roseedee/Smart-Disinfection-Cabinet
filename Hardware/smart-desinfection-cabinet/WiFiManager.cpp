#include "WiFiManager.h"


// =====================================================
// CONSTRUCTOR
// =====================================================

WiFiManager::WiFiManager(
    const char* ssid,
    const char* password,
    unsigned long timeout,
    unsigned long retryInterval
)
    : _ssid(ssid),
      _password(password),
      _timeout(timeout),
      _retryInterval(retryInterval)
{
}


// =====================================================
// BEGIN
// =====================================================

void WiFiManager::begin()
{
    WiFi.mode(WIFI_STA);

    WiFi.disconnect();

    delay(50);

    startConnection();
}


// =====================================================
// START CONNECTION
// =====================================================

void WiFiManager::startConnection()
{
    Serial.println("[WiFi] Connecting...");

    WiFi.begin(
        _ssid,
        _password
    );

    _connectStart = millis();

    _state = CONNECTING;
}


// =====================================================
// UPDATE
// =====================================================

void WiFiManager::update()
{
    // =================================================
    // CONNECTED
    // =================================================

    if (WiFi.status() == WL_CONNECTED)
    {
        if (_state != ONLINE)
        {
            _state = ONLINE;

            Serial.println("[WiFi] ONLINE");

            Serial.print("[WiFi] IP: ");
            Serial.println(WiFi.localIP());
        }

        return;
    }


    // =================================================
    // CONNECTING
    // =================================================

    if (_state == CONNECTING)
    {
        if (
            millis() - _connectStart
            >= _timeout
        )
        {
            Serial.println("[WiFi] TIMEOUT");

            enterOffline();
        }

        return;
    }


    // =================================================
    // OFFLINE
    // =================================================

    if (_state == OFFLINE)
    {
        if (
            millis() - _retryTimer
            >= _retryInterval
        )
        {
            startConnection();
        }
    }
}


// =====================================================
// ENTER OFFLINE
// =====================================================

void WiFiManager::enterOffline()
{
    WiFi.disconnect();

    _state = OFFLINE;

    _retryTimer = millis();

    Serial.println("[WiFi] OFFLINE");
}


// =====================================================
// IS CONNECTING
// =====================================================

bool WiFiManager::isConnecting() const
{
    return _state == CONNECTING;
}


// =====================================================
// IS ONLINE
// =====================================================

bool WiFiManager::isOnline() const
{
    return _state == ONLINE;
}


// =====================================================
// IS OFFLINE
// =====================================================

bool WiFiManager::isOffline() const
{
    return _state == OFFLINE;
}


// =====================================================
// GET STATE
// =====================================================

WiFiManager::State
WiFiManager::getState() const
{
    return _state;
}


// =====================================================
// GET IP
// =====================================================

IPAddress WiFiManager::getIP() const
{
    return WiFi.localIP();
}