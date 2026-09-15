#pragma once

#include <Arduino.h>
#include <WiFi.h>


class WiFiManager
{
public:

    enum State
    {
        CONNECTING,
        ONLINE,
        OFFLINE
    };


    WiFiManager(
        const char* ssid,
        const char* password,
        unsigned long timeout = 15000,
        unsigned long retryInterval = 30000
    );


    void begin();

    void update();


    bool isConnecting() const;

    bool isOnline() const;

    bool isOffline() const;


    State getState() const;

    IPAddress getIP() const;


private:

    const char* _ssid;

    const char* _password;


    unsigned long _timeout;

    unsigned long _retryInterval;


    unsigned long _connectStart = 0;

    unsigned long _retryTimer = 0;


    State _state = OFFLINE;


    void startConnection();

    void enterOffline();
};