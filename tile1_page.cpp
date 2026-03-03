#include <stdio.h>
#include "tile1_page.h"
#include "my_font_30.h"
#include "my_font_25.h"

tile1_row_t tile1_rows[TILE1_ROW_NUM];
lv_obj_t *tile1_title_bg[3];
lv_obj_t *tile1_title_label[3];
lv_style_t style_tile1_item[4];

// 等级颜色和汉字（优良差：红、黄、绿，顺序与0差1良2优一致）
static const lv_color_t level_colors[3] = {lv_color_hex(0xFF0000), lv_color_hex(0xFFD700), lv_color_hex(0x00FF00)};
static const char *chs_text[3] = {"差", "良", "优"};
static const char *item_name[TILE1_ROW_NUM] = {"进食", "饮水", "排泄", "体重"};
static const char *unit[TILE1_ROW_NUM] = {"g", "ml", "g", "Kg"};

static int last_level[TILE1_ROW_NUM] = {2,2,2,2};

void tile1_page_create(lv_obj_t *parent)
{
    // --------- 标题栏 ---------
    const char *title_str[3] = {"昨天", "过去10天历史", "今天"};
    int title_x[3] = {7, 126, 360};
    int title_w[3] = {113, 228, 113};

    for (int i = 0; i < 3; i++)
    {
        tile1_title_bg[i] = lv_obj_create(parent);
        lv_obj_remove_style_all(tile1_title_bg[i]);
        lv_obj_set_size(tile1_title_bg[i], title_w[i], 53);
        lv_obj_set_pos(tile1_title_bg[i], title_x[i], 4);
        lv_obj_set_style_bg_color(tile1_title_bg[i], lv_color_black(), 0);
        lv_obj_set_style_bg_opa(tile1_title_bg[i], LV_OPA_40, 0);
        lv_obj_set_style_radius(tile1_title_bg[i], 8, 0);
        lv_obj_set_style_border_width(tile1_title_bg[i], 0, 0);

        tile1_title_label[i] = lv_label_create(tile1_title_bg[i]);
        lv_label_set_text(tile1_title_label[i], title_str[i]);
        lv_obj_set_style_text_font(tile1_title_label[i], &my_font_30, 0);
        lv_obj_set_style_text_color(tile1_title_label[i], lv_color_hex(0x1E90FF), 0);
        lv_obj_center(tile1_title_label[i]);
    }

    // --------- 每行内容 ---------
    int row_y[TILE1_ROW_NUM] = {64, 121, 178, 235};
    for (int i = 0; i < TILE1_ROW_NUM; i++)
    {
        tile1_rows[i].row_bg = lv_obj_create(parent);
        lv_obj_remove_style_all(tile1_rows[i].row_bg);
        lv_obj_set_size(tile1_rows[i].row_bg, 466, 50);
        lv_obj_set_pos(tile1_rows[i].row_bg, 7, row_y[i]);
        lv_obj_set_style_bg_color(tile1_rows[i].row_bg, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(tile1_rows[i].row_bg, LV_OPA_30, 0);
        lv_obj_set_style_radius(tile1_rows[i].row_bg, 8, 0);
        lv_obj_set_style_border_width(tile1_rows[i].row_bg, 0, 0);

        // 左侧等级（优良差）
        tile1_rows[i].label_level = lv_label_create(tile1_rows[i].row_bg);
        lv_label_set_text(tile1_rows[i].label_level, chs_text[2]);
        lv_obj_set_style_text_color(tile1_rows[i].label_level, level_colors[2], 0);
        lv_obj_set_style_text_font(tile1_rows[i].label_level, &my_font_25, 0);
        lv_obj_set_pos(tile1_rows[i].label_level, 14, 12);

        // 项目名
        lv_style_init(&style_tile1_item[i]);
        lv_style_set_text_color(&style_tile1_item[i], lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&style_tile1_item[i], &my_font_25);

        tile1_rows[i].label_item = lv_label_create(tile1_rows[i].row_bg);
        lv_label_set_text(tile1_rows[i].label_item, item_name[i]);
        lv_obj_add_style(tile1_rows[i].label_item, &style_tile1_item[i], 0);
        lv_obj_set_pos(tile1_rows[i].label_item, 52, 12);

        // 历史曲线图
        tile1_rows[i].chart = lv_chart_create(tile1_rows[i].row_bg);
        lv_obj_set_size(tile1_rows[i].chart, 228, 50);
        lv_obj_set_pos(tile1_rows[i].chart, 126, 0);
        lv_chart_set_type(tile1_rows[i].chart, LV_CHART_TYPE_LINE);
        lv_chart_set_point_count(tile1_rows[i].chart, 10);

        lv_obj_set_style_bg_opa(tile1_rows[i].chart, LV_OPA_0, 0);
        lv_obj_set_style_radius(tile1_rows[i].chart, 0, 0);
        lv_obj_set_style_border_color(tile1_rows[i].chart, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(tile1_rows[i].chart, 2, 0);
        lv_obj_set_style_line_color(tile1_rows[i].chart, lv_color_hex(0xFF0000), LV_PART_ITEMS);
        lv_obj_set_style_line_width(tile1_rows[i].chart, 1, LV_PART_ITEMS);

        tile1_rows[i].series = lv_chart_add_series(tile1_rows[i].chart, lv_color_hex(0x00FF00), LV_CHART_AXIS_PRIMARY_Y);
        int def[10] = {0};
        for(int k=0;k<10;k++) {
            lv_chart_set_value_by_id(tile1_rows[i].chart, tile1_rows[i].series, k, def[k]);
        }

        // 右侧数值
        tile1_rows[i].label_value = lv_label_create(tile1_rows[i].row_bg);
        char buf[16]; snprintf(buf, sizeof(buf), "0%s", unit[i]);
        lv_label_set_text(tile1_rows[i].label_value, buf);
        lv_obj_set_style_text_color(tile1_rows[i].label_value, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(tile1_rows[i].label_value, &my_font_25, 0);
        lv_obj_align(tile1_rows[i].label_value, LV_ALIGN_TOP_RIGHT, -16, 12);

        last_level[i] = 2; // 默认优
    }
}

// 刷新左侧等级内容和颜色
void tile1_set_level(int row, const char *text, lv_color_t color)
{
    if (row < 0 || row >= TILE1_ROW_NUM) return;
    lv_label_set_text(tile1_rows[row].label_level, text);
    lv_obj_set_style_text_color(tile1_rows[row].label_level, color, 0);
}

// 刷新优良差汉字与颜色（0差1良2优）
void tile1_set_level_chs(int row, int level)
{
    if (row < 0 || row >= TILE1_ROW_NUM) return;
    if (level < 0 || level > 2) level = 2;
    last_level[row] = level;
    lv_label_set_text(tile1_rows[row].label_level, chs_text[level]);
    lv_obj_set_style_text_color(tile1_rows[row].label_level, level_colors[level], 0);
}

// 刷新曲线（10点数据）
// 注意：声明为 const int *data 以兼容主循环和MQTT回调
void tile1_set_chart(int row, const int *data, int len)
{
    if (row < 0 || row >= TILE1_ROW_NUM || !data || len <= 0) return;
    for (int i = 0; i < len && i < 10; i++) {
        lv_chart_set_value_by_id(tile1_rows[row].chart, tile1_rows[row].series, i, data[i]);
    }
    lv_chart_refresh(tile1_rows[row].chart);
}

// 刷新右侧数值
void tile1_set_value(int row, const char *text)
{
    if (row < 0 || row >= TILE1_ROW_NUM) return;
    lv_label_set_text(tile1_rows[row].label_value, text);
}

// 刷新项目名颜色
void tile1_set_item_color(int row, lv_color_t color)
{
    if (row < 0 || row >= TILE1_ROW_NUM) return;
    lv_style_set_text_color(&style_tile1_item[row], color);
    lv_obj_refresh_style(tile1_rows[row].label_item, LV_PART_MAIN, LV_STYLE_PROP_INV);
}