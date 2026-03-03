// include/Display/Pages/OverviewPage.h
#pragma once
#include <TFT_eSPI.h>
#include "Config.h"

// 前向声明
extern SensorReadings sensorReadings;
extern SystemState systemState;

class OverviewPage {
private:
    TFT_eSPI& _tft;
    unsigned long _lastUpdateTime;
    
    void drawLeftPanel();
    void drawCenterCharts();
    void drawRightControls();
    void drawVerticalBar(int x, int y, int width, int height, uint16_t color, const char* label);
    
public:
    explicit OverviewPage(TFT_eSPI& tft);
    void draw();
    void update();
    void handleTouch(int x, int y);
};