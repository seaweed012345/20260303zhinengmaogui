#include "StorageManager.h"
#include <Preferences.h>

Preferences prefs;

void StorageManager::saveTareValue(uint8_t sensorId, float tareValue) {
    prefs.begin("tare", false);
    String key = String("tare") + sensorId;
    prefs.putFloat(key.c_str(), tareValue);
    prefs.end();
}
float StorageManager::loadTareValue(uint8_t sensorId) {
    prefs.begin("tare", true);
    String key = String("tare") + sensorId;
    float val = prefs.getFloat(key.c_str(), 0.0f);
    prefs.end();
    return val;
}
void StorageManager::saveEvent(const EventRecord& evt) {
    // 这里只做示例，实际建议用文件保存历史事件
}
void StorageManager::loadEvents(EventRecord* events, int maxCount, int& outCount) {
    // 这里只做示例，实际建议用文件保存历史事件
    outCount = 0;
}