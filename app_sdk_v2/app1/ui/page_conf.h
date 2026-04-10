#ifndef _PAGE_CONF_H_
#define _PAGE_CONF_H_
#include "lvgl.h"
#include "music_conf.h"
#include "em_hal_audio.h"
#include "audio_player_async.h"
typedef enum
{
    WARN_BODY_SENSOR_TRIGGER = 0,
    WARN_SMART_COASTER_TRIGGER,
    WARN_FLAME_SENSOR_TRIGGER,
} WRANING_TYPE_E;

typedef enum
{
    ALARM_ALARM_CLOCK_TRIGGER = 0,
} ALARM_TYPE_E;
typedef enum
{
    TYPE_BODY_SENSOR = 0,
    TYPE_SMART_COASTER,
    TYPE_FLAME_SENSOR,
} SETTING_PARAM_TYPE_E;
void page_test_init(void);
void page_seeting(void);
void page_alarm(void);
void page_alarm_dialog(void);
void page_time_setting1(void);
void page_time_setting2(void);
void page_time_setting3(void);
void page_wifi_setting(void);
void page_tomato_setting(void);
void page_city_setting(void);
void page_music_search(void);
void page_music_list(void);
void page_music_ing(void);
void init_page_ai(void);
void refresh_page_ai(void);
void delete_current_page(lv_style_t *style);
#endif