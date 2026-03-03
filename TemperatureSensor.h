#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <DHT.h>
#include "Config.h"

#define DHT_TYPE DHT22

class TemperatureSensor {
public:
    TemperatureSensor();
    // 新增timeout参数，主程序可直接传，默认1000ms
    bool init(uint8_t pin, unsigned long timeout_ms = 1000);
    float readTemperature();
    float readHumidity();
    bool isAvailable();

private:
    DHT _dht;
    uint8_t _pin;
    bool _initialized;
};

#endif // TEMPERATURE_SENSOR_H