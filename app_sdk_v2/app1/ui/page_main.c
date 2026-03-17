#include <stdio.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"
#include <time.h>
#include "wpa_manager.h"

static lv_style_t com_style;
static lv_obj_t *time_label;
static lv_obj_t *wifi_icon = NULL; // 【修改】显式初始化为 NULL
static time_t timep;
static struct tm time_temp;
static char time_str[20];

typedef enum
{
    MENU_SMALL_GAME = 0,
    MENU_BLUETOOTH_SPEAKER,
    MENU_DIAL_SETTING,
    MENU_CITY_SETTING,
    MENU_TOMATO_CLOCK,
    MENU_WIFI_SETTING,
    MENU_ALARM_SETTING,
    MENU_SYSTEM_SETTING,
} menu_id_t;

static void lv_event_cb_func(lv_event_t *e)
{
    // 直接从 user_data 拿到编号
    menu_id_t id = (menu_id_t)(uintptr_t)lv_event_get_user_data(e);

    switch (id)
    {
    case MENU_SMALL_GAME:
        printf("小游戏 click\n");
        break;

    case MENU_BLUETOOTH_SPEAKER:
        printf("蓝牙音响 click\n");
        lv_obj_t *act_yinxiang_setting = lv_scr_act();
        lv_obj_clean(act_yinxiang_setting);
        page_yingxiang_setting();
        break;

    case MENU_DIAL_SETTING:
        printf("表盘设置 click\n");
        lv_obj_t *act_time_setting = lv_scr_act();
        lv_obj_clean(act_time_setting);
        page_time_setting1();
        break;

    case MENU_CITY_SETTING:
        printf("城市设置 click\n");
        lv_obj_t *act_city_setting = lv_scr_act();
        lv_obj_clean(act_city_setting);
        page_city_setting();
        break;

    case MENU_TOMATO_CLOCK:
        printf("番茄时钟 click\n");
        lv_obj_t *act_tomato_setting = lv_scr_act();
        lv_obj_clean(act_tomato_setting);
        page_tomato_setting();
        break;

    case MENU_WIFI_SETTING:
        printf("wifi设置 click\n");
        lv_obj_t *act_wifi_setting = lv_scr_act();
        lv_obj_clean(act_wifi_setting);
        page_wifi_setting();
        break;

    case MENU_ALARM_SETTING:
        printf("闹钟设置 click\n");
        lv_obj_t *act_scr_alarm = lv_scr_act();
        lv_obj_clean(act_scr_alarm);
        page_alarm();
        break;

    case MENU_SYSTEM_SETTING:
        printf("系统设置 click\n");
        lv_obj_t *act_scr_setting = lv_scr_act();
        lv_obj_clean(act_scr_setting);
        page_seeting();
        break;
    }
}

// 初始化通用样式
static void com_style_init()
{
    lv_style_init(&com_style);
    if (lv_style_is_empty(&com_style) == false)
        lv_style_reset(&com_style);
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
static lv_obj_t *init_imag_text(lv_obj_t *parent, const char *src, const char *str, menu_id_t id)
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
    lv_obj_align_to(label, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);

    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon, lv_event_cb_func, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    return cont;
}

// 创建一个 图片排列的对象
static lv_obj_t *init_image_view(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(cont, 20, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    init_imag_text(cont, GET_IMAGE_PATH("image_game.png"), "小游戏", MENU_SMALL_GAME);
    init_imag_text(cont, GET_IMAGE_PATH("image_yinxiang.png"), "蓝牙音响", MENU_BLUETOOTH_SPEAKER);
    init_imag_text(cont, GET_IMAGE_PATH("icon_menu_dial.png"), "表盘设置", MENU_DIAL_SETTING);
    init_imag_text(cont, GET_IMAGE_PATH("icon_menu_city.png"), "城市设置", MENU_CITY_SETTING);
    init_imag_text(cont, GET_IMAGE_PATH("icon_menu_tomato_time.png"), "番茄时钟", MENU_TOMATO_CLOCK);
    init_imag_text(cont, GET_IMAGE_PATH("icon_menu_wifi.png"), "wifi设置", MENU_WIFI_SETTING);
    init_imag_text(cont, GET_IMAGE_PATH("icon_menu_time.png"), "闹钟设置", MENU_ALARM_SETTING);
    init_imag_text(cont, GET_IMAGE_PATH("icon_menu_setting.png"), "系统设置", MENU_SYSTEM_SETTING);

    return cont;
}

// 创建一个 按钮 带 文字
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

// 创建一个 文字排列的 对象
static lv_obj_t *txt_info_view(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    time_label = lv_label_create(cont);
    lv_label_set_text(time_label, "08:20");
    obj_font_set(time_label, FONT_TYPE_CN, 60);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xffffff), 0);

    lv_obj_t *weather_label = lv_label_create(cont);
    lv_label_set_text(weather_label, "重庆：晴 15℃");
    obj_font_set(weather_label, FONT_TYPE_CN, 24);
    lv_obj_set_style_text_color(weather_label, lv_color_hex(0xffffff), 0);

    return cont;
}

// WiFi状态更新函数
static void update_wifi_icon(WPA_WIFI_CONNECT_STATUS_E status)
{
    if (wifi_icon == NULL)
    {
        printf("page_main: 错误 - WiFi图标对象为空\n");
        return;
    }

    if (status == WPA_WIFI_CONNECT)
    {
        lv_img_set_src(wifi_icon, GET_IMAGE_PATH("icon_wifi_connect.png"));
        printf("page_main: WiFi状态更新为连接成功\n");
    }
    else
    {
        lv_img_set_src(wifi_icon, GET_IMAGE_PATH("icon_wifi_disconnect.png"));
        printf("page_main: WiFi状态更新为断开/其他状态: %d\n", status);
    }
    lv_obj_invalidate(wifi_icon);
}

// 【修改】移除 static 关键字，让其可以作为 extern 被 main.c 调用
void wifi_connect_status_callback(WPA_WIFI_CONNECT_STATUS_E status)
{
    printf("page_main: WiFi连接状态变化: %d\n", status);
    update_wifi_icon(status);
}

// 获取当地时间
void timer_cb_func(lv_timer_t *timer)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    lv_label_set_text_fmt(time_label, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
}

static void timer_delete_event_cb(lv_event_t *e)
{
    lv_timer_t *timer = (lv_timer_t *)lv_event_get_user_data(e);
    if (timer)
    {
        lv_timer_del(timer);
    }
}

void init_timer(void)
{
    lv_timer_t *timer = lv_timer_create(timer_cb_func, 1000, NULL);
    lv_obj_add_event_cb(time_label, timer_delete_event_cb, LV_EVENT_DELETE, timer);
}

void page_test_init()
{
    com_style_init();

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    // 【核心修改】将WiFi图标创建在系统顶层 (lv_layer_top)，像状态栏一样始终悬浮
    // 增加 == NULL 判断，防止返回主页时重复创建导致内存泄露
    if (wifi_icon == NULL)
    {
        wifi_icon = lv_img_create(lv_layer_top());
        lv_img_set_src(wifi_icon, GET_IMAGE_PATH("icon_wifi_disconnect.png"));
        // 对齐时，右上角向左偏移需给负值 (通常为负x)，保证显示不贴边
        lv_obj_align(wifi_icon, LV_ALIGN_TOP_RIGHT, -30, 10);
    }

    // 设计主页头像
    lv_obj_t *img_bg = lv_img_create(cont);
    lv_img_set_src(img_bg, GET_IMAGE_PATH("icon_user.png"));
    lv_img_set_zoom(img_bg, 256);
    lv_obj_align(img_bg, LV_ALIGN_LEFT_MID, 30, 0);

    // 设计时间和天气
    lv_obj_t *time_weather = txt_info_view(cont);
    lv_obj_align_to(time_weather, img_bg, LV_ALIGN_OUT_RIGHT_MID, 50, -10);

    // 刷新时间
    init_timer();

    lv_obj_t *img_text1 = init_image_view(cont);
    lv_obj_align_to(img_text1, time_weather, LV_ALIGN_OUT_RIGHT_MID, 80, -8);

    // 【修改】删除原有的回调函数注册，统一挪到 main.c 中执行，避免被覆盖
    // wpa_manager_add_callback(NULL, wifi_connect_status_callback);
}