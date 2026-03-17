#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"
#include "wpa_manager.h"
#include "http_manager.h"

static lv_style_t com_style;
static lv_obj_t *time_label;
static lv_obj_t *weather_label;
static char weather_ifo[100];
static lv_obj_t *wifi_icon = NULL;

/* 【新增】当前全局城市拼音，默认重庆 */
static char current_city[32] = "chongqing";

/* 网络时间同步相关变量 */
static bool use_network_time = false;
static bool network_time_initialized = false;
static uint32_t sync_tick = 0;
static time_t sync_net_time = 0;

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

/* ================== 【新增】供外部调用的更新城市接口 ================== */
void update_weather_city(const char *city_pinyin)
{
    // 如果城市变了，才重新请求
    if (strcmp(current_city, city_pinyin) != 0)
    {
        strncpy(current_city, city_pinyin, sizeof(current_city) - 1);
        current_city[sizeof(current_city) - 1] = '\0';

        // 清空旧数据，给用户正在刷新的视觉反馈
        strcpy(weather_ifo, "切换城市中...");

        // 立即发起新城市的请求
        http_get_weather_async("SPmMXp8vqCtnT4TpM", current_city);
    }
}
/* ==================================================================== */

/* 天气数据回调函数 - 在后台网络线程中触发 */
static void weather_callback_func(char *data)
{
    if (data != NULL)
    {
        strcpy(weather_ifo, data);
    }
}

/* 网络时间回调函数 - 在后台网络线程中触发 */
static void network_time_callback(char *data)
{
    if (data == NULL)
        return;

    const char *key = "servertime=";
    char *pos = strstr(data, key);

    if (pos != NULL)
    {
        pos += strlen(key);
        time_t timestamp = (time_t)atoll(pos);

        if (timestamp > 0)
        {
            sync_net_time = timestamp;
            sync_tick = lv_tick_get();
            use_network_time = true;
            network_time_initialized = true;
        }
    }
}

/* 菜单项点击事件处理函数 */
static void lv_event_cb_func(lv_event_t *e)
{
    menu_id_t id = (menu_id_t)(uintptr_t)lv_event_get_user_data(e);
    switch (id)
    {
    case MENU_SMALL_GAME:
        break;
    case MENU_BLUETOOTH_SPEAKER:
        lv_obj_clean(lv_scr_act());
        page_yingxiang_setting();
        break;
    case MENU_DIAL_SETTING:
        lv_obj_clean(lv_scr_act());
        page_time_setting1();
        break;
    case MENU_CITY_SETTING:
        lv_obj_clean(lv_scr_act());
        page_city_setting();
        break;
    case MENU_TOMATO_CLOCK:
        lv_obj_clean(lv_scr_act());
        page_tomato_setting();
        break;
    case MENU_WIFI_SETTING:
        lv_obj_clean(lv_scr_act());
        page_wifi_setting();
        break;
    case MENU_ALARM_SETTING:
        lv_obj_clean(lv_scr_act());
        page_alarm();
        break;
    case MENU_SYSTEM_SETTING:
        lv_obj_clean(lv_scr_act());
        page_seeting();
        break;
    }
}

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

static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}

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

static lv_obj_t *txt_info_view(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    time_label = lv_label_create(cont);
    obj_font_set(time_label, FONT_TYPE_CN, 60);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xffffff), 0);

    time_t now_time;
    if (use_network_time && network_time_initialized)
    {
        uint32_t elapsed_sec = (lv_tick_get() - sync_tick) / 1000;
        now_time = sync_net_time + elapsed_sec;
    }
    else
    {
        time(&now_time);
    }

    struct tm *timeinfo = localtime(&now_time);
    if (timeinfo != NULL)
    {
        lv_label_set_text_fmt(time_label, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    }
    else
    {
        lv_label_set_text(time_label, "--:--");
    }

    weather_label = lv_label_create(cont);
    obj_font_set(weather_label, FONT_TYPE_CN, 24);
    lv_obj_set_style_text_color(weather_label, lv_color_hex(0xffffff), 0);

    if (strlen(weather_ifo) > 0)
    {
        lv_label_set_text(weather_label, weather_ifo);
    }
    else
    {
        lv_label_set_text(weather_label, "获取天气中...");
    }

    return cont;
}

static void update_wifi_icon(WPA_WIFI_CONNECT_STATUS_E status)
{
    if (wifi_icon == NULL)
        return;

    if (status == WPA_WIFI_CONNECT)
    {
        lv_img_set_src(wifi_icon, GET_IMAGE_PATH("icon_wifi_connect.png"));
        // 【修改】使用动态城市变量代替写死的"chongqing"
        http_get_weather_async("SPmMXp8vqCtnT4TpM", current_city);
        http_get_time_async("http://tptm.hd.mi.com/gettimestamp");
    }
    else
    {
        lv_img_set_src(wifi_icon, GET_IMAGE_PATH("icon_wifi_disconnect.png"));
    }
}

void wifi_connect_status_callback(WPA_WIFI_CONNECT_STATUS_E status)
{
    update_wifi_icon(status);
}

static void network_sync_timer_cb(lv_timer_t *timer)
{
    http_get_time_async("http://tptm.hd.mi.com/gettimestamp");
}

void timer_cb_func(lv_timer_t *timer)
{
    if (time_label == NULL || !lv_obj_is_valid(time_label))
        return;

    time_t now_time;
    if (use_network_time && network_time_initialized)
    {
        uint32_t elapsed_sec = (lv_tick_get() - sync_tick) / 1000;
        now_time = sync_net_time + elapsed_sec;
    }
    else
    {
        time(&now_time);
    }

    struct tm *timeinfo = localtime(&now_time);
    if (timeinfo != NULL)
    {
        lv_label_set_text_fmt(time_label, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    }

    if (weather_label != NULL && strlen(weather_ifo) > 0)
    {
        lv_label_set_text(weather_label, weather_ifo);
    }
}

static void timer_delete_event_cb(lv_event_t *e)
{
    lv_timer_t *timer = (lv_timer_t *)lv_event_get_user_data(e);
    if (timer)
        lv_timer_del(timer);
}

void init_timer(void)
{
    lv_timer_t *timer = lv_timer_create(timer_cb_func, 1000, NULL);
    lv_obj_add_event_cb(time_label, timer_delete_event_cb, LV_EVENT_DELETE, timer);

    lv_timer_t *network_timer = lv_timer_create(network_sync_timer_cb, 3600000, NULL);
    lv_obj_add_event_cb(time_label, timer_delete_event_cb, LV_EVENT_DELETE, network_timer);
}

void page_test_init()
{
    com_style_init();

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    http_set_weather_callback(weather_callback_func);
    http_set_time_callback(network_time_callback);

    static bool is_first_enter = true;
    if (is_first_enter)
    {
        // 【修改】使用动态城市变量代替写死的"chongqing"
        http_get_weather_async("SPmMXp8vqCtnT4TpM", current_city);
        http_get_time_async("http://tptm.hd.mi.com/gettimestamp");
        is_first_enter = false;
    }

    if (wifi_icon == NULL)
    {
        wifi_icon = lv_img_create(lv_layer_top());
        lv_img_set_src(wifi_icon, GET_IMAGE_PATH("icon_wifi_disconnect.png"));
        lv_obj_align(wifi_icon, LV_ALIGN_TOP_RIGHT, -30, 10);
    }

    lv_obj_t *img_bg = lv_img_create(cont);
    lv_img_set_src(img_bg, GET_IMAGE_PATH("icon_user.png"));
    lv_img_set_zoom(img_bg, 256);
    lv_obj_align(img_bg, LV_ALIGN_LEFT_MID, 30, 0);

    lv_obj_t *time_weather = txt_info_view(cont);
    lv_obj_align_to(time_weather, img_bg, LV_ALIGN_OUT_RIGHT_MID, 50, -10);

    init_timer();

    lv_obj_t *img_text1 = init_image_view(cont);
    lv_obj_align_to(img_text1, time_weather, LV_ALIGN_OUT_RIGHT_MID, 80, -8);
}