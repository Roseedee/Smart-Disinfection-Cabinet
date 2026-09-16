#include "FirebaseManager.h"

#include <ESP32Time.h>
#include <time.h>

ESP32Time rtc(0);


// =====================================================
// CONSTRUCTOR
// =====================================================

FirebaseManager::FirebaseManager(
  const char* apiKey,
  const char* databaseUrl,
  const char* email,
  const char* password,
  const char* deviceSN)
  : _apiKey(apiKey),
    _databaseUrl(databaseUrl),
    _email(email),
    _password(password),
    _deviceSN(deviceSN),
    _ready(false),
    _lastSeenUpdate(0),
    _startTime(0),
    _timeout(false) {
}


// =====================================================
// BEGIN
// =====================================================

void FirebaseManager::begin() {
  Serial.println();
  Serial.println("==============================");
  Serial.println("      FIREBASE START");
  Serial.println("==============================");


  _config.api_key = _apiKey;

  _config.database_url = _databaseUrl;


  // Firebase Account
  _auth.user.email = _email;
  _auth.user.password = _password;


  Firebase.begin(
    &_config,
    &_auth);


  Firebase.reconnectWiFi(true);

  _startTime = millis();
  _timeout = false;

  // NTP Time Sync - Thailand GMT+7
  configTime(
    7 * 3600,
    0,
    "pool.ntp.org",
    "time.nist.gov");

  Serial.println("[NTP] Time sync started");

  Serial.println("[Firebase] Begin");
}
// =====================================================
// UPDATE
// =====================================================

void FirebaseManager::update(unsigned long now) {
  // ---------------------------------------------
  // CHECK WIFI
  // ---------------------------------------------

  if (WiFi.status() != WL_CONNECTED) {
    _ready = false;
    return;
  }


  // ---------------------------------------------
  // CHECK FIREBASE
  // ---------------------------------------------

  if (!Firebase.ready()) {
    _ready = false;

    if (!_timeout && now - _startTime >= FIREBASE_CONNECT_TIMEOUT) {
      _timeout = true;

      Serial.println();
      Serial.println("[Firebase] CONNECTION TIMEOUT");
      Serial.println("[Firebase] OFFLINE MODE");
    }

    return;
  }


  // ---------------------------------------------
  // FIREBASE READY
  // ---------------------------------------------

  if (!_ready) {
    _ready = true;

    Serial.println();
    Serial.println("[Firebase] READY");
    Serial.print("[Firebase] Device SN: ");
    Serial.println(_deviceSN);
  }


  // ---------------------------------------------
  // LAST SEEN INTERVAL
  // ---------------------------------------------

  if (now - _lastSeenUpdate < LASTSEEN_INTERVAL)
    return;


  // ---------------------------------------------
  // GET NTP TIME
  // ---------------------------------------------

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 1000)) {
    Serial.println("[NTP] Time not ready");
    return;
  }


  // ---------------------------------------------
  // UNIX TIMESTAMP
  // ---------------------------------------------

  time_t timestamp = time(nullptr);


  // ---------------------------------------------
  // DEBUG TIME
  // ---------------------------------------------

  Serial.printf(
    "[NTP] %04d-%02d-%02d %02d:%02d:%02d\n",
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec);


  // ---------------------------------------------
  // UPDATE LAST SEEN
  // ---------------------------------------------

  updateLastSeen(timestamp);

  _lastSeenUpdate = now;
}

// =====================================================
// STATUS
// =====================================================

bool FirebaseManager::isReady() const {
  return _ready;
}

bool FirebaseManager::isTimeout() const
{
    return _timeout;
}

void FirebaseManager::updateLastSeen(time_t timestamp) {
  String path = "/devices/";
  path += _deviceSN;
  path += "/lastseen";

  if (Firebase.RTDB.setInt(
        &_fbdo,
        path.c_str(),
        (int)timestamp)) {
    Serial.print("[Firebase] lastseen: ");
    Serial.println((long)timestamp);
  } else {
    Serial.print("[Firebase] lastseen FAILED: ");
    Serial.println(_fbdo.errorReason());
  }
}