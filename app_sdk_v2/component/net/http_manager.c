/*
 * @Author: xiaozhi
 * @Date: 2024-09-30 00:21:03
 * @Last Modified by: xiaozhi
 * @Last Modified time: 2024-10-08 23:45:16
 */

#include <stdio.h>
#include <stdlib.h>
#include "cJSON/cJSON.h"
#include <curl/curl.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "http_manager.h"
#include "osal_thread.h"
#include "osal_queue.h"

static osal_queue_t net_queue = NULL;
static osal_thread_t net_thread = NULL;

static weather_callback_fun weather_callback_func = NULL;
static time_callback_fun time_callback_func = NULL; // 【新增】记录时间回调函数的全局变量

/**
 * @brief 组装HTTP请求URL
 */
static int assemble_url(const char *host, const char *path, char **url)
{
    *url = malloc(strlen(host) + strlen(path) + 1);
    strcpy(*url, host);
    strcat(*url, path);
    return 0;
}

/**
 * @brief CURL数据接收回调函数
 */
static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    http_resp_data_t *mem = (http_resp_data_t *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr)
        return 0; // 内存分配失败

    mem->data = ptr;
    memcpy(mem->data + mem->size, data, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';
    return realsize;
}

int http_request_method(const char *host, const char *path, const char *method, const char *request_json, char **response_json)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return -1;

    // 组装并设置URL
    char *url = NULL;
    assemble_url(host, path, &url);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // 通用配置
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);        // 调试模式：启用详细输出模式
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);       // 设置请求超时时间（单位：秒）
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 禁用SSL证书验证
    // 设置响应处理
    http_resp_data_t response_data = {0};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback); // 注册响应数据接收回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);     // 指定回调函数的用户数据

    // POST方法特殊处理
    if (strcmp(method, "POST") == 0)
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json);
    }
    // 设置HTTP头部
    struct curl_slist *header = curl_slist_append(NULL, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    // 执行请求
    CURLcode code = curl_easy_perform(curl);
    int ret = (code == CURLE_OK) ? 0 : -1;
    // 处理响应
    if (ret == 0)
    {
        printf("Response len: %ld, data: %s\n", response_data.size, response_data.data);
        *response_json = response_data.data; // 转移内存所有权
    }
    else
    {
        printf("Request failed: %s (%d)\n", curl_easy_strerror(code), code);
        free(response_data.data); // 失败时释放内存
    }
    // 资源清理
    curl_slist_free_all(header);
    free(url);
    curl_easy_cleanup(curl);
    return ret;
}

void parseWeatherData(const char *json_data)
{
    cJSON *root = cJSON_Parse(json_data);
    if (!root)
    {
        fprintf(stderr, "Error parsing JSON data.\n");
        return;
    }
    // 获取 results 数组
    cJSON *results = cJSON_GetObjectItem(root, "results");
    if (!results || !cJSON_IsArray(results))
    {
        fprintf(stderr, "Invalid JSON format: missing 'results' array.\n");
        cJSON_Delete(root);
        return;
    }
    int num_results = cJSON_GetArraySize(results);
    if (num_results <= 0)
    {
        fprintf(stderr, "No results found.\n");
        cJSON_Delete(root);
        return;
    }
    // 处理第一个结果
    cJSON *result = cJSON_GetArrayItem(results, 0);
    if (!result)
    {
        fprintf(stderr, "Invalid JSON format: missing first result.\n");
        cJSON_Delete(root);
        return;
    }
    // 获取 location 对象
    cJSON *location = cJSON_GetObjectItem(result, "location");
    if (!location || !cJSON_IsObject(location))
    {
        fprintf(stderr, "Invalid JSON format: missing 'location' object.\n");
        cJSON_Delete(root);
        return;
    }
    // 获取 now 对象
    cJSON *now = cJSON_GetObjectItem(result, "now");
    if (!now || !cJSON_IsObject(now))
    {
        fprintf(stderr, "Invalid JSON format: missing 'now' object.\n");
        cJSON_Delete(root);
        return;
    }
    // 打印 location 字段
    printf("Location Name: %s\n", cJSON_GetObjectItem(location, "name")->valuestring);
    // 打印 now 字段
    printf("Current Weather: %s\n", cJSON_GetObjectItem(now, "text")->valuestring);
    printf("Temperature: %s\n", cJSON_GetObjectItem(now, "temperature")->valuestring);

    char weather_info[50];
    memset(weather_info, 0, sizeof(weather_info));
    strcat(weather_info, cJSON_GetObjectItem(location, "name")->valuestring);
    strcat(weather_info, " ");
    strcat(weather_info, cJSON_GetObjectItem(now, "text")->valuestring);
    strcat(weather_info, " ");
    strcat(weather_info, cJSON_GetObjectItem(now, "temperature")->valuestring);
    strcat(weather_info, "°C");
    if (weather_callback_func != NULL)
        weather_callback_func(weather_info);
    cJSON_Delete(root);
}

// 网络模块线程
static void *net_thread_fun(void *arg)
{
    int ret = OSAL_ERROR;
    net_obj obj;
    memset(&obj, 0, sizeof(net_obj));
    char *response_json_str = NULL; // 初始化防野指针

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while (1)
    {
        ret = osal_queue_recv(&net_queue, (void *)&obj, 100);
        if (ret == OSAL_SUCCESS)
        {
            NET_COMM_ID id = obj.id;
            switch (id)
            {
            case NET_GET_WEATHER:
                printf("handle NET_GET_WEATHER\n");
                response_json_str = NULL;
                // 请求前置空，由于可能失败，必须验证请求方法返回值是否等于 0
                if (http_request_method(obj.host, obj.path, obj.type, obj.data, &response_json_str) == 0)
                {
                    if (response_json_str != NULL)
                    {
                        parseWeatherData(response_json_str);
                        free(response_json_str);
                    }
                }
                break;

            // 【新增】处理获取时间消息
            case NET_GET_TIME:
                printf("handle NET_GET_TIME\n");
                response_json_str = NULL;
                if (http_request_method(obj.host, obj.path, obj.type, obj.data, &response_json_str) == 0)
                {
                    if (response_json_str != NULL)
                    {
                        // 直接调用绑定的回调函数，将文本抛给 page_main.c 处理
                        if (time_callback_func != NULL)
                        {
                            time_callback_func(response_json_str);
                        }
                        free(response_json_str);
                    }
                }
                break;

            default:
                break;
            }
        }
        osal_thread_sleep(500);
    }
}

// 异步获取天气
void http_get_weather_async(char *key, char *city)
{
    net_obj obj;
    memset(&obj, 0, sizeof(net_obj));
    strcpy(obj.host, "https://api.seniverse.com");
    sprintf(obj.path, "/v3/weather/now.json?key=%s&location=%s&language=zh-Hans&unit=c", key, city);
    obj.id = NET_GET_WEATHER;
    strcpy(obj.data, "");
    strcpy(obj.type, "GET");
    int ret = osal_queue_send(&net_queue, &obj, sizeof(net_obj), 1000);
    if (ret == OSAL_ERROR)
    {
        printf("queue send error");
    }
}

// 设置获取天气回调函数
void http_set_weather_callback(weather_callback_fun func)
{
    weather_callback_func = func;
}

// 【新增】异步获取时间
void http_get_time_async(const char *url)
{
    net_obj obj;
    memset(&obj, 0, sizeof(net_obj));

    // 我们将整个网址作为 host，path 留空。
    // 因为底层 assemble_url 是拼接这两者，所以 "http://.../gettimestamp" + "" 是完全合法的
    strncpy(obj.host, url, sizeof(obj.host) - 1);
    strcpy(obj.path, "");

    obj.id = NET_GET_TIME;
    strcpy(obj.data, "");
    strcpy(obj.type, "GET");

    int ret = osal_queue_send(&net_queue, &obj, sizeof(net_obj), 1000);
    if (ret == OSAL_ERROR)
    {
        printf("queue send error in time fetch\n");
    }
}

// 【新增】设置获取时间回调函数
void http_set_time_callback(time_callback_fun func)
{
    time_callback_func = func;
}

// HTTP模块创建
int http_request_create()
{
    int ret = OSAL_ERROR;
    ret = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (ret != 0)
        return -1;
    ret = osal_queue_create(&net_queue, "net_queue", sizeof(net_obj), 50);
    if (ret == OSAL_ERROR)
    {
        printf("create queue error");
        return -1;
    }
    ret = osal_thread_create(&net_thread, net_thread_fun, NULL);
    if (ret == OSAL_ERROR)
    {
        printf("create thread error");
        return -1;
    }
    return 0;
}