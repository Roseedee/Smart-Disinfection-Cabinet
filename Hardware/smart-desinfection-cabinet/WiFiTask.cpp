#include "WiFiTask.h"

WiFiTask::WiFiTask(const char* ssid, const char* password)
    : _ssid(ssid),
      _password(password),
      _connected(false),
      _offline(false),
      _everConnected(false),
      _startTime(0),
      _lastReconnectAttempt(0) {}

void WiFiTask::begin() {
    _connected = false;
    _offline = false;
    _everConnected = false;
    _startTime = millis();
    _lastReconnectAttempt = 0;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _password);

    Serial.println();
    Serial.println("======================");
    Serial.println("      WIFI START");
    Serial.println("======================");
    Serial.print("[WiFi] Connecting to: ");
    Serial.println(_ssid);
}

void WiFiTask::update(unsigned long now) {
    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED) {
        if (!_connected) {
            _connected = true;
            _everConnected = true;
            _offline = false;

            Serial.println("[WiFi] CONNECTED");
            Serial.print("[WiFi] IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("[WiFi] RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
        }
        return;
    }

    if (_connected) {
        _connected = false;
        Serial.println("[WiFi] CONNECTION LOST - reconnecting in background");
    }

    // Initial connection timeout keeps the existing OFFLINE behavior.
    if (!_everConnected && !_offline && now - _startTime >= CONNECT_TIMEOUT) {
        _offline = true;
        Serial.println("[WiFi] INITIAL CONNECTION TIMEOUT - OFFLINE MODE");
        WiFi.disconnect(false);
        return;
    }

    // After a successful connection, never block the front panel waiting for WiFi.
    if (_everConnected && now - _lastReconnectAttempt >= RECONNECT_INTERVAL) {
        _lastReconnectAttempt = now;
        WiFi.reconnect();
    }
}

void WiFiTask::reconnect() {
    _connected = false;
    _offline = false;
    _startTime = millis();
    _lastReconnectAttempt = 0;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _password);

    Serial.println("[WiFi] Manual reconnect requested");
}

bool WiFiTask::isConnecting() const { return !_connected && !_offline; }
bool WiFiTask::isConnected() const { return _connected; }
bool WiFiTask::isOffline() const { return _offline; }
