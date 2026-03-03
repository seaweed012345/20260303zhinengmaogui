#include "DataLogger.h"

DataLogger::DataLogger() : _currentIndex(0) {}

bool DataLogger::begin() {
    return _prefs.begin("catcabinet", false);
}

void DataLogger::end() {
    _prefs.end();
}

void DataLogger::loadHistoricalData() {
    size_t len = _prefs.getBytesLength("history");
    if (len > 0 && len <= sizeof(_history)) {
        _prefs.getBytes("history", &_history, len);
    }
}

void DataLogger::saveHistoricalData() {
    _prefs.putBytes("history", &_history, sizeof(_history));
}

void DataLogger::addDailyData(float food, float water, float excretion, float weight, int count) {
    if (_history.size() < 10) {
        _history.resize(10);
    }
    
    DailyData data;
    data.food = food;
    data.water = water;
    data.excretion = excretion;
    data.weight = weight;
    data.count = count;
    data.timestamp = time(nullptr);
    
    _history[_currentIndex] = data;
    _currentIndex = (_currentIndex + 1) % 10;
}

void DataLogger::clearData() {
    _history.clear();
    _currentIndex = 0;
    _prefs.remove("history");
}

// 其他函数保持原样或根据需要实现
void DataLogger::logSensorData() {}
void DataLogger::logEvent(const String& eventType, const String& data) {}
void DataLogger::saveToStorage() {}
void DataLogger::loadFromStorage() {}