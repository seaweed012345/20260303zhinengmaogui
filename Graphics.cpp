// src/Display/Graphics.cpp
#include "Graphics.h"

void Graphics::drawButton(TFT_eSPI& tft, int x, int y, int width, int height, 
                         const String& text, uint16_t color, uint16_t textColor) {
    tft.fillRoundRect(x, y, width, height, 5, color);
    tft.drawRoundRect(x, y, width, height, 5, TFT_WHITE);
    
    tft.setTextColor(textColor);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, x + width / 2, y + height / 2);
    tft.setTextDatum(TL_DATUM);
}

void Graphics::drawProgressBar(TFT_eSPI& tft, int x, int y, int width, int height, 
                              int value, uint16_t color) {
    value = constrain(value, 0, 100);
    int fillWidth = (width * value) / 100;
    
    tft.drawRect(x, y, width, height, TFT_WHITE);
    tft.fillRect(x, y, fillWidth, height, color);
    
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString(String(value) + "%", x + width + 5, y);
}

void Graphics::drawChart(TFT_eSPI& tft, int x, int y, int width, int height, 
                        const float data[], int dataSize, uint16_t color, const String& title) {
    if (dataSize < 2) return;
    
    // 找出最大值用于缩放
    float maxVal = 0;
    for (int i = 0; i < dataSize; i++) {
        if (data[i] > maxVal) maxVal = data[i];
    }
    if (maxVal == 0) maxVal = 1;
    
    // 绘制坐标轴
    tft.drawLine(x, y, x, y + height, TFT_WHITE);
    tft.drawLine(x, y + height, x + width, y + height, TFT_WHITE);
    
    // 绘制数据线
    int pointSpacing = width / (dataSize - 1);
    for (int i = 0; i < dataSize - 1; i++) {
        int x1 = x + i * pointSpacing;
        int y1 = y + height - (data[i] / maxVal * height);
        int x2 = x + (i + 1) * pointSpacing;
        int y2 = y + height - (data[i + 1] / maxVal * height);
        tft.drawLine(x1, y1, x2, y2, color);
    }
    
    // 绘制标题
    tft.setTextColor(TFT_WHITE);
    tft.drawString(title, x, y - 15);
}

void Graphics::drawSplashScreen(TFT_eSPI& tft) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Smart Cat Cabinet", tft.width() / 2, tft.height() / 2 - 20);
    tft.setTextSize(1);
    tft.drawString("Initializing...", tft.width() / 2, tft.height() / 2 + 20);
    tft.setTextDatum(TL_DATUM);
}