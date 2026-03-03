#include <cstdio>
#include "ui_main.h"
#include "bg_main.h"
#include "my_font_35.h"
#include "my_font_30.h"
#include "my_font_25.h"
#include "tubiao_1.h"
#include "tubiao_2.h"
#include "tile1_page.h"
#include "ext_page.h"

#include "RelayController.h"
extern RelayController relayController;

// ========== 新增：MQTT联动 ==========
#include "MQTTManager.h"
extern MQTTManager mqttManager;

// ========== 新风模式全局标志 ==========
bool fresh_auto_mode = false;
bool fresh_manual_mode = false;

// ========== 继电器按钮状态 ==========
static bool main_light_on = false;
static bool ambient_light_on = false;

// ========== 继电器按钮对象 ==========
lv_obj_t* btn_mainlight = nullptr;
lv_obj_t* btn_ambientlight = nullptr;
lv_obj_t* btn_fresh_auto = nullptr;
lv_obj_t* btn_fresh_manual = nullptr;

lv_obj_t* tileview;

// ========== 前置声明 ==========
static lv_obj_t *create_btn(lv_obj_t *parent, int x, int y, int w, int h, const char *txt, lv_event_cb_t cb, void *ud);
void ui_show_extpage(int idx);

// ========== MQTT publishStatus 安全标志 ==========
extern volatile bool needPublishStatus;

// ========== 样式和对象 ==========
static lv_style_t style_35;
static lv_style_t style_30;
static lv_style_t style_25;
static lv_style_t style_25_level;
static lv_style_t style_btn, style_btn_pr, style_transp, style_tile_transp;
static lv_style_t style_bar_percent_14;

static lv_obj_t *tile[3];
static lv_obj_t *nav_dot[3];

// ★ 新增：5个标签对象（时间、温度、湿度、CO2、AQI）
static lv_obj_t *label_time, *label_temp, *label_hum, *label_co2, *label_aqi;
static lv_obj_t *bar_health, *bar_fresh;
static lv_obj_t *label_bar_health_percent, *label_bar_fresh_percent;

// ========== 百分数显示 ==========
static void set_bar_percent_label(lv_obj_t *label, int value, int x, int y)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", value);
    lv_label_set_text(label, buf);
    lv_obj_set_pos(label, x, y);
}

// ========== 微压传感器分值映射（-35到-9线性映射为0-50分，>=-9为50，<=-35为0） ==========
static int convert_micro_pressure_val(int raw_val) {
    if (raw_val >= -9) return 50;
    if (raw_val <= -35) return 0;
    float val = (raw_val + 35) * (50.0f / 26.0f);
    return (int)(val + 0.5f);
}

// ========== 主灯 ==========
static void on_btn_mainlight_event_cb(lv_event_t *e) {
    main_light_on = !main_light_on;
    relayController.setMainLight(main_light_on);
  
}

// ========== 氛围灯 ==========
static void on_btn_ambientlight_event_cb(lv_event_t *e) {
    ambient_light_on = !ambient_light_on;
    relayController.setAmbientLight(ambient_light_on);
  
}

// ========== 新风自动 ==========
static void on_btn_fresh_auto_event_cb(lv_event_t *e) {
    fresh_auto_mode = true;
    fresh_manual_mode = false;
  
}

// ========== 新风手动 ==========
static void on_btn_fresh_manual_event_cb(lv_event_t *e) {
    fresh_manual_mode = true;
    fresh_auto_mode = false;
   
}

// ========== 按钮 ==========
static lv_obj_t* create_btn(lv_obj_t* parent, int x, int y, int w, int h, const char* txt, lv_event_cb_t cb, void* ud) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &style_btn, LV_PART_MAIN);
    lv_obj_add_style(btn, &style_btn_pr, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, txt);
    lv_obj_center(lbl);
    if(cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ud);
    return btn;
}

// ========== 底部圆点 ==========
static void nav_dot_event_cb(lv_event_t *e) {
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_set_tile(tileview, tile[idx], LV_ANIM_ON);
}
static void update_nav_dot(int idx) {
    for(int i=0;i<3;i++)
        lv_obj_set_style_bg_color(nav_dot[i], i==idx?lv_color_hex(0xFF0000):lv_color_white(), 0);
}
static void tileview_event_cb(lv_event_t *e) {
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *act_tile = lv_tileview_get_tile_act(tv);
    int idx = 0;
    for(int i=0;i<3;i++) if(act_tile == tile[i]) idx = i;
    update_nav_dot(idx);
}

// ========== 延申页 ==========
void ui_show_extpage(int idx) {
    lv_obj_add_flag(tileview, LV_OBJ_FLAG_HIDDEN);
    for(int i=0; i<7; i++)
        if(page_ext[i]) lv_obj_add_flag(page_ext[i], LV_OBJ_FLAG_HIDDEN);
    if(page_ext[idx]) {
        lv_obj_clear_flag(page_ext[idx], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(page_ext[idx]);
    }
}

// ========== 主页面显示 ==========
#ifdef __cplusplus
extern "C" {
#endif
void ui_show_mainpage(void) {
    lv_obj_clear_flag(tileview, LV_OBJ_FLAG_HIDDEN);
}
#ifdef __cplusplus
}
#endif

// ========== 全局等级缓存（微信小程序回传，0差1良2优，未回传时默认优） ==========
static int g_level_eat = 2;
static int g_level_drink = 2;
static int g_level_toilet = 2;
static int g_level_weight = 2;

static int g_micro_pressure = 50;
static int g_filter_life = 50;

// ========== 声明tile1_set_level_chs ==========
#ifdef __cplusplus
extern "C" {
#endif
void tile1_set_level_chs(int idx, int level);
#ifdef __cplusplus
}
#endif

// ========== 总览页数据柱刷新函数前置声明 ==========
void ui_update_health_bar_from_level();
void ui_update_fresh_bar();

// ========== 今日累计量缓存 ==========
static int today_eat = 0;
static int today_drink = 0;
static int today_toilet = 0;
static float today_weight = 0;

// ========== 事件检测时实时上报事件（供事件检测模块调用） ==========
void report_food_event(int delta, int total, time_t eventTime) {
    today_eat += delta;
    ui_update_health(today_eat, today_drink, today_toilet, today_weight);
    StaticJsonDocument<256> doc;
    doc["event_type"] = "food";
    doc["event_time"] = (long)eventTime;
    doc["delta"] = delta;
    doc["total"] = total;
    String data;
    serializeJson(doc, data);
    mqttManager.publishEventWithId("food", data, mqttManager.generateEventId());
}
void report_drink_event(int delta, int total, time_t eventTime) {
    today_drink += delta;
    ui_update_health(today_eat, today_drink, today_toilet, today_weight);
    StaticJsonDocument<256> doc;
    doc["event_type"] = "drink";
    doc["event_time"] = (long)eventTime;
    doc["delta"] = delta;
    doc["total"] = total;
    String data;
    serializeJson(doc, data);
    mqttManager.publishEventWithId("drink", data, mqttManager.generateEventId());
}
void report_toilet_event(int delta, int total, time_t eventTime) {
    today_toilet += delta;
    ui_update_health(today_eat, today_drink, today_toilet, today_weight);
    StaticJsonDocument<256> doc;
    doc["event_type"] = "toilet";
    doc["event_time"] = (long)eventTime;
    doc["delta"] = delta;
    doc["total"] = total;
    String data;
    serializeJson(doc, data);
    mqttManager.publishEventWithId("toilet", data, mqttManager.generateEventId());
}
void report_weight_event(float value, time_t eventTime) {
    today_weight = value;
    ui_update_health(today_eat, today_drink, today_toilet, today_weight);
    StaticJsonDocument<256> doc;
    doc["event_type"] = "weight";
    doc["event_time"] = (long)eventTime;
    doc["value"] = value;
    String data;
    serializeJson(doc, data);
    mqttManager.publishEventWithId("weight", data, mqttManager.generateEventId());
}

// ========== 每天凌晨清空今日累计量 ==========
void reset_today_counters() {
    today_eat = 0;
    today_drink = 0;
    today_toilet = 0;
    today_weight = 0;
    ui_update_health(0, 0, 0, 0);
}

// ========== UI初始化 ==========
void ui_init(void) {
    // ...样式初始化等...
    lv_style_init(&style_btn);
    lv_style_set_bg_opa(&style_btn, LV_OPA_60);
    lv_style_set_bg_color(&style_btn, lv_color_black());
    lv_style_set_radius(&style_btn, 16);
    lv_style_set_border_width(&style_btn, 2);
    lv_style_set_border_color(&style_btn, lv_color_white());
    lv_style_set_text_color(&style_btn, lv_color_white());
    lv_style_init(&style_btn_pr);
    lv_style_set_bg_opa(&style_btn_pr, LV_OPA_60);
    lv_style_set_bg_color(&style_btn_pr, lv_color_hex(0x800000));
    lv_style_set_radius(&style_btn_pr, 16);
    lv_style_set_border_width(&style_btn_pr, 2);
    lv_style_set_border_color(&style_btn_pr, lv_color_white());
    lv_style_set_text_color(&style_btn_pr, lv_color_white());
    lv_style_init(&style_transp);
    lv_style_set_bg_opa(&style_transp, LV_OPA_TRANSP);
    lv_style_init(&style_tile_transp);
    lv_style_set_bg_opa(&style_tile_transp, LV_OPA_TRANSP);
    lv_style_set_border_opa(&style_tile_transp, LV_OPA_TRANSP);
    lv_style_init(&style_35);
    lv_style_set_text_font(&style_35, &my_font_35);
    lv_style_init(&style_30);
    lv_style_set_text_font(&style_30, &my_font_30);
    lv_style_init(&style_25);
    lv_style_set_text_font(&style_25, &my_font_25);
    lv_style_init(&style_25_level);
    lv_style_set_text_font(&style_25_level, &my_font_25);
    lv_style_init(&style_bar_percent_14);
    lv_style_set_text_color(&style_bar_percent_14, lv_color_white());
    lv_style_set_text_font(&style_bar_percent_14, &lv_font_montserrat_14);

    // 背景
    lv_obj_t *img_bg = lv_img_create(lv_scr_act());
    lv_img_set_src(img_bg, &bg_main);
    lv_obj_set_pos(img_bg, 0, 0);
    lv_obj_set_size(img_bg, 480, 320);
    lv_obj_move_background(img_bg);

    // tileview结构
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, 480, 320);
    lv_obj_add_style(tileview, &style_tile_transp, 0);

    for(int i=0;i<3;i++) {
        tile[i] = lv_tileview_add_tile(tileview, i, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
        lv_obj_add_style(tile[i], &style_tile_transp, 0);
    }

    // tile[0]内容
    lv_obj_t *info_bg = lv_obj_create(tile[0]);
    lv_obj_remove_style_all(info_bg);
    lv_obj_add_style(info_bg, &style_transp, 0);
    lv_obj_set_size(info_bg, 136, 287);
    lv_obj_set_pos(info_bg, 9, 9);
    lv_obj_set_style_bg_color(info_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(info_bg, LV_OPA_20, 0);
    lv_obj_set_style_border_width(info_bg, 0, 0);
    lv_obj_set_style_radius(info_bg, 15, 0);

    // ★ 新增：5个标签，按用户指定顺序排列
    // 时间：Y=21（保持）
    // 温度：Y=77
    // 湿度：Y=133
    // CO2：Y=189
    // AQI：Y=246（保持）
    
    label_time = lv_label_create(tile[0]); 
    lv_obj_set_pos(label_time, 22, 21);
    lv_obj_set_style_text_color(label_time, lv_color_white(), 0);
    lv_label_set_text(label_time, "12:30");
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_36, 0);

    label_temp = lv_label_create(tile[0]); 
    lv_obj_set_pos(label_temp, 22, 77);
    lv_obj_set_style_text_color(label_temp, lv_color_white(), 0);
    lv_label_set_text(label_temp, "26°C");
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_30, 0);

    label_hum  = lv_label_create(tile[0]); 
    lv_obj_set_pos(label_hum, 22, 133);
    lv_obj_set_style_text_color(label_hum, lv_color_white(), 0);
    lv_label_set_text(label_hum, "45%");
    lv_obj_set_style_text_font(label_hum, &lv_font_montserrat_30, 0);

    label_co2 = lv_label_create(tile[0]);  // ★ CO2在湿度后面
    lv_obj_set_pos(label_co2, 22, 189);
    lv_obj_set_style_text_color(label_co2, lv_color_white(), 0);
    lv_label_set_text(label_co2, "400ppm");
    lv_obj_set_style_text_font(label_co2, &lv_font_montserrat_30, 0);

    label_aqi  = lv_label_create(tile[0]); 
    lv_obj_set_pos(label_aqi, 22, 246);
    lv_obj_set_style_text_color(label_aqi, lv_color_white(), 0);
    lv_label_set_text(label_aqi, "89 AQI");
    lv_obj_set_style_text_font(label_aqi, &lv_font_montserrat_30, 0);

    bar_health = lv_bar_create(tile[0]); lv_obj_set_size(bar_health, 46, 175); lv_obj_set_pos(bar_health, 181, 103);
    lv_obj_set_style_radius(bar_health, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_health, 4, LV_PART_INDICATOR);
    lv_bar_set_range(bar_health, 0, 100); lv_bar_set_value(bar_health, 80, LV_ANIM_OFF);

    bar_fresh  = lv_bar_create(tile[0]); lv_obj_set_size(bar_fresh, 46, 175); lv_obj_set_pos(bar_fresh, 259, 103);
    lv_obj_set_style_radius(bar_fresh, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_fresh, 4, LV_PART_INDICATOR);
    lv_bar_set_range(bar_fresh, 0, 100); lv_bar_set_value(bar_fresh, 55, LV_ANIM_OFF);

    label_bar_health_percent = lv_label_create(tile[0]);
    lv_obj_add_style(label_bar_health_percent, &style_bar_percent_14, 0);
    set_bar_percent_label(label_bar_health_percent, 80, 190, 83);

    label_bar_fresh_percent = lv_label_create(tile[0]);
    lv_obj_add_style(label_bar_fresh_percent, &style_bar_percent_14, 0);
    set_bar_percent_label(label_bar_fresh_percent, 55, 265, 83);

    lv_obj_t *icon1 = lv_img_create(tile[0]);
    lv_img_set_src(icon1, &tubiao_1);
    lv_obj_set_pos(icon1, 176, 10);
    lv_obj_t *icon2 = lv_img_create(tile[0]);
    lv_img_set_src(icon2, &tubiao_2);
    lv_obj_set_pos(icon2, 245, 10);

    // ========== 新增：创建主灯/氛围灯/新风按钮并绑定事件 ==========
    btn_mainlight = create_btn(tile[0], 330, 4, 150, 60, "主 灯", on_btn_mainlight_event_cb, NULL);
    lv_obj_add_style(lv_obj_get_child(btn_mainlight, 0), &style_30, 0);

    btn_ambientlight = create_btn(tile[0], 330, 78, 150, 60, "氛围灯", on_btn_ambientlight_event_cb, NULL);
    lv_obj_add_style(lv_obj_get_child(btn_ambientlight, 0), &style_30, 0);

    btn_fresh_auto = create_btn(tile[0], 330, 155, 150, 60, "新风自动", on_btn_fresh_auto_event_cb, NULL);
    lv_obj_add_style(lv_obj_get_child(btn_fresh_auto, 0), &style_30, 0);

    btn_fresh_manual = create_btn(tile[0], 330, 230, 150, 60, "新风手动", on_btn_fresh_manual_event_cb, NULL);
    lv_obj_add_style(lv_obj_get_child(btn_fresh_manual, 0), &style_30, 0);

    lv_style_set_border_color(&style_btn, lv_color_hex(0x549ACC));

    // ========== tile[1]内容 ==========
    tile1_page_create(tile[1]);

    // ========== tile[2]内容 ==========
    lv_obj_t *reset_title_bg = lv_obj_create(tile[2]);
    lv_obj_remove_style_all(reset_title_bg);
    lv_obj_add_style(reset_title_bg, &style_transp, 0);
    lv_obj_set_size(reset_title_bg, 170, 47);
    lv_obj_set_pos(reset_title_bg, 27, 5);
    lv_obj_set_style_bg_color(reset_title_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(reset_title_bg, LV_OPA_40, 0);
    lv_obj_set_style_border_width(reset_title_bg, 0, 0);
    lv_obj_set_style_radius(reset_title_bg, 10, 0);

    lv_obj_t *reset_title_label = lv_label_create(reset_title_bg);
    lv_label_set_text(reset_title_label, "复 位 栏");
    lv_obj_add_style(reset_title_label, &style_30, 0);
    lv_obj_set_style_text_color(reset_title_label, lv_color_hex(0x1E90FF), 0);
    lv_obj_center(reset_title_label);

    lv_obj_t *sys_title_bg = lv_obj_create(tile[2]);
    lv_obj_remove_style_all(sys_title_bg);
    lv_obj_add_style(sys_title_bg, &style_transp, 0);
    lv_obj_set_size(sys_title_bg, 228, 47);
    lv_obj_set_pos(sys_title_bg, 224, 5);
    lv_obj_set_style_bg_color(sys_title_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(sys_title_bg, LV_OPA_40, 0);
    lv_obj_set_style_border_width(sys_title_bg, 0, 0);
    lv_obj_set_style_radius(sys_title_bg, 10, 0);

    lv_obj_t *sys_title_label = lv_label_create(sys_title_bg);
    lv_label_set_text(sys_title_label, "系 统 设 置");
    lv_obj_add_style(sys_title_label, &style_30, 0);
    lv_obj_set_style_text_color(sys_title_label, lv_color_hex(0x1E90FF), 0);
    lv_obj_center(sys_title_label);

    lv_obj_t* btn20 = create_btn(tile[2], 41, 62, 142, 46, "饭盆复位", [](lv_event_t*e){ui_show_extpage(0);}, NULL);
    lv_obj_add_style(lv_obj_get_child(btn20, 0), &style_25, 0);

    lv_obj_t* btn21 = create_btn(tile[2], 41, 119, 142, 46, "水盆复位", [](lv_event_t*e){ui_show_extpage(1);}, NULL);
    lv_obj_add_style(lv_obj_get_child(btn21, 0), &style_25, 0);

    lv_obj_t* btn22 = create_btn(tile[2], 41, 175, 142, 46, "新风复位", [](lv_event_t*e){ui_show_extpage(2);}, NULL);
    lv_obj_add_style(lv_obj_get_child(btn22, 0), &style_25, 0);

    lv_obj_t* btn23 = create_btn(tile[2], 239, 62, 189, 46, "网络配置", [](lv_event_t*e){ui_show_extpage(3);}, NULL);
    lv_obj_add_style(lv_obj_get_child(btn23, 0), &style_25, 0);

    lv_obj_t* btn24 = create_btn(tile[2], 239, 119, 189, 46, "设备设置", [](lv_event_t*e){ui_show_extpage(4);}, NULL);
    lv_obj_add_style(lv_obj_get_child(btn24, 0), &style_25, 0);

    lv_obj_t* btn25 = create_btn(tile[2], 239, 175, 189, 46, "系统信息", [](lv_event_t*e){ui_show_extpage(5);}, NULL);
    lv_obj_add_style(lv_obj_get_child(btn25, 0), &style_25, 0);

    lv_obj_t* btn26 = create_btn(tile[2], 249, 232, 170, 44, "恢复出厂", [](lv_event_t*e){ui_show_extpage(6);}, NULL);   
    lv_obj_add_style(lv_obj_get_child(btn26, 0), &style_25, 0);

    int nav_y = 295;
    int nav_dot_width = 16 * 4;
    int nav_dot_height = 16;
    int nav_dot_gap = 15;
    int nav_dot_start_x = (480 - ((nav_dot_width * 3) + nav_dot_gap * 2)) / 2;
    for (int i = 0; i < 3; i++) {
        nav_dot[i] = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(nav_dot[i]);
        lv_obj_set_size(nav_dot[i], nav_dot_width, nav_dot_height);
        lv_obj_set_style_radius(nav_dot[i], nav_dot_height / 2, 0);
        lv_obj_set_style_bg_color(nav_dot[i], i==0?lv_color_hex(0xFF0000):lv_color_white(), 0);
        lv_obj_set_style_bg_opa(nav_dot[i], LV_OPA_COVER, 0);
        lv_obj_set_pos(nav_dot[i], nav_dot_start_x + i * (nav_dot_gap + nav_dot_width), nav_y);
        lv_obj_add_event_cb(nav_dot[i], nav_dot_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    ext_pages_init();

    lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    ui_update_level_labels(2, 2, 2, 2);
    ui_update_micro_pressure(-9);
    ui_update_filter_life(50);
}

// ========== 百分数动态刷新接口 ==========
void ui_update_bar_health_percent(int value) {
    set_bar_percent_label(label_bar_health_percent, value, 182, 83);
}
void ui_update_bar_fresh_percent(int value) {
    set_bar_percent_label(label_bar_fresh_percent, value, 256, 83);
}

// ========== 其它数据刷新接口 ==========
void ui_update_time(const char* timestr) {
    lv_label_set_text(label_time, timestr);
}

// ★ 新增：更新环境参数（包括CO2）
void ui_update_env(float temp, float hum, int co2_ppm, int aqi) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%.1f°C", temp); 
    lv_label_set_text(label_temp, buf);
    
    snprintf(buf, sizeof(buf), "%.1f%%", hum);  
    lv_label_set_text(label_hum, buf);
    
    snprintf(buf, sizeof(buf), "%d ppm", co2_ppm);  
    lv_label_set_text(label_co2, buf);
    
    snprintf(buf, sizeof(buf), "%d AQI", aqi);  
    lv_label_set_text(label_aqi, buf);
}

void ui_update_health(int eat, int drink, int toilet, float weight) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%d g", eat);
    tile1_set_value(0, buf);
    snprintf(buf, sizeof(buf), "%d ml", drink);
    tile1_set_value(1, buf);
    snprintf(buf, sizeof(buf), "%d g", toilet);
    tile1_set_value(2, buf);
    // weight 参数当前以克 (g) 传入 -> 转为 kg 显示（保留一位小数）
    float kg = weight / 1000.0f;
    snprintf(buf, sizeof(buf), "%.1f kg", kg);
    tile1_set_value(3, buf);
}
void ui_update_fresh(int value) {
    lv_bar_set_value(bar_fresh, value, LV_ANIM_OFF);
    ui_update_bar_fresh_percent(value);
}
void ui_update_health_bar(int value) {
    lv_bar_set_value(bar_health, value, LV_ANIM_OFF);
    ui_update_bar_health_percent(value);
}

// ========== 优良差等级刷新接口 ==========
void ui_update_level_labels(int eat, int drink, int toilet, int weight) {
    g_level_eat = eat;
    g_level_drink = drink;
    g_level_toilet = toilet;
    g_level_weight = weight;
    tile1_set_level_chs(0, eat);
    tile1_set_level_chs(1, drink);
    tile1_set_level_chs(2, toilet);
    tile1_set_level_chs(3, weight);
    ui_update_health_bar_from_level();
}

// ========== 新风健康参数刷新接口 ==========
void ui_update_micro_pressure(int raw_val) {
    g_micro_pressure = raw_val;
    int micro_score = convert_micro_pressure_val(raw_val);
    int filter_score = g_filter_life > 0 ? g_filter_life : 50;
    int total_score = micro_score + filter_score;
    if (total_score > 100) total_score = 100;
    lv_bar_set_value(bar_fresh, total_score, LV_ANIM_OFF);
    ui_update_bar_fresh_percent(total_score);
}
void ui_update_filter_life(int val) {
    g_filter_life = val;
    int micro_score = convert_micro_pressure_val(g_micro_pressure);
    int filter_score = val > 0 ? val : 50;
    int total_score = micro_score + filter_score;
    if (total_score > 100) total_score = 100;
    lv_bar_set_value(bar_fresh, total_score, LV_ANIM_OFF);
    ui_update_bar_fresh_percent(total_score);
}

// ========== 总览页中间2个数据柱刷新接口 ==========
void ui_update_health_bar_from_level() {
    int score = 10;
    int level_array[3] = {g_level_eat, g_level_drink, g_level_toilet};
    for (int i = 0; i < 3; ++i) {
        if (level_array[i] == 2) score += 30;
        else if (level_array[i] == 1) score += 15;
    }
    if (score > 100) score = 100;
    lv_bar_set_value(bar_health, score, LV_ANIM_OFF);
    ui_update_bar_health_percent(score);
}

void ui_update_fresh_bar() {
    int micro_score = convert_micro_pressure_val(g_micro_pressure);
    int filter_score = g_filter_life > 0 ? g_filter_life : 50;
    int total_score = micro_score + filter_score;
    if (total_score > 100) total_score = 100;
    lv_bar_set_value(bar_fresh, total_score, LV_ANIM_OFF);
    ui_update_bar_fresh_percent(total_score);
}

// ========== tile[1]数据刷新示例 ==========
void ui_update_tile1_demo() {
    int eat_level = 2;
    int drink_level = 2;
    int toilet_level = 2;
    int weight_level = 2;
    ui_update_level_labels(eat_level, drink_level, toilet_level, weight_level);

    ui_update_micro_pressure(-9);
    ui_update_filter_life(50);

    tile1_set_level(0, "优", lv_color_hex(0x00FF00));
    tile1_set_level(1, "优", lv_color_hex(0x00FF00));
    tile1_set_level(2, "优", lv_color_hex(0x00FF00));
    tile1_set_level(3, "优", lv_color_hex(0x00FF00));

    int eat_curve[10] = {110, 120, 130, 128, 140, 136, 135, 125, 134, 132};
    int drink_curve[10] = {200, 210, 215, 220, 225, 230, 240, 238, 230, 229};
    int toilet_curve[10] = {180, 190, 185, 188, 195, 200, 198, 196, 192, 190};
    int weight_curve[10] = {2500, 2510, 2505, 2520, 2515, 2525, 2530, 2535, 2532, 2528};

    tile1_set_chart(0, eat_curve, 10);
    tile1_set_chart(1, drink_curve, 10);
    tile1_set_chart(2, toilet_curve, 10);
    tile1_set_chart(3, weight_curve, 10);

    tile1_set_value(0, "152g");
    tile1_set_value(1, "230ml");
    tile1_set_value(2, "198g");
    tile1_set_value(3, "2.5kg");

    tile1_set_item_color(0, lv_color_hex(0x1E90FF));
}