#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

#define FIREBASE_CONNECT_TIMEOUT 15000

class FirebaseFrontPanelTask;
class StatusLED;

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
        FirebaseFrontPanelTask& firebaseTask
    );

    bool isReady() const;

    bool isTimeout() const;


private:

    StatusLED& _statusLED;

    void updateLastSeen(
        time_t timestamp
    );

    void updateTask(
        unsigned long now,
        FirebaseFrontPanelTask& firebaseTask
    );

    void updateTaskValues(
        const String& path,
        const char* status,
        bool isRunning,
        const char* command,
        uint32_t remaining,
        uint8_t progress
    );

    void deleteTask(
        const String& path
    );


    const char* _apiKey;
    const char* _databaseUrl;
    const char* _email;
    const char* _password;
    const char* _deviceSN;


    FirebaseData _fbdo;
    FirebaseAuth _auth;
    FirebaseConfig _config;


    bool _ready;

    unsigned long _lastSeenUpdate;

    static const unsigned long LASTSEEN_INTERVAL = 5000;


    unsigned long _startTime;
    bool _timeout;


    unsigned long _lastTaskCheck;

    static const unsigned long TASK_CHECK_INTERVAL = 1000;
};

#endif