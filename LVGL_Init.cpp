#include "LVGL_Init.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "esp_heap_caps.h"

extern TFT_eSPI tft;

#define LVGL_BUFFER_LINES 480 // 你可尝试放大
#define LVGL_BUFFER_SIZE (320 * LVGL_BUFFER_LINES)

static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1 = nullptr;

static void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp_drv);
}

bool LVGL_Init() {
    Serial.begin(115200);
    Serial.printf("PSRAM size=%d, free=%d\n", ESP.getPsramSize(), ESP.getFreePsram());

    lv_init();

    size_t buf_bytes = LVGL_BUFFER_SIZE * sizeof(lv_color_t);
    buf1 = (lv_color_t*)heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
    Serial.printf("buf1=%p (%d bytes)\n", buf1, buf_bytes);

    if (!buf1) {
        Serial.println("PSRAM分配失败，使用SRAM小缓冲！");
        static lv_color_t fallback_buf[320 * 10];
        buf1 = fallback_buf;
        lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 320 * 10);
    } else {
        lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LVGL_BUFFER_SIZE);
    }

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    Serial.println("LVGL初始化完成");
    return true;
}