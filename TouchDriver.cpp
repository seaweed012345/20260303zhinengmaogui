#include <Wire.h>
#include "TouchDriver.h"
#include "PinDefinitions.h" // 推荐所有引脚集中定义

#define FT6336_ADDR     0x38
#define FT6336_REG_TOUCH_DATA 0x02


#define TOUCH_X_MIN 2
#define TOUCH_X_MAX 478
#define TOUCH_Y_MIN 0
#define TOUCH_Y_MAX 319


#define DISPLAY_WIDTH  480  // 竖屏
#define DISPLAY_HEIGHT 320  // 竖屏

// 确保 userInteraction 可被调用（在 main.ino 中实现）
extern "C" void userInteraction();

TouchDriver::TouchDriver() : _lastTouchTime(0), _isTouched(false) {}

bool TouchDriver::init() {
    pinMode(TOUCH_RST_PIN, OUTPUT);
    digitalWrite(TOUCH_RST_PIN, LOW);
    delay(10);
    digitalWrite(TOUCH_RST_PIN, HIGH);
    delay(50);

    Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
    Wire.setClock(400000);

    pinMode(TOUCH_INT_PIN, INPUT_PULLUP);

    Wire.beginTransmission(FT6336_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("Touch IC not found on I2C!");
        return false;
    }

    Serial.println("Touch IC found!");
    return true;
}

bool TouchDriver::readTouch(uint16_t* x, uint16_t* y) {
    uint8_t touchData[6];

    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(FT6336_REG_TOUCH_DATA);
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom(FT6336_ADDR, 6) != 6) return false;
    for (int i = 0; i < 6; i++) touchData[i] = Wire.read();

    uint8_t numTouches = touchData[0] & 0x0F;
    if (numTouches > 0) {
        uint16_t rawX = ((touchData[1] & 0x0F) << 8) | touchData[2];
        uint16_t rawY = ((touchData[3] & 0x0F) << 8) | touchData[4];

        // X轴直接用rawY，Y轴将rawX反转并map到0~479
        *x = rawY;
        *y = map(rawX, 319, 0, 0, DISPLAY_HEIGHT - 1);

        _isTouched = true;
        _lastTouchTime = millis();
        Serial.printf("Touch: RAW_X=%d, RAW_Y=%d | X=%d, Y=%d\n", rawX, rawY, *x, *y);

        // 立即通知上层有用户交互（确保唤醒/重置计时）
        userInteraction();

        return true;
    }
    _isTouched = false;
    return false;
}

// 这部分是原来结尾的常规接口，不能少
void TouchDriver::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t TouchDriver::readRegister(uint8_t reg) {
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom((uint8_t)FT6336_ADDR, (uint8_t)1);
    if (Wire.available()) return Wire.read();
    return 0;
}

bool TouchDriver::isTouched() {
    return _isTouched;
}

uint32_t TouchDriver::getLastTouchTime() {
    return _lastTouchTime;
}