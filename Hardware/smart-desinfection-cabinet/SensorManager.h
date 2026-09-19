#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

class SensorManager
{
public:
    SensorManager(
        uint8_t uvPin,
        uint8_t dhtPin,
        uint8_t doorPin
    );

    void begin();
    void update(unsigned long now);

    int getUVRaw() const;
    float getUVVoltage() const;

    float getTemperature() const;
    float getHumidity() const;

    bool isDoorOpen() const;

    bool hasDoorStateChanged() const;
    void clearDoorStateChanged();

    bool hasDHTData() const;

private:
    uint8_t _uvPin;
    uint8_t _dhtPin;
    uint8_t _doorPin;

    DHT _dht;

    int _uvRaw;
    float _uvVoltage;

    float _temperature;
    float _humidity;

    // Door:
    // LOW  = CLOSED
    // HIGH = OPEN
    bool _doorOpen;
    bool _doorRawState;
    bool _doorChanged;

    bool _dhtValid;

    unsigned long _lastDHTRead;
    unsigned long _doorChangeStart;

    static const unsigned long DHT_INTERVAL = 2000;
    static const unsigned long DOOR_DEBOUNCE = 30;

    void readUV();
    void readDoor(unsigned long now);
    void readDHT();
};

#endif
