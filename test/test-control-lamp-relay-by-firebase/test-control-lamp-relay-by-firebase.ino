#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define API_KEY ""
#define DATABASE_URL ""

#define USER_EMAIL ""
#define USER_PASSWORD ""

#define DEVICE_PATH "/devices/esp_001"

const int n_lamp = 4;
const int lamp_pin[n_lamp] = { 15, 2, 4, 16 };

FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool streamStarted = false;

void updateLamps();

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < n_lamp; i++) {
    pinMode(lamp_pin[i], OUTPUT);
    digitalWrite(lamp_pin[i], 0);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Serial.println("Starting Firebase...");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase.begin finished");
}

void loop() {
  if (Firebase.ready()) {
    if (!streamStarted) {
      updateLamps();
      if (Firebase.RTDB.beginStream(&stream, DEVICE_PATH)) {
        streamStarted = true;
        Serial.println("Stream started");
      } else {
        Serial.println(stream.errorReason());
      }
    } else {

      if (Firebase.RTDB.readStream(&stream)) {

        if (stream.streamAvailable()) {
          String path = stream.dataPath();

          Serial.print("Path: ");
          Serial.println(path);
          Serial.print("Type: ");
          Serial.println(stream.dataType());

          if (stream.dataType() == "boolean") {
            bool state = stream.boolData();

            if (path == "/lamp/L1") {
              digitalWrite(lamp_pin[0], state ? HIGH : LOW);
            } else if (path == "/lamp/L2") {
              digitalWrite(lamp_pin[1], state ? HIGH : LOW);
            } else if (path == "/lamp/L3") {
              digitalWrite(lamp_pin[2], state ? HIGH : LOW);
            } else if (path == "/lamp/L4") {
              digitalWrite(lamp_pin[3], state ? HIGH : LOW);
            }
          }
        }
      } else {
        Serial.println(stream.errorReason());
      }
    }
  }
}

void updateLamps() {
  if (Firebase.RTDB.getJSON(&fbdo, DEVICE_PATH "/lamp")) {
    FirebaseJson &json = fbdo.jsonObject();

    FirebaseJsonData data;

    if (json.get(data, "L1") && data.typeNum == FirebaseJson::JSON_BOOL)
      digitalWrite(lamp_pin[0], data.boolValue ? HIGH : LOW);

    if (json.get(data, "L2") && data.typeNum == FirebaseJson::JSON_BOOL)
      digitalWrite(lamp_pin[1], data.boolValue ? HIGH : LOW);

    if (json.get(data, "L3") && data.typeNum == FirebaseJson::JSON_BOOL)
      digitalWrite(lamp_pin[2], data.boolValue ? HIGH : LOW);

    if (json.get(data, "L4") && data.typeNum == FirebaseJson::JSON_BOOL)
      digitalWrite(lamp_pin[3], data.boolValue ? HIGH : LOW);

    Serial.println("Current lamp status loaded");
  } else {
    Serial.println(fbdo.errorReason());
  }
}