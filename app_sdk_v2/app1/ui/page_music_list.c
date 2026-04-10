#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"

static lv_style_t com_style;

// ======================= API 数据接口 =======================
// 预留用于网络 API 获取歌曲列表，当前使用模拟数据
// 替换 music_api_fetch_list() 实现即可接入真实 API

typedef struct {
    int index;           // 序号
    const char *name;    // 歌曲名
    const char *artist;  // 歌手
    const char *album;   // 专辑
    const char *duration; // 时长 MM:SS
} music_track_t;

/** API 接口：获取歌曲列表
 * 当前为模拟数据，后续可替换为 HTTP 请求
 * @param out_tracks 输出歌曲数组（调用方负责释放）
 * @return 歌曲数量，失败返回 0
 */
static int music_api_fetch_list(music_track_t **out_tracks)
{
    (void)out_tracks;
    /* TODO: 接入网络 API 时在此实现
     * 示例: http_get_music_list() -> 解析 JSON -> 填充 music_track_t 数组
     */
    return 0; /* 返回 0 表示使用本地模拟数据 */
}

// 模拟数据（API 未就绪时使用）
#define MOCK_TRACK_COUNT 12
static const music_track_t mock_tracks[MOCK_TRACK_COUNT] = {
    {1, "夜曲", "周杰伦", "十一月的萧邦", "03:46"},
    {2, "七里香", "周杰伦", "七里香", "04:59"},
    {3, "晴天", "周杰伦", "叶惠美", "04:29"},
    {4, "一路向北", "周杰伦", "J III MP3 Player", "04:54"},
    {5, "稻香", "周杰伦", "魔杰座", "03:43"},
    {6, "简单爱", "周杰伦", "范特西", "04:30"},
    {7, "告白气球", "周杰伦", "周杰伦的床边故事", "03:24"},
    {8, "听妈妈的话", "周杰伦", "依然范特西", "04:23"},
    {9, "东风破", "周杰伦", "叶惠美", "05:15"},
    {10, "发如雪", "周杰伦", "十一月的萧邦", "05:04"},
    {11, "兰亭序", "周杰伦", "魔杰座", "04:13"},
    {12, "菊花台", "周杰伦", "依然范特西", "04:53"},
};

#define SONGS_PER_PAGE 5

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
    const char *icon_name = lv_event_get_user_data(e);

    if (icon_name == NULL)
        return;

    if (strcmp(icon_name, "icon_sousuolist") == 0)
    {
        printf("搜索列表图标被点击\n");
        lv_obj_clean(lv_scr_act());
        page_music_search();
    }
    else if (strcmp(icon_name, "icon_bofanglist") == 0)
    {
        printf("播放列表图标被点击\n");
    }
    else if (strcmp(icon_name, "icon_bofang") == 0)
    {
        printf("播放图标被点击\n");
        lv_obj_clean(lv_scr_act());
        page_music_ing();
    }
}

// 行内播放按钮点击 -> 进入播放页
static void play_row_click_handler(lv_event_t *e)
{
    int track_idx = (int)(intptr_t)lv_event_get_user_data(e);
    printf("播放歌曲索引: %d\n", track_idx);
    lv_obj_clean(lv_scr_act());
    page_music_ing();
}

// ======================= UI 组件生成器 =======================

#define ROW_HEIGHT 36
#define HEADER_HEIGHT 32
#define COL_IDX_W 45
#define COL_NAME_W 350
#define COL_ARTIST_W 200
#define COL_ALBUM_W 280
#define COL_ACTION_W 100

static void create_table_header(lv_obj_t *parent)
{
    lv_obj_t *header = lv_obj_create(parent);
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
}

static lv_obj_t *create_song_row(lv_obj_t *parent, const music_track_t *track, int global_idx)
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
    lv_snprintf(idx_buf, sizeof(idx_buf), "%02d", track->index);

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

    lv_obj_t *lb_artist = lv_label_create(row);
    obj_font_set(lb_artist, FONT_TYPE_CN, 14);
    lv_label_set_text(lb_artist, track->artist);
    lv_obj_set_style_text_color(lb_artist, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_width(lb_artist, COL_ARTIST_W);

    lv_obj_t *lb_album = lv_label_create(row);
    obj_font_set(lb_album, FONT_TYPE_CN, 14);
    lv_label_set_text(lb_album, track->album);
    lv_obj_set_style_text_color(lb_album, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_width(lb_album, COL_ALBUM_W);

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

    return row;
}

static void create_music_list_content(lv_obj_t *parent)
{
    music_track_t *tracks = NULL;
    int count = music_api_fetch_list(&tracks);

    if (count <= 0 || tracks == NULL)
    {
        tracks = (music_track_t *)mock_tracks;
        count = MOCK_TRACK_COUNT;
    }

    int page_cnt = (count + SONGS_PER_PAGE - 1) / SONGS_PER_PAGE;
    int page_h = SONGS_PER_PAGE * ROW_HEIGHT;

    lv_obj_t *scroll = lv_obj_create(parent);
    lv_obj_remove_style_all(scroll);
    lv_obj_set_size(scroll, lv_pct(100), page_h);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_snap_y(scroll, LV_SCROLL_SNAP_START);
    lv_obj_set_style_width(scroll, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(scroll, lv_color_hex(0x8a8a8a), LV_PART_SCROLLBAR);

    for (int p = 0; p < page_cnt; p++)
    {
        lv_obj_t *page = lv_obj_create(scroll);
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

void page_music_list()
{
    lv_obj_t *cont = init();

    // ----------------- 左侧侧边栏 cont_bg1 -----------------
    lv_obj_t *cont_bg1 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg1, LV_OBJ_FLAG_SCROLLABLE);
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
    lv_img_set_src(icon1, GET_IMAGE_PATH("icon_unsousuo.png"));
    lv_obj_set_style_img_recolor(icon1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_add_flag(icon1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon1, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_sousuolist");

    /* 播放列表图标 - 当前页高亮（青绿色） */
    lv_obj_t *icon2 = lv_img_create(cont_bg1);
    lv_img_set_src(icon2, GET_IMAGE_PATH("icon_bofanglist.png"));
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0x1DB954), LV_PART_MAIN);
    lv_obj_add_flag(icon2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon2, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofanglist");

    lv_obj_t *icon3 = lv_img_create(cont_bg1);
    lv_img_set_src(icon3, GET_IMAGE_PATH("icon_unbofang.png"));
    lv_obj_set_style_img_recolor(icon3, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_add_flag(icon3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(icon3, icon_click_handler, LV_EVENT_CLICKED, (void *)"icon_bofang");

    // ----------------- 右侧主内容区 cont_bg2 -----------------
    lv_obj_t *cont_bg2 = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont_bg2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont_bg2, 1318, 220);
    lv_obj_align_to(cont_bg2, cont_bg1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_style_bg_color(cont_bg2, lv_color_make(11, 12, 16), LV_PART_MAIN); /* #0B0C10 */
    lv_obj_set_style_bg_opa(cont_bg2, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_bg2, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bg2, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont_bg2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_bg2, 0, LV_PART_MAIN);

    create_table_header(cont_bg2);
    create_music_list_content(cont_bg2);
}