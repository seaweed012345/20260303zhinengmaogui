#pragma once
#include <Arduino.h>

enum EventType {
    EVENT_NONE = 0,
    EVENT_FEED,          // 进食
    EVENT_DRINK,         // 饮水
    EVENT_CAT_ENTER,     // 猫进入猫砂盆
    EVENT_CAT_EXIT,      // 猫离开猫砂盆
    EVENT_DEFECATE,      // 排泄
    EVENT_ENTER_NO_DEFECATE, // 进盆未排便
    EVENT_LITTER_CHANGE, // 换猫砂
    EVENT_CLEANING_END,  // 铲便/清理结束
};

struct EventRecord {
    EventType type;
    uint8_t sensorId;        // 0:食盆, 1:水盆, 2:猫砂盆
    uint32_t timestampStart;
    uint32_t timestampEnd;
    float weightBefore;
    float weightAfter;
    float deltaWeight;
    bool valid;
};