#pragma once
#include <TFT_eSPI.h>

class BasePage {
protected:
    TFT_eSPI& _tft;
    
public:
    BasePage(TFT_eSPI& tft) : _tft(tft) {}
    virtual ~BasePage() {}
    
    virtual void draw() = 0;
    virtual void handleTouch(int x, int y) = 0;
    virtual void update() {}
    
    void drawButton(int x, int y, int width, int height, 
                   const String& text, uint16_t color, bool pressed = false) {
        uint16_t bgColor = pressed ? TFT_DARKGRAY : color;
        _tft.fillRoundRect(x, y, width, height, 5, bgColor);
        _tft.drawRoundRect(x, y, width, height, 5, TFT_WHITE);
        
        _tft.setTextColor(TFT_WHITE);
        _tft.setTextSize(1);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString(text, x + width / 2, y + height / 2);
        _tft.setTextDatum(TL_DATUM);
    }
};