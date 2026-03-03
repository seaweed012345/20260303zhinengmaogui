
#pragma once
#ifndef PINDEFINITIONS_H
#define PINDEFINITIONS_H
#include <Arduino.h>

// 显示引脚
constexpr uint8_t LCD_MOSI_PIN = 11;
constexpr uint8_t LCD_MISO_PIN = 13;
constexpr uint8_t LCD_SCLK_PIN = 12;
constexpr uint8_t LCD_CS_PIN = 10;
constexpr uint8_t LCD_DC_PIN = 2;
constexpr uint8_t LCD_RST_PIN = 15;
constexpr uint8_t LCD_BL_PIN = 21;

// 触摸屏引脚
constexpr uint8_t TOUCH_SDA_PIN = 5;
constexpr uint8_t TOUCH_SCL_PIN = 6;
constexpr uint8_t TOUCH_RST_PIN = 4;
constexpr uint8_t TOUCH_INT_PIN = 3;
constexpr uint8_t FT6336_ADDR = 0x38;


// 称重传感器引脚
constexpr uint8_t WEIGHT_FOOD_DOUT   = 47;   // 食物称重
constexpr uint8_t WEIGHT_FOOD_SCK    = 48;
constexpr uint8_t WEIGHT_WATER_DOUT  = 9;   // 饮水称重
constexpr uint8_t WEIGHT_WATER_SCK   = 20;
constexpr uint8_t WEIGHT_LITTER_DOUT = 40;  // 猫砂称重
constexpr uint8_t WEIGHT_LITTER_SCK  = 41;

// 环境传感器引脚
constexpr uint8_t DHT_PIN         = 14; // DHT22单线信号
// SGP30采用I2C总线，推荐共用一组I2C引脚

constexpr uint8_t PRESSURE_PIN    = 42; // 微压传感器单线，接ADC功能的IO

// 继电器控制引脚
constexpr uint8_t RELAY_AMBIENT_PIN =  8;
constexpr uint8_t RELAY_MAIN_PIN    = 18;
constexpr uint8_t RELAY_FAN_PIN     = 19;
constexpr uint8_t RELAY_HEATER_PIN  = 44;

// 风扇PWM控制
constexpr uint8_t FAN1_PWM_PIN = 45;
constexpr uint8_t FAN2_PWM_PIN = 46;

// 其他功能
constexpr uint8_t BUZZER_PIN = -1;
constexpr uint8_t BUTTON_PIN =0;


#endif // PINDEFINITIONS_H