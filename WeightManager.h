#pragma once
#include "WeightSensor.h"
#include "event_types.h"

#define SENSOR_COUNT 3

class WeightManager {
public:
    WeightManager();
    void begin();
    void update(); // 不推荐loop中直接频繁调用，建议后台定时任务调用
    WeightSensor& getSensor(uint8_t idx);
    float getNetWeight(uint8_t idx);

    // 新增：线程安全的缓存读取接口
    float getLastWeight(uint8_t idx);

    // 新增：后台定时、非阻塞采样入口（建议在setup中调xTaskCreate或Ticker）
    void sensorTask();

private:
    WeightSensor sensors[SENSOR_COUNT];
    volatile float lastWeights[SENSOR_COUNT]; // 用volatile保证多线程安全
};