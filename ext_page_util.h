
#ifndef EXT_PAGE_UTIL_H
#define EXT_PAGE_UTIL_H
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* create_ext_btn(lv_obj_t* parent, int x, int y, int w, int h,
                         const char* txt, lv_event_cb_t cb, void* user_data);

void back_ext_cb(lv_event_t* e);

#ifdef __cplusplus
}
#endif
#endif