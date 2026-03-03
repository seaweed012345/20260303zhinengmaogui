// include/Display/Graphics.h
#pragma once
#include <TFT_eSPI.h>
#include <Arduino.h>




namespace Graphics {
    void drawButton(TFT_eSPI& tft, int x, int y, int width, int height, 
                   const String& text, uint16_t color, uint16_t textColor = TFT_WHITE);
    
    void drawProgressBar(TFT_eSPI& tft, int x, int y, int width, int height, 
                        int value, uint16_t color);
    
    void drawChart(TFT_eSPI& tft, int x, int y, int width, int height, 
                  const float data[], int dataSize, uint16_t color, const String& title);
    
    void drawSplashScreen(TFT_eSPI& tft);
};