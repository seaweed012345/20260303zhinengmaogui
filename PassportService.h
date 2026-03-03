#pragma once
#include <Arduino.h>
#include <Preferences.h>

class PassportService {
private:
    Preferences _preferences;
    bool loadFromStorage();
    
public:
    PassportService();
    bool begin();
    bool saveToStorage();
    void update();
    String getPassportJson();
    String getDeviceId();
    String getAuthToken();
    bool validateRequest(const String& authToken);
};