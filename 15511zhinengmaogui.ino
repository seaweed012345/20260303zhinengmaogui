#include <Arduino.h>
#include "TouchDriver.h"
#include "LVGL_TouchDriver.h"
#include <TFT_eSPI.h>
#include <Wire.h>
#include <SPI.h>
#include "PinDefinitions.h"
#include "Config.h"
#include "WeightSensor.h"
#include "TemperatureSensor.h"
#include "FanController.h"
#include "RelayController.h"
#include "DisplayManager.h"
#include "Display.h"
#include "TouchDriver.h"
#include "LVGL_Init.h"
#include <Preferences.h>
#include "ui_main.h"
#include "lvgl.h"
#include "WeightManager.h"
#include "EventDetector.h"
#include "StorageManager.h"
#include "MQTTManager.h"
#include "tile1_page.h"
#include "PassportService.h"
#include <time.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_SGP30.h>
#include <WebServer.h>
#include <vector>

// ========== Backlight config (adjust per your hardware) ==========
#define BACKLIGHT_PIN 21
#define BACKLIGHT_LEDC_CHANNEL 7
#define BACKLIGHT_LEDC_FREQ 5000
#define BACKLIGHT_LEDC_RES 8
#define BACKLIGHT_ON_VALUE 255
#define BACKLIGHT_OFF_VALUE 0

// ========== 新增：DHT22和SGP30对象 ==========
#define DHTPIN DHT_PIN
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SGP30 sgp;

// ========== 新增：SGP30 baseline初始值 ==========
#define SGP30_BASELINE_ECO2  0x9B2A
#define SGP30_BASELINE_TVOC  0x8D47

// ========== 新增：ACD10 CO2传感器UART配置（HardwareSerial） ==========
#define CO2_RX_PIN 1
#define CO2_TX_PIN 3
#define CO2_BAUD_RATE 1200
#define CO2_SENSOR_RETRY_INTERVAL 5000

// ★ 创建硬件串口对象（UART1）
HardwareSerial CO2Serial(1);

// 全局对象定义
TFT_eSPI tft;
TouchDriver touchDriver;
Preferences preferences;
DisplayManager displayManager(tft);
WeightSensor weightSensor;
TemperatureSensor tempSensor;
FanController fanController(FAN1_PWM_PIN, FAN2_PWM_PIN, BUZZER_PIN);
RelayController relayController;
PassportService passportService;

WiFiClient espClient;
PubSubClient pubSubClient(espClient);
MQTTManager mqttManager(pubSubClient);

SystemState systemState;
SystemConfig systemConfig;
SensorReadings sensorReadings;
WeightManager weightMgr;
EventDetector eventDet(&weightMgr);
StorageManager storage;

// ========== 重要修正：主线程MQTT/UI安全上报和刷新标志 ==========
volatile bool needPublishStatus = false;
volatile bool needUpdateLevelLabels = false;
int levelLabelsEat = 2, levelLabelsDrink = 2, levelLabelsToilet = 2, levelLabelsWeight = 2;
volatile bool needUpdateCurve = false;
int curveEat[10], curveDrink[10], curveToilet[10], curveWeight[10];

// 传感器防呆状态和重试定时器
bool weightSensorOK = false;
bool tempSensorOK = false;
unsigned long lastWeightRetryMillis = 0;
unsigned long lastTempRetryMillis = 0;
const unsigned long SENSOR_RETRY_INTERVAL = 5 * 60 * 1000UL; // 5分钟

// ========== 新增：CO2传感器防呆状态 ==========
bool co2SensorOK = false;
unsigned long lastCO2RetryMillis = 0;
unsigned long co2FailureCount = 0;

// ========== 新风模式标志 ==========
extern bool fresh_auto_mode;
extern bool fresh_manual_mode;

// ========== 风扇PWM相关 ==========
constexpr int FAN_PWM_MAX = 255;

// ========== 保留旧全局缓存（兼容） ==========
volatile float g_temp = 0;
volatile float g_hum = 0;
volatile int g_aqi = 0;
volatile int g_co2_ppm = 0;
volatile float g_weight[SENSOR_COUNT] = {0};

// ======================================================================
// ========== NTP / 时间 与 时区支持 ===============================
// ======================================================================
static const char* PREF_NS_SYS = "sys";
static const char* PREF_KEY_TZ_OFFSET = "tz_offset";

bool timeInitialized = false;
unsigned long lastTimeUpdateMs = 0;
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
int tzOffsetHours_cached = 8;

static void applyTimezoneAndConfigTime(int tzHours)
{
    long gmtOffset_sec = tzHours * 3600L;
    int daylightOffset_sec = 0;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
}

static void initTimeViaNTP() {
    if (timeInitialized) return;

    preferences.begin(PREF_NS_SYS, true);
    int tz = preferences.getInt(PREF_KEY_TZ_OFFSET, tzOffsetHours_cached);
    preferences.end();
    tzOffsetHours_cached = tz;
    applyTimezoneAndConfigTime(tzOffsetHours_cached);

    const unsigned long waitStart = millis();
    while (millis() - waitStart < 3000) {
        time_t now = time(nullptr);
        if (now > 8 * 3600) {
            timeInitialized = true;
            Serial.println("NTP initial sync OK");
            break;
        }
        delay(200);
    }
    if (!timeInitialized) Serial.println("NTP initial sync pending");
}

void setTimezoneOffsetHours(int tzHours) {
    preferences.begin(PREF_NS_SYS, false);
    preferences.putInt(PREF_KEY_TZ_OFFSET, tzHours);
    preferences.end();

    tzOffsetHours_cached = tzHours;
    applyTimezoneAndConfigTime(tzOffsetHours_cached);

    const unsigned long waitStart = millis();
    bool synced = false;
    while (millis() - waitStart < 2000) {
        time_t now = time(nullptr);
        if (now > 8 * 3600) { synced = true; break; }
        delay(200);
    }
    timeInitialized = synced;

    time_t now = time(nullptr);
    struct tm timeinfo;
    if (localtime_r(&now, &timeinfo)) {
        char timestr[16];
        if (strftime(timestr, sizeof(timestr), "%H:%M", &timeinfo) > 0) {
            ui_update_time(timestr);
        }
    }
    lastTimeUpdateMs = millis();

    Serial.printf("Timezone set to GMT%+d, timeInitialized=%d\n", tzOffsetHours_cached, timeInitialized ? 1 : 0);
}

// ======================================================================
// ========== 自动息屏 / 触摸唤醒 相关实现 =======================
// ======================================================================

static const char* PREF_KEY_AUTOSLEEP = "autosleep_mode";

static int autosleep_mode = 0;
static unsigned long lastUserInteractionMs = 0;
static bool screenSleeping = false;

static void notifyBacklightChange(bool on) {
    if (on) {
        ledcWrite(BACKLIGHT_LEDC_CHANNEL, BACKLIGHT_ON_VALUE);
    } else {
        ledcWrite(BACKLIGHT_LEDC_CHANNEL, BACKLIGHT_OFF_VALUE);
    }
    Serial.printf("notifyBacklightChange: %s (pin=%d)\n", on ? "ON" : "OFF", BACKLIGHT_PIN);
}

static void onScreenSleep() {
    if (screenSleeping) return;
    screenSleeping = true;
    notifyBacklightChange(false);
    Serial.println("Screen -> SLEEP");
}

static void onScreenWake() {
    if (!screenSleeping) return;
    screenSleeping = false;
    notifyBacklightChange(true);
    Serial.println("Screen -> WAKE");
}

extern "C" void userInteraction() {
    lastUserInteractionMs = millis();
    Serial.println("userInteraction()");
    if (screenSleeping) {
        onScreenWake();
    }
}

void setAutoSleepMode(int mode) {
    if (mode < 0) mode = 0;
    if (mode > 2) mode = 2;
    autosleep_mode = mode;

    preferences.begin(PREF_NS_SYS, false);
    preferences.putInt(PREF_KEY_AUTOSLEEP, autosleep_mode);
    preferences.end();

    lastUserInteractionMs = millis();

    Serial.printf("AutoSleep mode set to %d\n", autosleep_mode);
}

static void global_touch_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_CLICKED || code == LV_EVENT_VALUE_CHANGED) {
        userInteraction();
    }
}

// ======================================================================
// ========== WeightManager 后台任务包装器 ===============================
// ======================================================================
void startWeightManagerTask(void* pvParameters) {
    WeightManager* wm = (WeightManager*)pvParameters;
    if (wm) {
        wm->sensorTask();
    }
    vTaskDelete(NULL);
}

void resetSPIPins() {}
void initializeSPI() {}
void debugPinTest() {}
void analyzeSPIBus() {}
void checkSystemHealth() {}
void initializeHardware() {
    Serial.println("步骤2: 初始化触摸...");
    if (initTouch()) {
        Serial.println("触摸初始化成功");
    } else {
        Serial.println("触摸初始化失败");
    }
}
void tryReconnectSensors() {}

// ======================================================================
// ================= SoftAP + Captive Portal (Provisioning) ==============
// ======================================================================

WebServer webServer(80);
bool provAPRunning = false;

enum ProvState { PROV_IDLE = 0, PROV_AP = 1, PROV_CONNECTING = 2, PROV_CONNECTED = 3, PROV_FAILED = 4 };
volatile ProvState provState = PROV_IDLE;
String provSsid = "";
String provPass = "";
unsigned long provConnectStartMs = 0;
const unsigned long PROV_CONNECT_TIMEOUT_MS = 15000UL;

void handleRoot() {
    const char* page =
        "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Device WiFi Setup</title>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'></head><body>"
        "<h3>Device WiFi Provision</h3>"
        "<form method='POST' action='/connect'>"
        "SSID:<br><input name='ssid' maxlength='32' /><br>"
        "Password:<br><input name='pass' type='password' maxlength='64' /><br><br>"
        "<input type='submit' value='Connect' /></form>"
        "<p>After submit, check <a href='/status'>/status</a> for progress.</p>"
        "</body></html>";
    webServer.send(200, "text/html", page);
}

void handleConnect() {
    if (!webServer.hasArg("ssid")) {
        webServer.send(400, "text/plain", "Missing ssid");
        return;
    }
    String ssid = webServer.arg("ssid");
    String pass = webServer.arg("pass");
    ssid.trim();
    pass.trim();
    if (ssid.length() == 0) {
        webServer.send(400, "text/plain", "Empty ssid");
        return;
    }

    provSsid = ssid;
    provPass = pass;
    provState = PROV_CONNECTING;
    provConnectStartMs = millis();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    WiFi.begin(provSsid.c_str(), provPass.c_str());

    String resp = "<html><body><p>Connecting to SSID: " + ssid + "</p>"
                  "<p>Check <a href='/status'>/status</a> for result.</p></body></html>";
    webServer.send(200, "text/html", resp);
}

void handleStatus() {
    String s;
    switch (provState) {
        case PROV_IDLE: s = "{\"state\":\"idle\"}"; break;
        case PROV_AP: s = "{\"state\":\"ap_running\"}"; break;
        case PROV_CONNECTING:
            s = "{\"state\":\"connecting\",\"ssid\":\"" + provSsid + "\"}"; break;
        case PROV_CONNECTED:
            s = "{\"state\":\"connected\",\"ssid\":\"" + provSsid + "\"}"; break;
        case PROV_FAILED:
            s = "{\"state\":\"failed\",\"ssid\":\"" + provSsid + "\"}"; break;
        default: s = "{\"state\":\"unknown\"}"; break;
    }
    webServer.send(200, "application/json", s);
}

void startProvisionAP() {
    if (provAPRunning) return;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char suffix[6];
    snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
    String apName = String("MyDevice_") + suffix;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), NULL);
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("Provision AP started: SSID=%s, IP=%s\n", apName.c_str(), ip.toString().c_str());

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/connect", HTTP_POST, handleConnect);
    webServer.on("/status", HTTP_GET, handleStatus);
    webServer.begin();

    provAPRunning = true;
    provState = PROV_AP;
}

void stopProvisionAP() {
    if (!provAPRunning) return;
    webServer.stop();
    WiFi.softAPdisconnect(true);
    provAPRunning = false;
}

void beginWifiWithSavedCredentials() {
    preferences.begin("wifi", true);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");
    preferences.end();

    if (ssid.length() > 0) {
        Serial.printf("Attempting WiFi connect to saved SSID: %s\n", ssid.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        unsigned long start = millis();
        const unsigned long timeout = 5000UL;
        bool ok = false;
        while (millis() - start < timeout) {
            if (WiFi.status() == WL_CONNECTED) { ok = true; break; }
            delay(200);
        }
        if (ok) {
            Serial.printf("Connected to saved WiFi: %s, IP=%s\n", ssid.c_str(), WiFi.localIP().toString().c_str());
            provState = PROV_CONNECTED;
            return;
        } else {
            Serial.println("Failed to connect to saved WiFi credentials.");
        }
    } else {
        Serial.println("No saved WiFi credentials found.");
    }
    startProvisionAP();
}

// ======================================================================
// ================= 产测串口命令处理 ==========================
// ======================================================================
void printProdHelp() {
    Serial.println("=== PRODUCTION CALIBRATION INTERFACE ===");
    Serial.println("Commands:");
    Serial.println("  HELP");
    Serial.println("  LIST");
    Serial.println("  TAKE_TARE <id>            -- tare and record empty weight for channel id");
    Serial.println("  TAKE_WEIGHT <id> <grams>  -- use known weight (grams) to compute factor");
    Serial.println("  SAVE <id>                 -- persist calibration factor and tare to flash");
    Serial.println("  GET <id>                 -- return current factor/tare/lastWeight");
    Serial.println("Example: TAKE_TARE 0   (then put 100g)   TAKE_WEIGHT 0 100   SAVE 0");
}

static std::vector<String> splitTokens(const String &line) {
    std::vector<String> parts;
    int len = line.length();
    String cur;
    for (int i = 0; i < len; ++i) {
        char c = line.charAt(i);
        if (c == ' ' || c == '\t') {
            if (cur.length() > 0) {
                parts.push_back(cur);
                cur = "";
            }
        } else {
            cur += c;
        }
    }
    if (cur.length() > 0) parts.push_back(cur);
    return parts;
}

void processProductionCommand(const String& line) {
    if (line.length() == 0) return;
    std::vector<String> parts = splitTokens(line);
    if (parts.size() == 0) return;
    String cmd = parts[0];
    cmd.toUpperCase();

    if (cmd == "HELP") {
        printProdHelp();
        return;
    } else if (cmd == "LIST") {
        Serial.printf("SENSOR_COUNT=%d\n", SENSOR_COUNT);
        for (int i = 0; i < SENSOR_COUNT; ++i) {
            WeightSensor &s = weightMgr.getSensor(i);
            Serial.printf("  id=%d available=%d last=%.2f\n", i, s.isAvailable() ? 1 : 0, weightMgr.getLastWeight(i));
        }
        return;
    } else if (cmd == "TAKE_TARE") {
        if (parts.size() < 2) { Serial.println("ERR: TAKE_TARE requires id"); return; }
        int id = parts[1].toInt();
        if (id < 0 || id >= SENSOR_COUNT) { Serial.println("ERR: invalid id"); return; }
        Serial.printf("Taring sensor %d ...\n", id);
        WeightSensor &s = weightMgr.getSensor(id);
        s.tare();
        float empty = s.readWeight();
        Serial.printf("TARE_DONE id=%d emptyWeight=%.2f\n", id, empty);
        return;
    } else if (cmd == "TAKE_WEIGHT") {
        if (parts.size() < 3) { Serial.println("ERR: TAKE_WEIGHT requires id and grams"); return; }
        int id = parts[1].toInt();
        float grams = parts[2].toFloat();
        if (id < 0 || id >= SENSOR_COUNT) { Serial.println("ERR: invalid id"); return; }
        if (grams <= 0.0f) { Serial.println("ERR: invalid grams"); return; }
        Serial.printf("Calibrating sensor %d with %.2fg ...\n", id, grams);
        WeightSensor &s = weightMgr.getSensor(id);
        s.calibrate(grams);
        float factor = s.getCalibrationFactor();
        Serial.printf("CALIB_DONE id=%d factor=%.6f\n", id, factor);
        return;
    } else if (cmd == "SAVE") {
        if (parts.size() < 2) { Serial.println("ERR: SAVE requires id"); return; }
        int id = parts[1].toInt();
        if (id < 0 || id >= SENSOR_COUNT) { Serial.println("ERR: invalid id"); return; }
        WeightSensor &s = weightMgr.getSensor(id);
        bool ok = s.saveCalibration(id);
        if (ok) {
            Serial.printf("SAVED id=%d factor=%.6f tare=%.2f\n", id, s.getCalibrationFactor(), weightMgr.getLastWeight(id));
        } else {
            Serial.println("ERR: SAVE failed");
        }
        return;
    } else if (cmd == "GET") {
        if (parts.size() < 2) { Serial.println("ERR: GET requires id"); return; }
        int id = parts[1].toInt();
        if (id < 0 || id >= SENSOR_COUNT) { Serial.println("ERR: invalid id"); return; }
        WeightSensor &s = weightMgr.getSensor(id);
        Serial.printf("ID=%d factor=%.6f last=%.2f\n", id, s.getCalibrationFactor(), weightMgr.getLastWeight(id));
        return;
    } else {
        Serial.println("ERR: unknown command. Send HELP");
        return;
    }
}

void processProductionSerial() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            processProductionCommand(line);
        }
    }
}

// ========== CRC8校验 ==========
uint8_t co2_calc_crc8(uint8_t *dat, uint8_t num) {
    uint8_t i, byte, crc = 0xFF;
    for (byte = 0; byte < num; byte++) {
        crc ^= (dat[byte]);
        for (i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

// ========== 初始化UART（HardwareSerial版本） ==========
void init_co2_uart() {
    CO2Serial.begin(CO2_BAUD_RATE, SERIAL_8N1, CO2_RX_PIN, CO2_TX_PIN);
    delay(100);
    Serial.println("CO2 UART initialized at 1200 baud");
}

// ========== 读取CO2传感器（HardwareSerial版本） ==========
void read_co2_sensor() {
    unsigned long startTime = millis();
    
    // 检查是否需要重试
    if (!co2SensorOK && (millis() - lastCO2RetryMillis < CO2_SENSOR_RETRY_INTERVAL)) {
        g_co2_ppm = 400;  // 默认值
        return;
    }
    
    lastCO2RetryMillis = millis();
    
    // 清空旧数据
    while (CO2Serial.available()) {
        CO2Serial.read();
        delay(1);
    }
    
    // 发送命令：0x54 0x03 0x00
    uint8_t cmd[3] = {0x54, 0x03, 0x00};
    CO2Serial.write(cmd, 3);
    
    Serial.println("CO2: CMD sent");
    
    // 等待响应
    delay(20);
    
    // 检查缓冲区
    int available = CO2Serial.available();
    Serial.printf("CO2: available = %d bytes\n", available);
    
    if (available < 9) {
        Serial.printf("CO2: insufficient data\n");
        g_co2_ppm = 400;
        co2SensorOK = false;
        co2FailureCount++;
        return;
    }
    
    // 读取9个字节
    uint8_t response[9] = {0};
    for (int i = 0; i < 9; i++) {
        if (CO2Serial.available()) {
            response[i] = CO2Serial.read();
        }
    }
    
    // 清空缓冲区
    while (CO2Serial.available()) {
        CO2Serial.read();
    }
    
    Serial.printf("CO2: received: ");
    for (int i = 0; i < 9; i++) {
        Serial.printf("%02X ", response[i]);
    }
    Serial.println();
    
    // 检查CRC
    uint8_t crc1 = co2_calc_crc8(response, 2);
    uint8_t crc2 = co2_calc_crc8(response + 3, 2);
    uint8_t crc3 = co2_calc_crc8(response + 6, 2);
    
    if (crc1 != response[2] || crc2 != response[5] || crc3 != response[8]) {
        Serial.println("CO2: CRC FAILED");
        g_co2_ppm = 400;
        co2SensorOK = false;
        co2FailureCount++;
        return;
    }
    
    // 解析CO2浓度
    int co2 = (response[0] << 8) | response[1];
    
    Serial.printf("CO2: value = %d ppm\n", co2);
    
    // 数据有效性检查
    if (co2 > 5000 || co2 < 350) {
        Serial.printf("CO2: out of range\n");
        g_co2_ppm = 400;
        co2SensorOK = false;
        co2FailureCount++;
        return;
    }
    
    // 读取成功
    g_co2_ppm = co2;
    co2SensorOK = true;
    co2FailureCount = 0;
    
    Serial.printf("CO2 OK: %d ppm\n", co2);
}

// ========== 主程序入口 ==========
void setup() {
    resetSPIPins();
    delay(200);

    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);

    Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
    Wire.setClock(100000);

    if (!sgp.begin()){
        Serial.println("SGP30 init failed!");
        while (1);
    }
    sgp.setIAQBaseline(SGP30_BASELINE_ECO2, SGP30_BASELINE_TVOC);
    for (int i = 0; i < 10; i++) { if (!sgp.IAQmeasure()) delay(100); }

    tft.init();
    tft.setRotation(1);

    LVGL_Init();
    touchDriver.init();
    ui_init();
    LVGL_TouchInit();

    // 初始化背光控制
    pinMode(BACKLIGHT_PIN, OUTPUT);
    ledcSetup(BACKLIGHT_LEDC_CHANNEL, BACKLIGHT_LEDC_FREQ, BACKLIGHT_LEDC_RES);
    ledcAttachPin(BACKLIGHT_PIN, BACKLIGHT_LEDC_CHANNEL);
    ledcWrite(BACKLIGHT_LEDC_CHANNEL, BACKLIGHT_ON_VALUE);
    lastUserInteractionMs = millis();
    Serial.printf("Backlight initialized on pin %d (LEDC ch %d)\n", BACKLIGHT_PIN, BACKLIGHT_LEDC_CHANNEL);

    lv_obj_add_event_cb(lv_scr_act(), global_touch_cb, LV_EVENT_ALL, NULL);

    std::vector<uint8_t> relayPins = {RELAY_AMBIENT_PIN, RELAY_MAIN_PIN, RELAY_FAN_PIN};
    relayController.init(relayPins);

    dht.begin();

    ledcSetup(0, 25000, 8);
    ledcAttachPin(FAN1_PWM_PIN, 0);
    ledcSetup(1, 25000, 8);
    ledcAttachPin(FAN2_PWM_PIN, 1);

    for (int i = 0; i < SENSOR_COUNT; ++i) {
        weightMgr.getSensor(i).loadCalibration(i);
    }

    weightMgr.begin();
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        float tare = storage.loadTareValue(i);
    }

    beginWifiWithSavedCredentials();

    xTaskCreatePinnedToCore(
        startWeightManagerTask,
        "WM_SensorTask",
        4096,
        &weightMgr,
        1,
        NULL,
        1
    );

    preferences.begin(PREF_NS_SYS, true);
    tzOffsetHours_cached = preferences.getInt(PREF_KEY_TZ_OFFSET, tzOffsetHours_cached);
    int savedAuto = preferences.getInt(PREF_KEY_AUTOSLEEP, 0);
    preferences.end();
    autosleep_mode = savedAuto;
    lastUserInteractionMs = millis();

    // ★ 初始化CO2 UART
    init_co2_uart();
    co2SensorOK = false;
    lastCO2RetryMillis = 0;
    co2FailureCount = 0;
    Serial.println("CO2 sensor (ACD10) UART mode ready");

    Serial.println("\n\n===== SETUP COMPLETE =====\n");  // ★ 新增诊断行
    Serial.println("Device setup complete. Send HELP on serial for production commands.");
}

void set_fan_pwm_manual() {
    int pwm2 = FAN_PWM_MAX / 2;
    int pwm1 = max(pwm2 - 51, 0);
    ledcWrite(1, pwm2);
    ledcWrite(0, pwm1);
}
void set_fan_pwm_stop() {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
}
void set_fan_pwm_auto(int aqi) {
    const int PWM_MIN = 60;
    const int PWM_MAX = 255;
    int pwm2 = 0;
    if (aqi <= 50) {
        pwm2 = 0;
    } else if (aqi >= 2000) {
        pwm2 = PWM_MAX;
    } else {
        pwm2 = PWM_MIN + (aqi - 50) * (PWM_MAX - PWM_MIN) / 50;
    }
    int pwm1 = max(pwm2 - 51, 0);
    ledcWrite(1, pwm2);
    ledcWrite(0, pwm1);
    Serial.printf("AQI=%d, FAN2_PWM=%d, FAN1_PWM=%d\n", aqi, pwm2, pwm1);
}

void report_food_event(int delta, int total, time_t eventTime);
void report_drink_event(int delta, int total, time_t eventTime);
void report_toilet_event(int delta, int total, time_t eventTime);
void report_weight_event(float value, time_t eventTime);
void reset_today_counters();

static int last_day = -1;


void loop() {
    static unsigned long lastSensorRead = 0;
    static unsigned long lastStatusPrint = 0;
    static int last_aqi = 0;
    static bool sgp30_ready = false;
    static int sgp30_warmup = 0;

    unsigned long currentMillis = millis();

    lv_tick_inc(5);
    lv_timer_handler();
    displayManager.handleTouch();

    if (needUpdateLevelLabels) {
        ui_update_level_labels(levelLabelsEat, levelLabelsDrink, levelLabelsToilet, levelLabelsWeight);
        needUpdateLevelLabels = false;
    }
    if (needUpdateCurve) {
        tile1_set_chart(0, curveEat, 10);
        tile1_set_chart(1, curveDrink, 10);
        tile1_set_chart(2, curveToilet, 10);
        tile1_set_chart(3, curveWeight, 10);
        needUpdateCurve = false;
    }

    processProductionSerial();

    if (provAPRunning) {
        webServer.handleClient();
    }

    if (provState == PROV_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            provState = PROV_CONNECTED;
            Serial.printf("Provisioning: Connected to %s, IP=%s\n", provSsid.c_str(), WiFi.localIP().toString().c_str());
            preferences.begin("wifi", false);
            preferences.putString("ssid", provSsid);
            preferences.putString("pass", provPass);
            preferences.end();
            stopProvisionAP();
            initTimeViaNTP();
        } else {
            if (millis() - provConnectStartMs > PROV_CONNECT_TIMEOUT_MS) {
                provState = PROV_FAILED;
                Serial.println("Provisioning: connect attempt timed out.");
            }
        }
    }

    if (currentMillis - lastSensorRead >= 1000) {
        // ★ 每次都读取CO2（不限制重试）
        read_co2_sensor();

        float t = dht.readTemperature();
        float h = dht.readHumidity();
        g_temp = isnan(t) ? -100.0f : t;
        g_hum = isnan(h) ? -1.0f : h;

        if (!sgp30_ready) {
            if (sgp.IAQmeasure()) {
                sgp30_warmup++;
            }
            if (sgp30_warmup >= 15) sgp30_ready = true;
            g_aqi = 0;
        } else {
            if (sgp.IAQmeasure()) {
                g_aqi = sgp.TVOC;
            } else {
                g_aqi = -1;
            }
        }
        last_aqi = g_aqi;

        // ★ 更新UI显示
        ui_update_env(g_temp, g_hum, g_co2_ppm, last_aqi);

        float foodWeight = weightMgr.getLastWeight(0);
        float waterWeight = weightMgr.getLastWeight(1);
        float litterWeight = weightMgr.getLastWeight(2);

        g_weight[0] = foodWeight;
        g_weight[1] = waterWeight;
        g_weight[2] = litterWeight;

        eventDet.update();

        if (!timeInitialized && WiFi.status() == WL_CONNECTED) {
            initTimeViaNTP();
        }

        time_t now = time(nullptr);

        if (timeInitialized || true) {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            char timestr[16];
            if (strftime(timestr, sizeof(timestr), "%H:%M", &timeinfo) > 0) {
                ui_update_time(timestr);
            }
        }

        if (!screenSleeping) {
            if (autosleep_mode != 2) {
                bool shouldSleep = false;
                unsigned long idle = millis() - lastUserInteractionMs;
                if (autosleep_mode == 0) {
                    if (idle >= 3600UL * 1000UL) shouldSleep = true;
                } else if (autosleep_mode == 1) {
                    time_t tt = time(nullptr);
                    struct tm tinfo;
                    localtime_r(&tt, &tinfo);
                    int hour = tinfo.tm_hour;
                    if (hour >= 23 || hour <= 5) {
                        if (idle >= 5000UL) shouldSleep = true;
                    }
                }
                if (shouldSleep) onScreenSleep();
            }
        }

        if (eventDet.hasNewFoodEvent()) {
            int delta = eventDet.getFoodDelta();
            int total = eventDet.getFoodTotal();
            report_food_event(delta, total, now);
        }
        if (eventDet.hasNewDrinkEvent()) {
            int delta = eventDet.getDrinkDelta();
            int total = eventDet.getDrinkTotal();
            report_drink_event(delta, total, now);
        }
        if (eventDet.hasNewToiletEvent()) {
            int delta = eventDet.getToiletDelta();
            int total = eventDet.getToiletTotal();
            report_toilet_event(delta, total, now);
        }
        if (eventDet.hasNewWeightEvent()) {
            float value = eventDet.getWeightValue();
            report_weight_event(value, now);
        }

        struct tm *tm_now = localtime(&now);
        if (tm_now && tm_now->tm_mday != last_day) {
            reset_today_counters();
            last_day = tm_now->tm_mday;
        }

        lastSensorRead = currentMillis;
    }

    if (fresh_auto_mode) {
        if (last_aqi > 50) {
            relayController.setFreshPower(true);
            set_fan_pwm_auto(last_aqi);
        } else {
            relayController.setFreshPower(false);
            set_fan_pwm_stop();
        }
    } else if (fresh_manual_mode) {
        relayController.setFreshPower(true);
        set_fan_pwm_manual();
    } else {
        relayController.setFreshPower(false);
        set_fan_pwm_stop();
    }

    if (currentMillis - lastStatusPrint >= 5000) {
        Serial.printf("状态: 温度:%.1f 湿度:%.1f CO2:%d AQI:%d WeightF:%.2f WeightW:%.2f WeightL:%.2f CO2Status:%s Heap:%dB\n",
            g_temp,
            g_hum,
            g_co2_ppm,
            last_aqi,
            weightMgr.getLastWeight(0), weightMgr.getLastWeight(1), weightMgr.getLastWeight(2),
            co2SensorOK ? "OK" : "FAIL",
            ESP.getFreeHeap());
        lastStatusPrint = currentMillis;
    }

    if (needPublishStatus) {
        mqttManager.publishStatus();
        needPublishStatus = false;
    }

    delay(1);
}


void on_wechat_level_received(int eat, int drink, int toilet, int weight) {
    levelLabelsEat = eat;
    levelLabelsDrink = drink;
    levelLabelsToilet = toilet;
    levelLabelsWeight = weight;
    needUpdateLevelLabels = true;
}
void on_wechat_curve_received(const int* eat_curve, const int* drink_curve, const int* toilet_curve, const int* weight_curve) {
    for(int i=0; i<10; ++i) {
        curveEat[i] = eat_curve[i];
        curveDrink[i] = drink_curve[i];
        curveToilet[i] = toilet_curve[i];
        curveWeight[i] = weight_curve[i];
    }
    needUpdateCurve = true;
}