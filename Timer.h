#pragma once
#include <Arduino.h>

class Timer {
private:
    unsigned long _previousMillis;
    unsigned long _interval;
    
public:
    Timer(unsigned long interval = 0);
    
    void update();
    bool isTime();
    void setInterval(unsigned long interval);
    unsigned long getElapsedTime();
    void reset();
};