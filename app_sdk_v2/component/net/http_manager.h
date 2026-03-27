#ifndef _HTTP_MANAGER_H
#define _HTTP_MANAGER_H

typedef enum
{
    NET_GET_WEATHER = 0,
    NET_GET_TIME,         // 【新增】网络时间的队列命令 ID
    NET_MUSIC_SEARCH,     // 搜索歌曲
    NET_MUSIC_GET_URL,    // 获取播放地址
    NET_MUSIC_GET_LYRIC,  // 获取歌词
    NET_MUSIC_GET_DETAIL, // 获取歌曲详情
    NET_MUSIC_DOWNLOAD,   // 【新增】下载音乐
} NET_COMM_ID;

typedef struct
{
    NET_COMM_ID id;
    char host[512];
    char path[512];
    char data[50];
    char type[10];
    int loop_flag;
} net_obj;

typedef struct
{
    char *data;
    size_t size;
} http_resp_data_t;

// 定义回调函数的类型
typedef void (*weather_callback_fun)(char *str);
typedef void (*time_callback_fun)(char *str); // 【新增】时间获取的回调定义

// 音乐相关回调函数类型
typedef void (*music_search_callback_fun)(char *str);
typedef void (*music_url_callback_fun)(char *str);
typedef void (*music_lyric_callback_fun)(char *str);
typedef void (*music_detail_callback_fun)(char *str);

typedef void (*music_download_callback_fun)(int success);

// 【新增】下载 API
void http_music_download_async(const char *url);
void http_set_music_download_callback(music_download_callback_fun func);

int http_request_create(void);

// 天气 API
void http_get_weather_async(char *key, char *city);
void http_set_weather_callback(weather_callback_fun func);

// 【新增】时间 API
void http_get_time_async(const char *url);
void http_set_time_callback(time_callback_fun func);

// 音乐 API
void http_music_search_async(const char *keyword, int page, int limit);
void http_music_get_url_async(int song_id);
void http_music_get_lyric_async(int song_id);
void http_music_get_detail_async(int song_id);

// 设置回调函数
void http_set_music_search_callback(music_search_callback_fun func);
void http_set_music_url_callback(music_url_callback_fun func);
void http_set_music_lyric_callback(music_lyric_callback_fun func);
void http_set_music_detail_callback(music_detail_callback_fun func);

#endif