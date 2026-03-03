// DisplayConfig.h
#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#include <Arduino.h>

// ==================== 屏幕参数 ====================
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 480
#define DISPLAY_ROTATION 1

//==================== 触摸校准参数 ====================
#define FT6336_ADDR 0x38

// 原始触摸坐标范围
#define TOUCH_RAW_X_MIN 0
#define TOUCH_RAW_X_MAX 319
#define TOUCH_RAW_Y_MIN 2
#define TOUCH_RAW_Y_MAX 478


// 原始触摸数据范围（FT6336典型值）
#define TOUCH_RAW_X_MIN     150
#define TOUCH_RAW_X_MAX     3850
#define TOUCH_RAW_Y_MIN     150  
#define TOUCH_RAW_Y_MAX     3850

// 触摸寄存器定义
#define FT6336_REG_WORKMODE         0x00
#define FT6336_REG_ACTIVEMODE       0x01
#define FT6336_REG_MONITORMODE      0x02
#define FT6336_REG_THRESHOLD        0x80
#define FT6336_REG_SCAN_TIME        0x8C
#define FT6336_REG_ACTIVE_PERIOD    0x8D
#define FT6336_REG_TOUCH_DATA       0x02

// 校准转换函数
inline uint16_t calibrateTouchX(uint16_t rawX, uint16_t rawY) {
    return map(rawY, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, DISPLAY_WIDTH - 1);
}

inline uint16_t calibrateTouchY(uint16_t rawX, uint16_t rawY) {
    return map(TOUCH_RAW_X_MAX - rawX, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, DISPLAY_HEIGHT - 1);
}

#endif // DISPLAY_CONFIG_H