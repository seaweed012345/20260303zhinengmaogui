#pragma once
#include "lvgl.h"
#ifndef UI_MAIN_H
#define UI_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif
extern lv_obj_t* tileview;      // 声明tileview供其它文件访问

// UI初始化
void ui_init(void);

// 页面切换，0=总览 1=健康详细 2=设置
void ui_show_page(int idx);



// 各项数据刷新接口
void ui_update_time(const char* timestr);
void ui_update_env(float temp, float hum, int co2_ppm, int aqi);  // ★ 新增：CO2参数
void ui_update_health(int eat, int drink, int toilet, float weight);
void ui_update_fresh(int value);

void ui_update_level_labels(int eat, int drink, int toilet, int weight);
void ui_update_micro_pressure(int val);
void ui_update_filter_life(int val);

void ui_update_bar_health_percent(int value);
void ui_update_bar_fresh_percent(int value);
void ui_update_health_bar(int value);

// 进入/退出延申页（0-6）
void ui_show_mainpage(void);    // 主界面显示函数声明
void ui_show_extpage(int idx);
void ui_hide_extpage(int idx);

#ifdef __cplusplus
}
#endif

#endif // UI_MAIN_H