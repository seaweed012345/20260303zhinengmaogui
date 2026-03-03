#include "Config.h"
#include <Preferences.h>

// 全局变量定义（只在这里定义一次）
extern SystemState systemState;
extern SystemConfig systemConfig;
extern SensorReadings sensorReadings;

extern Preferences preferences;

void initializeConfigDefaults() {
    // 设置默认配置值
    systemConfig.foodCalibrationFactor = 1.0f;
    systemConfig.litterCalibrationFactor = 1.0f;
    systemConfig.waterCalibrationFactor = 1.0f;
    systemConfig.temperatureOffset = 0.0f;
    systemConfig.humidityOffset = 0.0f;
    systemConfig.fanAutoThreshold = 25;
    systemConfig.fanMaxSpeed = 255;
    systemConfig.heaterThreshold = 18;
    
    // 初始化系统状态
    systemState.currentPage = 0;
    systemState.lastTouchX = 0;
    systemState.lastTouchY = 0;
    systemState.touchDetected = false;
    systemState.temperature = 0.0f;
    systemState.humidity = 0.0f;
    systemState.weight = 0.0f;
    systemState.fanRunning = false;
    systemState.heaterOn = false;
    systemState.ionizerOn = false;
    systemState.fanAutoMode = true;
    systemState.fanManualOn = false;
    systemState.lightsOn = false;
    systemState.ambientLight = false;
    systemState.mainLight = false;
    
    Serial.println("Default configuration initialized");
}

void loadConfiguration() {
    preferences.begin("cat-cabinet", true);
    
    // 尝试从NVS加载配置，如果不存在则使用默认值
    systemConfig.foodCalibrationFactor = preferences.getFloat("foodCal", 1.0f);
    systemConfig.litterCalibrationFactor = preferences.getFloat("litterCal", 1.0f);
    systemConfig.waterCalibrationFactor = preferences.getFloat("waterCal", 1.0f);
    systemConfig.temperatureOffset = preferences.getFloat("tempOffset", 0.0f);
    systemConfig.humidityOffset = preferences.getFloat("humOffset", 0.0f);
    systemConfig.fanAutoThreshold = preferences.getInt("fanThresh", 25);
    systemConfig.fanMaxSpeed = preferences.getInt("fanMax", 255);
    systemConfig.heaterThreshold = preferences.getInt("heatThresh", 18);
    
    preferences.end();
    
    Serial.println("Configuration loaded from NVS");
}

void saveConfiguration() {
    preferences.begin("cat-cabinet", false);
    
    // 保存配置到NVS
    preferences.putFloat("foodCal", systemConfig.foodCalibrationFactor);
    preferences.putFloat("litterCal", systemConfig.litterCalibrationFactor);
    preferences.putFloat("waterCal", systemConfig.waterCalibrationFactor);
    preferences.putFloat("tempOffset", systemConfig.temperatureOffset);
    preferences.putFloat("humOffset", systemConfig.humidityOffset);
    preferences.putInt("fanThresh", systemConfig.fanAutoThreshold);
    preferences.putInt("fanMax", systemConfig.fanMaxSpeed);
    preferences.putInt("heatThresh", systemConfig.heaterThreshold);
    
    preferences.end();
    
    Serial.println("Configuration saved to NVS");
}