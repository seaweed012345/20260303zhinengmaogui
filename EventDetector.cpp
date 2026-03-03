#include "EventDetector.h"
#include <Arduino.h>
#include <algorithm>
#include <math.h>

// 判定参数（避免与头文件宏冲突）
static const int COMPLETE_SAMPLES = 10;
static const unsigned long COMPLETE_WINDOW_MS = 30UL * 1000UL;      // 30 秒窗口内采集 COMPLETE_SAMPLES 次
static const float COMPLETE_TOLERANCE_G = 1.5f;                     // 完成判定时 10 次样本 max-min <= 1.5g
static const unsigned long MIN_PERSIST_MS = 2000UL;                 // 浮动持续至少 2s 才进入收样期（防短触发）
static const unsigned long MIN_EVENT_DURATION_MS = 2000UL;         // 首次浮动到最后一次浮动 >= 2s 才算有效事件
static const unsigned long MAX_EVENT_DURATION_MS = 5UL * 60UL * 1000UL; // 浮动过程必须小于等于 5 分钟，否则判为无效
static const float FLUCT_AMPLITUDE_MIN_G = 1.0f;                    // 浮动幅度至少 1g
static const int FLUCT_MIN_COUNT = 3;                               // 浮动样本至少 3 次
static const float STABLE_NOISE_G = 0.5f;                           // IDLE 时认为“稳定”的微小波动阈值（用于更新 lastStable）

EventDetector::EventDetector(WeightManager* weightMgr)
    : wm(weightMgr), eventCount(0),
      lastFoodWeight(0), lastWaterWeight(0),
      baselineLitterWeight(0), lastLitterWeight(0), baselineBeforeEntry(0),
      stableCount(0), candidateStartMs(0), candidatePeak(0),
      exitConfirmCount(0), catEnterTime(0),
      catWeightMedian(0),
      newFoodEvent(false), lastFoodDelta(0), lastFoodTotal(0),
      newDrinkEvent(false), lastDrinkDelta(0), lastDrinkTotal(0),
      newToiletEvent(false), lastToiletDelta(0), lastToiletTotal(0),
      newWeightEvent(false), lastWeightValue(0)
{
    litterState = LITTER_IDLE;
    for (int i = 0; i < STABLE_SAMPLES; ++i) stableSamples[i] = 0.0f;
}

// 主更新入口
void EventDetector::update() {
    detectFoodEvent();
    detectWaterEvent();
    detectLitterEvent();
}

// ---- 辅助函数 ----
float EventDetector::medianOf(float* arr, int n) {
    if (n <= 0) return 0.0f;
    float tmp[16];
    int m = (n < 16) ? n : 16;
    for (int i = 0; i < m; ++i) tmp[i] = arr[i];
    std::sort(tmp, tmp + m);
    if (m % 2 == 1) return tmp[m/2];
    return (tmp[m/2 - 1] + tmp[m/2]) / 2.0f;
}
float EventDetector::maxOf(float* arr, int n) {
    if (n <= 0) return 0.0f;
    float m = arr[0];
    for (int i = 1; i < n; ++i) if (arr[i] > m) m = arr[i];
    return m;
}
float EventDetector::minOf(float* arr, int n) {
    if (n <= 0) return 0.0f;
    float m = arr[0];
    for (int i = 1; i < n; ++i) if (arr[i] < m) m = arr[i];
    return m;
}

// ================= 食物事件：IDLE -> CANDIDATE -> IN_PROGRESS -> CONFIRM =================
// 依据：
// 1) 浮动开始前的 lastStable 作为 preFluctWeight（进食前重量）
// 2) 浮动必须持续 >= MIN_PERSIST_MS 才进入 IN_PROGRESS
// 3) 浮动必须至少有 FLUCT_MIN_COUNT 次、幅度 >= FLUCT_AMPLITUDE_MIN_G
// 4) 浮动的 (lastFluct - firstFluct) 必须 >= MIN_EVENT_DURATION_MS 且 <= MAX_EVENT_DURATION_MS
// 5) 确认完成使用 COMPLETE_SAMPLES 个样本（30s 窗口）且 max-min <= COMPLETE_TOLERANCE_G，postStable 用样本中位数
void EventDetector::detectFoodEvent() {
    float cur = wm->getNetWeight(0);

    enum FState { F_IDLE = 0, F_CANDIDATE = 1, F_IN_PROGRESS = 2 };
    static FState state = F_IDLE;

    static unsigned long candidateStart = 0;
    static unsigned long firstFluctMs = 0;
    static unsigned long lastFluctMs = 0;
    static int fluctSampleCount = 0;
    static float fluctMax = 0.0f;
    static float fluctMin = 0.0f;
    static float preFluctWeight = 0.0f; // 浮动前的稳定值
    static float samples[COMPLETE_SAMPLES];
    static int sampleCount = 0;
    static float lastStable = 0.0f;    // IDLE 下的稳定参考值

    if (lastStable == 0.0f) lastStable = cur;

    #if LITTER_DEBUG_VERBOSE
    Serial.printf("[EventDetector] FOOD sample cur=%.2f state=%d lastStable=%.2f\n", cur, (int)state, lastStable);
    #endif

    if (state == F_IDLE) {
        if (fabs(cur - lastStable) <= STABLE_NOISE_G) {
            // 小波动：更新 lastStable（平滑）
            lastStable = lastStable * (1.0f - BASELINE_ALPHA) + cur * BASELINE_ALPHA;
        } else {
            // 大波动：可能进入候选
            if (fabs(cur - lastStable) >= FOOD_WATER_THRESHOLD_G) {
                state = F_CANDIDATE;
                candidateStart = millis();
                firstFluctMs = candidateStart;
                lastFluctMs = candidateStart;
                fluctSampleCount = 1;
                fluctMax = cur;
                fluctMin = cur;
                preFluctWeight = lastStable; // 浮动前的稳定值
                sampleCount = 0;
                samples[sampleCount++] = cur;
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] FOOD -> CANDIDATE preFluct=%.2f cur=%.2f\n", preFluctWeight, cur);
                #endif
            } else {
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] FOOD small deviation ignored diff=%.2f\n", fabs(cur - lastStable));
                #endif
            }
        }
    }
    else if (state == F_CANDIDATE) {
        unsigned long now = millis();
        // 若回落到接近 preFluctWeight，则取消候选
        if (fabs(cur - preFluctWeight) < FOOD_WATER_THRESHOLD_G) {
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("[EventDetector] FOOD candidate rebound -> cancel cur=%.2f pre=%.2f\n", cur, preFluctWeight);
            #endif
            state = F_IDLE;
            sampleCount = 0;
            // 更新 lastStable
            lastStable = lastStable * (1.0f - BASELINE_ALPHA) + cur * BASELINE_ALPHA;
        } else {
            // 仍在浮动，更新浮动统计
            if (cur > fluctMax) fluctMax = cur;
            if (cur < fluctMin) fluctMin = cur;
            fluctSampleCount++;
            lastFluctMs = now;
            if (sampleCount < COMPLETE_SAMPLES) samples[sampleCount++] = cur;
            else {
                for (int i = 1; i < COMPLETE_SAMPLES; ++i) samples[i-1] = samples[i];
                samples[COMPLETE_SAMPLES-1] = cur;
            }

            // 若浮动持续 MIN_PERSIST_MS，则考虑进入 IN_PROGRESS，但需满足最小次数与幅度
            if ((now - firstFluctMs) >= MIN_PERSIST_MS) {
                float amplitude = fluctMax - fluctMin;
                if (fluctSampleCount >= FLUCT_MIN_COUNT && amplitude >= FLUCT_AMPLITUDE_MIN_G) {
                    state = F_IN_PROGRESS;
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] FOOD CANDIDATE -> IN_PROGRESS persisted %ums, count=%d amp=%.2f\n",
                                  (unsigned)(now - firstFluctMs), fluctSampleCount, amplitude);
                    #endif
                } else {
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] FOOD candidate persisted but weak (count=%d amp=%.2f) wait\n",
                                  fluctSampleCount, amplitude);
                    #endif
                }
            }
            // 如果超过 MAX_EVENT_DURATION_MS（5分钟）还没满足条件，取消
            if ((now - firstFluctMs) > MAX_EVENT_DURATION_MS) {
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] FOOD candidate exceeded max duration %ums -> cancel\n", (unsigned)(now - firstFluctMs));
                #endif
                state = F_IDLE;
                sampleCount = 0;
                fluctSampleCount = 0;
                fluctMax = fluctMin = 0.0f;
                lastStable = cur;
            }
        }
    }
    else if (state == F_IN_PROGRESS) {
        unsigned long now = millis();
        // 继续收样（滑动窗口）
        if (sampleCount < COMPLETE_SAMPLES) samples[sampleCount++] = cur;
        else {
            for (int i = 1; i < COMPLETE_SAMPLES; ++i) samples[i-1] = samples[i];
            samples[COMPLETE_SAMPLES-1] = cur;
        }
        // 如果当前仍为浮动，更新浮动统计与 lastFluctMs
        if (fabs(cur - preFluctWeight) >= FOOD_WATER_THRESHOLD_G) {
            if (cur > fluctMax) fluctMax = cur;
            if (cur < fluctMin) fluctMin = cur;
            fluctSampleCount++;
            lastFluctMs = now;
        }
        float mx = maxOf(samples, sampleCount);
        float mn = minOf(samples, sampleCount);

        if ((now - firstFluctMs) <= COMPLETE_WINDOW_MS) {
            if (sampleCount >= COMPLETE_SAMPLES && (mx - mn) <= COMPLETE_TOLERANCE_G) {
                unsigned long fluctDuration = lastFluctMs - firstFluctMs;
                float amplitude = fluctMax - fluctMin;
                // 检查所有约束：持续时间区间、最少浮动次数、幅度
                if (fluctDuration < MIN_EVENT_DURATION_MS || fluctSampleCount < FLUCT_MIN_COUNT || amplitude < FLUCT_AMPLITUDE_MIN_G) {
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] FOOD stable but insufficient dur=%ums count=%d amp=%.2f -> ignore\n",
                                  (unsigned)fluctDuration, fluctSampleCount, amplitude);
                    #endif
                    lastStable = medianOf(samples, sampleCount);
                    state = F_IDLE;
                    sampleCount = 0;
                    fluctSampleCount = 0;
                    fluctMax = fluctMin = 0.0f;
                } else if (fluctDuration > MAX_EVENT_DURATION_MS) {
                    // 若波动持续超过最大允许时长（5 分钟），取消以避免一直等待下一次浮动
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] FOOD fluct duration > max (%ums) -> cancel\n", (unsigned)fluctDuration);
                    #endif
                    state = F_IDLE;
                    sampleCount = 0;
                    fluctSampleCount = 0;
                    fluctMax = fluctMin = 0.0f;
                    lastStable = cur;
                } else {
                    // 确认事件：preFluctWeight - postStable (median)
                    float postStable = medianOf(samples, sampleCount);
                    float delta = preFluctWeight - postStable;
                    if (delta < 0.0f) delta = 0.0f;
                    int deltaInt = (int)(delta + 0.5f);

                    // 新增：上限/下限检查（食物）
                    if (delta >= FOOD_WATER_THRESHOLD_G && delta < FOOD_MAX_G && deltaInt > 0) {
                        unsigned long tnow = now;
                        EventRecord evt = {EVENT_FEED, 0, tnow, 0, preFluctWeight, postStable, delta, true};
                        recordEvent(evt);
                        newFoodEvent = true;
                        lastFoodDelta = deltaInt;
                        lastFoodTotal = (int)postStable;
                        #if LITTER_DEBUG_VERBOSE
                        Serial.printf("[EventDetector] FOOD CONFIRMED pre=%.2f post=%.2f delta=%.2f dur=%ums samples=%d amp=%.2f\n",
                                      preFluctWeight, postStable, delta, (unsigned)fluctDuration, fluctSampleCount, amplitude);
                        #endif
                    } else {
                        #if LITTER_DEBUG_VERBOSE
                        Serial.printf("[EventDetector] FOOD ignored by bounds or delta~0: delta=%.2f (allowed [%.2f, %.2f))\n",
                                      delta, FOOD_WATER_THRESHOLD_G, FOOD_MAX_G);
                        #endif
                    }
                    // 重置并更新 lastStable
                    lastStable = medianOf(samples, sampleCount);
                    state = F_IDLE;
                    sampleCount = 0;
                    fluctSampleCount = 0;
                    fluctMax = fluctMin = 0.0f;
                }
            } // else 继续收样
        } else {
            // 超过 COMPLETE_WINDOW_MS 未完成稳定判定 -> 取消
            #if LITTER_DEBUG_VERBOSE
            Serial.println("[EventDetector] FOOD IN_PROGRESS COMPLETE_WINDOW timeout -> cancel");
            #endif
            state = F_IDLE;
            sampleCount = 0;
            fluctSampleCount = 0;
            fluctMax = fluctMin = 0.0f;
            lastStable = cur;
        }

        // 任何时刻若浮动总时长超过 MAX_EVENT_DURATION_MS，也应取消避免无限等待
        if ((now - firstFluctMs) > MAX_EVENT_DURATION_MS) {
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("[EventDetector] FOOD overall fluct exceeded MAX duration %ums -> cancel\n", (unsigned)(now - firstFluctMs));
            #endif
            state = F_IDLE;
            sampleCount = 0;
            fluctSampleCount = 0;
            fluctMax = fluctMin = 0.0f;
            lastStable = cur;
        }
    }

    lastFoodWeight = cur;
}

// ================= 饮水事件（与食物同逻辑，参数同样生效） =================
void EventDetector::detectWaterEvent() {
    float cur = wm->getNetWeight(1);

    enum WState { W_IDLE = 0, W_CANDIDATE = 1, W_IN_PROGRESS = 2 };
    static WState state = W_IDLE;

    static unsigned long candidateStart = 0;
    static unsigned long firstFluctMs = 0;
    static unsigned long lastFluctMs = 0;
    static int fluctSampleCount = 0;
    static float fluctMax = 0.0f;
    static float fluctMin = 0.0f;
    static float preFluctWeight = 0.0f;
    static float samples[COMPLETE_SAMPLES];
    static int sampleCount = 0;
    static float lastStable = 0.0f;

    if (lastStable == 0.0f) lastStable = cur;

    #if LITTER_DEBUG_VERBOSE
    Serial.printf("[EventDetector] WATER sample cur=%.2f state=%d lastStable=%.2f\n", cur, (int)state, lastStable);
    #endif

    if (state == W_IDLE) {
        if (fabs(cur - lastStable) <= STABLE_NOISE_G) {
            lastStable = lastStable * (1.0f - BASELINE_ALPHA) + cur * BASELINE_ALPHA;
        } else {
            if (fabs(cur - lastStable) >= FOOD_WATER_THRESHOLD_G) {
                state = W_CANDIDATE;
                candidateStart = millis();
                firstFluctMs = candidateStart;
                lastFluctMs = candidateStart;
                fluctSampleCount = 1;
                fluctMax = cur;
                fluctMin = cur;
                preFluctWeight = lastStable;
                sampleCount = 0;
                samples[sampleCount++] = cur;
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] WATER -> CANDIDATE preFluct=%.2f cur=%.2f\n", preFluctWeight, cur);
                #endif
            } else {
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] WATER small dev ignored diff=%.2f\n", fabs(cur - lastStable));
                #endif
            }
        }
    }
    else if (state == W_CANDIDATE) {
        unsigned long now = millis();
        if (fabs(cur - preFluctWeight) < FOOD_WATER_THRESHOLD_G) {
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("[EventDetector] WATER candidate rebound -> cancel cur=%.2f pre=%.2f\n", cur, preFluctWeight);
            #endif
            state = W_IDLE;
            sampleCount = 0;
            lastStable = lastStable * (1.0f - BASELINE_ALPHA) + cur * BASELINE_ALPHA;
        } else {
            if (cur > fluctMax) fluctMax = cur;
            if (cur < fluctMin) fluctMin = cur;
            fluctSampleCount++;
            lastFluctMs = now;
            if (sampleCount < COMPLETE_SAMPLES) samples[sampleCount++] = cur;
            else {
                for (int i = 1; i < COMPLETE_SAMPLES; ++i) samples[i-1] = samples[i];
                samples[COMPLETE_SAMPLES-1] = cur;
            }
            if ((now - firstFluctMs) >= MIN_PERSIST_MS) {
                float amplitude = fluctMax - fluctMin;
                if (fluctSampleCount >= FLUCT_MIN_COUNT && amplitude >= FLUCT_AMPLITUDE_MIN_G) {
                    state = W_IN_PROGRESS;
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] WATER CANDIDATE -> IN_PROGRESS persisted %ums count=%d amp=%.2f\n",
                                  (unsigned)(now - firstFluctMs), fluctSampleCount, amplitude);
                    #endif
                } else {
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] WATER persisted but weak (count=%d amp=%.2f)\n", fluctSampleCount, amplitude);
                    #endif
                }
            }
            if ((now - firstFluctMs) > MAX_EVENT_DURATION_MS) {
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] WATER candidate exceeded max duration %ums -> cancel\n", (unsigned)(now - firstFluctMs));
                #endif
                state = W_IDLE;
                sampleCount = 0;
                fluctSampleCount = 0;
                fluctMax = fluctMin = 0.0f;
                lastStable = cur;
            }
        }
    }
    else if (state == W_IN_PROGRESS) {
        unsigned long now = millis();
        if (sampleCount < COMPLETE_SAMPLES) samples[sampleCount++] = cur;
        else {
            for (int i = 1; i < COMPLETE_SAMPLES; ++i) samples[i-1] = samples[i];
            samples[COMPLETE_SAMPLES-1] = cur;
        }
        if (fabs(cur - preFluctWeight) >= FOOD_WATER_THRESHOLD_G) {
            if (cur > fluctMax) fluctMax = cur;
            if (cur < fluctMin) fluctMin = cur;
            fluctSampleCount++;
            lastFluctMs = now;
        }
        float mx = maxOf(samples, sampleCount);
        float mn = minOf(samples, sampleCount);

        if ((now - firstFluctMs) <= COMPLETE_WINDOW_MS) {
            if (sampleCount >= COMPLETE_SAMPLES && (mx - mn) <= COMPLETE_TOLERANCE_G) {
                unsigned long fluctDuration = lastFluctMs - firstFluctMs;
                float amplitude = fluctMax - fluctMin;
                if (fluctDuration < MIN_EVENT_DURATION_MS || fluctSampleCount < FLUCT_MIN_COUNT || amplitude < FLUCT_AMPLITUDE_MIN_G) {
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] WATER stable but insufficient dur=%ums count=%d amp=%.2f -> ignore\n",
                                  (unsigned)fluctDuration, fluctSampleCount, amplitude);
                    #endif
                    lastStable = medianOf(samples, sampleCount);
                    state = W_IDLE;
                    sampleCount = 0;
                    fluctSampleCount = 0;
                    fluctMax = fluctMin = 0.0f;
                } else if (fluctDuration > MAX_EVENT_DURATION_MS) {
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] WATER fluct duration > max (%ums) -> cancel\n", (unsigned)fluctDuration);
                    #endif
                    state = W_IDLE;
                    sampleCount = 0;
                    fluctSampleCount = 0;
                    fluctMax = fluctMin = 0.0f;
                    lastStable = cur;
                } else {
                    float postStable = medianOf(samples, sampleCount);
                    float delta = preFluctWeight - postStable;
                    if (delta < 0.0f) delta = 0.0f;
                    int deltaInt = (int)(delta + 0.5f);

                    // 新增：上限/下限检查（饮水）
                    if (delta >= FOOD_WATER_THRESHOLD_G && delta < WATER_MAX_G && deltaInt > 0) {
                        unsigned long tnow = now;
                        EventRecord evt = {EVENT_DRINK, 1, tnow, 0, preFluctWeight, postStable, delta, true};
                        recordEvent(evt);
                        newDrinkEvent = true;
                        lastDrinkDelta = deltaInt;
                        lastDrinkTotal = (int)postStable;
                        #if LITTER_DEBUG_VERBOSE
                        Serial.printf("[EventDetector] WATER CONFIRMED pre=%.2f post=%.2f delta=%.2f dur=%ums samples=%d amp=%.2f\n",
                                      preFluctWeight, postStable, delta, (unsigned)(lastFluctMs - firstFluctMs), fluctSampleCount, amplitude);
                        #endif
                    } else {
                        #if LITTER_DEBUG_VERBOSE
                        Serial.printf("[EventDetector] WATER ignored by bounds or delta~0: delta=%.2f (allowed [%.2f, %.2f))\n",
                                      delta, FOOD_WATER_THRESHOLD_G, WATER_MAX_G);
                        #endif
                    }

                    lastStable = medianOf(samples, sampleCount);
                    state = W_IDLE;
                    sampleCount = 0;
                    fluctSampleCount = 0;
                    fluctMax = fluctMin = 0.0f;
                }
            }
        } else {
            #if LITTER_DEBUG_VERBOSE
            Serial.println("[EventDetector] WATER IN_PROGRESS COMPLETE_WINDOW timeout -> cancel");
            #endif
            state = W_IDLE;
            sampleCount = 0;
            fluctSampleCount = 0;
            fluctMax = fluctMin = 0.0f;
            lastStable = cur;
        }

        if ((now - firstFluctMs) > MAX_EVENT_DURATION_MS) {
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("[EventDetector] WATER overall fluct exceeded MAX duration %ums -> cancel\n", (unsigned)(now - firstFluctMs));
            #endif
            state = W_IDLE;
            sampleCount = 0;
            fluctSampleCount = 0;
            fluctMax = fluctMin = 0.0f;
            lastStable = cur;
        }
    }

    lastWaterWeight = cur;
}

// --------------------------- 猫砂盆与其余函数沿用原实现 ---------------------------
void EventDetector::detectLitterEvent() {
    float cur = wm->getNetWeight(2);

    #if LITTER_DEBUG_VERBOSE
    Serial.printf("[EventDetector] LITTER sample: cur=%.2f g, baseline=%.2f g, state=%d, stableCount=%d, exitCnt=%d\n",
                  cur, baselineLitterWeight, (int)litterState, stableCount, exitConfirmCount);
    #endif

    static bool baselineInitialized = false;
    if (!baselineInitialized) {
        baselineLitterWeight = cur;
        baselineInitialized = true;
    }

    if (litterState == LITTER_IDLE) {
        if (cur < baselineLitterWeight + ENTER_THRESHOLD_G) {
            baselineLitterWeight = baselineLitterWeight * (1.0f - BASELINE_ALPHA) + cur * BASELINE_ALPHA;
        }
    }

    if (litterState == LITTER_IDLE) {
        if (cur > baselineLitterWeight + ENTER_THRESHOLD_G) {
            litterState = LITTER_IN_PROGRESS;
            candidateStartMs = millis();
            baselineBeforeEntry = baselineLitterWeight;
            stableCount = 0;
            candidatePeak = cur;
            stableSamples[0] = cur;
            stableCount = 1;
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("[EventDetector] enter candidate started: baselineBefore=%.2f cur=%.2f\n",
                          baselineBeforeEntry, cur);
            #endif
        }
    } else if (litterState == LITTER_IN_PROGRESS) {
        if (cur >= baselineBeforeEntry + ENTER_THRESHOLD_G) {
            if (stableCount < STABLE_SAMPLES) {
                stableSamples[stableCount++] = cur;
            } else {
                for (int i = 1; i < STABLE_SAMPLES; ++i) stableSamples[i-1] = stableSamples[i];
                stableSamples[STABLE_SAMPLES-1] = cur;
            }
            if (cur > candidatePeak) candidatePeak = cur;

            if (stableCount >= STABLE_SAMPLES) {
                float mx = maxOf(stableSamples, STABLE_SAMPLES);
                float mn = minOf(stableSamples, STABLE_SAMPLES);
                unsigned long now = millis();
                unsigned long dwellCandidate = now - candidateStartMs;
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] candidate samples: max=%.2f min=%.2f dwellCandidate=%ums\n", mx, mn, (unsigned)dwellCandidate);
                #endif
                if ((mx - mn) <= STABLE_TOLERANCE_G && dwellCandidate >= MIN_DWELL_MS) {
                    catWeightMedian = medianOf(stableSamples, STABLE_SAMPLES);
                    litterState = LITTER_CONFIRMED;
                    catEnterTime = now;
                    exitConfirmCount = 0;
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] CAT_ENTER confirmed. catWeightMedian=%.2f g, time=%u\n", catWeightMedian, (unsigned)catEnterTime);
                    #endif

                    float catAlone = catWeightMedian - baselineBeforeEntry;
                    if (catAlone < 0.0f) catAlone = 0.0f;
                    newWeightEvent = true;
                    lastWeightValue = catAlone;
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] WEIGHT event set: lastWeightValue=%.2f g (cat only)\n", lastWeightValue);
                    #endif

                    EventRecord evt = {EVENT_CAT_ENTER, 2, catEnterTime, 0, baselineBeforeEntry, catWeightMedian, catWeightMedian - baselineBeforeEntry, true};
                    recordEvent(evt);
                } else {
                    #if LITTER_DEBUG_VERBOSE
                    Serial.println("[EventDetector] IN_PROGRESS: not stable or dwell too short, continue collecting.");
                    #endif
                }
            }
        } else {
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("[EventDetector] enter candidate broken, cur=%.2f below threshold, reset.\n", cur);
            #endif
            litterState = LITTER_IDLE;
            stableCount = 0;
            candidatePeak = 0;
        }
    } else if (litterState == LITTER_CONFIRMED) {
        if (cur < baselineBeforeEntry + EXIT_THRESHOLD_G) {
            exitConfirmCount++;
            #if LITTER_DEBUG_VERBOSE
            Serial.printf("[EventDetector] exit condition met (%d/%d)\n", exitConfirmCount, EXIT_CONFIRM_SAMPLES);
            #endif
        } else {
            if (exitConfirmCount > 0) {
                #if LITTER_DEBUG_VERBOSE
                Serial.println("[EventDetector] exit condition broken, reset exitConfirmCount");
                #endif
            }
            exitConfirmCount = 0;
        }

        if (exitConfirmCount >= EXIT_CONFIRM_SAMPLES) {
            unsigned long now = millis();
            unsigned long dwell = now - catEnterTime;
            if (dwell < MIN_DWELL_MS) {
                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] exit samples but dwell too short: %ums < %ums. Ignoring for now.\n",
                              (unsigned)dwell, (unsigned)MIN_DWELL_MS);
                #endif
                exitConfirmCount = 0;
            } else {
                float exitSamples[STABLE_SAMPLES];
                int exitSampleCount = 0;
                exitSamples[exitSampleCount++] = cur;
                for (int i = 1; i < STABLE_SAMPLES; ++i) {
                    float v = wm->getNetWeight(2);
                    exitSamples[exitSampleCount++] = v;
                }

                float baselineAfter = medianOf(exitSamples, exitSampleCount);
                float delta = baselineAfter - baselineBeforeEntry;

                #if LITTER_DEBUG_VERBOSE
                Serial.printf("[EventDetector] exit confirmed. baselineBefore=%.2f baselineAfter=%.2f delta=%.2f\n",
                              baselineBeforeEntry, baselineAfter, delta);
                #endif

                if (delta >= EXIT_RECOVERY_MIN && delta <= EXIT_RECOVERY_MAX) {
                    // 新增：对排泄事件增加上限判断（仅在 delta > DEFECATE_THRESHOLD_G 且小于 DEFECATE_MAX_G 时视为正常排泄）
                    if (delta > DEFECATE_THRESHOLD_G && delta < DEFECATE_MAX_G) {
                        int deltaInt = (int)(delta + 0.5f);
                        EventRecord evt2 = {EVENT_DEFECATE, 2, catEnterTime, now, baselineBeforeEntry, baselineAfter, delta, true};
                        recordEvent(evt2);
                        newToiletEvent = true;
                        lastToiletDelta = deltaInt;
                        lastToiletTotal = (int)baselineAfter;
                        #if LITTER_DEBUG_VERBOSE
                        Serial.printf("[EventDetector] DEFECATE event: delta=%d g\n", deltaInt);
                        #endif
                    } else if (delta >= DEFECATE_MAX_G) {
                        // 过大变化，可能是异常（例如整盆被移动/有人清理/放入大量物品），标记为 ENTER_NO_DEFECATE 并记录 delta 以便排查
                        EventRecord evt2 = {EVENT_ENTER_NO_DEFECATE, 2, catEnterTime, now, baselineBeforeEntry, baselineAfter, delta, true};
                        recordEvent(evt2);
                        #if LITTER_DEBUG_VERBOSE
                        Serial.printf("[EventDetector] EXIT abnormal large delta (%.2f g) >= DEFECATE_MAX_G (%.2f) -> recorded as ENTER_NO_DEFECATE\n",
                                      delta, DEFECATE_MAX_G);
                        #endif
                    } else {
                        // delta <= DEFECATE_THRESHOLD_G -> 进入无排泄的离开事件
                        EventRecord evt2 = {EVENT_ENTER_NO_DEFECATE, 2, catEnterTime, now, baselineBeforeEntry, baselineAfter, 0, true};
                        recordEvent(evt2);
                        #if LITTER_DEBUG_VERBOSE
                        Serial.println("[EventDetector] ENTER_NO_DEFECATE event (delta too small)");
                        #endif
                    }
                } else {
                    #if LITTER_DEBUG_VERBOSE
                    Serial.printf("[EventDetector] EXIT recovery out of allowed range: delta=%.2f (allowed [%0.1f, %0.1f]) -> mark abnormal\n",
                                  delta, EXIT_RECOVERY_MIN, EXIT_RECOVERY_MAX);
                    #endif
                    EventRecord evt2 = {EVENT_ENTER_NO_DEFECATE, 2, catEnterTime, now, baselineBeforeEntry, baselineAfter, delta, true};
                    recordEvent(evt2);
                }

                litterState = LITTER_IDLE;
                stableCount = 0;
                exitConfirmCount = 0;
                candidatePeak = 0;
                baselineLitterWeight = baselineLitterWeight * (1.0f - BASELINE_ALPHA) + baselineAfter * BASELINE_ALPHA;
            }
        }
    }

    lastLitterWeight = cur;
}

void EventDetector::recordEvent(const EventRecord& evt) {
    if (eventCount < MAX_EVENT_HISTORY) {
        eventHistory[eventCount++] = evt;
    } else {
        for (int i = 1; i < MAX_EVENT_HISTORY; ++i) eventHistory[i-1] = eventHistory[i];
        eventHistory[MAX_EVENT_HISTORY-1] = evt;
    }
    #if LITTER_DEBUG_VERBOSE
    Serial.printf("[EventDetector] recordEvent: type=%d delta=%.2f\n", evt.type, evt.deltaWeight);
    #endif
}

bool EventDetector::getLastEvent(EventRecord& outEvent) {
    if (eventCount > 0) {
        outEvent = eventHistory[eventCount-1];
        return true;
    }
    return false;
}

void EventDetector::getTodayStats(int& feedCount, float& feedTotal,
                                  int& drinkCount, float& drinkTotal,
                                  int& defecateCount, float& defecateTotal) {
    feedCount = drinkCount = defecateCount = 0;
    feedTotal = drinkTotal = defecateTotal = 0.0f;
    for (int i = 0; i < eventCount; ++i) {
        if (eventHistory[i].type == EVENT_FEED) {
            feedCount++; feedTotal += eventHistory[i].deltaWeight;
        } else if (eventHistory[i].type == EVENT_DRINK) {
            drinkCount++; drinkTotal += eventHistory[i].deltaWeight;
        } else if (eventHistory[i].type == EVENT_DEFECATE) {
            defecateCount++; defecateTotal += eventHistory[i].deltaWeight;
        }
    }
}

bool EventDetector::hasNewFoodEvent() {
    if (newFoodEvent) { newFoodEvent = false; return true; }
    return false;
}
int EventDetector::getFoodDelta() { return lastFoodDelta; }
int EventDetector::getFoodTotal() { return lastFoodTotal; }

bool EventDetector::hasNewDrinkEvent() {
    if (newDrinkEvent) { newDrinkEvent = false; return true; }
    return false;
}
int EventDetector::getDrinkDelta() { return lastDrinkDelta; }
int EventDetector::getDrinkTotal() { return lastDrinkTotal; }

bool EventDetector::hasNewToiletEvent() {
    if (newToiletEvent) { newToiletEvent = false; return true; }
    return false;
}
int EventDetector::getToiletDelta() { return lastToiletDelta; }
int EventDetector::getToiletTotal() { return lastToiletTotal; }

bool EventDetector::hasNewWeightEvent() {
    if (newWeightEvent) { newWeightEvent = false; return true; }
    return false;
}
float EventDetector::getWeightValue() { return lastWeightValue; }

void EventDetector::resetTodayStats() {
    eventCount = 0;
}