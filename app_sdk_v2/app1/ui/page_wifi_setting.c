#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"
#include "wpa_manager.h"

static lv_style_t com_style;

// ======================= 全局状态管理 =======================

// 当前激活的文本框指针
static lv_obj_t *active_textarea = NULL;

// WiFi连接状态
static bool wifi_connecting = false;
static bool wifi_connected = false;
static lv_timer_t *connect_timer = NULL;
static lv_obj_t *status_label = NULL;

// WiFi账号和密码输入框
static lv_obj_t *ssid_ta = NULL;
static lv_obj_t *psw_ta = NULL;

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

// 声明外部的WiFi连接状态回调函数
extern void wifi_connect_status_callback(WPA_WIFI_CONNECT_STATUS_E status);

// WiFi连接状态回调函数
static void wifi_setting_connect_status_callback(WPA_WIFI_CONNECT_STATUS_E status)
{
    if (status == WPA_WIFI_CONNECT)
    {
        wifi_connecting = false;
        wifi_connected = true;
        if (connect_timer) {
            lv_timer_del(connect_timer);
            connect_timer = NULL;
        }
        if (status_label) {
            lv_label_set_text(status_label, "连接成功");
            lv_obj_set_style_text_color(status_label, lv_color_hex(0x00ff00), 0);
        }
        printf("WiFi连接成功\n");
    }
    else if (status == WPA_WIFI_DISCONNECT)
    {
        wifi_connecting = false;
        wifi_connected = false;
        if (!connect_timer) {
            // 只有在非连接过程中的断开才显示错误
            if (status_label) {
                lv_label_set_text(status_label, "连接失败，请重试");
                lv_obj_set_style_text_color(status_label, lv_color_hex(0xff0000), 0);
            }
        }
        printf("WiFi连接失败\n");
    }
    
    // 调用外部的回调函数，更新WiFi图标和天气时间
    wifi_connect_status_callback(status);
}

// WiFi连接超时处理函数
static void wifi_connect_timeout(lv_timer_t *timer)
{
    if (wifi_connecting) {
        wifi_connecting = false;
        if (status_label) {
            lv_label_set_text(status_label, "连接超时，请重试");
            lv_obj_set_style_text_color(status_label, lv_color_hex(0xff0000), 0);
        }
        printf("WiFi连接超时\n");
    }
    connect_timer = NULL;
}

// 开始连接WiFi按钮点击事件处理
static void btn_connect_event_cb(lv_event_t *e)
{
    if (wifi_connecting) return;

    // 获取输入的WiFi账号和密码
    const char *ssid = lv_textarea_get_text(ssid_ta);
    const char *psw = lv_textarea_get_text(psw_ta);

    if (!ssid || !psw || strlen(ssid) == 0 || strlen(psw) == 0) {
        if (status_label) {
            lv_label_set_text(status_label, "请输入WiFi账号和密码");
            lv_obj_set_style_text_color(status_label, lv_color_hex(0xffff00), 0);
        }
        return;
    }

    // 设置连接状态
    wifi_connecting = true;
    wifi_connected = false;
    if (status_label) {
        lv_label_set_text(status_label, "正在连接...");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00ffff), 0);
    }

    // 准备WiFi连接信息
    wpa_ctrl_wifi_info_t wifi_info;
    memset(&wifi_info, 0, sizeof(wpa_ctrl_wifi_info_t));
    strcpy(wifi_info.ssid, ssid);
    strcpy(wifi_info.psw, psw);

    printf("准备连接 WiFi, SSID: %s\n", wifi_info.ssid);
    wpa_manager_wifi_connect(&wifi_info);

    // 启动10秒超时定时器
    if (connect_timer) {
        lv_timer_del(connect_timer);
    }
    connect_timer = lv_timer_create(wifi_connect_timeout, 10000, NULL);
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

    // 注册WiFi连接状态回调函数
    wpa_manager_add_callback(NULL, wifi_setting_connect_status_callback);

    lv_obj_t *cont1 = lv_obj_create(cont);
    lv_obj_set_size(cont1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont1, &com_style, LV_PART_MAIN);
    lv_obj_align(cont1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cont1, 40, LV_PART_MAIN);

    // 创建WiFi账号输入框
    ssid_ta = lv_textarea_create(cont);
    lv_textarea_set_one_line(ssid_ta, true);
    lv_textarea_set_placeholder_text(ssid_ta, "input your user");
    lv_obj_add_event_cb(ssid_ta, ta_focus_event_cb, LV_EVENT_ALL, NULL);

    // 创建WiFi密码输入框
    psw_ta = lv_textarea_create(cont);
    lv_textarea_set_one_line(psw_ta, true);
    lv_textarea_set_placeholder_text(psw_ta, "input your code");
    lv_obj_add_event_cb(psw_ta, ta_focus_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *wifi_user = init_imag_text(cont1, GET_IMAGE_PATH("icon_2.png"), "WiFi账号:");
    lv_obj_align_to(ssid_ta, wifi_user, LV_ALIGN_OUT_RIGHT_MID, 10, -30);
    lv_obj_t *wifi_code = init_imag_text(cont1, GET_IMAGE_PATH("icon_3.png"), "WiFi密码:");
    lv_obj_align_to(psw_ta, wifi_code, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 创建开始连接按钮
    lv_obj_t *btn = init_select_btn(cont, 151, 66, 35, "开始连接", 30, 0, -5);
    lv_obj_add_event_cb(btn, btn_connect_event_cb, LV_EVENT_CLICKED, NULL);

    // 创建状态标签
    status_label = lv_label_create(cont);
    obj_font_set(status_label, FONT_TYPE_CN, 20);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(status_label, "请输入WiFi账号和密码");
    lv_obj_align_to(status_label, psw_ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

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
    lv_obj_align_to(kb, ssid_ta, LV_ALIGN_OUT_RIGHT_MID, 0, 40);
    // 放在label设置后才能居中对齐
    lv_obj_align_to(btn, kb, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    // 为文本框添加点击事件，显示键盘
    lv_obj_add_event_cb(ssid_ta, kb_show_event_cb, LV_EVENT_CLICKED, kb);
    lv_obj_add_event_cb(psw_ta, kb_show_event_cb, LV_EVENT_CLICKED, kb);
}