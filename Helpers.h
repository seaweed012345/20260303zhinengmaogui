// include/Utilities/Helpers.h
#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>

class Helpers {
public:
    static String getCurrentTime();
    static String formatTime(unsigned long seconds);
    static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    static void safeDelay(unsigned long ms);
    static bool isInRange(float value, float min, float max);
};

#endif // HELPERS_H