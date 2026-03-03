#include "SettingsPage.h"
#include "Config.h"
#include "WeightSensor.h"

// 前向声明
extern WeightSensor weightSensor;
extern SystemConfig systemConfig;

SettingsPage::SettingsPage(TFT_eSPI& tft) 
    : _tft(tft), _lastUpdateTime(0), _selectedItem(0) {
}

void SettingsPage::draw() {
    _tft.fillScreen(TFT_BLACK);
    
    // 标题
    _tft.setTextColor(TFT_WHITE);
    _tft.setTextSize(2);
    _tft.setCursor(10, 10);
    _tft.print("Settings");
    
    drawCalibrationSettings();
    drawNetworkSettings();
    drawDisplaySettings();
    drawSystemSettings();
}

void SettingsPage::update() {
    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < 1000) {
        return;
    }
    
    _lastUpdateTime = currentTime;
    draw();
}

void SettingsPage::drawCalibrationSettings() {
    _tft.setTextColor(TFT_YELLOW);
    _tft.setTextSize(1);
    _tft.setCursor(10, 40);
    _tft.print("Calibration:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(10, 60);
    _tft.printf("Food: %.1f", systemConfig.foodCalibrationFactor);
    
    _tft.setCursor(10, 80);
    // 修改：使用正确的结构体成员
    _tft.printf("Water: %.1f", systemConfig.waterCalibrationFactor);
    
    _tft.setCursor(10, 100);
    _tft.printf("Litter: %.1f", systemConfig.litterCalibrationFactor);
    
    drawButton(120, "Tare Food", _selectedItem == 0);
    drawButton(150, "Tare Water", _selectedItem == 1);
    drawButton(180, "Tare Litter", _selectedItem == 2);
}

void SettingsPage::drawNetworkSettings() {
    _tft.setTextColor(TFT_CYAN);
    _tft.setCursor(200, 40);
    _tft.print("Network:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(200, 60);
    _tft.print("WiFi: Connected");
    
    _tft.setCursor(200, 80);
    _tft.print("MQTT: Disconnected");
}

void SettingsPage::drawDisplaySettings() {
    _tft.setTextColor(TFT_GREEN);
    _tft.setCursor(10, 210);
    _tft.print("Display:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(10, 230);
    _tft.printf("Brightness: %d", 100);
    
    drawButton(250, "Calibrate Touch", _selectedItem == 3);
}

void SettingsPage::drawSystemSettings() {
    _tft.setTextColor(TFT_MAGENTA);
    _tft.setCursor(200, 210);
    _tft.print("System:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(200, 230);
    _tft.print("Version: 1.0.0");
    
    drawButton(250, "Reset Stats", _selectedItem == 4);
    drawButton(280, "Reboot", _selectedItem == 5);
}

void SettingsPage::drawButton(int y, const char* text, bool selected) {
    _tft.fillRect(10, y, 150, 25, selected ? TFT_BLUE : TFT_DARKGREY);
    _tft.setTextColor(selected ? TFT_BLACK : TFT_WHITE);
    _tft.setCursor(15, y + 5);
    _tft.print(text);
}

void SettingsPage::handleTouch(int x, int y) {
    // 设置项选择
    if (x >= 10 && x <= 160) {
        if (y >= 120 && y <= 145) _selectedItem = 0;
        else if (y >= 150 && y <= 175) _selectedItem = 1;
        else if (y >= 180 && y <= 205) _selectedItem = 2;
        else if (y >= 250 && y <= 275) _selectedItem = 3;
        else if (y >= 250 && y <= 275) _selectedItem = 4;
        else if (y >= 280 && y <= 305) _selectedItem = 5;
    }
    
    draw(); // 重绘更新选择状态
}

void SettingsPage::handleButtonPress() {
    switch (_selectedItem) {
        case 0: 
            weightSensor.tareFood(); 
            break;
        case 1: 
            weightSensor.tareWater(); 
            break;
        case 2: 
            weightSensor.tareLitter(); 
            break;
        case 3: 
            // 触摸校准
            Serial.println("Touch calibration started");
            break;
        case 4: 
            // 重置统计
            Serial.println("Reset statistics");
            break;
        case 5: 
            // 重启系统
            Serial.println("Rebooting system...");
            ESP.restart();
            break;
    }
}