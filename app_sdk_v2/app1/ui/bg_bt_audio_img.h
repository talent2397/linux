#ifndef BG_BT_AUDIO_IMG_H
#define BG_BT_AUDIO_IMG_H

#include "lvgl.h"
#include "output.h"

// 图片宽度和高度（从output.h中的注释获取）
#define BG_BT_AUDIO_WIDTH 1424
#define BG_BT_AUDIO_HEIGHT 280

// LVGL图片描述符
const lv_img_dsc_t bg_bt_audio_img = {
    .header = {
        .cf = LV_IMG_CF_TRUE_COLOR_ALPHA, // 32位带透明度的真彩色
        .always_zero = 0,
        .reserved = 0,
        .w = BG_BT_AUDIO_WIDTH,
        .h = BG_BT_AUDIO_HEIGHT,
    },
    .data_size = sizeof(bg_bt_audio),
    .data = bg_bt_audio,
};

#endif // BG_BT_AUDIO_IMG_H