#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

class FirebaseManager {
public:

  FirebaseManager(
    const char* apiKey,
    const char* databaseUrl,
    const char* email,
    const char* password,
    const char* deviceSN);

  void begin();

  void update(unsigned long now);

  bool isReady() const;

private:

  void updateLastSeen(time_t timestamp);

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
};

#endif