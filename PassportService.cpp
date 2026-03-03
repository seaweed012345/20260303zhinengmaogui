#include "PassportService.h"
#include <ArduinoJson.h>

PassportService::PassportService() {
    // 构造函数
}

bool PassportService::begin() {
    _preferences.begin("passport", false);
    return loadFromStorage();
}

bool PassportService::loadFromStorage() {
    String deviceId = _preferences.getString("deviceId", "");
    String authToken = _preferences.getString("authToken", "");
    
    if (deviceId.isEmpty() || authToken.isEmpty()) {
        // 生成新的设备ID和认证令牌
        deviceId = "CAT_" + String(ESP.getEfuseMac(), HEX);
        authToken = String(random(0xffff), HEX) + String(random(0xffff), HEX);
        
        _preferences.putString("deviceId", deviceId);
        _preferences.putString("authToken", authToken);
    }
    
    return true;
}

bool PassportService::saveToStorage() {
    // 如果需要保存数据到存储
    return true;
}

void PassportService::update() {
    // 定期更新护照信息
    // 可以在这里添加心跳检测、令牌刷新等逻辑
}

String PassportService::getPassportJson() {
    StaticJsonDocument<256> doc;
    doc["device_id"] = getDeviceId();
    doc["auth_token"] = getAuthToken();
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String PassportService::getDeviceId() {
    return _preferences.getString("deviceId", "");
}

String PassportService::getAuthToken() {
    return _preferences.getString("authToken", "");
}

bool PassportService::validateRequest(const String& authToken) {
    return authToken == getAuthToken();
}