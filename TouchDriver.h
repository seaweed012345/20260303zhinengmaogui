#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// FT6336寄存器定义
#define MY_FT6336_ADDR 0x38
#define FT6336_REG_WORKMODE 0x00
#define FT6336_REG_ACTIVEMODE 0x01
#define FT6336_REG_MONITORMODE 0x02
#define FT6336_REG_THRESHOLD 0x80
#define FT6336_REG_SCAN_TIME 0x8C
#define FT6336_REG_ACTIVE_PERIOD 0x8D
#define FT6336_REG_TOUCH_DATA 0x02


class TouchDriver {
public:
    TouchDriver();
    bool init();
    bool readTouch(uint16_t* x, uint16_t* y);
    bool isTouched();
    uint32_t getLastTouchTime();
    
private:
    void configureTouchChip();
    uint16_t calibrateX(uint16_t rawX);
    uint16_t calibrateY(uint16_t rawY);
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    
    uint32_t _lastTouchTime;
    bool _isTouched;
};

#endif // TOUCH_DRIVER_H