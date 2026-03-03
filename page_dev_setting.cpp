#include <cstdio>
#include "page_dev_setting.h"
#include "ext_page_util.h"
#include "my_font_25.h"
#include "lvgl.h"
#include "ui_main.h"
#include <Preferences.h>

// 外部接口（主程序中需要实现或已实现）
extern void setTimezoneOffsetHours(int tzHours); // 在 main.ino 中已有实现
extern void setAutoSleepMode(int mode);          // 请在主程序中实现：mode 对应下拉索引的策略

static const char *timezone_opts = "北京(UTC+8)\n日本(UTC+9)\n美国(UTC-5)\n英国(UTC+0)\n法国(UTC+1)";
static const char *autosleep_opts = "1小时\n时段(23-6时)\n从不";
static int timezone_sel = 0;
static int autosleep_sel = 0;
static lv_obj_t *dropdown_timezone = NULL;
static lv_obj_t *dropdown_autosleep = NULL;

static const char* PREF_NS_SYS = "sys";
static const char* PREF_KEY_TZ_OFFSET = "tz_offset";
static const char* PREF_KEY_AUTOSLEEP = "autosleep_mode";

// 动态设置下拉菜单字体和单项高度，支持滑动
static void fix_dropdown_list_font(lv_obj_t *dropdown, int item_height) {
    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    if(list) {
        // 提升list到最前面，确保覆盖dropdown本体
        lv_obj_move_foreground(list);

        lv_obj_set_style_text_font(list, &my_font_25, 0);
        uint32_t cnt = lv_obj_get_child_cnt(list);
        for(uint32_t i=0; i<cnt; ++i) {
            lv_obj_t *btn = lv_obj_get_child(list, i);
            lv_obj_set_style_text_font(btn, &my_font_25, 0);
            lv_obj_set_height(btn, item_height);
            lv_obj_set_style_pad_ver(btn, 24, 0);
        }
    }
}

static void show_toast(const char* msg, int ms = 1000) {
    lv_obj_t* toast = lv_label_create(lv_scr_act());
    lv_label_set_text(toast, msg);
    lv_obj_set_style_text_font(toast, &my_font_25, 0);
    lv_obj_align(toast, LV_ALIGN_CENTER, 0, 0);
    lv_timer_t* t = lv_timer_create([](lv_timer_t* t){
        lv_obj_del((lv_obj_t*)t->user_data);
        lv_timer_del(t);
    }, ms, toast);
    t->user_data = toast;
}

// 时区下拉：选中即生效（直接持久化并调用 setTimezoneOffsetHours）
static void timezone_dropdown_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dropdown = lv_event_get_target(e);
    if(!dropdown) return;

    if(code == LV_EVENT_READY) {
        // 当打开列表完成时，修正样式以适配大字号
        lv_obj_set_height(dropdown, 0);
        fix_dropdown_list_font(dropdown, 176);
    }

    if(code == LV_EVENT_VALUE_CHANGED) {
        // 立即生效：获取选中项索引并映射到偏移小时
        timezone_sel = lv_dropdown_get_selected(dropdown);
        int tz_offsets_map[] = {8, 9, -5, 0, 1}; // 对应 timezone_opts 的偏移小时
        int tz = 8;
        if (timezone_sel >= 0 && timezone_sel < (int)(sizeof(tz_offsets_map)/sizeof(tz_offsets_map[0])))
            tz = tz_offsets_map[timezone_sel];

        // 持久化到 Preferences
        Preferences pref;
        pref.begin(PREF_NS_SYS, false);
        pref.putInt(PREF_KEY_TZ_OFFSET, tz);
        pref.end();

        // 立即调用主程序接口应用时区（主程序会更新 UI 时间）
        setTimezoneOffsetHours(tz);

        // 恢复下拉高度和显示 toast
        lv_obj_set_height(dropdown, 44);
        show_toast("时区已应用", 800);
    }

    if(code == LV_EVENT_CANCEL) {
        // 恢复高度（如果用户取消）
        lv_obj_set_height(dropdown, 44);
    }
}

// 自动息屏下拉：选中即生效（直接持久化并调用 setAutoSleepMode）
static void autosleep_dropdown_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dropdown = lv_event_get_target(e);
    if(!dropdown) return;

    if(code == LV_EVENT_READY) {
        fix_dropdown_list_font(dropdown, 176);
    }

    if(code == LV_EVENT_VALUE_CHANGED) {
        autosleep_sel = lv_dropdown_get_selected(dropdown);

        // 持久化 autosleep 模式 (直接使用下拉索引)
        Preferences pref;
        pref.begin(PREF_NS_SYS, false);
        pref.putInt(PREF_KEY_AUTOSLEEP, autosleep_sel);
        pref.end();

        // 立即调用主程序实现自动息屏策略
        setAutoSleepMode(autosleep_sel);

        // 显示提示
        show_toast("自动息屏已应用", 800);
    }
}

static void create_title_bar(lv_obj_t* parent, const char* title) {
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 420, 54);
    lv_obj_set_pos(bar, 30, 10);
    lv_obj_set_style_radius(bar, 16, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_60, 0);
    lv_obj_set_style_border_width(bar, 2, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x222222), 0);

    lv_obj_t* label = lv_label_create(bar);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &my_font_25, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x1E90FF), 0);
    lv_obj_center(label);
}

void create_setting_options(lv_obj_t *parent) {
    // ---时区设置文字描述---
    lv_obj_t *label_tz = lv_label_create(parent);
    lv_label_set_text(label_tz, "时区设置:");
    lv_obj_set_style_text_font(label_tz, &my_font_25, 0);
    lv_obj_set_style_text_color(label_tz, lv_color_hex(0x222222), 0);
    lv_obj_set_pos(label_tz, 60, 120);

    // ---时区下拉菜单---
    dropdown_timezone = lv_dropdown_create(parent);
    lv_dropdown_set_options_static(dropdown_timezone, timezone_opts);
    lv_obj_set_size(dropdown_timezone, 220, 44);
    lv_obj_set_pos(dropdown_timezone, 180, 110);
    lv_obj_set_style_text_font(dropdown_timezone, &my_font_25, 0);
    lv_obj_set_style_text_color(dropdown_timezone, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(dropdown_timezone, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dropdown_timezone, 0, 0);

    // 初始化选择项：读取 Preferences 中的 tz_offset 并映射到索引（若存在）
    Preferences pref;
    pref.begin(PREF_NS_SYS, true);
    int savedTz = pref.getInt(PREF_KEY_TZ_OFFSET, 8);
    pref.end();
    int tz_map_vals[] = {8,9,-5,0,1};
    int selIndex = 0;
    for (int i = 0; i < (int)(sizeof(tz_map_vals)/sizeof(tz_map_vals[0])); ++i) {
        if (tz_map_vals[i] == savedTz) { selIndex = i; break; }
    }
    timezone_sel = selIndex;
    lv_dropdown_set_selected(dropdown_timezone, timezone_sel);
    lv_obj_add_event_cb(dropdown_timezone, timezone_dropdown_event_cb, LV_EVENT_ALL, NULL);

    // ---自动息屏描述---
    lv_obj_t *label_sleep = lv_label_create(parent);
    lv_label_set_text(label_sleep, "自动息屏:");
    lv_obj_set_style_text_font(label_sleep, &my_font_25, 0);
    lv_obj_set_style_text_color(label_sleep, lv_color_hex(0x222222), 0);
    lv_obj_set_pos(label_sleep, 60, 170);

    // ---自动息屏下拉---
    dropdown_autosleep = lv_dropdown_create(parent);
    lv_dropdown_set_options_static(dropdown_autosleep, autosleep_opts);
    lv_obj_set_size(dropdown_autosleep, 220, 44);
    lv_obj_set_pos(dropdown_autosleep, 180, 160);
    lv_obj_set_style_text_font(dropdown_autosleep, &my_font_25, 0);
    lv_obj_set_style_text_color(dropdown_autosleep, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(dropdown_autosleep, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dropdown_autosleep, 0, 0);

    // 初始化 autosleep 选择项（读取 Preferences）
    pref.begin(PREF_NS_SYS, true);
    int savedAuto = pref.getInt(PREF_KEY_AUTOSLEEP, 0);
    pref.end();
    autosleep_sel = savedAuto;
    lv_dropdown_set_selected(dropdown_autosleep, autosleep_sel);
    lv_obj_add_event_cb(dropdown_autosleep, autosleep_dropdown_event_cb, LV_EVENT_ALL, NULL);
}

void page_dev_setting_create(lv_obj_t *parent) {
    create_title_bar(parent, "设备设置");
    create_setting_options(parent);
    create_ext_btn(parent, 300, 230, 120, 48, "返回", back_ext_cb, (void*)4);
}