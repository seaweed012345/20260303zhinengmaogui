// include/Display/Display.h
#pragma once
#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <Wire.h>

// 显示旋转设置
#define DISPLAY_ROTATION 1

// 全局TFT对象声明
extern TFT_eSPI tft;

// ===== 显示初始化函数 =====
bool initDisplay();
bool initTouch();
void testDisplay();

// ===== 显示控制函数 =====
void setDisplayBrightness(uint8_t brightness);
void sleepDisplay();
void wakeupDisplay();

// ===== 图形绘制函数 =====
void drawText(const char* text, int16_t x, int16_t y, uint16_t color = TFT_WHITE, uint8_t size = 2);
void drawCenteredText(const char* text, int16_t y, uint16_t color = TFT_WHITE, uint8_t size = 2);

// ===== 触摸处理函数 =====
struct TouchPoint {
    int16_t x;
    int16_t y;
    bool touched;
};

TouchPoint readTouch();
bool isTouched();

// ===== 工具函数 =====
uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b);
void clearScreen(uint16_t color = TFT_BLACK);

#endif // DISPLAY_H