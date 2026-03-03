#pragma once
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// 事件上报缓存队列容量
#define EVENT_CACHE_SIZE 16

struct CachedEvent {
    String eventType;
    String data;
    String eventId;
    unsigned long timestamp;
    bool sent;
};

class MQTTManager {
private:
    PubSubClient& _client;
    String _deviceId;
    CachedEvent _eventCache[EVENT_CACHE_SIZE];
    int _eventCacheHead = 0;
    int _eventCacheTail = 0;

    bool reconnect();
    void callback(char* topic, byte* payload, unsigned int length);

    void handleCommand(const JsonObject& command);
    void sendAck(const String& cmdId, bool success, const String& msg);
    void signJsonDoc(JsonDocument& doc);

public:
    MQTTManager(PubSubClient& client);
    void init();
    bool isConnected();
    void publishData();
    void publishStatus();
    // 事件相关
    void publishEvent(const String& eventType, const String& data);
    void publishEventWithId(const String& eventType, const String& data, const String& eventId);
    String generateEventId();
    // ACK/回执
    void publishAck(const String& cmdId, bool success, const String& message = "");
    // 曲线/历史/设备信息等
    void publishDailyReport(const JsonObject& report);
    void publishCurveHistory(const JsonObject& history);
    void publishDeviceInfo(const JsonObject& deviceInfo);

    void loop();
    // 本地事件缓存与重发
    bool saveEventForRetry(const String& eventType, const String& data, const String& eventId);
    void retryUnsentEvents();
};