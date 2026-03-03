#ifndef LVGL_CONFIG_H
#define LVGL_CONFIG_H

#include "Config.h"
#include "PinDefinitions.h"

// LVGL配置
#define LVGL_TICK_PERIOD_MS 5
#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 10) // 使用部分缓冲

// 颜色格式配置
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1

// 内存配置
#define LV_MEM_SIZE (32 * 1024) // 32KB for LVGL
#define LV_MEM_CUSTOM 1

// 功能启用
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_USE_THEME_MATERIAL 1
#define LV_USE_FONT_MONTSERRAT_16 1
#define LV_USE_FONT_MONTSERRAT_24 1

#endif // LVGL_CONFIG_H