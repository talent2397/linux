#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"

static lv_style_t com_style;
static lv_obj_t *kb = NULL; // 全局顶层悬浮键盘对象

// ======================= API 数据接口模拟 =======================

// 模拟网络返回的数据结构
typedef struct
{
    char **items;
    int count;
} ApiResult_t;

// API接口：获取搜索历史
static ApiResult_t api_get_search_history()
{
    ApiResult_t res;
    static char *default_history[] = {"周杰伦", "漠河舞厅", "BGM", "陈奕迅"};
    res.items = default_history;
    res.count = 4;
    return res;
}

// API接口：获取热门搜索
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

// ======================= 全局状态管理 =======================

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

// 图标点击事件处理函数
static void icon_click_handler(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    const char *icon_name = lv_event_get_user_data(e);

    if (icon_name == NULL)
        return;

    if (strcmp(icon_name, "icon_sousuolist") == 0)
    {
        printf("搜索列表图标被点击\n");
    }
    else if (strcmp(icon_name, "icon_bofanglist") == 0)
    {
        printf("播放列表图标被点击\n");
        lv_obj_clean(lv_scr_act());
        page_music_list();
    }
    else if (strcmp(icon_name, "icon_bofang") == 0)
    {
        printf("播放图标被点击\n");
        lv_obj_clean(lv_scr_act());
        page_music_ing();
    }
    else if (strcmp(icon_name, "icon_delete") == 0)
    {
        printf("删除图标被点击：此处可触发API清除历史记录\n");
    }
}

// 文本框焦点事件回调：控制顶层键盘的弹出与隐藏
static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED)
    {
        if (kb == NULL)
        {
            // 在最顶层创建键盘，防止被遮挡，绝对不全屏
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

// ======================= UI 组件生成器 =======================

// 动态生成药丸状 Tag 按钮
static lv_obj_t *create_tag_btn(lv_obj_t *parent, const char *text, lv_color_t text_color)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE); // 绝对禁止滑动
    lv_obj_set_height(btn, 36);
    lv_obj_set_style_pad_left(btn, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, 16, LV_PART_MAIN);

    // 背景样式（半透明灰，圆角，无边框）
    lv_obj_set_style_bg_color(btn, lv_color_make(128, 128, 128), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, 50, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 18, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);

    // 标签文本
    lv_obj_t *label = lv_label_create(btn);
    obj_font_set(label, FONT_TYPE_CN, 14);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, text_color, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    return btn;
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
    lv_img_set_src(menu_img, GET_IMAGE_PATH("icon_music1.png"));
    lv_obj_set_align(menu_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_top(menu_img, 20, 0);
    lv_obj_align_to(menu_img, back_img, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_t *title = lv_label_create(cont);
    obj_font_set(title, FONT_TYPE_CN, 24);
    lv_label_set_text(title, "音乐播放");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_align_to(title, menu_img, LV_ALIGN_OUT_RIGHT_MID, 20, 3);

    lv_obj_add_event_cb(cont, lv_event_cb_func, LV_EVENT_CLICKED, NULL);

    return cont;
}

static lv_obj_t *init()
{
    com_style_init();

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);

    init_back_view(cont);
    return cont;
}

// ======================= 页面主函数 =======================

void page_music_search()
{
    lv_obj_t *cont = init();

    // ----------------- 左侧侧边栏 cont_bg1 -----------------
    lv_obj_t *cont_bg1 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg1, LV_OBJ_FLAG_SCROLLABLE); // 禁滑
    lv_obj_set_size(cont_bg1, 106, 220);
    lv_obj_align(cont_bg1, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(cont_bg1, lv_color_make(48, 47, 47), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_bg1, 80, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_bg1, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg1, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(cont_bg1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_bg1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont_bg1, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(cont_bg1, 30, LV_PART_MAIN);

    lv_obj_t *icon1 = lv_img_create(cont_bg1);
    lv_img_set_src(icon1, GET_IMAGE_PATH("icon_sousuolist.png"));
    lv_obj_set_style_img_recolor(icon1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_add_flag(icon1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon1, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_sousuolist");

    lv_obj_t *icon2 = lv_img_create(cont_bg1);
    lv_img_set_src(icon2, GET_IMAGE_PATH("icon_unbofanglist.png"));
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_add_flag(icon2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon2, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofanglist");

    lv_obj_t *icon3 = lv_img_create(cont_bg1);
    lv_img_set_src(icon3, GET_IMAGE_PATH("icon_unbofang.png"));
    lv_obj_set_style_img_recolor(icon3, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_add_flag(icon3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon3, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofang");

    // ----------------- 右侧主内容区 cont_bg2 -----------------
    lv_obj_t *cont_bg2 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg2, LV_OBJ_FLAG_SCROLLABLE); // 禁滑
    lv_obj_set_size(cont_bg2, 1318, 220);
    lv_obj_align_to(cont_bg2, cont_bg1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_style_bg_color(cont_bg2, lv_color_make(23, 20, 20), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_bg2, 80, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_bg2, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg2, 0, LV_PART_MAIN);

    // ★ 修正：给外置搜索图标一个基准坐标，否则默认在左上角0,0，排版会很奇怪
    lv_obj_t *icon4 = lv_img_create(cont_bg2);
    lv_img_set_src(icon4, GET_IMAGE_PATH("icon_sousuo.png"));
    lv_obj_set_style_img_recolor(icon4, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(icon4, LV_ALIGN_TOP_LEFT, 0, 0); // 稍微往右下偏移一点

    // 创建搜索输入框
    lv_obj_t *sousuo_ta = lv_textarea_create(cont_bg2);
    lv_obj_clear_flag(sousuo_ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(sousuo_ta, 650, 50);
    lv_textarea_set_one_line(sousuo_ta, true);
    obj_font_set(sousuo_ta, FONT_TYPE_CN, 16);
    lv_textarea_set_placeholder_text(sousuo_ta, "搜索歌手，歌曲，专辑...");
    lv_obj_align_to(sousuo_ta, icon4, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_set_style_bg_color(sousuo_ta, lv_color_make(128, 128, 128), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sousuo_ta, 50, LV_PART_MAIN);
    lv_obj_set_style_radius(sousuo_ta, 25, LV_PART_MAIN);
    lv_obj_set_style_border_width(sousuo_ta, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(sousuo_ta, 15, LV_PART_MAIN);
    lv_obj_set_style_text_color(sousuo_ta, lv_color_hex(0xffffff), LV_PART_MAIN);

    // 绑定弹出键盘事件
    lv_obj_add_event_cb(sousuo_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // 创建搜索按钮
    lv_obj_t *search_btn = lv_btn_create(cont_bg2);
    lv_obj_clear_flag(search_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(search_btn, 100, 50);
    lv_obj_align_to(search_btn, sousuo_ta, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(search_btn, lv_color_make(128, 128, 128), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(search_btn, 50, LV_PART_MAIN);
    lv_obj_set_style_radius(search_btn, 25, LV_PART_MAIN);
    lv_obj_set_style_border_width(search_btn, 0, LV_PART_MAIN);

    lv_obj_t *btn_label = lv_label_create(search_btn);
    obj_font_set(btn_label, FONT_TYPE_CN, 20);
    lv_label_set_text(btn_label, "搜索");
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(btn_label, LV_ALIGN_CENTER, 0, 0);

    // ----------------- 左半区：搜索历史 -----------------

    // ★ 修正：原代码父类是 lv_scr_act()，现改为 cont_bg2 以防止层级覆盖
    lv_obj_t *label = lv_label_create(cont_bg2);
    lv_obj_align_to(label, sousuo_ta, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 30); // 往下挪一点
    obj_font_set(label, FONT_TYPE_CN, 20);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, "#f8f8f8 搜索历史#");

    // 垃圾桶图标
    lv_obj_t *icon5 = lv_img_create(cont_bg2);
    lv_img_set_src(icon5, GET_IMAGE_PATH("icon_delete.png"));
    lv_obj_set_style_img_recolor(icon5, lv_color_hex(0xffffff), LV_PART_MAIN);
    // 将垃圾桶对齐到搜索按钮下方，保持两边对齐
    lv_obj_align_to(icon5, sousuo_ta, LV_ALIGN_OUT_BOTTOM_RIGHT, -20, 30);
    lv_obj_add_flag(icon5, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon5, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_delete");

    // ★ 新增：搜索历史动态 Tag 容器
    lv_obj_t *history_flex = lv_obj_create(cont_bg2);
    lv_obj_clear_flag(history_flex, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(history_flex, 760, 80); // 宽760足够容纳左侧部分
    lv_obj_align_to(history_flex, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    lv_obj_set_style_bg_opa(history_flex, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(history_flex, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(history_flex, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(history_flex, LV_FLEX_FLOW_ROW_WRAP); // 自动换行
    lv_obj_set_style_pad_gap(history_flex, 15, LV_PART_MAIN);

    // 获取数据并生成历史标签
    ApiResult_t history_data = api_get_search_history();
    for (int i = 0; i < history_data.count; i++)
    {
        create_tag_btn(history_flex, history_data.items[i], lv_color_hex(0xffffff));
    }

    // ----------------- 右半区：热搜排行榜 -----------------

    // 标题：定位在右侧可用空间
    lv_obj_t *hot_title = lv_label_create(cont_bg2);
    obj_font_set(hot_title, FONT_TYPE_CN, 20); // 字体大小与您的历史标题保持一致
    lv_label_set_recolor(hot_title, true);
    lv_label_set_text(hot_title, "#f8f8f8 热门搜索排行榜#");
    lv_obj_align_to(hot_title, btn_label, LV_ALIGN_OUT_RIGHT_MID, 60, -10);

    // 热门搜索 动态 Tag 容器
    lv_obj_t *hot_flex = lv_obj_create(cont_bg2);
    lv_obj_clear_flag(hot_flex, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(hot_flex, 480, 150); // 占满右侧剩余宽度
    lv_obj_align_to(hot_flex, hot_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 25);
    lv_obj_set_style_bg_opa(hot_flex, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(hot_flex, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hot_flex, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(hot_flex, LV_FLEX_FLOW_ROW_WRAP); // 自动换行
    lv_obj_set_style_pad_column(hot_flex, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_row(hot_flex, 15, LV_PART_MAIN);

    // 获取数据并生成热搜标签 (前三名彩色)
    ApiResult_t hot_data = api_get_hot_search();
    for (int i = 0; i < hot_data.count; i++)
    {
        lv_color_t color;
        if (i == 0)
            color = lv_color_hex(0xff4d4f);
        else if (i == 1)
            color = lv_color_hex(0xff7a45);
        else if (i == 2)
            color = lv_color_hex(0xffa940);
        else
            color = lv_color_hex(0xffffff);

        create_tag_btn(hot_flex, hot_data.items[i], color);
    }
}