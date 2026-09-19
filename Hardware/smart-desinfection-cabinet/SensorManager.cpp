#include "SensorManager.h"


SensorManager::SensorManager(
    uint8_t uvPin,
    uint8_t dhtPin,
    uint8_t doorPin
)
    : _uvPin(uvPin),
      _dhtPin(dhtPin),
      _doorPin(doorPin),
      _dht(dhtPin, DHT11),
      _uvRaw(0),
      _uvVoltage(0.0f),
      _temperature(0.0f),
      _humidity(0.0f),
      _doorOpen(false),
      _doorRawState(false),
      _doorChanged(false),
      _dhtValid(false),
      _lastDHTRead(0),
      _doorChangeStart(0)
{
}


void SensorManager::begin()
{
    pinMode(_uvPin, INPUT);
    analogSetPinAttenuation(_uvPin, ADC_11db);

    // GPIO34 เป็น input-only
    // ใช้ external pull-up 10K ไป 3.3V
    // Door switch ต่อ GPIO34 -> GND เมื่อประตู CLOSED
    // LOW  = CLOSED
    // HIGH = OPEN
    pinMode(_doorPin, INPUT);

    _dht.begin();

    readUV();

    bool initialDoorOpen =
        (digitalRead(_doorPin) == HIGH);

    _doorRawState = initialDoorOpen;
    _doorOpen = initialDoorOpen;
    _doorChanged = false;
    _doorChangeStart = millis();

    readDHT();

    _lastDHTRead = millis();
}


void SensorManager::update(unsigned long now)
{
    readUV();
    readDoor(now);

    if (now - _lastDHTRead >= DHT_INTERVAL)
    {
        _lastDHTRead = now;
        readDHT();
    }
}


void SensorManager::readUV()
{
    _uvRaw = analogRead(_uvPin);

    _uvVoltage =
        ((float)_uvRaw / 4095.0f) * 3.3f;
}


void SensorManager::readDoor(unsigned long now)
{
    // LOW  = CLOSED
    // HIGH = OPEN
    bool rawOpen =
        (digitalRead(_doorPin) == HIGH);

    if (rawOpen != _doorRawState)
    {
        _doorRawState = rawOpen;
        _doorChangeStart = now;
        return;
    }

    if (rawOpen != _doorOpen)
    {
        if (now - _doorChangeStart >= DOOR_DEBOUNCE)
        {
            _doorOpen = rawOpen;
            _doorChanged = true;

            Serial.print("[SENSOR] DOOR: ");
            Serial.println(
                _doorOpen ? "OPEN" : "CLOSED"
            );
        }
    }
}


void SensorManager::readDHT()
{
    float humidity = _dht.readHumidity();
    float temperature = _dht.readTemperature();

    if (!isnan(humidity) && !isnan(temperature))
    {
        _humidity = humidity;
        _temperature = temperature;
        _dhtValid = true;
    }
}


int SensorManager::getUVRaw() const
{
    return _uvRaw;
}


float SensorManager::getUVVoltage() const
{
    return _uvVoltage;
}


float SensorManager::getTemperature() const
{
    return _temperature;
}


float SensorManager::getHumidity() const
{
    return _humidity;
}


bool SensorManager::isDoorOpen() const
{
    return _doorOpen;
}


bool SensorManager::hasDoorStateChanged() const
{
    return _doorChanged;
}


void SensorManager::clearDoorStateChanged()
{
    _doorChanged = false;
}


bool SensorManager::hasDHTData() const
{
    return _dhtValid;
}
