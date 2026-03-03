#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Config.h"  // 类型定义在 Config.h 中

class StateManager {
private:
    unsigned long _lastDailyReset;
    
public:
    StateManager();
    void update();
    
    void checkDailyReset();
    void resetDailyData();
    void updateFanHealth();
    void processLitterEvents();
    void updateFanState();
    void handleCommand(const JsonObject& command);
};