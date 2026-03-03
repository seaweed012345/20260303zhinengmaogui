#include "WeightSensor.h"
#include "Config.h"
#include "PinDefinitions.h"

#include <Preferences.h>

WeightSensor::WeightSensor() 
    : _doutPin(0), _sckPin(0), _calibrationFactor(2230.0f), _initialized(false),
      _lastStateChangeTime(0), _lastWeight(0.0f), _emptyWeight(0.0f) {}

// 初始化：注意调用前可先通过 loadCalibration(id) 加载 calibration
bool WeightSensor::init(uint8_t doutPin, uint8_t sckPin, unsigned long timeout_ms) {
    _doutPin = doutPin;
    _sckPin = sckPin;

    _loadCell.begin(doutPin, sckPin);
    // 应用当前 _calibrationFactor（loadCalibration 可能已经修改了它）
    _loadCell.set_scale(_calibrationFactor);

    unsigned long t_start = millis();
    while (!_loadCell.is_ready()) {
        if (millis() - t_start > timeout_ms) {
            Serial.println("【WeightSensor】HX711未响应，初始化超时！");
            _initialized = false;
            return false;
        }
        delay(10);
    }

    _loadCell.tare();
    _emptyWeight = readWeight();
    _initialized = true;
    Serial.printf("HX711 initialized on pins DOUT:%d, SCK:%d, Empty weight: %.2fg\n", 
                 doutPin, sckPin, _emptyWeight);
    Serial.printf("Weight sensor calibration factor: %.2f\n", _calibrationFactor);
    return true;
}

float WeightSensor::readWeight() {
    if (!_initialized) return 0.0f;
    if (_loadCell.is_ready()) {
        // get_units uses current scale factor
        float weight = _loadCell.get_units(3);
        return weight > 0 ? weight : 0;
    }
    return 0.0f;
}

float WeightSensor::readFoodWeight() { return readWeight(); }
float WeightSensor::readWaterWeight() { return readWeight(); }
float WeightSensor::readLitterWeight() { return readWeight(); }

// calibrate: 假设 caller 已经在空载时执行了 tare()
// 流程（手动/产线）:
//  1) 发送 TAKE_TARE id  -> 设备执行 tare() 并记录 emptyWeight
//  2) 将已知砝码放到该传感器上并等待稳定
//  3) 发送 TAKE_WEIGHT id <grams> -> 调用本函数, 它会读取 raw counts 并计算 factor
void WeightSensor::calibrate(float knownWeight) {
    if (!_initialized) {
        Serial.println("WeightSensor not initialized - cannot calibrate");
        return;
    }
    if (knownWeight <= 0.0f) {
        Serial.println("Invalid knownWeight for calibration");
        return;
    }

    Serial.println("Calibrating weight sensor...");

    // 1) 把 scale 设为 1.0，使 get_value()/get_units() 返回原始计数（get_value 更稳妥）
    _loadCell.set_scale(1.0f);

    // NOTE: 不要在这里再执行 tare()，因为你应该在空载时单独执行 TAKE_TARE。
    // 如果在这里执行 tare() 会把放了砝码的读数清零导致无效校准（这是你之前遇到的问题根源）。

    delay(500); // 给 HX711 一点时间稳定

    // 2) 读取原始计数值（get_value 返回的通常是 raw - offset）
    long raw = 0;
    // 使用库的 get_value(samples) 如果可用；如果你的库不同请替换为相应的 API
    raw = _loadCell.get_value(10); // 10 samples average

    if (raw <= 0) {
        Serial.println("Calibration failed: invalid raw reading");
        return;
    }

    // 3) 计算校准因子：raw_counts_per_gram = raw / knownWeight
    float factor = (float)raw / knownWeight;

    // 保护性检查：防止把明显错误的因子写入
    if (factor < 1.0f || factor > 1000000.0f) {
        Serial.printf("Calibration produced out-of-range factor: %.2f (abort)\n", factor);
        return;
    }

    _calibrationFactor = factor;
    // 将新因子应用到 HX711（以便后续 get_units 返回克）
    _loadCell.set_scale(_calibrationFactor);

    Serial.printf("Calibration complete. Factor: %.2f\n", _calibrationFactor);
}

// 可用性判断
bool WeightSensor::isAvailable() { return _initialized; }

// 去皮函数（此函数会更新内部 _emptyWeight）
void WeightSensor::tare() {
    if (_initialized) {
        _loadCell.tare();
        // give a moment then read current weight (should be 0 after tare)
        delay(200);
        _emptyWeight = readWeight();
    }
}
void WeightSensor::tareFood() { tare(); Serial.println("Food scale tared"); }
void WeightSensor::tareWater() { tare(); Serial.println("Water scale tared"); }
void WeightSensor::tareLitter() { tare(); Serial.println("Litter scale tared"); }

// --- set/get calibration factor（可选择保存） ---
void WeightSensor::setCalibrationFactor(float factor, bool save) {
    if (factor <= 0.0f) return;
    _calibrationFactor = factor;
    _loadCell.set_scale(_calibrationFactor);
    if (save) {
        Serial.println("setCalibrationFactor: factor set (not saved). Use saveCalibration(id) to persist.");
    }
}
float WeightSensor::getCalibrationFactor() const {
    return _calibrationFactor;
}

// --- 持久化保存/读取校准因子与 tare（使用 Preferences） ---
bool WeightSensor::saveCalibration(uint8_t id) {
    Preferences prefs;
    if (id > 10) return false;
    char key[16];
    snprintf(key, sizeof(key), "scale%d", id);
    prefs.begin("scale", false);
    prefs.putFloat(key, _calibrationFactor);
    prefs.end();

    // 保存空载读数（以克为单位）
    snprintf(key, sizeof(key), "tare%d", id);
    prefs.begin("scale", false);
    prefs.putFloat(key, _emptyWeight);
    prefs.end();

    Serial.printf("Saved calibration: id=%d factor=%.2f tare=%.2f\n", id, _calibrationFactor, _emptyWeight);
    return true;
}

bool WeightSensor::loadCalibration(uint8_t id) {
    Preferences prefs;
    if (id > 10) return false;
    char key[16];
    snprintf(key, sizeof(key), "scale%d", id);
    prefs.begin("scale", true);
    float f = prefs.getFloat(key, NAN);
    prefs.end();

    if (!isnan(f) && f > 0.0f) {
        _calibrationFactor = f;
        Serial.printf("Loaded calibration factor for id=%d : %.2f\n", id, _calibrationFactor);
    } else {
        Serial.printf("No saved calibration factor for id=%d, using default %.2f\n", id, _calibrationFactor);
    }

    // 读取 tare（如果有）
    snprintf(key, sizeof(key), "tare%d", id);
    prefs.begin("scale", true);
    float t = prefs.getFloat(key, NAN);
    prefs.end();
    if (!isnan(t)) {
        _emptyWeight = t;
        Serial.printf("Loaded tare for id=%d : %.2f\n", id, _emptyWeight);
    } else {
        Serial.printf("No saved tare for id=%d\n", id);
    }

    // 如果已经初始化，立即应用 scale
    if (_initialized) {
        _loadCell.set_scale(_calibrationFactor);
        // if a tare value was loaded, optionally set offset - HX711 library handles tare by offset only at runtime
    }
    return true;
}

// 单独保存/加载 tare（若需要）
bool WeightSensor::saveTare(uint8_t id) {
    Preferences prefs;
    if (id > 10) return false;
    char key[16];
    snprintf(key, sizeof(key), "tare%d", id);
    prefs.begin("scale", false);
    prefs.putFloat(key, _emptyWeight);
    prefs.end();
    Serial.printf("Saved tare for id=%d : %.2f\n", id, _emptyWeight);
    return true;
}

bool WeightSensor::loadTare(uint8_t id) {
    Preferences prefs;
    if (id > 10) return false;
    char key[16];
    snprintf(key, sizeof(key), "tare%d", id);
    prefs.begin("scale", true);
    float t = prefs.getFloat(key, NAN);
    prefs.end();
    if (!isnan(t)) {
        _emptyWeight = t;
        Serial.printf("Loaded tare for id=%d : %.2f\n", id, _emptyWeight);
        return true;
    }
    return false;
}