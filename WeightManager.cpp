#include "WeightManager.h"
#include "PinDefinitions.h"
#include <Arduino.h>

// ========== 推荐：后台线程/定时任务调用此方法 ==========
// 注意：如果你使用了 WeightSensor::loadCalibration(id) ，应该在 init 前调用
void WeightManager::sensorTask() {
    while (true) {
        for (int i = 0; i < SENSOR_COUNT; ++i) {
            lastWeights[i] = sensors[i].readWeight();
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms采样一次
    }
}

WeightManager::WeightManager() {}

void WeightManager::begin() {
    // 在 init 之前加载每通道的校准因子（如果存在），以便 init 时 set_scale 使用正确的因子
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        sensors[i].loadCalibration(i); // 尝试从 Preferences 加载 scale/tare（若存在则覆盖默认）
    }

    sensors[0].init(WEIGHT_FOOD_DOUT, WEIGHT_FOOD_SCK, 1000);
    sensors[1].init(WEIGHT_WATER_DOUT, WEIGHT_WATER_SCK, 1000);
    sensors[2].init(WEIGHT_LITTER_DOUT, WEIGHT_LITTER_SCK, 1000);
    // 初始化一次缓存
    for (int i = 0; i < SENSOR_COUNT; ++i) lastWeights[i] = sensors[i].readWeight();
}

// 建议：以后不再在主循环频繁调用update()
void WeightManager::update() {
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        lastWeights[i] = sensors[i].readWeight();
    }
}

WeightSensor& WeightManager::getSensor(uint8_t idx) {
    return sensors[idx];
}

float WeightManager::getNetWeight(uint8_t idx) {
    // 以后如有去皮等逻辑在这里加
    return getLastWeight(idx);
}

float WeightManager::getLastWeight(uint8_t idx) {
    return lastWeights[idx];
}