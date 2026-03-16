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

// 番茄时钟设置值
static int study_duration = 25; // 默认学习时长25分钟
static int relax_duration = 5;  // 默认休息时长5分钟

// 滚轮对象指针
static lv_obj_t *study_roller = NULL;
static lv_obj_t *relax_roller = NULL;

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
// 滚轮值改变事件处理
static void roller_event_handler(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target(e);
    int selected_index = lv_roller_get_selected(roller);

    // 根据索引计算实际分钟数 (索引0对应10分钟，索引1对应15分钟，以此类推)
    int minutes = (selected_index + 1) * 5 + 5; // 10, 15, 20, 25, 30, 35, 40, 45, 50, 55

    if (roller == study_roller)
    {
        study_duration = minutes;
        printf("学习时长设置为: %d分钟\n", study_duration);
    }
    else if (roller == relax_roller)
    {
        relax_duration = minutes;
        printf("休息时长设置为: %d分钟\n", relax_duration);
    }
}

// 开始学习按钮事件处理
static void start_study_event_cb(lv_event_t *e)
{
    printf("开始学习 - 学习时长: %d分钟, 休息时长: %d分钟\n", study_duration, relax_duration);

    // 这里可以调用其他函数，传递参数
    // 例如: start_tomato_timer(study_duration, relax_duration);
}

// 取消设定按钮事件处理
static void cancel_setting_event_cb(lv_event_t *e)
{
    // 重置为默认值
    study_duration = 25;
    relax_duration = 5;

    // 重置滚轮选择
    if (study_roller)
    {
        lv_roller_set_selected(study_roller, 3, LV_ANIM_ON); // 25分钟对应索引3
    }
    if (relax_roller)
    {
        lv_roller_set_selected(relax_roller, 0, LV_ANIM_ON); // 5分钟对应索引0
    }

    printf("取消设定，已重置为默认值\n");
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
    lv_img_set_src(menu_img, GET_IMAGE_PATH("icon_tomato.png"));
    lv_obj_set_align(menu_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_top(menu_img, 20, 0);
    lv_obj_align_to(menu_img, back_img, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_t *title = lv_label_create(cont);
    obj_font_set(title, FONT_TYPE_CN, 24);
    lv_label_set_text(title, "番茄时钟设置");
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

    lv_obj_t *bg_time = lv_img_create(cont);
    lv_img_set_src(bg_time, GET_IMAGE_PATH("bg_tomato_time.png"));
    lv_obj_align(bg_time, LV_ALIGN_RIGHT_MID, 0, 0);

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

static lv_obj_t *init_gunlun(lv_obj_t *parent)
{
    lv_obj_t *roller1 = lv_roller_create(parent);
    lv_roller_set_options(roller1,
                          "10min\n"
                          "15min\n"
                          "20min\n"
                          "25min\n"
                          "30min\n"
                          "35min\n"
                          "40min\n"
                          "45min\n"
                          "50min\n"
                          "55min\n",
                          LV_ROLLER_MODE_NORMAL);
    lv_obj_center(roller1);
    lv_obj_set_style_bg_color(roller1, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_border_width(roller1, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller1, lv_color_hex(0x000000), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller1, lv_color_hex(0xffffff), LV_PART_SELECTED);
    lv_obj_clear_state(roller1, LV_STATE_FOCUS_KEY);

    lv_obj_set_style_text_font(roller1, get_font(FONT_TYPE_CN, 48), LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller1, get_font(FONT_TYPE_CN, 24), LV_PART_MAIN);

    lv_roller_set_visible_row_count(roller1, 3);

    lv_obj_add_event_cb(roller1, roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    return roller1;
}

// ======================= 页面主函数 =======================

void page_tomato_setting() // 日期格式
{
    lv_obj_t *cont = init();

    lv_obj_t *cont1 = lv_obj_create(cont);
    lv_obj_set_size(cont1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont1, &com_style, LV_PART_MAIN);
    lv_obj_align(cont1, LV_ALIGN_LEFT_MID, 120, 0); // 整体靠左对齐，距离左边50像素

    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont1, 25, LV_PART_MAIN);

    // 学习时长标签
    lv_obj_t *study_time = init_imag_text(cont1, GET_IMAGE_PATH("icon_clock.png"), "学习时长:");

    // 学习时长滚轮
    study_roller = init_gunlun(cont1);
    lv_obj_add_event_cb(study_roller, roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_roller_set_selected(study_roller, 3, LV_ANIM_OFF); // 默认选择25分钟

    // 休息时长标签
    lv_obj_t *relax_time = init_imag_text(cont1, GET_IMAGE_PATH("icon_clock.png"), "休息时长:");

    // 休息时长滚轮
    relax_roller = init_gunlun(cont1);
    lv_obj_add_event_cb(relax_roller, roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_roller_set_selected(relax_roller, 0, LV_ANIM_OFF); // 默认选择5分钟

    // 按钮容器 - 垂直布局
    lv_obj_t *btn_container = lv_obj_create(cont);
    lv_obj_set_size(btn_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(btn_container, &com_style, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn_container, 20, LV_PART_MAIN);

    // 对齐到第二个滚轮右侧，确保水平对齐
    lv_obj_align_to(btn_container, relax_roller, LV_ALIGN_OUT_RIGHT_MID, 30, -60);

    // 开始学习按钮
    lv_obj_t *start_btn = init_select_btn(btn_container, 151, 66, 35, "开始学习", 24, 0, 0);
    lv_obj_add_event_cb(start_btn, start_study_event_cb, LV_EVENT_CLICKED, NULL);

    // 取消设定按钮
    lv_obj_t *cancel_btn = init_select_btn(btn_container, 151, 66, 35, "取消设定", 24, 0, 0);
    lv_obj_add_event_cb(cancel_btn, cancel_setting_event_cb, LV_EVENT_CLICKED, NULL);
}