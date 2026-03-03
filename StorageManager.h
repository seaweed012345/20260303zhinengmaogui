#pragma once
#include "event_types.h"

class StorageManager {
public:
    void saveTareValue(uint8_t sensorId, float tareValue);
    float loadTareValue(uint8_t sensorId);
    void saveEvent(const EventRecord& evt);
    void loadEvents(EventRecord* events, int maxCount, int& outCount);
};