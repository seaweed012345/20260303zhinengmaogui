// src/Display/Pages/HealthPage.cpp
#include "HealthPage.h"
#include "Config.h"
#include "Helpers.h"

// 外部变量声明
extern SensorReadings sensorReadings;
extern SystemState systemState;

HealthPage::HealthPage(TFT_eSPI& tft) 
    : _tft(tft), _lastUpdateTime(0) {
}

void HealthPage::draw() {
    _tft.fillScreen(TFT_BLACK);
    
    // 标题
    _tft.setTextColor(TFT_WHITE);
    _tft.setTextSize(2);
    _tft.setCursor(10, 10);
    _tft.print("Health Monitor");
    
    drawCatHealth();
    drawSystemHealth();
    drawFanHealth();
    drawUsageStats();
}

void HealthPage::update() {
    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < 2000) { // 每2秒更新一次
        return;
    }
    
    _lastUpdateTime = currentTime;
    draw();
}

void HealthPage::drawCatHealth() {
    _tft.setTextColor(TFT_YELLOW);
    _tft.setTextSize(1);
    _tft.setCursor(10, 40);
    _tft.print("Cat Health:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(10, 60);
    _tft.printf("Weight: %.1fg", sensorReadings.cat.weight);
    
    _tft.setCursor(10, 80);
    _tft.printf("Excretion: %d times", sensorReadings.cat.excretionCount);
}

void HealthPage::drawSystemHealth() {
    _tft.setTextColor(TFT_CYAN);
    _tft.setCursor(10, 110);
    _tft.print("System Health:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(10, 130);
    _tft.printf("Uptime: %s", Helpers::getCurrentTime().c_str()); // 修复：使用 getCurrentTime()
    
    _tft.setCursor(10, 150);
    _tft.printf("Food: %.1f days", sensorReadings.food.remainingDays);
    
    _tft.setCursor(10, 170);
    _tft.printf("Water: %.1f days", sensorReadings.water.remainingDays);
}

void HealthPage::drawFanHealth() {
    _tft.setTextColor(TFT_GREEN);
    _tft.setCursor(10, 200);
    _tft.print("Fan Status:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(10, 220);
    _tft.printf("Health: %.1f%%", sensorReadings.system.fanHealth);
    
    _tft.setCursor(10, 240);
    _tft.printf("Runtime: %s", Helpers::getCurrentTime().c_str()); // 修复：使用 getCurrentTime()
    
    _tft.setCursor(10, 260);
    _tft.print("Mode: ");
    _tft.print(systemState.fanAutoMode ? "Auto" : "Manual");
}

void HealthPage::drawUsageStats() {
    _tft.setTextColor(TFT_MAGENTA);
    _tft.setCursor(200, 40);
    _tft.print("Daily Usage:");
    
    _tft.setTextColor(TFT_WHITE);
    _tft.setCursor(200, 60);
    _tft.printf("Food: %.1fg", sensorReadings.food.current);
    
    _tft.setCursor(200, 80);
    _tft.printf("Water: %.1fg", sensorReadings.water.current);
    
    _tft.setCursor(200, 100);
    _tft.printf("Litter: %.1fg", sensorReadings.litter.current);
}

void HealthPage::handleTouch(int x, int y) {
    // 健康页面的触摸处理
    if (y > 200 && y < 280) {
        // 风扇区域点击
        systemState.fanAutoMode = !systemState.fanAutoMode;
    }
}