#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include "Config.h"

class OverviewPage;

class DisplayManager {
public:
    DisplayManager(TFT_eSPI& tft);     // 构造函数声明
    ~DisplayManager();

    bool init();
    void update();
    void handleTouch();

    void showSplashScreen();
    void nextPage();
    void prevPage();
    void setPage(uint8_t page);

    void setBacklight(bool state);
    void adjustBacklight(uint8_t brightness);

    void drawHealthPage();
    void drawSettingsPage();
    void calibrateTouch();

private:
    TFT_eSPI& _tft;
    OverviewPage* _overviewPage;

    unsigned long _lastTouchTime;
    unsigned long _lastUpdateTime;

    void initDisplay();
    void initTouch();
    bool readTouch(uint16_t* x, uint16_t* y);
    void processTouchEvents();
};

#endif // DISPLAY_MANAGER_H