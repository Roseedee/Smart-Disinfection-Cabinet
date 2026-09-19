#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

#define FIREBASE_CONNECT_TIMEOUT 15000


// =====================================================
// FORWARD DECLARATION
// =====================================================

class FirebaseFrontPanelTask;
class StatusLED;
class DisinfectionController;
class TaskManager;
class SensorManager;

// =====================================================
// FIREBASE MANAGER
// =====================================================

class FirebaseManager {
public:

  FirebaseManager(
    const char* apiKey,
    const char* databaseUrl,
    const char* email,
    const char* password,
    const char* deviceSN,
    StatusLED& statusLED);


  void begin();


  // =================================================
  // UPDATE
  // =================================================

  void update(
    unsigned long now,
    FirebaseFrontPanelTask& firebaseTask,
    DisinfectionController& disinfection,
    TaskManager& taskManager,
    SensorManager& sensors);


  bool isReady() const;

  bool isTimeout() const;


private:

  // =================================================
  // STATUS LED
  // =================================================

  StatusLED& _statusLED;


  // =================================================
  // CONFIG
  // =================================================

  const char* _apiKey;
  const char* _databaseUrl;
  const char* _email;
  const char* _password;
  const char* _deviceSN;


  // =================================================
  // FIREBASE
  // =================================================

  FirebaseData _fbdo;

  FirebaseAuth _auth;

  FirebaseConfig _config;


  // =================================================
  // STATE
  // =================================================

  bool _ready;

  unsigned long _startTime;

  bool _timeout;


  // =================================================
  // LAST SEEN
  // =================================================

  unsigned long _lastSeenUpdate;
  unsigned long _lastSensorUpdate;

  static const unsigned long LASTSEEN_INTERVAL = 5000;
  static const unsigned long SENSOR_UPDATE_INTERVAL = 5000;


  // =================================================
  // FIREBASE TASK
  // =================================================

  unsigned long _lastTaskCheck;

  static const unsigned long TASK_CHECK_INTERVAL = 1000;


  // =================================================
  // HARDWARE STATE
  // =================================================

  unsigned long _lastHardwareStateAttempt;

  static const unsigned long HARDWARE_STATE_RETRY_INTERVAL = 1000;


  // =================================================
  // FUNCTIONS
  // =================================================

  void updateHeartbeat(
    unsigned long now,
    SensorManager& sensors);

  void updateDoorState(
    unsigned long now,
    SensorManager& sensors);


  void updateTask(
    unsigned long now,
    FirebaseFrontPanelTask& firebaseTask,
    TaskManager& taskManager);


  void updateHardwareState(
    unsigned long now,
    DisinfectionController& disinfection,
    TaskManager& taskManager);


  void updateTaskValues(
    const String& path,
    const char* status,
    bool isRunning,
    const char* command,
    uint32_t remaining,
    uint8_t progress);


  void deleteTask(
    const String& path);
};

#endif