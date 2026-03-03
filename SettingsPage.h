#pragma once
#include <TFT_eSPI.h>
#include "Config.h"

class SettingsPage {
private:
    TFT_eSPI& _tft;
    unsigned long _lastUpdateTime;
    uint8_t _selectedItem;
    
    void drawCalibrationSettings();
    void drawNetworkSettings();
    void drawDisplaySettings();
    void drawSystemSettings();
    void drawButton(int y, const char* text, bool selected);
    
public:
    explicit SettingsPage(TFT_eSPI& tft);
    void draw();
    void update();
    void handleTouch(int x, int y);
    void handleButtonPress();
};