#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "Password"

#define API_KEY "api key"
#define DATABASE_URL "database url"

#define USER_EMAIL "email"
#define USER_PASSWORD "password"

#define LED_PIN 2

FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;
bool streamStarted = false;

void setup()
{
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
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

void loop()
{
    if (Firebase.ready())
    {
        if (!streamStarted)
        {
            if (Firebase.RTDB.beginStream(&stream, "/devices/esp_001"))
            {
                streamStarted = true;
                Serial.println("Stream started");
            }
            else
            {
                Serial.println(stream.errorReason());
            }
        }

        if (streamStarted)
        {
            if (Firebase.RTDB.readStream(&stream))
            {
                if (stream.streamAvailable())
                {
                    Serial.println(stream.dataType());
                    Serial.println(stream.boolData());
                    digitalWrite(LED_PIN, stream.boolData() ? 1 : 0);
                }
            }else {
                Serial.println(stream.errorReason());
            }
        }
    }
}