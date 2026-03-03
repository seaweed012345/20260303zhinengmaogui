// src/Actuators/FanController.cpp
#include "FanController.h"
#include "PinDefinitions.h"

FanController::FanController(uint8_t fan1Pin, uint8_t fan2Pin, uint8_t buzzerPin)
    : _fan1Pin(fan1Pin), _fan2Pin(fan2Pin), _buzzerPin(buzzerPin),
      _fan1Speed(0), _fan2Speed(0), _buzzerState(false), _initialized(false) {}

bool FanController::init() {
    pinMode(_fan1Pin, OUTPUT);
    pinMode(_fan2Pin, OUTPUT);
    pinMode(_buzzerPin, OUTPUT);
    
    // 初始关闭所有输出
    analogWrite(_fan1Pin, 0);
    analogWrite(_fan2Pin, 0);
    digitalWrite(_buzzerPin, LOW);
    
    _initialized = true;
    Serial.printf("Fan controller initialized on pins FAN1:%d, FAN2:%d, BUZZER:%d\n", 
                 _fan1Pin, _fan2Pin, _buzzerPin);
    return true;
}

void FanController::setSpeed(uint8_t fan1Speed, uint8_t fan2Speed) {
    if (!_initialized) return;
    
    _fan1Speed = constrain(fan1Speed, 0, 255);
    _fan2Speed = constrain(fan2Speed, 0, 255);
    
    analogWrite(_fan1Pin, _fan1Speed);
    analogWrite(_fan2Pin, _fan2Speed);
    
    Serial.printf("Fan speeds set to: FAN1=%d, FAN2=%d\n", _fan1Speed, _fan2Speed);
}

void FanController::setBuzzer(bool state) {
    if (!_initialized) return;
    
    _buzzerState = state;
    digitalWrite(_buzzerPin, state ? HIGH : LOW);
    Serial.printf("Buzzer: %s\n", state ? "ON" : "OFF");
}

void FanController::update() {
    // 可以在这里添加风扇控制逻辑
}