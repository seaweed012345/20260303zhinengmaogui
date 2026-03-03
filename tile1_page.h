#pragma once
#include "lvgl.h"

// ---------------------- 结构体定义 ----------------------
typedef struct {
    lv_obj_t *row_bg;         // 行底框（半透明背景）
    lv_obj_t *label_level;    // 左侧等级（优/良/差等，变动）
    lv_obj_t *label_item;     // 项目名（进食/饮水/排泄/体重）
    lv_obj_t *chart;          // 历史曲线图
    lv_chart_series_t *series;// 曲线数据序列
    lv_obj_t *label_value;    // 右侧今日累计数值
} tile1_row_t;

// ---------------------- 变量声明 ----------------------
#define TILE1_ROW_NUM 4
extern tile1_row_t tile1_rows[TILE1_ROW_NUM];
extern lv_obj_t *tile1_title_bg[3];
extern lv_obj_t *tile1_title_label[3];
extern lv_style_t style_tile1_item[4];

// ---------------------- 刷新函数声明 ----------------------
#ifdef __cplusplus
extern "C" {
#endif
void tile1_page_create(lv_obj_t* parent);
// 刷新左侧等级（优/良/差等）、颜色
void tile1_set_level(int row, const char *text, lv_color_t color);
// 刷新曲线（支持C++和C主函数调用）
void tile1_set_chart(int row, const int *data, int len);
// 刷新右侧数值
void tile1_set_value(int row, const char *text);
// 刷新项目名颜色
void tile1_set_item_color(int row, lv_color_t color);
// 新增: 刷新优良差汉字和颜色（0差1良2优），和ui_main.cpp联动
void tile1_set_level_chs(int row, int level);
#ifdef __cplusplus
}
#endif