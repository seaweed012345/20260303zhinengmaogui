#include "Timer.h"

Timer::Timer(unsigned long interval) 
    : _interval(interval), _previousMillis(millis()) {
}

void Timer::update() {
    _previousMillis = millis();
}

bool Timer::isTime() {
    unsigned long currentMillis = millis();
    if (currentMillis - _previousMillis >= _interval) {
        _previousMillis = currentMillis;
        return true;
    }
    return false;
}

void Timer::setInterval(unsigned long interval) {
    _interval = interval;
}

unsigned long Timer::getElapsedTime() {
    return millis() - _previousMillis;
}

void Timer::reset() {
    _previousMillis = millis();
}