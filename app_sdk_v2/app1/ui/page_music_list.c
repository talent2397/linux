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
#include "cJSON/cJSON.h"

static lv_style_t com_style;
static bool g_is_list_page_active = false;
static lv_obj_t *g_list_cont = NULL;

extern void set_current_play_song(int song_id, const char *url, const char *name, const char *artist, const char *album);

typedef struct
{
    int index;
    int song_id;
    char name[256];
    char artist[256];
    char album[256];
    char duration[16];
    char play_url[512]; // ★ 新增：直接把播放地址存下来
} music_track_t;

static music_track_t *g_search_results = NULL;
static int g_search_result_count = 0;

static void free_search_results(void)
{
    if (g_search_results != NULL)
    {
        free(g_search_results);
        g_search_results = NULL;
    }
    g_search_result_count = 0;
}

#define MOCK_TRACK_COUNT 12
static const music_track_t mock_tracks[MOCK_TRACK_COUNT] = {
    {1, 0, "夜曲", "周杰伦", "十一月的萧邦", "03:46"},
    {2, 0, "七里香", "周杰伦", "七里香", "04:59"},
    {3, 0, "晴天", "周杰伦", "叶惠美", "04:29"},
    {4, 0, "一路向北", "周杰伦", "J III MP3 Player", "04:54"},
    {5, 0, "稻香", "周杰伦", "魔杰座", "03:43"},
    {6, 0, "简单爱", "周杰伦", "范特西", "04:30"},
    {7, 0, "告白气球", "周杰伦", "周杰伦的床边故事", "03:24"},
    {8, 0, "听妈妈的话", "周杰伦", "依然范特西", "04:23"},
    {9, 0, "东风破", "周杰伦", "叶惠美", "05:15"},
    {10, 0, "发如雪", "周杰伦", "十一月的萧邦", "05:04"},
    {11, 0, "兰亭序", "周杰伦", "魔杰座", "04:13"},
    {12, 0, "菊花台", "周杰伦", "依然范特西", "04:53"},
};

#define SONGS_PER_PAGE 5
#define ROW_HEIGHT 36
#define HEADER_HEIGHT 32
#define COL_IDX_W 45
#define COL_NAME_W 350
#define COL_ARTIST_W 200
#define COL_ALBUM_W 280
#define COL_ACTION_W 100

static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}

static void cleanup_before_exit(void)
{
    g_is_list_page_active = false;
    g_list_cont = NULL;
}

static void play_row_click_handler(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    printf("准备播放: %s - %s\n", g_search_results[idx].name, g_search_results[idx].artist);

    // ★ 核心修改：把 song_id 一并传过去！
    set_current_play_song(g_search_results[idx].song_id,
                          g_search_results[idx].play_url,
                          g_search_results[idx].name,
                          g_search_results[idx].artist,
                          g_search_results[idx].album);

    cleanup_before_exit();
    lv_obj_clean(lv_scr_act());
    page_music_ing(); // 跳转到播放界面
}

static void create_song_row(lv_obj_t *parent, const music_track_t *track, int global_idx)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), ROW_HEIGHT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_left(row, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x2a2525), LV_STATE_PRESSED);

    char idx_buf[8];
    snprintf(idx_buf, sizeof(idx_buf), "%02d", track->index);

    lv_obj_t *lb_idx = lv_label_create(row);
    obj_font_set(lb_idx, FONT_TYPE_CN, 14);
    lv_label_set_text(lb_idx, idx_buf);
    lv_obj_set_style_text_color(lb_idx, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_width(lb_idx, COL_IDX_W);

    lv_obj_t *lb_name = lv_label_create(row);
    obj_font_set(lb_name, FONT_TYPE_CN, 14);
    lv_label_set_text(lb_name, track->name);
    lv_obj_set_style_text_color(lb_name, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_width(lb_name, COL_NAME_W);
    lv_label_set_long_mode(lb_name, LV_LABEL_LONG_DOT);

    lv_obj_t *lb_artist = lv_label_create(row);
    obj_font_set(lb_artist, FONT_TYPE_CN, 14);
    lv_label_set_text(lb_artist, track->artist);
    lv_obj_set_style_text_color(lb_artist, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_width(lb_artist, COL_ARTIST_W);
    lv_label_set_long_mode(lb_artist, LV_LABEL_LONG_DOT);

    lv_obj_t *lb_album = lv_label_create(row);
    obj_font_set(lb_album, FONT_TYPE_CN, 14);
    lv_label_set_text(lb_album, track->album);
    lv_obj_set_style_text_color(lb_album, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_width(lb_album, COL_ALBUM_W);
    lv_label_set_long_mode(lb_album, LV_LABEL_LONG_DOT);

    lv_obj_t *action_cont = lv_obj_create(row);
    lv_obj_remove_style_all(action_cont);
    lv_obj_set_size(action_cont, COL_ACTION_W, ROW_HEIGHT);
    lv_obj_set_flex_flow(action_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(action_cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(action_cont, 4, LV_PART_MAIN);
    lv_obj_add_flag(action_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(action_cont, play_row_click_handler, LV_EVENT_CLICKED, (void *)(intptr_t)global_idx);

    lv_obj_t *play_img = lv_img_create(action_cont);
    lv_img_set_src(play_img, GET_IMAGE_PATH("icon_bofang1.png"));
    lv_obj_set_style_img_recolor(play_img, lv_color_hex(0xffffff), LV_PART_MAIN);

    lv_obj_t *lb_dur = lv_label_create(action_cont);
    obj_font_set(lb_dur, FONT_TYPE_CN, 14);
    lv_label_set_text(lb_dur, track->duration);
    lv_obj_set_style_text_color(lb_dur, lv_color_hex(0x8a8a8a), LV_PART_MAIN);
}

static void update_music_list_ui(void)
{
    if (!g_list_cont)
        return;
    lv_obj_clean(g_list_cont);

    music_track_t *tracks = g_search_results;
    int count = g_search_result_count;

    if (count == 0)
    {
        tracks = (music_track_t *)mock_tracks;
        count = MOCK_TRACK_COUNT;
    }

    int page_cnt = (count + SONGS_PER_PAGE - 1) / SONGS_PER_PAGE;
    int page_h = SONGS_PER_PAGE * ROW_HEIGHT;

    for (int p = 0; p < page_cnt; p++)
    {
        lv_obj_t *page = lv_obj_create(g_list_cont);
        lv_obj_remove_style_all(page);
        lv_obj_set_size(page, lv_pct(100), page_h);
        lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, LV_PART_MAIN);

        int start = p * SONGS_PER_PAGE;
        int end = start + SONGS_PER_PAGE;
        if (end > count)
            end = count;

        for (int i = start; i < end; i++)
        {
            create_song_row(page, &tracks[i], i);
        }
    }
}

// ★ 防御性数据提取：防止因为 API 返回 NULL 导致的崩溃
static const char *extract_str(cJSON *node)
{
    if (node && cJSON_IsString(node) && node->valuestring)
        return node->valuestring;
    return "未知";
}

static int extract_id(cJSON *node)
{
    if (!node)
        return 0;
    if (cJSON_IsNumber(node))
        return node->valueint;
    if (cJSON_IsString(node) && node->valuestring)
        return atoi(node->valuestring);
    return 0;
}

// 暴力提取歌手名（兼容字符串、数组、对象各种嵌套）
static void extract_artist(cJSON *song, char *buf, size_t len)
{
    cJSON *artist = cJSON_GetObjectItem(song, "artist");
    if (!artist)
        artist = cJSON_GetObjectItem(song, "artists");
    if (!artist)
        artist = cJSON_GetObjectItem(song, "SingerName");
    if (!artist)
        artist = cJSON_GetObjectItem(song, "ar"); // 网易云原生格式

    if (artist)
    {
        if (cJSON_IsString(artist))
        {
            snprintf(buf, len, "%s", artist->valuestring);
            return;
        }
        if (cJSON_IsArray(artist) && cJSON_GetArraySize(artist) > 0)
        {
            cJSON *item = cJSON_GetArrayItem(artist, 0);
            if (cJSON_IsString(item))
            {
                snprintf(buf, len, "%s", item->valuestring);
                return;
            }
            if (cJSON_IsObject(item))
            {
                cJSON *name = cJSON_GetObjectItem(item, "name");
                if (cJSON_IsString(name))
                {
                    snprintf(buf, len, "%s", name->valuestring);
                    return;
                }
            }
        }
    }
    snprintf(buf, len, "未知");
}

// 暴力提取专辑名
static void extract_album(cJSON *song, char *buf, size_t len)
{
    cJSON *album = cJSON_GetObjectItem(song, "album");
    if (!album)
        album = cJSON_GetObjectItem(song, "al");
    if (!album)
        album = cJSON_GetObjectItem(song, "AlbumName");

    if (album)
    {
        if (cJSON_IsString(album))
        {
            snprintf(buf, len, "%s", album->valuestring);
            return;
        }
        if (cJSON_IsObject(album))
        {
            cJSON *name = cJSON_GetObjectItem(album, "name");
            if (cJSON_IsString(name))
            {
                snprintf(buf, len, "%s", name->valuestring);
                return;
            }
        }
    }
    snprintf(buf, len, "未知");
}

static void async_music_search_cb(void *p)
{
    char *json_str = (char *)p;
    if (json_str)
    {
        if (g_is_list_page_active)
        {
            free_search_results();
            cJSON *root = cJSON_Parse(json_str);

            if (root)
            {
                cJSON *songs = NULL;

                // 1. 尝试寻找真正的数组入口 (兼容几乎所有主流API格式)
                if (cJSON_IsArray(root))
                {
                    songs = root;
                }
                else
                {
                    cJSON *data = cJSON_GetObjectItem(root, "data");
                    if (data && cJSON_IsArray(data))
                        songs = data;

                    if (!songs)
                    {
                        cJSON *result = cJSON_GetObjectItem(root, "result");
                        if (result)
                        {
                            if (cJSON_IsArray(result))
                                songs = result;
                            else
                                songs = cJSON_GetObjectItem(result, "songs");
                        }
                    }
                }

                // --- 这里加了一层保险！如果还解析失败，会把你的真实格式打印在终端 ---
                if (!songs || !cJSON_IsArray(songs))
                {
                    printf(">>> 致命错误：找不到歌曲数组！JSON 前300字符: %.*s\n", 300, json_str);
                }
                else
                {
                    g_search_result_count = cJSON_GetArraySize(songs);
                    if (g_search_result_count > 10)
                        g_search_result_count = 10; // 限制展示10条

                    if (g_search_result_count > 0)
                    {
                        g_search_results = (music_track_t *)malloc(sizeof(music_track_t) * g_search_result_count);
                        if (g_search_results)
                        {
                            memset(g_search_results, 0, sizeof(music_track_t) * g_search_result_count);

                            for (int i = 0; i < g_search_result_count; i++)
                            {
                                cJSON *song = cJSON_GetArrayItem(songs, i);
                                if (!song)
                                    continue;

                                g_search_results[i].index = i + 1;

                                cJSON *id_node = cJSON_GetObjectItem(song, "id");
                                if (!id_node)
                                    id_node = cJSON_GetObjectItem(song, "songmid");
                                g_search_results[i].song_id = extract_id(id_node);

                                // 1. 提取歌名 (APlayer格式叫 title)
                                cJSON *title_node = cJSON_GetObjectItem(song, "title");
                                snprintf(g_search_results[i].name, sizeof(g_search_results[i].name), "%s", extract_str(title_node));

                                // 2. 提取歌手 (APlayer格式叫 author)
                                cJSON *author_node = cJSON_GetObjectItem(song, "author");
                                snprintf(g_search_results[i].artist, sizeof(g_search_results[i].artist), "%s", extract_str(author_node));

                                // 3. 专辑 (APlayer没返回，直接写死)
                                snprintf(g_search_results[i].album, sizeof(g_search_results[i].album), "未知");

                                // 4. 时长写死
                                snprintf(g_search_results[i].duration, sizeof(g_search_results[i].duration), "--:--");

                                // 5. ★ 最关键的一步：提取 url 字段，并拼上服务器的 IP！
                                cJSON *url_node = cJSON_GetObjectItem(song, "url");
                                if (url_node && cJSON_IsString(url_node) && url_node->valuestring)
                                {
                                    // 拼接完整的 MP3 请求路径
                                    snprintf(g_search_results[i].play_url, sizeof(g_search_results[i].play_url),
                                             "http://10.26.246.145%s", url_node->valuestring);
                                }
                                else
                                {
                                    strcpy(g_search_results[i].play_url, ""); // 防错置空
                                }

                                // 成功验证打印 (把拼好的链接打印出来看看对不对！)
                                printf("解析成功[%d]: %s - %s \n播放链接: %s\n",
                                       i, g_search_results[i].name, g_search_results[i].artist, g_search_results[i].play_url);
                            }
                        }
                    }
                }
                cJSON_Delete(root);
            }
            else
            {
                printf(">>> JSON 格式错误或为空！\n");
            }
            update_music_list_ui();
        }
        free(json_str);
    }
}

static void music_search_callback(char *json_str)
{
    if (json_str == NULL)
        return;
    char *json_copy = strdup(json_str);
    if (json_copy)
        lv_async_call(async_music_search_cb, json_copy);
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

    if (strcmp(icon_name, "icon_sousuolist") == 0)
    {
        cleanup_before_exit();
        lv_obj_clean(lv_scr_act());
        page_music_search();
    }
    else if (strcmp(icon_name, "icon_bofang") == 0)
    {
        cleanup_before_exit();
        lv_obj_clean(lv_scr_act());
        page_music_ing();
    }
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

// ======================= 外部入口函数 =======================
// keyword != NULL : 发起新搜索
// keyword == NULL : 仅展示上次结果
void page_music_list_with_search(const char *keyword)
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
    lv_img_set_src(icon1, GET_IMAGE_PATH("icon_unsousuo.png"));
    lv_obj_add_flag(icon1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon1, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_sousuolist");

    lv_obj_t *icon2 = lv_img_create(cont_bg1);
    lv_img_set_src(icon2, GET_IMAGE_PATH("icon_bofanglist.png"));
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0x1DB954), LV_PART_MAIN);
    lv_obj_add_flag(icon2, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icon3 = lv_img_create(cont_bg1);
    lv_img_set_src(icon3, GET_IMAGE_PATH("icon_unbofang.png"));
    lv_obj_add_flag(icon3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon3, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofang");

    lv_obj_t *cont_bg2 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont_bg2, 1318, 220);
    lv_obj_align_to(cont_bg2, cont_bg1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_style_bg_color(cont_bg2, lv_color_make(11, 12, 16), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont_bg2, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg2, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont_bg2, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *header = lv_obj_create(cont_bg2);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, lv_pct(100), HEADER_HEIGHT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x171414), LV_PART_MAIN);
    lv_obj_set_style_pad_column(header, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_left(header, 12, LV_PART_MAIN);

    const char *headers[] = {"#", "歌曲名", "歌手", "专辑", "时长"};
    int widths[] = {COL_IDX_W, COL_NAME_W, COL_ARTIST_W, COL_ALBUM_W, COL_ACTION_W};
    for (int i = 0; i < 5; i++)
    {
        lv_obj_t *lb = lv_label_create(header);
        obj_font_set(lb, FONT_TYPE_CN, 14);
        lv_label_set_text(lb, headers[i]);
        lv_obj_set_style_text_color(lb, lv_color_hex(0x8a8a8a), LV_PART_MAIN);
        lv_obj_set_width(lb, widths[i]);
    }

    g_list_cont = lv_obj_create(cont_bg2);
    lv_obj_remove_style_all(g_list_cont);
    lv_obj_set_size(g_list_cont, lv_pct(100), 220 - HEADER_HEIGHT);
    lv_obj_set_flex_flow(g_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_snap_y(g_list_cont, LV_SCROLL_SNAP_START);
    lv_obj_set_style_width(g_list_cont, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(g_list_cont, lv_color_hex(0x8a8a8a), LV_PART_SCROLLBAR);

    g_is_list_page_active = true;

    if (keyword != NULL)
    {
        lv_obj_t *loading_lb = lv_label_create(g_list_cont);
        obj_font_set(loading_lb, FONT_TYPE_CN, 16);
        lv_label_set_text(loading_lb, "正在搜索中...");
        lv_obj_set_style_text_color(loading_lb, lv_color_hex(0xffffff), 0);
        lv_obj_align(loading_lb, LV_ALIGN_CENTER, 0, 0);

        http_set_music_search_callback(music_search_callback);
        http_music_search_async(keyword, 1, 20);
    }
    else
    {
        update_music_list_ui();
    }
}

void page_music_list()
{
    page_music_list_with_search(NULL);
}