#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"

// 【新增】声明主页面中的城市更新接口
extern void update_weather_city(const char *city_pinyin);

static lv_style_t com_style;

// ======================= 全局状态管理 =======================

// 当前选择的城市（默认与 page_main.c 一致，设为重庆）
static char selected_city[20] = "重庆";
static lv_obj_t *city_roller = NULL;
static lv_obj_t *update_label = NULL;

const char *cities[] = {
    "成都", "重庆", "宜宾", "北京", "上海",
    "广州", "深圳", "杭州", "武汉", "西安"};

// 【新增】中文到拼音的转换函数，保证 HTTP API 稳定调用
static const char *get_city_pinyin(const char *chinese_name)
{
    if (strcmp(chinese_name, "成都") == 0)
        return "chengdu";
    if (strcmp(chinese_name, "重庆") == 0)
        return "chongqing";
    if (strcmp(chinese_name, "宜宾") == 0)
        return "yibin";
    if (strcmp(chinese_name, "北京") == 0)
        return "beijing";
    if (strcmp(chinese_name, "上海") == 0)
        return "shanghai";
    if (strcmp(chinese_name, "广州") == 0)
        return "guangzhou";
    if (strcmp(chinese_name, "深圳") == 0)
        return "shenzhen";
    if (strcmp(chinese_name, "杭州") == 0)
        return "hangzhou";
    if (strcmp(chinese_name, "武汉") == 0)
        return "wuhan";
    if (strcmp(chinese_name, "西安") == 0)
        return "xian";
    return "chongqing"; // 默认兜底
}

// 【新增】根据名称寻找索引，用于界面记忆功能
static int get_city_index(const char *chinese_name)
{
    for (int i = 0; i < 10; i++)
    {
        if (strcmp(cities[i], chinese_name) == 0)
            return i;
    }
    return 1; // 默认返回重庆的索引
}

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

// 滚轮事件处理
static void city_roller_event_handler(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target(e);
    int selected_index = lv_roller_get_selected(roller);

    if (selected_index >= 0 && selected_index < 10)
    {
        strncpy(selected_city, cities[selected_index], sizeof(selected_city) - 1);
        selected_city[sizeof(selected_city) - 1] = '\0';
        printf("当前选择城市: %s\n", selected_city);
    }
}

// 更新城市按钮事件处理
static void update_city_event_cb(lv_event_t *e)
{
    // 更新提示信息
    if (update_label)
    {
        char update_text[50];
        snprintf(update_text, sizeof(update_text), "已更新城市：%s", selected_city);
        lv_label_set_text(update_label, update_text);
    }

    printf("城市已更新为: %s\n", selected_city);

    // 【修改】调用 page_main 提供的更新函数，传递城市的拼音给服务器发请求
    update_weather_city(get_city_pinyin(selected_city));
}

// ======================= UI初始化模块 =======================

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
    lv_img_set_src(menu_img, GET_IMAGE_PATH("icon_city.png"));
    lv_obj_set_align(menu_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_top(menu_img, 20, 0);
    lv_obj_align_to(menu_img, back_img, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_t *title = lv_label_create(cont);
    obj_font_set(title, FONT_TYPE_CN, 24);
    lv_label_set_text(title, "天气城市设置");
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
    lv_img_set_src(bg_time, GET_IMAGE_PATH("bg_weather.png"));
    lv_obj_align(bg_time, LV_ALIGN_RIGHT_MID, 0, 0);

    init_back_view(cont);
    return cont;
}

static lv_obj_t *init_imag_text(lv_obj_t *parent, const char *src, const char *str)
{
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

static lv_obj_t *init_select_btn(lv_obj_t *parent, int lenth, int width, int redius, const char *str, const char size, const char py_x, const char py_y)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &com_style, LV_PART_MAIN);
    lv_obj_set_size(btn, lenth, width);
    lv_obj_clear_state(btn, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, redius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1F94D2), 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    obj_font_set(btn_label, FONT_TYPE_CN, size);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(btn_label, str);
    lv_obj_align(btn_label, LV_ALIGN_CENTER, py_x, py_y);
    return btn;
}

static lv_obj_t *init_gunlun(lv_obj_t *parent)
{
    lv_obj_t *roller1 = lv_roller_create(parent);
    lv_roller_set_options(roller1,
                          "成都\n"
                          "重庆\n"
                          "宜宾\n"
                          "北京\n"
                          "上海\n"
                          "广州\n"
                          "深圳\n"
                          "杭州\n"
                          "武汉\n"
                          "西安\n",
                          LV_ROLLER_MODE_NORMAL);
    lv_obj_center(roller1);
    lv_obj_set_style_bg_color(roller1, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_border_width(roller1, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller1, lv_color_hex(0x000000), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller1, lv_color_hex(0xffffff), LV_PART_SELECTED);
    lv_obj_clear_state(roller1, LV_STATE_FOCUS_KEY);

    lv_obj_set_style_text_font(roller1, get_font(FONT_TYPE_CN, 36), LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller1, get_font(FONT_TYPE_CN, 24), LV_PART_MAIN);

    lv_roller_set_visible_row_count(roller1, 3);

    lv_obj_add_event_cb(roller1, city_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    return roller1;
}

// ======================= 页面主函数 =======================

void page_city_setting()
{
    lv_obj_t *cont = init();

    update_label = lv_label_create(lv_scr_act());
    obj_font_set(update_label, FONT_TYPE_CN, 28);
    lv_obj_set_style_text_color(update_label, lv_color_hex(0xFFA500), 0);
    lv_label_set_text(update_label, "请选择城市");
    lv_obj_align(update_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(update_label, LV_OPA_80, 0);
    lv_obj_set_style_bg_color(update_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(update_label, 15, 0);
    lv_obj_set_style_border_color(update_label, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_radius(update_label, 10, 0);
    lv_obj_move_foreground(update_label);

    lv_obj_t *main_container = lv_obj_create(cont);
    lv_obj_set_size(main_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(main_container, &com_style, LV_PART_MAIN);
    lv_obj_align(main_container, LV_ALIGN_CENTER, 0, 0);

    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(main_container, 30, LV_PART_MAIN);

    lv_obj_t *city_label = init_imag_text(main_container, GET_IMAGE_PATH("icon_city.png"), "城市：");

    city_roller = init_gunlun(main_container);
    // 【修改】动态获取当前缓存的城市索引，防止返回时重置为0
    lv_roller_set_selected(city_roller, get_city_index(selected_city), LV_ANIM_OFF);

    lv_obj_t *update_btn = init_select_btn(main_container, 151, 66, 35, "确定更新城市", 20, 0, 0);
    lv_obj_add_event_cb(update_btn, update_city_event_cb, LV_EVENT_CLICKED, NULL);
}