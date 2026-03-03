#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ext_pages_init(void);
void ui_show_extpage(int idx);
extern lv_obj_t* page_ext[7];

void food_bowl_tare_cb(lv_event_t *e);
void freshair_reset_cb(lv_event_t *e);
void back_ext_cb(lv_event_t* e);

#ifdef __cplusplus
}
#endif