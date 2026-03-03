#include "StateManager.h"
#include "Config.h"
#include "PinDefinitions.h"
#include "MQTTManager.h"
#include "EventDetector.h"
#include "event_types.h"
#include "FanController.h"  // 如果有风扇相关功能

// 前向声明
extern EventDetector eventDet;
extern MQTTManager mqttManager;
extern SystemState systemState;
extern FanController fanController;
extern SensorReadings sensorReadings;

StateManager::StateManager() : _lastDailyReset(millis()) {}

void StateManager::update() {
    checkDailyReset();
    updateFanHealth();
    processLitterEvents();
    updateFanState();
}

void StateManager::checkDailyReset() {
    unsigned long currentTime = millis();
    if (currentTime - _lastDailyReset >= 86400000) { // 24小时
        resetDailyData();
        _lastDailyReset = currentTime;
    }
}

void StateManager::resetDailyData() {
    sensorReadings.cat.excretionCount = 0;
    sensorReadings.cat.excretionAmount = 0.0f;
    Serial.println("Daily data reset");
}

void StateManager::updateFanHealth() {
    if (sensorReadings.environment.pressureDiff > 100) return;

    float pressureFactor = 1.0f - (sensorReadings.environment.pressureDiff / 100.0f);
    float timeFactor = 1.0f - ((float)sensorReadings.system.fanRuntime / FAN_LIFETIME);

    sensorReadings.system.fanHealth = (pressureFactor * 0.7f + timeFactor * 0.3f) * 100;
    sensorReadings.system.fanHealth = constrain(sensorReadings.system.fanHealth, 0, 100);
}

void StateManager::processLitterEvents() {
    EventRecord evt;
    // 获取最近一次猫砂盆相关事件
    if (eventDet.getLastEvent(evt) && evt.sensorId == 2) { // 2 = 猫砂盆
        // 这里只处理新事件（你可以用更好的判断，仅在新事件发生时上报）
        StaticJsonDocument<200> doc;
        switch (evt.type) {
            case EVENT_CAT_ENTER:        doc["event_type"] = "cat_enter"; break;
            case EVENT_CAT_EXIT:         doc["event_type"] = "cat_exit"; break;
            case EVENT_DEFECATE:         doc["event_type"] = "defecate"; break;
            case EVENT_ENTER_NO_DEFECATE:doc["event_type"] = "enter_no_defecate"; break;
            case EVENT_LITTER_CHANGE:    doc["event_type"] = "litter_change"; break;
            default: return; // 不是猫砂相关事件则不处理
        }
        doc["cat_weight"] = sensorReadings.cat.weight;
        doc["excretion_count"] = sensorReadings.cat.excretionCount;
        doc["timestamp"] = millis();

        char buffer[200];
        serializeJson(doc, buffer);

        if (mqttManager.isConnected()) {
            mqttManager.publishEvent("litter_event", buffer);
        }
    }
}

void StateManager::handleCommand(const JsonObject& command) {
    if (command.containsKey("ambient_light")) {
        systemState.ambientLight = command["ambient_light"];
        digitalWrite(RELAY_AMBIENT_PIN, systemState.ambientLight ? HIGH : LOW);
        Serial.printf("Ambient light: %s\n", systemState.ambientLight ? "ON" : "OFF");
    }

    if (command.containsKey("main_light")) {
        systemState.mainLight = command["main_light"];
        digitalWrite(RELAY_MAIN_PIN, systemState.mainLight ? HIGH : LOW);
        Serial.printf("Main light: %s\n", systemState.mainLight ? "ON" : "OFF");
    }

    if (command.containsKey("fan_mode")) {
        String mode = command["fan_mode"];
        if (mode == "auto") {
            systemState.fanAutoMode = true;
            systemState.fanManualOn = false;
            Serial.println("Fan mode: AUTO");
        } else if (mode == "manual") {
            systemState.fanAutoMode = false;
            if (command.containsKey("fan_state")) {
                systemState.fanManualOn = command["fan_state"];
                Serial.printf("Fan manual: %s\n", systemState.fanManualOn ? "ON" : "OFF");
            }
        }
        updateFanState();
    }
}

void StateManager::updateFanState() {
    bool shouldTurnOn = false;

    if (systemState.fanAutoMode) {
        shouldTurnOn = (sensorReadings.environment.airQuality > AIR_QUALITY_THRESHOLD);
    } else {
        shouldTurnOn = systemState.fanManualOn;
    }

    digitalWrite(RELAY_FAN_PIN, shouldTurnOn ? HIGH : LOW);

    // 更新运行时间
    static unsigned long lastFanUpdate = 0;
    if (shouldTurnOn && millis() - lastFanUpdate >= 1000) {
        sensorReadings.system.fanRuntime++;
        lastFanUpdate = millis();
    }
}