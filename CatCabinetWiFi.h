#pragma once
#ifndef CATCABINETWIFI_H
#define CATCABINETWIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h> // 加上这一行

class CatCabinetWiFi {
public:
    CatCabinetWiFi();
    bool connectWiFi();
    bool isConnected();           // 声明
    String getIPAddress();        // 声明
    // ...其它成员...
private:
    WiFiManager _wm; // 变量名和cpp一致
};

#endif