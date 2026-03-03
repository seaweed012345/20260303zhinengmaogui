// src/Actuators/RelayController.cpp
#include "RelayController.h"
#include "PinDefinitions.h"

// 构造函数
RelayController::RelayController()
    : _initialized(false) {}

// 继电器初始化
bool RelayController::init(const std::vector<uint8_t>& relayPins) {
    _relayPins = relayPins;
    _relayStates.resize(relayPins.size(), false);
    for (size_t i = 0; i < relayPins.size(); i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], LOW); // 默认低电平关闭
    }
    _initialized = true;
    Serial.printf("Relay controller initialized with %d relays\n", relayPins.size());
    return true;
}

// 按索引控制
void RelayController::setRelay(uint8_t index, bool state) {
    if (!_initialized || index >= _relayPins.size()) return;
    if (_relayStates[index] != state) { // 只在状态变化时打印
        _relayStates[index] = state;
        digitalWrite(_relayPins[index], state ? HIGH : LOW);
        Serial.printf("Relay %d (GPIO%d): %s\n", index, _relayPins[index], state ? "ON" : "OFF");
    } else {
        _relayStates[index] = state;
        digitalWrite(_relayPins[index], state ? HIGH : LOW);
        // 状态未变，不重复打印
    }
}

// 按引脚号控制
void RelayController::setRelayByPin(uint8_t pin, bool state) {
    for (size_t i = 0; i < _relayPins.size(); ++i) {
        if (_relayPins[i] == pin) {
            setRelay(i, state);
            return;
        }
    }
    Serial.printf("Relay pin %d not found!\n", pin);
}

// 查询状态
bool RelayController::getRelayState(uint8_t index) {
    if (!_initialized || index >= _relayPins.size()) return false;
    return _relayStates[index];
}
bool RelayController::getRelayStateByPin(uint8_t pin) {
    for (size_t i = 0; i < _relayPins.size(); ++i) {
        if (_relayPins[i] == pin) return _relayStates[i];
    }
    return false;
}

// 业务便捷接口：主灯
void RelayController::setMainLight(bool state) {
    setRelayByPin(RELAY_MAIN_PIN, state);
}
bool RelayController::getMainLight() {
    return getRelayStateByPin(RELAY_MAIN_PIN);
}

// 业务便捷接口：氛围灯
void RelayController::setAmbientLight(bool state) {
    setRelayByPin(RELAY_AMBIENT_PIN, state);
}
bool RelayController::getAmbientLight() {
    return getRelayStateByPin(RELAY_AMBIENT_PIN);
}

// 业务便捷接口：新风主电源（风扇/负离子/UVC）
void RelayController::setFreshPower(bool state) {
    setRelayByPin(RELAY_FAN_PIN, state);
}
bool RelayController::getFreshPower() {
    return getRelayStateByPin(RELAY_FAN_PIN);
}