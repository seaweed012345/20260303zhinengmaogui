#pragma once
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SGP30.h>
#include "Config.h"

class EnvironmentSensor {
private:
    DHT _dht;
    Adafruit_SGP30 _sgp;
    bool _sgp30Found;

    // AQ smoothing buffer
    static const int AQ_BUF_SIZE = 8;
    int _aqBuf[AQ_BUF_SIZE];
    int _aqIdx;
    bool _aqInit;

    float readTemperature();
    float readHumidity();
    int readAirQuality();
    int readPressureDiff();
    bool readSGP30(uint16_t* eCO2, uint16_t* TVOC);
    bool initSGP30();

    // helper to save baseline periodically
    void maybeSaveBaseline();

public:
    EnvironmentSensor();
    bool begin();
    void readAll();
    bool isSGP30Found() const;
};