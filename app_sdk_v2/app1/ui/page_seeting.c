
#include <stdio.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"
static lv_style_t com_style;

static void lv_event_cb_func(lv_event_t *e)
{
    // char *str = (char *)lv_event_get_user_data(e);
    // printf("%s click\n", str);
    lv_obj_t *act_scr = lv_scr_act();
    lv_obj_clean(act_scr);
    page_test_init();
}
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    // 从 user_data 中获取当前滑块专属的 label
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);

    int value = (int)lv_slider_get_value(slider);
    lv_label_set_recolor(label, true);
    lv_label_set_text_fmt(label, "#1fc505 %d%%", value);

    // 重新对齐
    // lv_obj_align_to(label, slider, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
}
// 初始化通用样式
static void com_style_init()
{
    // 初始化样式
    lv_style_init(&com_style);
    // 判断如果样式非空，那就先重置，再设置
    if (lv_style_is_empty(&com_style) == false)
        lv_style_reset(&com_style);
    // 样式背景设置为黑色，圆角设置为0，边框宽度设置为0，填充区域设置为0
    lv_style_set_bg_color(&com_style, lv_color_hex(0x000000));
    lv_style_set_radius(&com_style, 0);
    lv_style_set_border_width(&com_style, 0);
    lv_style_set_pad_all(&com_style, 0);
    lv_style_set_outline_width(&com_style, 0);
}

// 封装字库获取函数 获取字体的函数
static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}
// 创建一个 图片和文字统一体的对象
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
// 初始化一个滑块
// 【关键修改】：多加了一个参数 align_target，用于告诉滑块它应该跟随谁对齐
static lv_obj_t *init_slider(lv_obj_t *parent, lv_obj_t *align_target, uint32_t sra, uint32_t srb, uint32_t src, int size)
{
    lv_obj_t *slider = lv_slider_create(parent);

    // 设置样式
    lv_obj_set_style_bg_color(slider, lv_color_hex(sra), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(srb), LV_PART_KNOB);
    lv_obj_set_style_border_color(slider, lv_color_hex(src), LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, size, LV_PART_KNOB);

    // 【关键修改1】：先将滑块对齐到左侧的文本/图标，确定好滑块的最终位置！
    lv_obj_align_to(slider, align_target, LV_ALIGN_OUT_RIGHT_MID, 30, 0);

    // 创建局部 label
    lv_obj_t *local_label = lv_label_create(parent);

    // 【关键修改2】：给 Label 设置固定宽度，并让文字在内部左对齐（彻底解决抖动）
    lv_obj_set_width(local_label, 60); // 预留足够的宽度容纳 "100%"
    lv_obj_set_style_text_align(local_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

    // 设置初始文字
    lv_label_set_text_fmt(local_label, "%d%%", lv_slider_get_value(slider));

    // 【关键修改3】：滑块位置确定后，再把 Label 对齐到滑块的右边（只需对齐这一次）
    lv_obj_align_to(local_label, slider, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, local_label);

    return slider;
}

static lv_obj_t *init_back_view(lv_obj_t *parent)
{
    // 初始化容器对象
    lv_obj_t *cont = lv_obj_create(parent);
    // 设置大小 样式 对齐
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_align(cont, LV_ALIGN_TOP_MID);
    // 添加可点击标志
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);

    // 初始化返回图像控件
    lv_obj_t *back_img = lv_img_create(cont);
    // 设置显示的图片
    lv_img_set_src(back_img, GET_IMAGE_PATH("icon_back.png"));
    // 设置对齐方式为，父对象的左上角对齐
    lv_obj_set_align(back_img, LV_ALIGN_TOP_LEFT);
    // 设置左侧、顶部填充距离为20
    lv_obj_set_style_pad_left(back_img, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_top(back_img, 20, LV_PART_MAIN);

    // 初始化菜单图像控件
    lv_obj_t *menu_img = lv_img_create(cont);
    lv_img_set_src(menu_img, GET_IMAGE_PATH("icon_alarm.png"));
    lv_obj_set_align(menu_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_top(menu_img, 20, LV_PART_MAIN);
    // 设置控件对齐back_img，x轴偏移20
    lv_obj_align_to(menu_img, back_img, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // 初始化标签控件
    lv_obj_t *title = lv_label_create(cont);
    // 设置字体样式和字体大小
    obj_font_set(title, FONT_TYPE_CN, 24);
    // 设置显示内容
    lv_label_set_text(title, "系统设置");
    // 设置颜色
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    // 设置对齐方式
    lv_obj_align_to(title, menu_img, LV_ALIGN_OUT_RIGHT_MID, 20, 3);

    // 添加点击事件监听
    lv_obj_add_event_cb(cont, lv_event_cb_func, LV_EVENT_CLICKED, NULL);

    return cont;
}

void page_seeting()
{
    com_style_init();
    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), LV_PART_MAIN); // 设置背景颜色为黑色
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    init_back_view(cont);

    lv_obj_t *cont1 = lv_obj_create(cont);
    lv_obj_set_size(cont1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont1, &com_style, LV_PART_MAIN);
    lv_obj_align(cont1, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *liangdu_seeting = init_imag_text(cont1, GET_IMAGE_PATH("icon_brightness.png"), "亮度");
    lv_obj_t *yinliang_seeting = init_imag_text(cont1, GET_IMAGE_PATH("icon_volume.png"), "音量");
    lv_obj_align(liangdu_seeting, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align(yinliang_seeting, LV_ALIGN_TOP_LEFT, 0, 40);

    lv_obj_t *slider1 = init_slider(cont1, liangdu_seeting, 0x00ff00, 0x00ff00, 0x00ff00, 3);
    lv_obj_t *slider2 = init_slider(cont1, yinliang_seeting, 0x00ff00, 0x00ff00, 0x00ff00, 3);
}