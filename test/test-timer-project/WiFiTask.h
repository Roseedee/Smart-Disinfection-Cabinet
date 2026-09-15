#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiTask
{
public:
    WiFiTask(const char* ssid, const char* password);

    void begin();
    void update(unsigned long now);
    void reconnect();

    bool isConnecting() const;
    bool isConnected() const;
    bool isOffline() const;

private:
    const char* _ssid;
    const char* _password;

    bool _connected;
    bool _offline;

    unsigned long _startTime;

    static const unsigned long CONNECT_TIMEOUT = 15000;
};

#endif