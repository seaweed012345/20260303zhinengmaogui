#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor() 
    : _dht(0, DHT22), _pin(0), _initialized(false) {}

bool TemperatureSensor::init(uint8_t pin, unsigned long timeout_ms) {
    _pin = pin;
    _dht = DHT(pin, DHT22);
    _dht.begin();
    _initialized = false;

    unsigned long t_start = millis();
    while (millis() - t_start < timeout_ms) {
        float temp = _dht.readTemperature();
        float humi = _dht.readHumidity();
        if (!isnan(temp) && !isnan(humi)) {
            _initialized = true;
            Serial.printf("DHT22 initialized on pin %d\n", pin);
            return true;
        }
        delay(50);
    }
    Serial.printf("【TemperatureSensor】DHT22未响应，初始化超时！\n");
    return false;
}

float TemperatureSensor::readTemperature() {
    if (!_initialized) return 0.0f;
    float temperature = _dht.readTemperature();
    if (isnan(temperature)) {
        Serial.println("Failed to read temperature from DHT22");
        return 0.0f;
    }
    return temperature;
}

float TemperatureSensor::readHumidity() {
    if (!_initialized) return 0.0f;
    float humidity = _dht.readHumidity();
    if (isnan(humidity)) {
        Serial.println("Failed to read humidity from DHT22");
        return 0.0f;
    }
    return humidity;
}

bool TemperatureSensor::isAvailable() {
    return _initialized;

}