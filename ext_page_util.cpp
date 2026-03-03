#include <cstdio>
#include "ext_page_util.h"
#include "my_font_25.h"
#include "lvgl.h"
#include "ui_main.h"      

// 若需要，声明外部变量和函数
extern lv_obj_t* page_ext[];
#define EXT_PAGE_NUM 7

lv_obj_t* create_ext_btn(lv_obj_t* parent, int x, int y, int w, int h,
                         const char* txt, lv_event_cb_t cb, void* user_data)
{
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2E2E2E), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_60, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x1E90FF), 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, txt);
    lv_obj_set_style_text_font(label, &my_font_25, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);
    return btn;
}

void back_ext_cb(lv_event_t* e) {
    for (int i = 0; i < EXT_PAGE_NUM; i++) {
        if (page_ext[i]) lv_obj_add_flag(page_ext[i], LV_OBJ_FLAG_HIDDEN);
    }
    ui_show_mainpage();
}