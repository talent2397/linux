#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"
#include "http_manager.h"

static lv_style_t com_style;
static lv_obj_t *kb = NULL;
static lv_obj_t *g_ta_search = NULL;

extern void page_music_list_with_search(const char *keyword);

typedef struct
{
    char **items;
    int count;
} ApiResult_t;

static ApiResult_t api_get_search_history()
{
    ApiResult_t res;
    static char *default_history[] = {"周杰伦", "漠河舞厅", "BGM", "陈奕迅"};
    res.items = default_history;
    res.count = 4;
    return res;
}

static ApiResult_t api_get_hot_search()
{
    ApiResult_t res;
    static char *default_hot[] = {
        "1. 悬溺 - 汪苏泷", "2. 乌梅子酱 - 李荣浩", "3. 孤勇者 - 陈奕迅",
        "4. 花海 - 周杰伦", "5. 晚风心里吹", "6. 起风了",
        "7. 诀爱 - 詹雯婷", "8. 告白气球"};
    res.items = default_hot;
    res.count = 8;
    return res;
}

static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}

static void cleanup_before_exit(void)
{
    if (kb != NULL)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_del(kb);
        kb = NULL;
    }
    g_ta_search = NULL;
}

static void lv_event_cb_func(lv_event_t *e)
{
    cleanup_before_exit();
    lv_obj_clean(lv_scr_act());
    page_test_init();
}

static void icon_click_handler(lv_event_t *e)
{
    const char *icon_name = lv_event_get_user_data(e);
    if (icon_name == NULL)
        return;

    if (strcmp(icon_name, "icon_bofanglist") == 0)
    {
        cleanup_before_exit();
        lv_obj_clean(lv_scr_act());
        page_music_list_with_search(NULL);
    }
    else if (strcmp(icon_name, "icon_bofang") == 0)
    {
        cleanup_before_exit();
        lv_obj_clean(lv_scr_act());
        page_music_ing();
    }
}

static void search_btn_click_handler(lv_event_t *e)
{
    lv_obj_t *ta = g_ta_search;
    if (ta == NULL)
        ta = lv_event_get_user_data(e);
    const char *keyword = lv_textarea_get_text(ta);
    if (keyword && strlen(keyword) > 0)
    {
        // ★ 核心修复 1：把输入的文字拷贝一份存到安全区
        char *safe_keyword = strdup(keyword);

        cleanup_before_exit();
        lv_obj_clean(lv_scr_act()); // 清除UI（此刻旧 keyword 指针彻底失效）

        // 使用安全的副本传递
        page_music_list_with_search(safe_keyword);
        free(safe_keyword); // 用完后释放内存
    }
}

static void tag_click_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if (label)
    {
        const char *text = lv_label_get_text(label);
        const char *search_text = text;

        while (*search_text && ((*search_text >= '0' && *search_text <= '9') || *search_text == '.' || *search_text == ' '))
        {
            if (*search_text == '.' || *search_text == ' ')
            {
                search_text++;
                if (strncmp(search_text, "- ", 2) == 0)
                    search_text += 2;
                break;
            }
            search_text++;
        }

        while (*search_text == ' ')
            search_text++;

        if (strlen(search_text) > 0)
        {
            // ★ 核心修复 2：深拷贝
            char *safe_keyword = strdup(search_text);

            cleanup_before_exit();
            lv_obj_clean(lv_scr_act());

            page_music_list_with_search(safe_keyword);
            free(safe_keyword);
        }
    }
}

static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED)
    {
        if (kb == NULL)
        {
            kb = lv_keyboard_create(lv_layer_top());
            lv_obj_set_size(kb, 800, 220);
            lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
            obj_font_set(kb, FONT_TYPE_CN, 16);
        }
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(kb, ta);
    }
    else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_CANCEL || code == LV_EVENT_READY)
    {
        if (kb != NULL)
        {
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static lv_obj_t *create_tag_btn(lv_obj_t *parent, const char *text, lv_color_t text_color)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_height(btn, 36);
    lv_obj_set_style_pad_left(btn, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, 16, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_make(128, 128, 128), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, 50, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 18, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(btn);
    obj_font_set(label, FONT_TYPE_CN, 14);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, text_color, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

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
    lv_obj_set_style_pad_left(back_img, 20, 0);
    lv_obj_set_style_pad_top(back_img, 20, 0);

    lv_obj_add_event_cb(cont, lv_event_cb_func, LV_EVENT_CLICKED, NULL);
    return cont;
}

void page_music_search()
{
    com_style_init();
    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    init_back_view(cont);

    lv_obj_t *cont_bg1 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont_bg1, 106, 220);
    lv_obj_align(cont_bg1, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(cont_bg1, lv_color_make(48, 47, 47), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_bg1, 80, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg1, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont_bg1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_bg1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(cont_bg1, 30, LV_PART_MAIN);

    lv_obj_t *icon1 = lv_img_create(cont_bg1);
    lv_img_set_src(icon1, GET_IMAGE_PATH("icon_sousuolist.png"));
    lv_obj_add_flag(icon1, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icon2 = lv_img_create(cont_bg1);
    lv_img_set_src(icon2, GET_IMAGE_PATH("icon_unbofanglist.png"));
    lv_obj_add_flag(icon2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon2, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofanglist");

    lv_obj_t *icon3 = lv_img_create(cont_bg1);
    lv_img_set_src(icon3, GET_IMAGE_PATH("icon_unbofang.png"));
    lv_obj_add_flag(icon3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon3, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofang");

    lv_obj_t *cont_bg2 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont_bg2, 1318, 400);
    lv_obj_align_to(cont_bg2, cont_bg1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_style_bg_color(cont_bg2, lv_color_make(23, 20, 20), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_bg2, 80, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg2, 0, LV_PART_MAIN);

    lv_obj_t *icon4 = lv_img_create(cont_bg2);
    lv_img_set_src(icon4, GET_IMAGE_PATH("icon_sousuo.png"));
    lv_obj_align(icon4, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *sousuo_ta = lv_textarea_create(cont_bg2);
    g_ta_search = sousuo_ta;
    lv_obj_set_size(sousuo_ta, 650, 50);
    lv_textarea_set_one_line(sousuo_ta, true);
    obj_font_set(sousuo_ta, FONT_TYPE_CN, 16);
    lv_textarea_set_placeholder_text(sousuo_ta, "搜索歌手，歌曲，专辑...");
    lv_obj_align_to(sousuo_ta, icon4, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(sousuo_ta, lv_color_make(128, 128, 128), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sousuo_ta, 50, LV_PART_MAIN);
    lv_obj_set_style_radius(sousuo_ta, 25, LV_PART_MAIN);
    lv_obj_set_style_pad_left(sousuo_ta, 15, LV_PART_MAIN);
    lv_obj_set_style_text_color(sousuo_ta, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_add_event_cb(sousuo_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *search_btn = lv_btn_create(cont_bg2);
    lv_obj_set_size(search_btn, 100, 50);
    lv_obj_align_to(search_btn, sousuo_ta, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(search_btn, lv_color_make(128, 128, 128), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(search_btn, 50, LV_PART_MAIN);
    lv_obj_set_style_radius(search_btn, 25, LV_PART_MAIN);
    lv_obj_add_event_cb(search_btn, search_btn_click_handler, LV_EVENT_CLICKED, sousuo_ta);

    lv_obj_t *btn_label = lv_label_create(search_btn);
    obj_font_set(btn_label, FONT_TYPE_CN, 20);
    lv_label_set_text(btn_label, "搜索");
    lv_obj_align(btn_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *label = lv_label_create(cont_bg2);
    lv_obj_align_to(label, sousuo_ta, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 30);
    obj_font_set(label, FONT_TYPE_CN, 20);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, "#f8f8f8 搜索历史#");

    lv_obj_t *history_flex = lv_obj_create(cont_bg2);
    lv_obj_clear_flag(history_flex, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(history_flex, 760, 150);
    lv_obj_align_to(history_flex, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    lv_obj_set_style_bg_opa(history_flex, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(history_flex, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(history_flex, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(history_flex, 15, LV_PART_MAIN);

    ApiResult_t history_data = api_get_search_history();
    for (int i = 0; i < history_data.count; i++)
    {
        lv_obj_t *btn = create_tag_btn(history_flex, history_data.items[i], lv_color_hex(0xffffff));
        lv_obj_add_event_cb(btn, tag_click_handler, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *hot_title = lv_label_create(cont_bg2);
    obj_font_set(hot_title, FONT_TYPE_CN, 20);
    lv_label_set_recolor(hot_title, true);
    lv_label_set_text(hot_title, "#f8f8f8 热门搜索排行榜#");
    lv_obj_align_to(hot_title, btn_label, LV_ALIGN_OUT_RIGHT_MID, 60, -10);

    lv_obj_t *hot_flex = lv_obj_create(cont_bg2);
    lv_obj_clear_flag(hot_flex, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(hot_flex, 480, 250);
    lv_obj_align_to(hot_flex, hot_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 25);
    lv_obj_set_style_bg_opa(hot_flex, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(hot_flex, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(hot_flex, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(hot_flex, 15, LV_PART_MAIN);

    ApiResult_t hot_data = api_get_hot_search();
    for (int i = 0; i < hot_data.count; i++)
    {
        lv_color_t color = (i == 0) ? lv_color_hex(0xff4d4f) : ((i == 1) ? lv_color_hex(0xff7a45) : ((i == 2) ? lv_color_hex(0xffa940) : lv_color_hex(0xffffff)));
        lv_obj_t *btn = create_tag_btn(hot_flex, hot_data.items[i], color);
        lv_obj_add_event_cb(btn, tag_click_handler, LV_EVENT_CLICKED, NULL);
    }
}