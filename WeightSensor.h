#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include <Arduino.h>
#include <HX711.h>
#include <Preferences.h>

// 传感器数量常量如果在别处定义，这里可以删掉或保持一致
#ifndef SENSOR_COUNT
#define SENSOR_COUNT 3
#endif

class WeightSensor {
public:
    WeightSensor();

    // 初始化传感器（dout, sck 引脚, 超时 ms）
    bool init(uint8_t doutPin, uint8_t sckPin, unsigned long timeout_ms = 1000);

    // 读取实时重量（克）
    float readWeight();

    // 兼容命名
    float readFoodWeight();
    float readWaterWeight();
    float readLitterWeight();

    // 基于已知砝码进行校准（运行时），会更新内部 _calibrationFactor（但不自动保存）
    void calibrate(float knownWeight);

    // 去皮
    void tare();
    void tareFood();
    void tareWater();
    void tareLitter();

    // 是否就绪
    bool isAvailable();

    // 新接口：设置/读取校准因子，save=true 时写入 Preferences
    void setCalibrationFactor(float factor, bool save = false);
    float getCalibrationFactor() const;

    // 新接口：将此传感器的校准因子（以及去皮值）保存/加载到非易失存储
    // id 用于区分多个称重通道（0,1,2）
    bool saveCalibration(uint8_t id);
    bool loadCalibration(uint8_t id);

    // 如果你希望外部直接写入 tare 原始读数（可选）
    bool saveTare(uint8_t id);
    bool loadTare(uint8_t id);

private:
    HX711 _loadCell;
    uint8_t _doutPin;
    uint8_t _sckPin;
    float _calibrationFactor;
    bool _initialized;
    unsigned long _lastStateChangeTime;
    float _lastWeight;
    float _emptyWeight;
};

#endif // WEIGHT_SENSOR_H