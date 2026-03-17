#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "page_conf.h"
#include "font_conf.h"
#include "wpa_manager.h"
#include "http_manager.h"

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

    // 页面初始化
    page_test_init();

    wpa_manager_open();

    // 【修改点1】：关键延时！给 wpa_supplicant 守护进程和 Socket 建立留出时间
    // 很多嵌入式Linux板子在这里至少需要等待 1~2 秒，才能开始下发指令
    sleep(2);

    // 注册回调
    wpa_manager_add_callback(wifi_status_callback_func, wifi_connect_status_callback);

    // 【修改点2】：结构体必须先清零，防止脏数据导致密码末尾存在乱码！
    wpa_ctrl_wifi_info_t wifi_info;
    memset(&wifi_info, 0, sizeof(wpa_ctrl_wifi_info_t));

    // 【修改点3】：强烈建议使用 strcpy 替代 memcpy，这样会自动带上 '\0' 结束符
    strcpy(wifi_info.ssid, "talent");
    strcpy(wifi_info.psw, "1234567890");

    printf("准备连接 WiFi, SSID: %s\n", wifi_info.ssid);
    wpa_manager_wifi_connect(&wifi_info);

    while (1)
    {
        lv_task_handler();
        usleep(5000);
    }
    return 0;
}