#ifndef _HTTP_MANAGER_H
#define _HTTP_MANAGER_H

typedef enum
{
    NET_GET_WEATHER = 0,
    NET_GET_TIME, // 【新增】网络时间的队列命令 ID
} NET_COMM_ID;

typedef struct
{
    NET_COMM_ID id;
    char host[100];
    char path[100];
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

int http_request_create(void);

// 天气 API
void http_get_weather_async(char *key, char *city);
void http_set_weather_callback(weather_callback_fun func);

// 【新增】时间 API
void http_get_time_async(const char *url);
void http_set_time_callback(time_callback_fun func);

#endif