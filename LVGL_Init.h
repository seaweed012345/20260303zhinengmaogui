#ifndef LVGL_INIT_H
#define LVGL_INIT_H
#include <lvgl.h>

#include <Arduino.h>

// LVGL配置
#define LVGL_TICK_PERIOD_MS 5
#define LVGL_BUFFER_SIZE (320 * 480 / 10) // 使用部分缓冲

bool LVGL_Init();



#endif // LVGL_INIT_H