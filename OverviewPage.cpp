// src/Display/Pages/OverviewPage.cpp
#include "OverviewPage.h"
#include "Config.h"
#include "PinDefinitions.h"
#include "Helpers.h"

// 外部变量声明
extern SensorReadings sensorReadings;
extern SystemState systemState;

OverviewPage::OverviewPage(TFT_eSPI& tft) 
    : _tft(tft), _lastUpdateTime(0) {
}

void OverviewPage::draw() {
    _tft.fillScreen(TFT_BLACK);
    drawLeftPanel();
    drawCenterCharts();
    drawRightControls();
}

void OverviewPage::update() {
    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < 1000) {
        return;
    }
    
    _lastUpdateTime = currentTime;
    draw();
}

void OverviewPage::drawLeftPanel() {
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextSize(1);
    
    // 时间显示
    _tft.setCursor(10, 10);
    _tft.print("Time: ");
    _tft.print(millis() / 1000); // 简化时间显示
    
    // 温度
    _tft.setCursor(10, 30);
    _tft.printf("Temp: %.1fC", sensorReadings.environment.temperature);
    
    // 湿度
    _tft.setCursor(10, 50);
    _tft.printf("Humi: %.1f%%", sensorReadings.environment.humidity);
    
    // 空气质量
    _tft.setCursor(10, 70);
    _tft.printf("AQI: %d", sensorReadings.environment.airQuality);
    
    // 重量
    _tft.setCursor(10, 90);
    _tft.printf("Weight: %.1fg", sensorReadings.environment.weight);
}

void OverviewPage::drawCenterCharts() {
    // 食物剩余量
    drawVerticalBar(120, 30, 20, 60, TFT_GREEN, "Food");
    int foodHeight = map(constrain(sensorReadings.food.weight, 0, 1000), 0, 1000, 0, 60);
    _tft.fillRect(120, 30 + (60 - foodHeight), 20, foodHeight, TFT_GREEN);
    
    // 水量
    drawVerticalBar(150, 30, 20, 60, TFT_BLUE, "Water");
    int waterHeight = map(constrain(sensorReadings.water.weight, 0, 2000), 0, 2000, 0, 60);
    _tft.fillRect(150, 30 + (60 - waterHeight), 20, waterHeight, TFT_BLUE);
    
    // 猫砂
    drawVerticalBar(180, 30, 20, 60, TFT_YELLOW, "Litter");
    int litterHeight = map(constrain(sensorReadings.litter.weight, 0, 5000), 0, 5000, 0, 60);
    _tft.fillRect(180, 30 + (60 - litterHeight), 20, litterHeight, TFT_YELLOW);
}

void OverviewPage::drawRightControls() {
    const int startX = 250;
    const int startY = 20;
    const int buttonWidth = 60;
    const int buttonHeight = 30;
    const int buttonSpacing = 40;
    
    // 灯光按钮
    _tft.drawRect(startX, startY, buttonWidth, buttonHeight, 
                 systemState.ambientLight ? TFT_GREEN : TFT_WHITE);
    _tft.setCursor(startX + 15, startY + 10);
    _tft.print("Light");
    
    // 风扇按钮
    _tft.drawRect(startX, startY + buttonSpacing, buttonWidth, buttonHeight,
                 systemState.fanRunning ? TFT_GREEN : TFT_WHITE);
    _tft.setCursor(startX + 15, startY + buttonSpacing + 10);
    _tft.print("Fan");
}

void OverviewPage::drawVerticalBar(int x, int y, int width, int height, uint16_t color, const char* label) {
    _tft.drawRect(x, y, width, height, TFT_WHITE);
    _tft.setTextColor(TFT_WHITE);
    _tft.setTextSize(1);
    _tft.setCursor(x, y + height + 5);
    _tft.print(label);
}

void OverviewPage::handleTouch(int x, int y) {
    const int startX = 250;
    const int startY = 20;
    const int buttonWidth = 60;
    const int buttonHeight = 30;
    const int buttonSpacing = 40;
    
    // 灯光按钮区域
    if (x >= startX && x <= startX + buttonWidth && 
        y >= startY && y <= startY + buttonHeight) {
        systemState.ambientLight = !systemState.ambientLight;
    }
    
    // 风扇按钮区域
    if (x >= startX && x <= startX + buttonWidth && 
        y >= startY + buttonSpacing && y <= startY + buttonSpacing + buttonHeight) {
        systemState.fanManualOn = !systemState.fanManualOn;
        systemState.fanAutoMode = false;
    }
}