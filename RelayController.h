#pragma once

#include <vector>      // 必须加，解决 std::vector 错误
#include <cstdint>     // 或 #include <stdint.h>，解决 uint8_t 错误


class RelayController {
public:
    RelayController();
    bool init(const std::vector<uint8_t>& relayPins);

    void setRelay(uint8_t index, bool state);
    void setRelayByPin(uint8_t pin, bool state);
    bool getRelayState(uint8_t index);
    bool getRelayStateByPin(uint8_t pin);

    // 业务功能接口
    void setMainLight(bool state);
    bool getMainLight();

    void setAmbientLight(bool state);
    bool getAmbientLight();

    void setFreshPower(bool state);
    bool getFreshPower();

private:
    bool _initialized;
    std::vector<uint8_t> _relayPins;
    std::vector<bool> _relayStates;
};