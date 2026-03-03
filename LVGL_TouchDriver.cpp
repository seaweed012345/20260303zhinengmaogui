#include "lvgl.h"
#include "TouchDriver.h"

extern TouchDriver touchDriver;

void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    if (touchDriver.readTouch(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void LVGL_TouchInit()
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);
}