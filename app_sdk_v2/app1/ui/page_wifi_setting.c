#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"

static lv_style_t com_style;

// ======================= 全局状态管理 =======================

// 当前激活的文本框指针
static lv_obj_t *active_textarea = NULL;

// 封装字库获取函数
static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}

// ======================= 事件处理函数 =======================

// 返回主界面
static void lv_event_cb_func(lv_event_t *e)
{
    lv_obj_clean(lv_scr_act());

    page_test_init();
}

// 文本框焦点事件处理
static void ta_focus_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED)
    {
        // 设置当前激活的文本框
        active_textarea = ta;
        printf("Textarea focused\n");
    }
    else if (code == LV_EVENT_DEFOCUSED)
    {
        printf("Textarea defocused\n");
    }
    else if (code == LV_EVENT_READY)
    {
        const char *text = lv_textarea_get_text(ta);
        if (text != NULL)
        {
            printf("current text: %s\n", text);
        }
        else
        {
            printf("current text: (empty)\n");
        }
    }
}

// 键盘隐藏事件处理
static void kb_hide_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_CANCEL || code == LV_EVENT_READY)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        active_textarea = NULL;
    }
}

// 键盘显示事件处理
static void kb_show_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED)
    {
        // 设置当前激活的文本框
        active_textarea = ta;
        // 绑定键盘到当前文本框
        lv_keyboard_set_textarea(kb, ta);
        // 显示键盘
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        // 聚焦到文本框
        lv_obj_add_state(ta, LV_STATE_FOCUSED);
    }
}

static void btn_click_event_cb_func(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_user_data(e);
    lv_event_send(ta, LV_EVENT_READY, NULL);
}
// ======================= UI初始化模块 =======================

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

static lv_obj_t *init_back_view(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_align(cont, LV_ALIGN_TOP_LEFT);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *back_img = lv_img_create(cont);
    lv_img_set_src(back_img, GET_IMAGE_PATH("icon_back.png"));
    lv_obj_set_align(back_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_left(back_img, 20, 0);
    lv_obj_set_style_pad_top(back_img, 20, 0);

    lv_obj_t *menu_img = lv_img_create(cont);
    lv_img_set_src(menu_img, GET_IMAGE_PATH("icon_wifi.png"));
    lv_obj_set_align(menu_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_top(menu_img, 20, 0);
    lv_obj_align_to(menu_img, back_img, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_t *title = lv_label_create(cont);
    obj_font_set(title, FONT_TYPE_CN, 24);
    lv_label_set_text(title, "wifi设置");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_align_to(title, menu_img, LV_ALIGN_OUT_RIGHT_MID, 20, 3);

    lv_obj_add_event_cb(cont, lv_event_cb_func, LV_EVENT_CLICKED, NULL);

    return cont;
}

static lv_obj_t *init()
{
    com_style_init();

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);

    init_back_view(cont);
    return cont;
}

static lv_obj_t *init_imag_text(lv_obj_t *parent, const char *src, const char *str)
{
    // 创建对象作为容器
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);

    lv_obj_t *icon = lv_img_create(cont);
    lv_img_set_src(icon, src);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    obj_font_set(label, FONT_TYPE_CN, 20);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label, str);
    lv_obj_align_to(label, icon, LV_ALIGN_OUT_RIGHT_MID, 20, -5);

    return cont;
}

static lv_obj_t *init_select_btn(lv_obj_t *parent, int lenth,
                                 int width, int redius, const char *str,
                                 const char size, const char py_x, const char py_y)
{
    // 初始化按钮控件
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &com_style, LV_PART_MAIN);
    // 设置按钮大小
    lv_obj_set_size(btn, lenth, width);
    // 清除焦点状态
    lv_obj_clear_state(btn, LV_STATE_FOCUS_KEY);
    // 设置边框、阴影为0
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    // 设置圆角为35
    lv_obj_set_style_radius(btn, redius, LV_PART_MAIN);
    // 设置背景颜色为蓝色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1F94D2), 0);
    // 初始化按钮显示文字
    lv_obj_t *btn_label = lv_label_create(btn);
    obj_font_set(btn_label, FONT_TYPE_CN, size);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(btn_label, str);
    // 进行偏移对齐
    lv_obj_align(btn_label, LV_ALIGN_CENTER, py_x, py_y);
    return btn;
}
// ======================= 页面主函数 =======================

void page_wifi_setting() // 日期格式
{
    lv_obj_t *cont = init();

    lv_obj_t *cont1 = lv_obj_create(cont);
    lv_obj_set_size(cont1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont1, &com_style, LV_PART_MAIN);
    lv_obj_align(cont1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cont1, 40, LV_PART_MAIN);

    lv_obj_t *ta1 = lv_textarea_create(cont);
    lv_textarea_set_one_line(ta1, true);
    lv_textarea_set_placeholder_text(ta1, "input your user");
    lv_obj_add_event_cb(ta1, ta_focus_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *ta2 = lv_textarea_create(cont);
    lv_textarea_set_one_line(ta2, true);
    lv_textarea_set_placeholder_text(ta2, "input your code");
    lv_obj_add_event_cb(ta2, ta_focus_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *wifi_user = init_imag_text(cont1, GET_IMAGE_PATH("icon_2.png"), "WiFi账号:");
    lv_obj_align_to(ta1, wifi_user, LV_ALIGN_OUT_RIGHT_MID, 10, -30);
    lv_obj_t *wifi_code = init_imag_text(cont1, GET_IMAGE_PATH("icon_3.png"), "WiFi密码:");
    lv_obj_align_to(ta2, wifi_code, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *btn = init_select_btn(cont, 151, 66, 35, "开始连接", 30, 0, -5);
    lv_obj_add_event_cb(btn, btn_click_event_cb_func, LV_EVENT_CLICKED, ta1);
    lv_obj_add_event_cb(btn, btn_click_event_cb_func, LV_EVENT_CLICKED, ta2);

    lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
    // 设置键盘大小
    lv_obj_set_size(kb, 700, 280);
    // 设置键盘主体背景
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x000000), LV_PART_MAIN);
    // 清除对象焦点状态
    lv_obj_clear_state(kb, LV_STATE_FOCUS_KEY);
    // 添加键盘隐藏事件
    lv_obj_add_event_cb(kb, kb_hide_event_cb, LV_EVENT_ALL, NULL);
    // 初始隐藏键盘
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    // 对齐，使得键盘在文本框右侧
    lv_obj_align_to(kb, ta1, LV_ALIGN_OUT_RIGHT_MID, 0, 40);
    // 放在label设置后才能居中对齐
    lv_obj_align_to(btn, kb, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // 为文本框添加点击事件，显示键盘
    lv_obj_add_event_cb(ta1, kb_show_event_cb, LV_EVENT_CLICKED, kb);
    lv_obj_add_event_cb(ta2, kb_show_event_cb, LV_EVENT_CLICKED, kb);
}