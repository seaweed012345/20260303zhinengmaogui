// include/Display/Pages/HealthPage.h
#pragma once
#include <TFT_eSPI.h>
#include "Config.h"

// 前向声明
extern SensorReadings sensorReadings;
extern SystemState systemState;

class HealthPage {
private:
    TFT_eSPI& _tft;
    unsigned long _lastUpdateTime;
    
    void drawCatHealth();
    void drawSystemHealth();
    void drawFanHealth();
    void drawUsageStats();
    
public:
    explicit HealthPage(TFT_eSPI& tft);
    void draw();
    void update();
    void handleTouch(int x, int y);
};