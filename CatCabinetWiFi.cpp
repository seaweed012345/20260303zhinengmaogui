#include "Config.h"
#include "CatCabinetWiFi.h"

CatCabinetWiFi::CatCabinetWiFi() {
    // 构造函数
}

bool CatCabinetWiFi::connectWiFi() {
    Serial.println("Connecting to WiFi...");
    _wm.setTimeout(120);

    if (!_wm.autoConnect("SmartCatCabinet")) {
        Serial.println("Failed to connect to WiFi");
        return false;
    }

    Serial.println("WiFi connected successfully");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
}

bool CatCabinetWiFi::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String CatCabinetWiFi::getIPAddress() {
    return WiFi.localIP().toString();
}