// src/Utilities/Helpers.cpp
#include "Helpers.h"

String Helpers::getCurrentTime() {
    return formatTime(millis() / 1000);
}

String Helpers::formatTime(unsigned long seconds) {
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(buffer);
}

float Helpers::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Helpers::safeDelay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        delay(1);
    }
}

bool Helpers::isInRange(float value, float min, float max) {
    return value >= min && value <= max;
}