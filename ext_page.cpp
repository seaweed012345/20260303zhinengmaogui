#include "ext_page.h"
#include "my_font_25.h"
#include "my_font_30.h"
#include "ui_main.h"
#include "page_dev_setting.h"
#include "ext_page_util.h"
#include "lvgl.h"
#include "my_qrcode_page.h"
#include <WiFi.h>
#include <Preferences.h>

#define EXT_PAGE_NUM 7

extern lv_obj_t* tileview;

const char* ext_titles[EXT_PAGE_NUM] = {
    "饭盆复位","水盆复位","新风复位","网络配置","设备设置","系统信息","恢复出厂"
};
lv_obj_t* page_ext[EXT_PAGE_NUM];

// Preferences keys for timezone
static const char* PREF_NS_SYS = "sys";
static const char* PREF_KEY_TZ_OFFSET = "tz_offset"; // int hours offset

// ====== 基本控件创建函数声明 ======
static void create_ext_title_bar(lv_obj_t* parent, const char* title);
static lv_obj_t* create_ext_text(lv_obj_t* parent, const char* text, int x, int y, int w);

// ====== 网络状态控件引用 ======
static lv_obj_t *net_status_label = NULL;
static lv_obj_t *net_ip_label = NULL;
static lv_obj_t *net_rssi_label = NULL;
static lv_obj_t *tz_label = NULL;
static lv_obj_t *tz_dd = NULL; // dropdown

// ====== 工具函数：将RSSI转为信号图标文本 ======
static const char* rssi_to_icon(int rssi) {
    if(rssi >= -50) return "▂▄▆█";    // 极好
    else if(rssi >= -60) return "▂▄▆";
    else if(rssi >= -70) return "▂▄";
    else if(rssi >= -80) return "▂";
    else return "-";
}

// ====== 网络状态刷新函数 ======
static void update_net_status_cb(lv_timer_t * timer) {
    (void)timer;
    if(!net_status_label || !net_ip_label || !net_rssi_label) return;

    if(WiFi.status() == WL_CONNECTED) {
        lv_label_set_text_fmt(net_status_label, "状态: 已连接");
        lv_label_set_text_fmt(net_ip_label, "IP: %s", WiFi.localIP().toString().c_str());
        lv_label_set_text_fmt(net_rssi_label, "RSSI: [%s]", rssi_to_icon(WiFi.RSSI()));
    } else {
        lv_label_set_text_fmt(net_status_label, "状态: 未连接");
        lv_label_set_text_fmt(net_ip_label, "IP: --");
        lv_label_set_text_fmt(net_rssi_label, "RSSI: [-]");
    }
}

// ====== 时间/时区下拉相关 ======
// 下拉项文本（简洁显示）和对应偏移数组（小时）
// 你可以按需扩充/本地化显示文本
static const char* tz_items = "UTC-12\nUTC-11\nUTC-10\nUTC-9\nUTC-8\nUTC-7\nUTC-6\nUTC-5\nUTC-4\nUTC-3\nUTC-2\nUTC-1\nUTC+0\nUTC+1\nUTC+2\nUTC+3\nUTC+4\nUTC+5\nUTC+6\nUTC+7\nUTC+8\nUTC+9\nUTC+10\nUTC+11\nUTC+12";
static const int tz_offsets[] = {
    -12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,8,9,10,11,12
};
static const int TZ_COUNT = sizeof(tz_offsets)/sizeof(tz_offsets[0]);

// 下拉变化时立即生效的回调（选中即保存并应用）
static void tz_dd_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return; // 只在选中变化时处理

    lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
    if (!dd) return;
    uint16_t sel = lv_dropdown_get_selected(dd); // index
    if (sel >= TZ_COUNT) return;
    int offset = tz_offsets[sel];

    // 持久化
    Preferences pref;
    pref.begin(PREF_NS_SYS, false);
    pref.putInt(PREF_KEY_TZ_OFFSET, offset);
    pref.end();

    // 调用主程序的接口，立即应用时区并刷新 UI 时间
    extern void setTimezoneOffsetHours(int tzHours);
    setTimezoneOffsetHours(offset);

    // 更新显示标签
    lv_label_set_text_fmt(tz_label, "时区: UTC%+d", offset);

    // 简短提示
    lv_obj_t* toast = lv_label_create(lv_scr_act());
    lv_label_set_text(toast, "时区已应用");
    lv_obj_align(toast, LV_ALIGN_CENTER, 0, 0);
    lv_timer_t* t = lv_timer_create([](lv_timer_t* t){
        lv_obj_del((lv_obj_t*)t->user_data);
        lv_timer_del(t);
    }, 1000, toast);
    t->user_data = toast;
}

// ====== 回调实现 ======
void food_bowl_tare_cb(lv_event_t *e) {
    // TODO: 实现饭盆复位
    back_ext_cb(e);
}
void water_bowl_tare_cb(lv_event_t *e) {
    // TODO: 实现水盆复位
    back_ext_cb(e);
}
void freshair_reset_cb(lv_event_t *e) {
    // TODO: 实现新风复位
    back_ext_cb(e);
}
void factory_reset_cb(lv_event_t *e) {
    // TODO: 实现出厂复位
    back_ext_cb(e);
}

// ====== 各页面创建函数 ======
static void page_bowl_reset_create(lv_obj_t* parent) {
    create_ext_title_bar(parent, "饭盆复位");
    create_ext_text(parent, "请正确放置饭盆到指定区域，并确定饭盆是空的，盆内不含食物。", 40, 80, 400);
    create_ext_btn(parent, 60, 230, 120, 48, "确认", food_bowl_tare_cb, NULL);
    create_ext_btn(parent, 300, 230, 120, 48, "返回", back_ext_cb, (void*)0);
}
static void page_water_reset_create(lv_obj_t *parent) {
    create_ext_title_bar(parent, "水盆复位");
    create_ext_text(parent, "请正确放置水盆到指定区域，并确定盆内没水（自动喂水设备请添至最低可运行水位）。", 40, 80, 400);
    create_ext_btn(parent, 60, 230, 120, 48, "确认", water_bowl_tare_cb, NULL);
    create_ext_btn(parent, 300, 230, 120, 48, "返回", back_ext_cb, (void*)1);
}
static void page_freshair_reset_create(lv_obj_t* parent) {
    create_ext_title_bar(parent, "新风复位");
    create_ext_text(parent, "请确认已更换所有滤网（初级过滤2个、高效HEPA、活性炭滤网），系统将重置计入滤网时效。", 40, 80, 400);
    create_ext_btn(parent, 60, 230, 120, 48, "确认", freshair_reset_cb, NULL);
    create_ext_btn(parent, 300, 230, 120, 48, "返回", back_ext_cb, (void*)2);
}

// ====== 网络配置页 ======
static void page_net_config_create(lv_obj_t *parent) {
    // 1. 创建顶部标题栏
    create_ext_title_bar(parent, "网络配置");

    // 2. 创建内容区（左二维码+右状态信息） (与之前保持一致)
    lv_obj_t* content_area = lv_obj_create(parent);
    lv_obj_remove_style_all(content_area);
    lv_obj_set_size(content_area, 427, 164);
    lv_obj_set_pos(content_area, 24, 74);
    lv_obj_set_style_bg_opa(content_area, LV_OPA_0, 0);

    // 二维码
    int x_qr_tip = 12;
    int y_qr_tip = 6;
    int x_qr = 35;
    int y_qr = 36;
    lv_obj_t* tip_label = lv_label_create(content_area);
    lv_label_set_text(tip_label, "请扫码配置网络");
    lv_obj_set_style_text_font(tip_label, &my_font_25, 0);
    lv_obj_set_style_text_color(tip_label, lv_color_hex(0x222222), 0);
    lv_obj_set_pos(tip_label, x_qr_tip, y_qr_tip);

    String mac = WiFi.macAddress();
    char mac_buf[32];
    strncpy(mac_buf, mac.c_str(), sizeof(mac_buf));
    mac_buf[sizeof(mac_buf)-1] = '\0';
    lv_obj_t* qrcode = lv_qrcode_create(content_area, 120, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
    lv_obj_set_pos(qrcode, x_qr, y_qr);
    lv_qrcode_update(qrcode, mac_buf, strlen(mac_buf));

    // 右侧状态
    int x_status = 235;
    int y_status_base = 6;
    int y_status_gap = 50;
    net_status_label = lv_label_create(content_area);
    lv_label_set_text(net_status_label, "状态: 未连接");
    lv_obj_set_style_text_font(net_status_label, &my_font_25, 0);
    lv_obj_set_style_text_color(net_status_label, lv_color_hex(0x222222), 0);
    lv_obj_set_pos(net_status_label, x_status, y_status_base);

    net_ip_label = lv_label_create(content_area);
    lv_label_set_text(net_ip_label, "IP: --");
    lv_obj_set_style_text_font(net_ip_label, &my_font_25, 0);
    lv_obj_set_style_text_color(net_ip_label, lv_color_hex(0x222222), 0);
    lv_obj_set_pos(net_ip_label, x_status, y_status_base + y_status_gap);

    net_rssi_label = lv_label_create(content_area);
    lv_label_set_text(net_rssi_label, "RSSI: [-]");
    lv_obj_set_style_text_font(net_rssi_label, &my_font_25, 0);
    lv_obj_set_style_text_color(net_rssi_label, lv_color_hex(0x222222), 0);
    lv_obj_set_pos(net_rssi_label, x_status, y_status_base + y_status_gap * 2);

    // ========== 新增：时区选择UI ==========
    // 时区标签
    tz_label = lv_label_create(content_area);
    // 显示当前存储的时区偏移
    Preferences pref;
    pref.begin(PREF_NS_SYS, true);
    int savedOffset = pref.getInt(PREF_KEY_TZ_OFFSET, 8);
    pref.end();
    lv_label_set_text_fmt(tz_label, "", savedOffset);
    lv_obj_set_style_text_font(tz_label, &my_font_25, 0);
    lv_obj_set_style_text_color(tz_label, lv_color_hex(0x222222), 0);
    lv_obj_set_pos(tz_label, x_status, y_status_base + y_status_gap * 3);

    // 下拉选择（使用 LVGL 下拉列表）
    tz_dd = lv_dropdown_create(content_area);
    lv_dropdown_set_options_static(tz_dd, tz_items);
    lv_obj_set_width(tz_dd, 140);
    lv_obj_set_pos(tz_dd, x_status, y_status_base + y_status_gap * 4);
    // 设置初始选中项为 savedOffset 对应索引
    int selIndex = 0;
    for (int i = 0; i < TZ_COUNT; ++i) {
        if (tz_offsets[i] == savedOffset) { selIndex = i; break; }
    }
    lv_dropdown_set_selected(tz_dd, selIndex);
    // 重要：直接在 value_changed 事件处理里生效（选中即保存）
    lv_obj_add_event_cb(tz_dd, tz_dd_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 4. 启动定时器定时刷新网络状态
    static lv_timer_t* net_status_timer = NULL;
    if (!net_status_timer) {
        net_status_timer = lv_timer_create(update_net_status_cb, 2000, NULL); // 2秒刷新一次
    }

    // 5. 返回按钮
    create_ext_btn(parent, 300, 230, 120, 48, "返回", back_ext_cb, (void*)3);
}

// 设备设置页直接调用 page_dev_setting_create
static void page_sysinfo_create(lv_obj_t* parent) {
    create_ext_title_bar(parent, "系统信息");
    create_ext_text(parent, "固件版本：V1.2.3\nMAC 地址：AA:BB:CC:DD:EE:FF\n运行时间：12天13小时", 60, 90, 360);
    create_ext_btn(parent, 300, 230, 120, 48, "返回", back_ext_cb, (void*)5);
}
static void page_factory_reset_create(lv_obj_t *parent) {
    create_ext_title_bar(parent, "恢复出厂");
    create_ext_text(parent, "此操作将清空所有设置并恢复出厂状态。请谨慎操作！", 40, 80, 400);
    create_ext_btn(parent, 60, 230, 120, 48, "确定", factory_reset_cb, NULL);
    create_ext_btn(parent, 300, 230, 120, 48, "返回", back_ext_cb, (void*)6);
}

// ====== 页面控件实现 ======
static void create_ext_title_bar(lv_obj_t* parent, const char* title)
{
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
    lv_obj_set_style_text_font(label, &my_font_30, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x1E90FF), 0);
    lv_obj_center(label);
}
static lv_obj_t* create_ext_text(lv_obj_t* parent, const char* text, int x, int y, int w)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &my_font_25, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x222222), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, w);
    lv_obj_set_pos(label, x, y);
    return label;
}

// ====== 入口函数 ======
void ext_pages_init(void)
{
    for(int i=0; i<EXT_PAGE_NUM; i++) {
        page_ext[i] = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(page_ext[i]);
        lv_obj_set_size(page_ext[i], 480, 320);
        lv_obj_set_pos(page_ext[i], 0, 0);
        lv_obj_set_style_bg_opa(page_ext[i], LV_OPA_0, 0); // 透明
        lv_obj_add_flag(page_ext[i], LV_OBJ_FLAG_HIDDEN);
    }
    page_bowl_reset_create(page_ext[0]);
    page_water_reset_create(page_ext[1]);
    page_freshair_reset_create(page_ext[2]);
    page_net_config_create(page_ext[3]);
    page_dev_setting_create(page_ext[4]); // 设备设置，调用独立实现
    page_sysinfo_create(page_ext[5]);
    page_factory_reset_create(page_ext[6]);
}