#pragma once
#include "event_types.h"
#include "WeightManager.h"
#include <stdint.h>

#define MAX_EVENT_HISTORY 32

// 单位：克 (g)
// 可配置阈值（按需调整）
#ifndef FOOD_WATER_THRESHOLD_G
#define FOOD_WATER_THRESHOLD_G 2.0f         // 食物/水 阈值：2 g
#endif

// 新增：食物/饮水上限阈值（避免异常大变化被判为一次行为）
#ifndef FOOD_MAX_G
#define FOOD_MAX_G 150.0f                   // 食物单次上限 150 g（可调整）
#endif

#ifndef WATER_MAX_G
#define WATER_MAX_G 200.0f                  // 饮水单次上限 200 g（ml≈g）
#endif

#ifndef ENTER_THRESHOLD_G
#define ENTER_THRESHOLD_G 300.0f            // 任意一次超过 baseline + 300g 触发进入候选
#endif

#ifndef EXIT_THRESHOLD_G
#define EXIT_THRESHOLD_G 300.0f             // 离开回落阈值（用于早期判定回落）
#endif

#ifndef DEFECATE_THRESHOLD_G
#define DEFECATE_THRESHOLD_G 2.0f           // 排便 判定阈值：>2 g
#endif

// 新增：排便上限阈值（避免异常大量变化被误判为一次排泄）
#ifndef DEFECATE_MAX_G
#define DEFECATE_MAX_G 250.0f               // 排泄单次上限 250 g（可调整）
#endif

// 连续采样确认参数（用于判断稳定）
#ifndef STABLE_SAMPLES
#define STABLE_SAMPLES 3                    // 停留期间需观测到的稳定样本数
#endif

#ifndef STABLE_TOLERANCE_G
#define STABLE_TOLERANCE_G 10.0f            // 这 STABLE_SAMPLES 样本的 max-min <= 10g 视为稳定
#endif

// 离开确认需要的连续样本数
#ifndef EXIT_CONFIRM_SAMPLES
#define EXIT_CONFIRM_SAMPLES 2
#endif

// 最小停留时间（毫秒），猫必须在盆中停留至少此时间才可判定排便
#ifndef MIN_DWELL_MS
#define MIN_DWELL_MS 5000UL
#endif

// 退出恢复区间（相对于进入前基线）：允许的最小/最大变化量（g）
// baseline_after - baseline_before 必须在 [EXIT_RECOVERY_MIN, EXIT_RECOVERY_MAX] 才算合理
#ifndef EXIT_RECOVERY_MIN
#define EXIT_RECOVERY_MIN -50.0f            // 可能刨掉猫砂，最少可减少50g
#endif
#ifndef EXIT_RECOVERY_MAX
#define EXIT_RECOVERY_MAX 150.0f            // 最多增加150g（粪便）
#endif

// 基线指数平滑因子（0..1），越小基线越稳定
#ifndef BASELINE_ALPHA
#define BASELINE_ALPHA 0.02f
#endif

// 进入样本峰值容差（用于候选峰值匹配，可选，保留用于调试/备用）
#ifndef ENTER_PEAK_TOLERANCE_G
#define ENTER_PEAK_TOLERANCE_G 50.0f
#endif

// 串口调试开关：1 开，0 关
#ifndef LITTER_DEBUG_VERBOSE
#define LITTER_DEBUG_VERBOSE 1
#endif

class EventDetector {
public:
    EventDetector(WeightManager* weightMgr);
    void update();
    bool getLastEvent(EventRecord& outEvent);
    void getTodayStats(int& feedCount, float& feedTotal,
                       int& drinkCount, float& drinkTotal,
                       int& defecateCount, float& defecateTotal);

    // 主循环可用事件读取API
    bool hasNewFoodEvent();
    int getFoodDelta();
    int getFoodTotal();
    bool hasNewDrinkEvent();
    int getDrinkDelta();
    int getDrinkTotal();
    bool hasNewToiletEvent();
    int getToiletDelta();
    int getToiletTotal();
    bool hasNewWeightEvent();
    float getWeightValue();

    // 每日清零累计
    void resetTodayStats();

private:
    WeightManager* wm;
    EventRecord eventHistory[MAX_EVENT_HISTORY];
    int eventCount;

    // 进食/饮水事件相关状态
    float lastFoodWeight;
    float lastWaterWeight;
    unsigned long lastFoodChangeTime;
    unsigned long lastWaterChangeTime;

    // 猫砂盆事件相关状态（状态机）
    enum LitterState {
        LITTER_IDLE = 0,
        LITTER_IN_PROGRESS,   // 任意一次超过 300g 的候选期
        LITTER_CONFIRMED      // 已确认完全进盆并处于盆内（IN_LITTER）
    } litterState;

    // 基线与样本
    float baselineLitterWeight;       // 平稳基线（平时自动平滑）
    float lastLitterWeight;           // 最近一次采样的原始值（仅用于 debug/历史）
    float baselineBeforeEntry;        // 进入前的基线，供离开时比较
    float stableSamples[STABLE_SAMPLES];
    int stableCount;
    unsigned long candidateStartMs;   // 进入候选开始时间
    float candidatePeak;             // 候选峰值（便于容差判断）

    // 离开确认计数
    int exitConfirmCount;

    // IN_LITTER 时记录
    unsigned long catEnterTime;
    float catWeightMedian;           // 三次稳定样本的中位数（盆+猫）

    // 主循环边沿事件 API
    bool newFoodEvent;
    int lastFoodDelta;
    int lastFoodTotal;
    bool newDrinkEvent;
    int lastDrinkDelta;
    int lastDrinkTotal;
    bool newToiletEvent;
    int lastToiletDelta;
    int lastToiletTotal;
    bool newWeightEvent;
    float lastWeightValue;           // 存放 "猫体重（仅猫，单位 g）"

    // 内部方法
    void recordEvent(const EventRecord& evt);
    void detectFoodEvent();
    void detectWaterEvent();
    void detectLitterEvent();

    // 辅助函数
    float medianOf(float* arr, int n);
    float maxOf(float* arr, int n);
    float minOf(float* arr, int n);
};