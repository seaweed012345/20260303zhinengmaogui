#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <vector>

// 系统配置
#define FIRMWARE_VERSION "1.0.0"
#define SERIAL_BAUD_RATE 115200

// 显示配置
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 480
#define DISPLAY_ROTATION 1
#define DISPLAY_UPDATE_INTERVAL 100


// 新增常量定义
#define AIR_QUALITY_THRESHOLD 50
#define FAN_LIFETIME 10000





// 系统配置结构体
struct SystemConfig {
    float foodCalibrationFactor;
    float litterCalibrationFactor;
    float waterCalibrationFactor;
    float temperatureOffset;
    float humidityOffset;
    int fanAutoThreshold;
    int fanMaxSpeed;
    int heaterThreshold;
};

// 传感器读数结构体
struct SensorReadings {
    struct {
        unsigned long uptime;
        float cpuTemperature;
        float batteryLevel;
        unsigned long fanRuntime;
        int fanHealth;
    } system;
    
    struct {
        float weight;
        float temperature;
        float humidity;
        int airQuality;
        float pressure;
        float pressureDiff;
        float current;
        float remainingDays;
    } environment;
    
    struct {
        float weight;
        float current;
        float remainingDays;
    } food;
    
    struct {
        float weight;
        float current;
        float remainingDays;
    } litter;
    
    struct {
        float weight;
        int excretionCount;
        float excretionAmount;
    } cat;
    
    struct {
        float weight;
        float current;
        float remainingDays;
    } water;
};

// 系统状态结构体
struct SystemState {
    uint8_t currentPage;
    uint16_t lastTouchX;
    uint16_t lastTouchY;
    bool touchDetected;
    float temperature;
    float humidity;
    float weight;
    bool fanRunning;
    bool heaterOn;
    bool ionizerOn;
    bool fanAutoMode;
    bool fanManualOn;
    bool lightsOn;
    bool ambientLight;
    bool mainLight;
};

// 全局变量声明（使用extern）
extern SystemState systemState;
extern SystemConfig systemConfig;
extern SensorReadings sensorReadings;

// 函数声明
void initializeConfigDefaults();
void loadConfiguration();
void saveConfiguration();

#endif // CONFIG_H