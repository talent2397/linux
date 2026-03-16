#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"

static lv_style_t com_style;

static void btn_click_absolute_cb(lv_event_t *e)
{
    // 获取layer top对象
    lv_obj_t *act_scr = lv_layer_top();
    lv_obj_clean(act_scr);
    lv_style_reset(&com_style);
}

// 封装字库获取函数
static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}

// 返回界面
static void lv_event_cb_func(lv_event_t *e)
{
    lv_obj_clean(lv_scr_act());
    page_test_init();
}

// 初始化通用样式
static void com_style_init()
{
    lv_style_init(&com_style);
    if (!lv_style_is_empty(&com_style))
        lv_style_reset(&com_style);
    lv_style_set_bg_color(&com_style, lv_color_hex(0x000000));
    lv_style_set_radius(&com_style, 0);
    lv_style_set_border_width(&com_style, 0);
    lv_style_set_pad_all(&com_style, 0);
    lv_style_set_outline_width(&com_style, 0);
}

// 创建按钮
static lv_obj_t *init_select_btn(lv_obj_t *parent, int lenth, int width, int redius, const char *str)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &com_style, LV_PART_MAIN);
    lv_obj_set_size(btn, lenth, width);
    lv_obj_clear_state(btn, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, redius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1F94D2), 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    obj_font_set(btn_label, FONT_TYPE_CN, 24);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(btn_label, str);
    lv_obj_align(btn_label, LV_ALIGN_CENTER, 0, -5);
    lv_obj_add_event_cb(btn, btn_click_absolute_cb, LV_EVENT_SHORT_CLICKED, NULL);
    return btn;
}

static lv_obj_t *init_ui(lv_obj_t *parent, const char *sra, const char *stb,
                         int lenth, int width, int redius, const char *str)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);

    // 设置线性布局，排序方式为列排序
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cont, 30, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *icon = lv_img_create(cont);
    lv_img_set_src(icon, sra);

    lv_obj_t *label = lv_label_create(cont);
    obj_font_set(label, FONT_TYPE_CN, 20);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label, stb);

    init_select_btn(cont, lenth, width, redius, str);

    return cont;
}

// ======================= 页面主函数 =======================

void page_alarm_dialog()
{
    com_style_init();

    lv_obj_t *cont = lv_obj_create(lv_layer_top());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);

    lv_obj_t *setting = init_ui(cont, GET_IMAGE_PATH("icon_alarm.png"),
                                "嘿,闹铃时间到啦！", 150, 50, 35, "确定");

    lv_obj_center(setting);
}