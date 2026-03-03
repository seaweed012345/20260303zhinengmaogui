// src/Display/Display.cpp
#include "Display.h"
#include "PinDefinitions.h"  // 确保包含这个
#include <SPI.h>

// 全局TFT对象定义
extern TFT_eSPI tft;

// ST7796基本命令定义
#define ST7796_SWRESET 0x01
#define ST7796_SLPOUT  0x11
#define ST7796_DISPON  0x29

void manualST7796Init() {
    Serial.println("手动初始化ST7796...");
    
    // 初始化引脚 - 使用PinDefinitions.h中的定义
    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_BL, OUTPUT);
    
    // 设置初始状态
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_RST, HIGH);
    digitalWrite(TFT_BL, LOW);
    
    // 硬件复位
    digitalWrite(TFT_RST, LOW);
    delay(50);
    digitalWrite(TFT_RST, HIGH);
    delay(120);
    
    // 初始化SPI
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
    SPI.setFrequency(10000000);
    
    digitalWrite(TFT_CS, LOW);
    
    // 发送初始化命令序列
    digitalWrite(TFT_DC, LOW);
    SPI.transfer(ST7796_SWRESET);
    digitalWrite(TFT_DC, HIGH);
    delay(120);
    
    digitalWrite(TFT_DC, LOW);
    SPI.transfer(ST7796_SLPOUT);
    digitalWrite(TFT_DC, HIGH);
    delay(120);
    
    digitalWrite(TFT_DC, LOW);
    SPI.transfer(ST7796_DISPON);
    digitalWrite(TFT_DC, HIGH);
    delay(100);
    
    digitalWrite(TFT_CS, HIGH);
    
    // 开启背光
    digitalWrite(TFT_BL, HIGH);
    
    Serial.println("ST7796手动初始化完成");
}

bool initDisplay() {
    Serial.println("初始化TFT显示硬件...");
    
    try {
        // 先尝试手动初始化
        manualST7796Init();
        delay(100);
        
        // 然后使用TFT_eSPI的高级功能
        tft.init();
        tft.setRotation(1);
        tft.fillScreen(TFT_BLACK);
        
        // 提高SPI速度
        SPI.setFrequency(20000000);
        
        Serial.println("显示硬件初始化成功");
        return true;
    } catch (...) {
        Serial.println("显示初始化失败");
        return false;
    }
}

bool initTouch() {
    Serial.println("初始化触摸硬件...");
    
    try {
        // 初始化I2C用于触摸屏
        Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
        Wire.setClock(100000);
        
        pinMode(TOUCH_RST_PIN, OUTPUT);
        
        // 复位触摸芯片
        digitalWrite(TOUCH_RST_PIN, LOW);
        delay(10);
        digitalWrite(TOUCH_RST_PIN, HIGH);
        delay(50);
        
        // 检查触摸芯片是否存在
        Wire.beginTransmission(FT6336_ADDR);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.println("FT6336触摸控制器找到");
            Wire.setClock(400000);
        } else {
            Serial.printf("FT6336触摸控制器未找到, 错误: %d\n", error);
        }
        
        // 初始化中断引脚
        pinMode(TOUCH_INT_PIN, INPUT_PULLUP);
        
        Serial.println("触摸硬件初始化完成");
        return true;
    } catch (...) {
        Serial.println("触摸硬件初始化失败");
        return false;
    }
}

void testDisplay() {
    Serial.println("执行显示测试...");
    
    tft.fillScreen(TFT_RED);
    delay(300);
    tft.fillScreen(TFT_GREEN);
    delay(300);
    tft.fillScreen(TFT_BLUE);
    delay(300);
    tft.fillScreen(TFT_BLACK);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Display Test OK!");
    
    Serial.println("显示测试完成");
}

// 其他简化版的函数实现
void setDisplayBrightness(uint8_t brightness) {
    analogWrite(TFT_BL, brightness);
}

void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(text);
}

void drawCenteredText(const char* text, int16_t y, uint16_t color, uint8_t size) {
    tft.setTextColor(color);
    tft.setTextSize(size);
    int16_t textWidth = tft.textWidth(text);
    int16_t x = (tft.width() - textWidth) / 2;
    tft.setCursor(x, y);
    tft.print(text);
}

uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void clearScreen(uint16_t color) {
    tft.fillScreen(color);
}