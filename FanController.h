// include/Actuators/FanController.h
#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include <Arduino.h>
#include "Config.h"

class FanController {
public:
    FanController(uint8_t fan1Pin, uint8_t fan2Pin, uint8_t buzzerPin);
    bool init();
    void setSpeed(uint8_t fan1Speed, uint8_t fan2Speed);
    void setBuzzer(bool state);
    void update();
    
private:
    uint8_t _fan1Pin;
    uint8_t _fan2Pin;
    uint8_t _buzzerPin;
    uint8_t _fan1Speed;
    uint8_t _fan2Speed;
    bool _buzzerState;
    bool _initialized;
};

#endif // FAN_CONTROLLER_H