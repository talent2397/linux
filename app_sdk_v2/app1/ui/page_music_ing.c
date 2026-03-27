
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"
#include "http_manager.h"
#include "cJSON/cJSON.h"

static lv_style_t com_style;
static int g_current_song_id = 0;
// ======================= API 数据接口模拟 =======================

typedef struct
{
    const char *song_name;
    const char *artist;
    const char *album;
    const char *current_time;
    const char *total_time;
    int progress_value; /* 0-100 */
    bool liked;
    bool loop_on;
    int volume; /* 0-100 */
} music_playing_info_t;

static lv_obj_t *g_album_img = NULL;
static lv_obj_t *g_play_btn_img = NULL;
static lv_obj_t *g_progress = NULL;
static lv_obj_t *g_volume_slider = NULL;
static lv_obj_t *g_label_time_cur = NULL;
static lv_obj_t *g_label_time_total = NULL;
static lv_timer_t *g_album_rotate_timer = NULL;
static bool g_is_playing = true;
static int g_album_angle = 0; /* 0.1 degree unit */

// UI 元素引用 - 用于回调更新
static lv_obj_t *g_song_title_label = NULL;
static lv_obj_t *g_artist_label = NULL;

// 全局变量：当前播放的歌曲信息
static char g_current_song_url[512] = {0};
static char g_current_song_name[256] = {0};
static char g_current_artist[256] = {0};
static char g_current_album[256] = {0};

// ★ 新增：页面存活状态标记
static bool g_is_ing_page_active = false;

static void async_play_downloaded_cb(void *p)
{
    int success = (int)(intptr_t)p;
    if (success && g_is_ing_page_active)
    {
        printf(">>> 下载完成！系统后台呼叫 mpg123 播放 /tmp/music.mp3\n");
        // 先杀掉旧的，再播放新下载好的本地文件！
        system("killall -9 mpg123 2>/dev/null");
        system("mpg123 /tmp/music.mp3 > /dev/null 2>&1 &");
    }
    else if (!success)
    {
        printf(">>> 音乐下载失败！\n");
    }
}

// 下载完成后的回调（在子线程被触发）
static void music_download_callback(int success)
{
    lv_async_call(async_play_downloaded_cb, (void *)(intptr_t)success);
}

// ★ 新增：退出页面前的终极清理（解决核心崩溃问题）
static void cleanup_before_exit(void)
{
    g_is_ing_page_active = false;

    // 1. 彻底干掉悬空定时器！防止跑到别的页面还在转圈报错
    if (g_album_rotate_timer != NULL)
    {
        lv_timer_del(g_album_rotate_timer);
        g_album_rotate_timer = NULL;
    }

    // 2. 清理所有全局 UI 指针防野指针
    g_album_img = NULL;
    g_play_btn_img = NULL;
    g_progress = NULL;
    g_volume_slider = NULL;
    g_label_time_cur = NULL;
    g_label_time_total = NULL;
    g_song_title_label = NULL;
    g_artist_label = NULL;
}
// 修改接收函数
void set_current_play_song(int song_id, const char *url, const char *name, const char *artist, const char *album)
{
    g_current_song_id = song_id; // 保存ID
    if (url)
        snprintf(g_current_song_url, sizeof(g_current_song_url), "%s", url);
    if (name)
        snprintf(g_current_song_name, sizeof(g_current_song_name), "%s", name);
    if (artist)
        snprintf(g_current_artist, sizeof(g_current_artist), "%s", artist);
    if (album)
        snprintf(g_current_album, sizeof(g_current_album), "%s", album);
}

// ★ 将歌曲URL获取移入异步主线程回调
static void async_music_url_cb(void *p)
{
    char *json_str = (char *)p;
    if (json_str)
    {
        if (g_is_ing_page_active)
        {
            cJSON *root = cJSON_Parse(json_str);
            if (root)
            {
                cJSON *url_node = cJSON_GetObjectItem(root, "url");
                if (!url_node && cJSON_IsArray(root))
                {
                    cJSON *item = cJSON_GetArrayItem(root, 0);
                    if (item)
                        url_node = cJSON_GetObjectItem(item, "url");
                }
                if (!url_node)
                {
                    cJSON *data = cJSON_GetObjectItem(root, "data");
                    if (data && cJSON_IsArray(data))
                    {
                        cJSON *item = cJSON_GetArrayItem(data, 0);
                        if (item)
                            url_node = cJSON_GetObjectItem(item, "url");
                    }
                }

                if (url_node && cJSON_IsString(url_node) && url_node->valuestring)
                {
                    strncpy(g_current_song_url, url_node->valuestring, sizeof(g_current_song_url) - 1);
                    g_current_song_url[sizeof(g_current_song_url) - 1] = '\0';
                    printf(">>> 成功解析到真正的 MP3 播放链接: %s\n", g_current_song_url);

                    // ================= 核心修复：管道流处理HTTPS与重定向 =================
                    char cmd[1024];
                    // 1. 彻底干掉以前的 curl 和 mpg123
                    system("killall -9 curl mpg123 2>/dev/null");
                    // 2. 使用 curl -L 处理重定向，管道符传给 mpg123 - 。必须用单引号 '%s' 包裹防止URL里的&被系统截断！
                    snprintf(cmd, sizeof(cmd), "(curl -s -L '%s' | mpg123 - ) > /dev/null 2>&1 &", g_current_song_url);
                    system(cmd);
                    // ====================================================================
                }
                cJSON_Delete(root);
            }
        }
        free(json_str);
    }
}

// 歌曲播放地址网络回调（子线程执行）
static void music_url_callback(char *json_str)
{
    if (!json_str)
        return;
    char *json_copy = strdup(json_str);
    if (json_copy)
        lv_async_call(async_music_url_cb, json_copy);
}

/* API 接口：获取当前播放信息 */
static int music_api_get_playing_info(music_playing_info_t *out_info)
{
    if (!out_info)
        return 0;

    if (strlen(g_current_song_url) > 0)
    {
        out_info->song_name = g_current_song_name;
        out_info->artist = g_current_artist;
        out_info->album = g_current_album;
        out_info->current_time = "00:00";
        out_info->total_time = "--:--";
        out_info->progress_value = 0;
        out_info->liked = false;
        out_info->loop_on = true;
        out_info->volume = 68;

        printf(">>> UI 已经加载，呼叫 libcurl 异步下载到内存: %s\n", g_current_song_url);

        // 绑定回调，发起 C 代码级的纯正 HTTP 下载！
        http_set_music_download_callback(music_download_callback);
        http_music_download_async(g_current_song_url);

        return 1;
    }
    return 0;
}

static void get_mock_playing_info(music_playing_info_t *out_info)
{
    if (out_info == NULL)
        return;
    out_info->song_name = "夜曲 (Nocturne)";
    out_info->artist = "周杰伦";
    out_info->album = "十一月的萧邦";
    out_info->current_time = "01:23";
    out_info->total_time = "03:46";
    out_info->progress_value = 36;
    out_info->liked = false;
    out_info->loop_on = true;
    out_info->volume = 68;
}

// ======================= 全局状态管理 =======================

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
    cleanup_before_exit();
    lv_obj_clean(lv_scr_act());
    page_test_init();
}

static void album_rotate_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    // 防御性判断，防止野指针
    if (!g_is_ing_page_active || g_album_img == NULL || !g_is_playing)
        return;

    g_album_angle += 20; /* 每次 +2.0 度 */
    if (g_album_angle >= 3600)
        g_album_angle = 0;

    lv_img_set_angle(g_album_img, g_album_angle);
}

// --- 替换播放与暂停状态控制 (通过发送 STOP 和 CONT 信号挂起 mpg123) ---
static void set_play_state(bool play)
{
    g_is_playing = play;
    if (g_album_rotate_timer == NULL)
    {
        g_album_rotate_timer = lv_timer_create(album_rotate_timer_cb, 60, NULL);
    }
    if (play)
    {
        lv_timer_resume(g_album_rotate_timer);
        system("killall -CONT mpg123 2>/dev/null");
        if (g_play_btn_img)
            lv_img_set_src(g_play_btn_img, GET_IMAGE_PATH("icon_zanting2.png"));
    }
    else
    {
        lv_timer_pause(g_album_rotate_timer);
        system("killall -STOP mpg123 2>/dev/null");
        if (g_play_btn_img)
            lv_img_set_src(g_play_btn_img, GET_IMAGE_PATH("icon_bofang1.png"));
    }
}
// 图标点击事件处理函数
static void icon_click_handler(lv_event_t *e)
{
    const char *icon_name = (const char *)lv_event_get_user_data(e);

    if (icon_name == NULL)
        return;

    if (strcmp(icon_name, "icon_sousuolist") == 0)
    {
        cleanup_before_exit();
        lv_obj_clean(lv_scr_act());
        page_music_search();
    }
    else if (strcmp(icon_name, "icon_bofanglist") == 0)
    {
        cleanup_before_exit();
        lv_obj_clean(lv_scr_act());
        page_music_list();
    }
    else if (strcmp(icon_name, "icon_play_toggle") == 0)
    {
        set_play_state(!g_is_playing);
    }
    // 其他保留项
}

// ======================= UI 组件生成器 =======================
static lv_obj_t *create_control_icon(lv_obj_t *parent, const void *img_src, const char *event_name, lv_color_t color)
{
    lv_obj_t *icon = lv_img_create(parent);
    lv_img_set_src(icon, img_src);
    lv_obj_set_style_img_recolor(icon, color, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon, icon_click_handler, LV_EVENT_CLICKED, (void *)event_name);
    return icon;
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

void page_music_ing()
{
    // ★ 初始化存活标记
    g_is_ing_page_active = true;

    lv_obj_t *cont = init();
    music_playing_info_t info;
    if (!music_api_get_playing_info(&info))
    {
        get_mock_playing_info(&info);
    }

    // ----------------- 左侧侧边栏 cont_bg1 -----------------
    lv_obj_t *cont_bg1 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg1, LV_OBJ_FLAG_SCROLLABLE); // 禁滑
    lv_obj_set_size(cont_bg1, 106, 220);
    lv_obj_align(cont_bg1, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(cont_bg1, lv_color_make(20, 20, 20), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_bg1, 200, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_bg1, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg1, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(cont_bg1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_bg1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont_bg1, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(cont_bg1, 30, LV_PART_MAIN);

    lv_obj_t *icon1 = lv_img_create(cont_bg1);
    lv_img_set_src(icon1, GET_IMAGE_PATH("icon_unsousuo.png"));
    lv_obj_set_style_img_recolor(icon1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(icon1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon1, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_sousuolist");

    lv_obj_t *icon2 = lv_img_create(cont_bg1);
    lv_img_set_src(icon2, GET_IMAGE_PATH("icon_unbofanglist.png"));
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(icon2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon2, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofanglist");

    lv_obj_t *icon3 = lv_img_create(cont_bg1);
    lv_img_set_src(icon3, GET_IMAGE_PATH("icon_bofang1.png"));
    lv_obj_set_style_img_recolor(icon3, lv_color_hex(0x1DB954), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon3, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(icon3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon3, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofang");

    // ----------------- 右侧主内容区 cont_bg2 -----------------
    lv_obj_t *cont_bg2 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont_bg2, 1318, 220);
    lv_obj_align_to(cont_bg2, cont_bg1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_style_bg_color(cont_bg2, lv_color_hex(0x0F1115), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_bg2, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_bg2, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg2, 0, LV_PART_MAIN);

    /* 左侧：专辑图片 */
    lv_obj_t *album_wrap = lv_obj_create(cont_bg2);
    lv_obj_remove_style_all(album_wrap);
    lv_obj_set_size(album_wrap, 180, 180);
    lv_obj_align(album_wrap, LV_ALIGN_LEFT_MID, 40, 0);

    g_album_img = lv_img_create(album_wrap);
    lv_img_set_src(g_album_img, GET_IMAGE_PATH("icon_music.png"));
    lv_obj_center(g_album_img);

    /* 中间：歌曲信息 */
    lv_obj_t *mid_cont = lv_obj_create(cont_bg2);
    lv_obj_remove_style_all(mid_cont);
    lv_obj_set_size(mid_cont, 400, 180);
    lv_obj_align_to(mid_cont, album_wrap, LV_ALIGN_OUT_RIGHT_MID, 40, 0);

    lv_obj_t *song_title = lv_label_create(mid_cont);
    obj_font_set(song_title, FONT_TYPE_CN, 28);
    lv_label_set_text(song_title, info.song_name);
    lv_obj_set_style_text_color(song_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(song_title, LV_ALIGN_TOP_LEFT, 0, 30);
    g_song_title_label = song_title;

    lv_obj_t *artist_label = lv_label_create(mid_cont);
    obj_font_set(artist_label, FONT_TYPE_CN, 16);
    lv_label_set_text_fmt(artist_label, "%s - %s", info.artist, info.album);
    lv_obj_set_style_text_color(artist_label, lv_color_hex(0x1DB954), 0);
    lv_obj_align_to(artist_label, song_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 12);
    g_artist_label = artist_label;

    /* 标签区 */
    lv_obj_t *tag_row = lv_obj_create(mid_cont);
    lv_obj_remove_style_all(tag_row);
    lv_obj_set_size(tag_row, 190, 22);
    lv_obj_align_to(tag_row, artist_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    lv_obj_set_flex_flow(tag_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(tag_row, 10, LV_PART_MAIN);

    lv_obj_t *tag_a = lv_btn_create(tag_row);
    lv_obj_remove_style_all(tag_a);
    lv_obj_set_size(tag_a, 54, 20);
    lv_obj_set_style_radius(tag_a, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tag_a, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(tag_a, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(tag_a, lv_color_hex(0x1DB954), LV_PART_MAIN);

    lv_obj_t *tag_a_text = lv_label_create(tag_a);
    obj_font_set(tag_a_text, FONT_TYPE_CN, 11);
    lv_label_set_text(tag_a_text, "SQ 无损");
    lv_obj_set_style_text_color(tag_a_text, lv_color_hex(0x1DB954), 0);
    lv_obj_center(tag_a_text);

    lv_obj_t *tag_b = lv_btn_create(tag_row);
    lv_obj_remove_style_all(tag_b);
    lv_obj_set_size(tag_b, 74, 20);
    lv_obj_set_style_radius(tag_b, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tag_b, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(tag_b, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(tag_b, lv_color_hex(0x8E95A9), LV_PART_MAIN);

    lv_obj_t *tag_b_text = lv_label_create(tag_b);
    obj_font_set(tag_b_text, FONT_TYPE_CN, 11);
    lv_label_set_text(tag_b_text, "杜比全景声");
    lv_obj_set_style_text_color(tag_b_text, lv_color_hex(0x8E95A9), 0);
    lv_obj_center(tag_b_text);

    /* 右侧偏上：进度条区域 */
    lv_obj_t *progress_row = lv_obj_create(cont_bg2);
    lv_obj_remove_style_all(progress_row);
    lv_obj_set_size(progress_row, 700, 24);
    lv_obj_align(progress_row, LV_ALIGN_TOP_LEFT, 520, 65);

    g_label_time_cur = lv_label_create(progress_row);
    obj_font_set(g_label_time_cur, FONT_TYPE_CN, 13);
    lv_label_set_text(g_label_time_cur, info.current_time);
    lv_obj_set_style_text_color(g_label_time_cur, lv_color_hex(0x8E95A9), 0);
    lv_obj_align(g_label_time_cur, LV_ALIGN_LEFT_MID, 0, 0);

    g_progress = lv_slider_create(progress_row);
    lv_obj_set_size(g_progress, 580, 6);
    lv_obj_align_to(g_progress, g_label_time_cur, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
    lv_slider_set_range(g_progress, 0, 100);
    lv_slider_set_value(g_progress, info.progress_value, LV_ANIM_OFF);
    lv_obj_set_style_radius(g_progress, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_progress, lv_color_hex(0x282C34), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_progress, lv_color_hex(0x1DB954), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_progress, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(g_progress, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_pad_all(g_progress, -2, LV_PART_KNOB);

    g_label_time_total = lv_label_create(progress_row);
    obj_font_set(g_label_time_total, FONT_TYPE_CN, 13);
    lv_label_set_text(g_label_time_total, info.total_time);
    lv_obj_set_style_text_color(g_label_time_total, lv_color_hex(0x8E95A9), 0);
    lv_obj_align_to(g_label_time_total, g_progress, LV_ALIGN_OUT_RIGHT_MID, 15, 0);

    /* 右侧偏下：控制按钮区 */
    lv_obj_t *ctrl_row = lv_obj_create(cont_bg2);
    lv_obj_remove_style_all(ctrl_row);
    lv_obj_set_size(ctrl_row, 700, 60);
    lv_obj_align(ctrl_row, LV_ALIGN_TOP_LEFT, 520, 110);

    lv_obj_t *left_group = lv_obj_create(ctrl_row);
    lv_obj_remove_style_all(left_group);
    lv_obj_set_size(left_group, 100, 60);
    lv_obj_align(left_group, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_flow(left_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left_group, 25, LV_PART_MAIN);
    create_control_icon(left_group, GET_IMAGE_PATH("icon_xihuan.png"), "icon_like", lv_color_hex(0xFFFFFF));
    create_control_icon(left_group, GET_IMAGE_PATH("icon_xiazai.png"), "icon_download", lv_color_hex(0xFFFFFF));

    lv_obj_t *center_group = lv_obj_create(ctrl_row);
    lv_obj_remove_style_all(center_group);
    lv_obj_set_size(center_group, 360, 60);
    lv_obj_align(center_group, LV_ALIGN_CENTER, -30, 0);
    lv_obj_set_flex_flow(center_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(center_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(center_group, 30, LV_PART_MAIN);

    create_control_icon(center_group, GET_IMAGE_PATH("icon_suiji.png"), "icon_loop", lv_color_hex(0xFFFFFF));
    create_control_icon(center_group, GET_IMAGE_PATH("icon_shangyishou.png"), "icon_prev", lv_color_hex(0xFFFFFF));

    lv_obj_t *play_bg = lv_obj_create(center_group);
    lv_obj_remove_style_all(play_bg);
    lv_obj_set_size(play_bg, 60, 60);
    lv_obj_set_style_bg_color(play_bg, lv_color_hex(0x1DB954), LV_PART_MAIN);
    lv_obj_set_style_radius(play_bg, 30, LV_PART_MAIN);
    lv_obj_add_flag(play_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(play_bg, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_play_toggle");

    g_play_btn_img = lv_img_create(play_bg);
    lv_img_set_src(g_play_btn_img, GET_IMAGE_PATH("icon_zanting2.png"));
    lv_obj_center(g_play_btn_img);

    create_control_icon(center_group, GET_IMAGE_PATH("icon_xiayishou.png"), "icon_next", lv_color_hex(0xFFFFFF));
    create_control_icon(center_group, GET_IMAGE_PATH("icon_xunhuan.png"), "icon_loop", lv_color_hex(0xFFFFFF));

    lv_obj_t *right_group = lv_obj_create(ctrl_row);
    lv_obj_remove_style_all(right_group);
    lv_obj_set_size(right_group, 180, 60);
    lv_obj_align(right_group, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_flex_flow(right_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_group, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right_group, 15, LV_PART_MAIN);

    create_control_icon(right_group, GET_IMAGE_PATH("icon_volume.png"), "icon_volume", lv_color_hex(0xFFFFFF));
    g_volume_slider = lv_slider_create(right_group);
    lv_obj_set_size(g_volume_slider, 120, 4);
    lv_slider_set_range(g_volume_slider, 0, 100);
    lv_slider_set_value(g_volume_slider, info.volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_volume_slider, lv_color_hex(0x3B3E46), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_volume_slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_volume_slider, 0, LV_PART_KNOB);

    set_play_state(true);
}