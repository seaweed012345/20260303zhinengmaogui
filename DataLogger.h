// DataLogger.h
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

// 先定义历史数据结构
struct DailyData {
    float food;
    float water;
    float excretion;
    float weight;
    int count;
    time_t timestamp;
};

class DataLogger {
private:
    Preferences _prefs;
    std::vector<DailyData> _history;
    unsigned long _lastLogTime;
    int _currentIndex;
    
public:
    DataLogger();
    bool begin();
    void end();
    void logSensorData();
    void logEvent(const String& eventType, const String& data);
    void saveToStorage();
    void loadFromStorage();
    void loadHistoricalData();
    void saveHistoricalData();
    void addDailyData(float food, float water, float excretion, float weight, int count);
    void clearData();
};