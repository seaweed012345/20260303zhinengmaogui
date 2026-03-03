#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "lv_qrcode.h"
#include <qrcodegen.h>

void my_page_net_config_create(lv_obj_t* parent, const char* mac_addr) {
    char qr_buf[128];
    snprintf(qr_buf, sizeof(qr_buf), "https://yourserver.com/bind?device=%s", mac_addr);

    lv_obj_t* qrcode = lv_qrcode_create(parent, 120, lv_color_black(), lv_color_white());
    lv_qrcode_update(qrcode, qr_buf, strlen(qr_buf));
    lv_obj_set_pos(qrcode, 40, 80);

    lv_obj_t* qr_label = lv_label_create(parent);
    lv_label_set_text(qr_label, "请扫码配置网络:");
    lv_obj_set_pos(qr_label, 40, 60);
}