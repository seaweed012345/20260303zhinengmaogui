#include "EnvironmentSensor.h"
#include "Config.h"
#include "PinDefinitions.h"  // 添加这行
#include <Wire.h>
#include <Preferences.h>

EnvironmentSensor::EnvironmentSensor()
    : _dht(DHT_PIN, DHT22),
      _sgp(),
      _sgp30Found(false),
      _aqIdx(0),
      _aqInit(false)
{
    for (int i = 0; i < AQ_BUF_SIZE; ++i) _aqBuf[i] = 0;
}

static float calcAbsoluteHumidity_gm3(float temperature_c, float relative_humidity_percent) {
    // Tetens formula for saturation vapor pressure (hPa)
    float T = temperature_c;
    float RH = relative_humidity_percent;
    float svp = 6.112f * expf((17.62f * T) / (243.12f + T)); // in hPa
    float avp = (RH / 100.0f) * svp; // actual vapor pressure in hPa
    // Absolute humidity (g/m^3) = 216.7 * avp / (T + 273.15)
    float ah = 216.7f * avp / (T + 273.15f);
    return ah;
}

bool EnvironmentSensor::initSGP30() {
    if (_sgp.begin()) {
        _sgp.IAQinit();
        return true;
    }
    return false;
}

bool EnvironmentSensor::begin() {
    Serial.println("Initializing environment sensors...");

    // 初始化DHT22
    _dht.begin();

    // 初始化 SGP30（使用 Adafruit_SGP30 库）
    _sgp30Found = initSGP30();
    Serial.printf("SGP30 %s\n", _sgp30Found ? "found" : "not found");

    if (_sgp30Found) {
        // 尝试从 NVS 恢复 baseline（如果存在）
        Preferences prefs;
        prefs.begin("sgp", true);
        uint32_t b0 = prefs.getUInt("sgp_b0", 0);
        uint32_t b1 = prefs.getUInt("sgp_b1", 0);
        prefs.end();
        if (b0 != 0 || b1 != 0) {
            uint16_t bas_ec = (uint16_t)(b0 & 0xFFFF);
            uint16_t bas_tvoc = (uint16_t)(b1 & 0xFFFF);
            // 恢复 baseline 到传感器
            _sgp.setIAQBaseline(bas_ec, bas_tvoc);
            Serial.printf("SGP30 baseline restored: ec=%u tvoc=%u\n", bas_ec, bas_tvoc);
        } else {
            Serial.println("No saved SGP30 baseline found");
        }
    }

    return true;
}

bool EnvironmentSensor::readSGP30(uint16_t* eCO2, uint16_t* TVOC) {
    if (!_sgp30Found) return false;
    // Adafruit_SGP30 v2 API: IAQmeasure() returns boolean, results in _sgp.eCO2 and _sgp.TVOC
    if (!_sgp.IAQmeasure()) {
        return false;
    }
    // 读取到的值放到调用者提供的指针
    if (eCO2) *eCO2 = _sgp.eCO2;
    if (TVOC) *TVOC = _sgp.TVOC;
    return true;
}

void EnvironmentSensor::maybeSaveBaseline() {
    if (!_sgp30Found) return;
    static unsigned long lastSaveMs = 0;
    const unsigned long saveInterval = 12UL * 3600UL * 1000UL; // 12 hours
    if (millis() - lastSaveMs < saveInterval) return;

    uint16_t bas_ec = 0, bas_tvoc = 0;
    // Adafruit_SGP30 provides getIAQBaseline(&eco2, &tvoc)
    if (_sgp.getIAQBaseline(&bas_ec, &bas_tvoc)) {
        Preferences prefs;
        prefs.begin("sgp", false);
        prefs.putUInt("sgp_b0", (uint32_t)bas_ec);
        prefs.putUInt("sgp_b1", (uint32_t)bas_tvoc);
        prefs.end();
        lastSaveMs = millis();
        Serial.printf("Saved SGP30 baseline: ec=%u tvoc=%u\n", bas_ec, bas_tvoc);
    } else {
        Serial.println("Failed to get SGP30 baseline");
    }
}

void EnvironmentSensor::readAll() {
    float temp = readTemperature();
    float hum = readHumidity();
    sensorReadings.environment.temperature = temp;
    sensorReadings.environment.humidity = hum;

    // SGP30 humidity compensation + read
    if (_sgp30Found) {
        // 计算绝对湿度 (g/m^3)
        float ah = calcAbsoluteHumidity_gm3(temp, hum);
        // Adafruit_SGP30 expects absolute humidity scaled as uint32_t ah * 1000 (per common examples)
        uint32_t ah_scaled = (uint32_t)(ah * 1000.0f + 0.5f);
        _sgp.setHumidity(ah_scaled);

        uint16_t eCO2 = 0, TVOC = 0;
        if (readSGP30(&eCO2, &TVOC)) {
            // TVOC smoothing buffer (简单移动平均)
            _aqBuf[_aqIdx] = (int)TVOC;
            _aqIdx = (_aqIdx + 1) % AQ_BUF_SIZE;
            if (!_aqInit && _aqIdx == 0) _aqInit = true;
            int sum = 0;
            int cnt = _aqInit ? AQ_BUF_SIZE : _aqIdx;
            for (int i = 0; i < cnt; ++i) sum += _aqBuf[i];
            int tvoc_smoothed = (cnt > 0) ? (sum / cnt) : TVOC;

            int aqi = map(constrain(tvoc_smoothed, 0, 1000), 0, 1000, 0, 100);
            sensorReadings.environment.airQuality = aqi;
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("SGP30 read eCO2=%u TVOC=%u smoothedTVOC=%d AQ=%d\n", eCO2, TVOC, tvoc_smoothed, aqi);
            #endif
        } else {
            Serial.println("SGP30 IAQmeasure failed");
        }

        // 周期保存 baseline
        maybeSaveBaseline();
    } else {
        // 模拟数据
        static int simulatedAQI = 35;
        simulatedAQI = constrain(simulatedAQI + random(-2, 3), 20, 60);
        sensorReadings.environment.airQuality = simulatedAQI;
    }

    sensorReadings.environment.pressureDiff = readPressureDiff();

    Serial.printf("Environment: %.1f°C, %.1f%%, AQ: %d, PressureDiff: %d\n",
                 sensorReadings.environment.temperature,
                 sensorReadings.environment.humidity,
                 sensorReadings.environment.airQuality,
                 sensorReadings.environment.pressureDiff);
}

float EnvironmentSensor::readTemperature() {
    float temp = _dht.readTemperature();
    return isnan(temp) ? 25.0f : temp;
}

float EnvironmentSensor::readHumidity() {
    float humidity = _dht.readHumidity();
    return isnan(humidity) ? 50.0f : humidity;
}

int EnvironmentSensor::readAirQuality() {
    // 保留兼容接口（会在 readAll() 中填充结构）
    return sensorReadings.environment.airQuality;
}

int EnvironmentSensor::readPressureDiff() {
    const int numReadings = 5;
    long sum = 0;

    for (int i = 0; i < numReadings; i++) {
        sum += analogRead(PRESSURE_PIN);
        delay(2);
    }

    int average = sum / numReadings;
    int pressureDiff = map(average, 0, 4095, 0, 100);

    return constrain(pressureDiff, 0, 100);
}

bool EnvironmentSensor::isSGP30Found() const {
    return _sgp30Found;
}