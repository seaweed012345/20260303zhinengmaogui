#include "MQTTManager.h"
#include "Config.h"
#include "PassportService.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// ========== 新增：主线程MQTT安全上报标志 ==========
extern volatile bool needPublishStatus;

// ========== 新增：主线程安全UI刷新标志 ==========
extern volatile bool needUpdateLevelLabels;
extern int levelLabelsEat, levelLabelsDrink, levelLabelsToilet, levelLabelsWeight;
extern volatile bool needUpdateCurve;
extern int curveEat[10], curveDrink[10], curveToilet[10], curveWeight[10];

extern PassportService passportService;
extern SystemState systemState;
extern SensorReadings sensorReadings;

MQTTManager::MQTTManager(PubSubClient& client) : _client(client) {}

void MQTTManager::init() {
    _deviceId = passportService.getDeviceId();
    _client.setServer("mqtt.byyds.cn", 1883); // 修改为您的MQTT服务器
    _client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });

    Serial.println("MQTT initialized");
}

bool MQTTManager::reconnect() {
    if (_client.connected()) {
        return true;
    }

    Serial.print("Attempting MQTT connection...");

    if (_client.connect(_deviceId.c_str(), passportService.getAuthToken().c_str(), "")) {
        Serial.println("connected");
        String subscribeTopic = "cat-cabinet/" + _deviceId + "/command";
        _client.subscribe(subscribeTopic.c_str());
        return true;
    } else {
        Serial.print("failed, rc=");
        Serial.print(_client.state());
        Serial.println(" try again in 5 seconds");
        return false;
    }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    JsonObject command = doc.as<JsonObject>();
    handleCommand(command);
}

bool MQTTManager::isConnected() {
    return _client.connected();
}

void MQTTManager::publishData() {
    if (!reconnect()) return;

    StaticJsonDocument<512> doc;
    doc["mac"] = _deviceId;
    doc["timestamp"] = (long)time(nullptr);

    doc["food_weight"] = sensorReadings.food.weight;
    doc["water_weight"] = sensorReadings.water.weight;
    doc["litter_weight"] = sensorReadings.litter.weight;
    doc["temperature"] = sensorReadings.environment.temperature;
    doc["humidity"] = sensorReadings.environment.humidity;
    doc["air_quality"] = sensorReadings.environment.airQuality;
    doc["pressure"] = sensorReadings.environment.pressure;

    doc["uptime"] = sensorReadings.system.uptime;
    doc["fan_runtime"] = sensorReadings.system.fanRuntime;
    doc["fan_health"] = sensorReadings.system.fanHealth;

    signJsonDoc(doc);

    String output;
    serializeJson(doc, output);

    String topic = "cat-cabinet/" + _deviceId + "/data";
    _client.publish(topic.c_str(), output.c_str());
}

void MQTTManager::publishStatus() {
    if (!reconnect()) return;

    StaticJsonDocument<256> doc;
    doc["mac"] = _deviceId;
    doc["timestamp"] = (long)time(nullptr);

    doc["fan_auto_mode"] = systemState.fanAutoMode;
    doc["fan_manual_on"] = systemState.fanManualOn;
    doc["fan_running"] = systemState.fanRunning;
    doc["ambient_light"] = systemState.ambientLight;
    doc["main_light"] = systemState.mainLight;
    doc["current_page"] = systemState.currentPage;

    signJsonDoc(doc);

    String output;
    serializeJson(doc, output);

    String topic = "cat-cabinet/" + _deviceId + "/status";
    _client.publish(topic.c_str(), output.c_str());
}

// 发布带唯一ID的事件
void MQTTManager::publishEventWithId(const String& eventType, const String& data, const String& eventId) {
    if (!reconnect()) {
        saveEventForRetry(eventType, data, eventId);
        return;
    }

    StaticJsonDocument<512> doc;
    doc["mac"] = _deviceId;
    doc["timestamp"] = (long)time(nullptr);
    doc["event_type"] = eventType;
    doc["event_id"] = eventId;

    // data为JSON字符串，合并进doc["data"]
    StaticJsonDocument<384> dataDoc;
    deserializeJson(dataDoc, data);
    doc["data"] = dataDoc.as<JsonObject>();

    signJsonDoc(doc);

    String output;
    serializeJson(doc, output);

    String topic = "cat-cabinet/" + _deviceId + "/events/" + eventType;
    if (_client.publish(topic.c_str(), output.c_str())) {
        // 成功
    } else {
        saveEventForRetry(eventType, data, eventId);
    }
}

// 事件唯一ID（简单实现：mac+时间戳+随机数）
String MQTTManager::generateEventId() {
    char buf[48];
    snprintf(buf, sizeof(buf), "%s-%lu-%u", _deviceId.c_str(), (unsigned long)time(nullptr), (unsigned int)esp_random());
    return String(buf);
}

// 原publishEvent兼容旧接口
void MQTTManager::publishEvent(const String& eventType, const String& data) {
    String eventId = generateEventId();
    publishEventWithId(eventType, data, eventId);
}

// 事件本地缓存
bool MQTTManager::saveEventForRetry(const String& eventType, const String& data, const String& eventId) {
    int nextTail = (_eventCacheTail + 1) % EVENT_CACHE_SIZE;
    if (nextTail == _eventCacheHead) {
        _eventCacheHead = (_eventCacheHead + 1) % EVENT_CACHE_SIZE;
    }
    _eventCache[_eventCacheTail] = {eventType, data, eventId, millis(), false};
    _eventCacheTail = nextTail;
    Serial.println("Event cached for retry.");
    return true;
}

void MQTTManager::retryUnsentEvents() {
    if (!reconnect()) return;
    int pos = _eventCacheHead;
    while (pos != _eventCacheTail) {
        if (!_eventCache[pos].sent) {
            publishEventWithId(_eventCache[pos].eventType, _eventCache[pos].data, _eventCache[pos].eventId);
            _eventCache[pos].sent = true;
        }
        pos = (pos + 1) % EVENT_CACHE_SIZE;
    }
}

void MQTTManager::loop() {
    _client.loop();
    retryUnsentEvents();
}

void MQTTManager::publishAck(const String& cmdId, bool success, const String& message) {
    if (!reconnect()) return;
    StaticJsonDocument<192> doc;
    doc["mac"] = _deviceId;
    doc["timestamp"] = (long)time(nullptr);
    doc["cmd_id"] = cmdId;
    doc["result"] = success ? "ok" : "fail";
    doc["msg"] = message;
    signJsonDoc(doc);

    String output;
    serializeJson(doc, output);
    String topic = "cat-cabinet/" + _deviceId + "/ack";
    _client.publish(topic.c_str(), output.c_str());
}

void MQTTManager::sendAck(const String& cmdId, bool success, const String& msg) {
    publishAck(cmdId, success, msg);
}

// 命令回调处理
void MQTTManager::handleCommand(const JsonObject& command) {
    Serial.println("Received MQTT command");
    String cmdId = command["cmd_id"] | "";
    String action = command["action"] | "";

    bool success = false;

    if (action == "set_main_light") {
        int value = command["value"] | 0;
        // 控制主灯...
        success = true;
    } else if (action == "set_ambient_light") {
        int value = command["value"] | 0;
        // 控制氛围灯...
        success = true;
    } else if (action == "set_fresh_auto") {
        int value = command["value"] | 0;
        // 控制新风自动...
        success = true;
    } else if (action == "set_fresh_manual") {
        int value = command["value"] | 0;
        // 控制新风手动...
        success = true;
    } else if (action == "set_yesterday_level") {
        levelLabelsEat    = command["eat"]    | 2;
        levelLabelsDrink  = command["drink"]  | 2;
        levelLabelsToilet = command["toilet"] | 2;
        levelLabelsWeight = command["weight"] | 2;
        needUpdateLevelLabels = true;
        success = true;
    } else if (action == "set_curve_history") {
        const JsonArray eatCurve = command["eat_curve"].as<JsonArray>();
        const JsonArray drinkCurve = command["drink_curve"].as<JsonArray>();
        const JsonArray toiletCurve = command["toilet_curve"].as<JsonArray>();
        const JsonArray weightCurve = command["weight_curve"].as<JsonArray>();
        for (int i = 0; i < 10; ++i) {
            curveEat[i] = eatCurve[i] | 0;
            curveDrink[i] = drinkCurve[i] | 0;
            curveToilet[i] = toiletCurve[i] | 0;
            curveWeight[i] = weightCurve[i] | 0;
        }
        needUpdateCurve = true;
        success = true;
    } else {
        success = false;
    }

    sendAck(cmdId, success, success ? "success" : "unknown action");
    needPublishStatus = true;
}

void MQTTManager::publishCurveHistory(const JsonObject& history) {
    if (!reconnect()) return;
    StaticJsonDocument<1024> doc;
    doc["mac"] = _deviceId;
    doc["timestamp"] = (long)time(nullptr);
    doc["curve"] = history;
    signJsonDoc(doc);

    String output;
    serializeJson(doc, output);
    String topic = "cat-cabinet/" + _deviceId + "/curve";
    _client.publish(topic.c_str(), output.c_str());
}

void MQTTManager::publishDailyReport(const JsonObject& report) {
    if (!reconnect()) return;
    StaticJsonDocument<512> doc;
    doc["mac"] = _deviceId;
    doc["timestamp"] = (long)time(nullptr);
    doc["report"] = report;
    signJsonDoc(doc);

    String output;
    serializeJson(doc, output);
    String topic = "cat-cabinet/" + _deviceId + "/dailyreport";
    _client.publish(topic.c_str(), output.c_str());
}

void MQTTManager::publishDeviceInfo(const JsonObject& deviceInfo) {
    if (!reconnect()) return;
    StaticJsonDocument<384> doc;
    doc["mac"] = _deviceId;
    doc["timestamp"] = (long)time(nullptr);
    doc["device"] = deviceInfo;
    signJsonDoc(doc);

    String output;
    serializeJson(doc, output);
    String topic = "cat-cabinet/" + _deviceId + "/deviceinfo";
    _client.publish(topic.c_str(), output.c_str());
}

void MQTTManager::signJsonDoc(JsonDocument& doc) {
    doc["sign"] = "demo_sign";
}