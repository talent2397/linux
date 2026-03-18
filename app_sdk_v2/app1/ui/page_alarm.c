#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"
#include "music_conf.h"
#include "audio_player_async.h"

static lv_style_t com_style;

// 全局控件指针，方便按钮点击时直接获取它们的值
static lv_obj_t *label_top;
static lv_obj_t *roller_countdown; // 倒计时滚轮
static lv_obj_t *roller_hour;      // 定时-小时滚轮
static lv_obj_t *roller_min;       // 定时-分钟滚轮

// 闹钟管理相关变量
typedef struct
{
    bool active;          // 闹钟是否激活
    time_t target_time;   // 目标时间戳
    int type;             // 闹钟类型：0-倒计时，1-定时
    char description[64]; // 闹钟描述
} alarm_t;

static alarm_t alarm1 = {false, 0, 0, ""};
static alarm_t alarm2 = {false, 0, 0, ""};
static lv_timer_t *alarm_timer = NULL;

// 封装字库获取函数
static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}

// 返回主界面
static void lv_event_cb_func(lv_event_t *e)
{
    lv_obj_clean(lv_scr_act());
    page_test_init();
    // page_alarm_dialog();
}

// ======================= 按钮点击事件 =======================

/* 设置倒计时闹钟 */
static void set_countdown_alarm(int minutes)
{
    time_t current_time;
    time(&current_time);

    // 设置闹钟1（如果未激活）
    if (!alarm1.active)
    {
        alarm1.active = true;
        alarm1.target_time = current_time + minutes * 60;
        alarm1.type = 0;
        snprintf(alarm1.description, sizeof(alarm1.description), "%d分钟后提醒", minutes);
        lv_label_set_text_fmt(label_top, "闹钟1已设置：%d分钟后提醒", minutes);
    }
    // 设置闹钟2（如果闹钟1已激活且闹钟2未激活）
    else if (!alarm2.active)
    {
        alarm2.active = true;
        alarm2.target_time = current_time + minutes * 60;
        alarm2.type = 0;
        snprintf(alarm2.description, sizeof(alarm2.description), "%d分钟后提醒", minutes);
        lv_label_set_text_fmt(label_top, "闹钟2已设置：%d分钟后提醒", minutes);
    }
    // 两个闹钟都已激活
    else
    {
        lv_label_set_text(label_top, "闹钟已满，请先取消一个闹钟");
    }
}

/* 设置定时闹钟 */
static void set_absolute_alarm(int hour, int minute)
{
    time_t current_time;
    time(&current_time);
    struct tm *timeinfo = localtime(&current_time);

    // 设置目标时间
    struct tm target_tm = *timeinfo;
    target_tm.tm_hour = hour;
    target_tm.tm_min = minute;
    target_tm.tm_sec = 0;

    time_t target_time = mktime(&target_tm);

    // 如果设置的时间已经过去，则设置为明天的同一时间
    if (target_time <= current_time)
    {
        target_time += 24 * 60 * 60; // 加一天
    }

    // 设置闹钟1（如果未激活）
    if (!alarm1.active)
    {
        alarm1.active = true;
        alarm1.target_time = target_time;
        alarm1.type = 1;
        snprintf(alarm1.description, sizeof(alarm1.description), "%02d:%02d提醒", hour, minute);
        lv_label_set_text_fmt(label_top, "闹钟1已设置：%02d:%02d提醒", hour, minute);
    }
    // 设置闹钟2（如果闹钟1已激活且闹钟2未激活）
    else if (!alarm2.active)
    {
        alarm2.active = true;
        alarm2.target_time = target_time;
        alarm2.type = 1;
        snprintf(alarm2.description, sizeof(alarm2.description), "%02d:%02d提醒", hour, minute);
        lv_label_set_text_fmt(label_top, "闹钟2已设置：%02d:%02d提醒", hour, minute);
    }
    // 两个闹钟都已激活
    else
    {
        lv_label_set_text(label_top, "闹钟已满，请先取消一个闹钟");
    }
}

/* 播放闹钟提示音 */
static void play_alarm_sound(void)
{
    start_play_audio_async(GET_MUSIC_PATH("audio_finish2.wav"));
}

/* 取消闹钟1按钮回调 */
static void cancel_alarm1_cb(lv_event_t *e)
{
    alarm1.active = false;
    lv_label_set_text(label_top, "闹钟1已取消");
}

/* 取消闹钟2按钮回调 */
static void cancel_alarm2_cb(lv_event_t *e)
{
    alarm2.active = false;
    lv_label_set_text(label_top, "闹钟2已取消");
}

/* 闹钟定时器回调函数 */
static void alarm_timer_cb(lv_timer_t *timer)
{
    time_t current_time;
    time(&current_time);

    // 检查闹钟1
    if (alarm1.active && current_time >= alarm1.target_time)
    {
        lv_label_set_text_fmt(label_top, "闹钟1时间到：%s", alarm1.description);
        play_alarm_sound();
        alarm1.active = false; // 闹钟触发后关闭
    }

    // 检查闹钟2
    if (alarm2.active && current_time >= alarm2.target_time)
    {
        lv_label_set_text_fmt(label_top, "闹钟2时间到：%s", alarm2.description);
        play_alarm_sound();
        alarm2.active = false; // 闹钟触发后关闭
    }
}

// 倒计时设置按钮
static void btn_click_countdown_cb(lv_event_t *e)
{
    int min = lv_roller_get_selected(roller_countdown); // 直接获取滚轮当前值
    set_countdown_alarm(min);
}

// 定时设置按钮
static void btn_click_absolute_cb(lv_event_t *e)
{
    int h = lv_roller_get_selected(roller_hour);
    int m = lv_roller_get_selected(roller_min);
    set_absolute_alarm(h, m);
}

// ======================= 基础UI组件封装 =======================

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

// 提取的公共 Flex 容器，用于减少重复代码
static lv_obj_t *create_flex_row_cont(lv_obj_t *parent, int pad_column)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(cont, pad_column, 0);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return cont;
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
    return btn;
}

// ======================= 滚轮与页面模块 =======================

static void roller_del_cb(lv_event_t *e)
{
    char *options_str = (char *)lv_event_get_user_data(e);
    if (options_str != NULL)
        free(options_str);
}

static lv_obj_t *init_gunlun(lv_obj_t *parent, int min_val, int max_val, bool zero_pad)
{
    lv_obj_t *roller = lv_roller_create(parent);
    int buf_size = (max_val - min_val + 1) * 5 + 1;
    char *options_str = (char *)malloc(buf_size);
    memset(options_str, 0, buf_size);

    char tmp[10];
    for (int i = min_val; i <= max_val; i++)
    {
        snprintf(tmp, sizeof(tmp), zero_pad ? "%02d" : "%d", i);
        strcat(options_str, tmp);
        if (i < max_val)
            strcat(options_str, "\n");
    }

    lv_roller_set_options(roller, options_str, LV_ROLLER_MODE_NORMAL);
    lv_obj_add_event_cb(roller, roller_del_cb, LV_EVENT_DELETE, options_str);
    lv_obj_center(roller);

    lv_obj_set_style_text_font(roller, get_font(FONT_TYPE_CN, 48), LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller, get_font(FONT_TYPE_CN, 24), LV_PART_MAIN);

    lv_obj_set_style_bg_color(roller, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x000000), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_hex(0xffffff), LV_PART_SELECTED);
    lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN);
    lv_obj_clear_state(roller, LV_STATE_FOCUS_KEY);
    lv_roller_set_visible_row_count(roller, 3);

    return roller;
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
    lv_img_set_src(menu_img, GET_IMAGE_PATH("icon_alarm.png"));
    lv_obj_set_align(menu_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_top(menu_img, 20, 0);
    lv_obj_align_to(menu_img, back_img, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_t *title = lv_label_create(cont);
    obj_font_set(title, FONT_TYPE_CN, 24);
    lv_label_set_text(title, "闹钟设置");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_align_to(title, menu_img, LV_ALIGN_OUT_RIGHT_MID, 20, 3);

    lv_obj_add_event_cb(cont, lv_event_cb_func, LV_EVENT_CLICKED, NULL);
    return cont;
}

// ======================= 页面主函数 =======================

void page_alarm()
{
    com_style_init();

    // 初始化闹钟定时器（每秒检查一次）
    if (alarm_timer == NULL)
    {
        alarm_timer = lv_timer_create(alarm_timer_cb, 1000, NULL);
    }

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);

    lv_obj_t *bg_time = lv_img_create(cont);
    lv_img_set_src(bg_time, GET_IMAGE_PATH("bg_tomato_time.png"));
    lv_obj_align(bg_time, LV_ALIGN_RIGHT_MID, 0, 0);

    // 顶部提示标签
    label_top = lv_label_create(cont);
    obj_font_set(label_top, FONT_TYPE_CN, 40);
    lv_obj_set_style_text_color(label_top, lv_color_hex(0xffffff), 0);
    lv_obj_align(label_top, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(label_top, "");

    init_back_view(cont);

    // --- 区域 1：倒计时设置 ---
    lv_obj_t *alarm_min_cont = create_flex_row_cont(cont, 20);
    lv_obj_align(alarm_min_cont, LV_ALIGN_LEFT_MID, 200, 0);

    lv_obj_t *icon1 = lv_img_create(alarm_min_cont);
    lv_img_set_src(icon1, GET_IMAGE_PATH("icon_alarm.png"));

    roller_countdown = init_gunlun(alarm_min_cont, 0, 30, false);

    lv_obj_t *label1 = lv_label_create(alarm_min_cont);
    obj_font_set(label1, FONT_TYPE_CN, 20);
    lv_obj_set_style_text_color(label1, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label1, "分钟后提醒我");

    lv_obj_t *btn1 = init_select_btn(alarm_min_cont, 150, 50, 35, "设置");
    lv_obj_add_event_cb(btn1, btn_click_countdown_cb, LV_EVENT_SHORT_CLICKED, NULL);

    // --- 区域 2：定时设置 ---
    lv_obj_t *alarm_hour_cont = create_flex_row_cont(cont, 10);
    lv_obj_align_to(alarm_hour_cont, alarm_min_cont, LV_ALIGN_OUT_RIGHT_MID, 100, -80);

    lv_obj_t *icon2 = lv_img_create(alarm_hour_cont);
    lv_img_set_src(icon2, GET_IMAGE_PATH("icon_alarm.png"));

    roller_hour = init_gunlun(alarm_hour_cont, 0, 23, true);

    lv_obj_t *label_colon = lv_label_create(alarm_hour_cont);
    obj_font_set(label_colon, FONT_TYPE_CN, 20);
    lv_obj_set_style_text_color(label_colon, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label_colon, ":");

    roller_min = init_gunlun(alarm_hour_cont, 0, 59, true);

    lv_obj_t *label2 = lv_label_create(alarm_hour_cont);
    obj_font_set(label2, FONT_TYPE_CN, 20);
    lv_obj_set_style_text_color(label2, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label2, "提醒我");

    lv_obj_t *btn2 = init_select_btn(cont, 150, 50, 35, "设置");
    lv_obj_align_to(btn2, alarm_hour_cont, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_obj_add_event_cb(btn2, btn_click_absolute_cb, LV_EVENT_SHORT_CLICKED, NULL);

    // 添加取消闹钟按钮
    lv_obj_t *cancel_btn1 = init_select_btn(cont, 150, 50, 35, "取消闹钟1");
    lv_obj_align_to(cancel_btn1, alarm_min_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(cancel_btn1, cancel_alarm1_cb, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t *cancel_btn2 = init_select_btn(cont, 150, 50, 35, "取消闹钟2");
    lv_obj_align_to(cancel_btn2, alarm_hour_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(cancel_btn2, cancel_alarm2_cb, LV_EVENT_SHORT_CLICKED, NULL);
}