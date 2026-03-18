#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "page_conf.h"
#include "font_conf.h"
#include "wpa_manager.h"
#include "http_manager.h"
#include "audio_player_async.h"

extern void lv_port_disp_init(bool is_disp_orientation);
extern void lv_port_indev_init(void);
extern void wifi_connect_status_callback(WPA_WIFI_CONNECT_STATUS_E status);

void wifi_status_callback_func(WPA_WIFI_STATUS_E status)
{
    printf("----->wifi_status_callback_func: %d\n", status);
}

int main()
{
    // LVGL框架初始化
    lv_init();
    lv_port_disp_init(false);
    lv_port_indev_init();

    http_request_create();

    font_init();

    // 初始化音频播放器
    init_async_audio_player();

    // 页面初始化
    page_test_init();

    wpa_manager_open();

    // 【修改点1】：关键延时！给 wpa_supplicant 守护进程和 Socket 建立留出时间
    // 很多嵌入式Linux板子在这里至少需要等待 1~2 秒，才能开始下发指令
    sleep(2);

    // 注册回调
    wpa_manager_add_callback(wifi_status_callback_func, wifi_connect_status_callback);

    // WiFi连接现在通过 page_wifi_setting.c 中的用户输入界面处理
    printf("WiFi连接将通过设置界面手动配置\n");

    while (1)
    {
        lv_task_handler();
        usleep(5000);
    }
    return 0;
}