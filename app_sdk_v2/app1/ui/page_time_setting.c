#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"
#include "page_conf.h"

// 提前声明页面函数，防止编译报错
void page_time_setting1(void);
void page_time_setting2(void);
void page_time_setting3(void);

static lv_style_t com_style;

// ======================= 全局状态管理 =======================

// 定义5个全局选项ID：
// 0: 日期格式 (界面1)
// 1, 2, 3: 表盘格式 1/2/3 (界面2)
// 4: 数字格式 (界面3)
static int global_selected_format = 0; // 默认选中第一个(0)

// 用于保存当前屏幕上正在显示的勾选图标对象指针，切换页面时需清空
static lv_obj_t *current_page_icons[5] = {NULL};
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
    // 切屏前清空指针数组，防止野指针崩溃
    memset(current_page_icons, 0, sizeof(current_page_icons));
    lv_obj_clean(lv_scr_act());

    page_test_init();
}

// 统一的选项点击事件 (替代了之前的 btn_click_absolute_cb1 和 lv_event_cb_alarm)
static void format_select_event_cb(lv_event_t *e)
{
    // 获取传递过来的 全局选项ID
    int clicked_id = (int)(intptr_t)lv_event_get_user_data(e);

    printf("Selected format ID: %d\n", clicked_id);

    // 如果点击的就是当前已选中的，直接返回
    if (global_selected_format == clicked_id)
        return;

    // 更新全局状态
    global_selected_format = clicked_id;

    // 遍历刷新当前屏幕上的所有可选图标
    for (int i = 0; i < 5; i++)
    {
        if (current_page_icons[i] != NULL)
        { // 检查对象是否在当前页面存在
            if (i == global_selected_format)
            {
                lv_img_set_src(current_page_icons[i], GET_IMAGE_PATH("icon_select.png"));
            }
            else
            {
                lv_img_set_src(current_page_icons[i], GET_IMAGE_PATH("icon_unselect.png"));
            }
        }
    }
}

// 滚轮切换事件
static void event_handler(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target(e);
    int id = lv_roller_get_selected(roller);

    // 边界检查：确保id在有效范围内(0-2)
    if (id < 0 || id > 2)
    {
        // 如果id无效，重置为默认值0并返回
        lv_roller_set_selected(roller, 0, LV_ANIM_ON);
        return;
    }

    // 切换界面前，务必清空记录的图标指针数组，防止操作被销毁的对象
    memset(current_page_icons, 0, sizeof(current_page_icons));
    lv_obj_clean(lv_scr_act());

    switch (id)
    {
    case 0:
        page_time_setting1();
        break;
    case 1:
        page_time_setting2();
        break;
    case 2:
        page_time_setting3();
        break;
    default:
        // 理论上不会执行到这里，但为了安全起见
        lv_roller_set_selected(roller, 0, LV_ANIM_ON);
        page_time_setting1();
        break;
    }
}

// ======================= UI初始化模块 =======================

// 初始化通用样式
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
    lv_img_set_src(menu_img, GET_IMAGE_PATH("icon_alarm.png"));
    lv_obj_set_align(menu_img, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_pad_top(menu_img, 20, 0);
    lv_obj_align_to(menu_img, back_img, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

    lv_obj_t *title = lv_label_create(cont);
    obj_font_set(title, FONT_TYPE_CN, 24);
    lv_label_set_text(title, "时间格式设置");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_align_to(title, menu_img, LV_ALIGN_OUT_RIGHT_MID, 20, 3);

    lv_obj_add_event_cb(cont, lv_event_cb_func, LV_EVENT_CLICKED, NULL);

    return cont;
}

static lv_obj_t *init_gunlun(lv_obj_t *parent, int id)
{
    lv_obj_t *roller1 = lv_roller_create(lv_scr_act());
    lv_roller_set_options(roller1,
                          "日期格式\n"
                          "表盘格式\n"
                          "数字格式\n",
                          LV_ROLLER_MODE_NORMAL);
    lv_obj_center(roller1);
    lv_obj_set_style_bg_color(roller1, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_border_width(roller1, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller1, lv_color_hex(0x000000), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller1, lv_color_hex(0xffffff), LV_PART_SELECTED);
    lv_obj_clear_state(roller1, LV_STATE_FOCUS_KEY);
    lv_obj_align(roller1, LV_ALIGN_CENTER, -300, 0);

    lv_obj_set_style_text_font(roller1, get_font(FONT_TYPE_CN, 48), LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller1, get_font(FONT_TYPE_CN, 24), LV_PART_MAIN);

    lv_roller_set_visible_row_count(roller1, 3);
    lv_obj_add_event_cb(roller1, event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_roller_set_selected(roller1, id, LV_ANIM_ON);

    return roller1;
}

static lv_obj_t *init(int id)
{
    com_style_init();

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);

    lv_obj_t *bg_time = lv_img_create(cont);
    lv_img_set_src(bg_time, GET_IMAGE_PATH("bg_tomato_time.png"));
    lv_obj_align(bg_time, LV_ALIGN_RIGHT_MID, 0, 0);

    init_back_view(cont);
    init_gunlun(cont, id);
    return cont;
}

// 创建一个 文字排列的 对象 (用于界面1)
static lv_obj_t *txt_info_view(lv_obj_t *parent, int global_id)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont, &com_style, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "12:05");
    obj_font_set(label, FONT_TYPE_CN, 60);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);

    lv_obj_t *labe2 = lv_label_create(cont);
    lv_label_set_text(labe2, "2026年3月12日 星期四");
    obj_font_set(labe2, FONT_TYPE_CN, 24);
    lv_obj_set_style_text_color(labe2, lv_color_hex(0xffffff), 0);

    // 设置为可点击并绑定全局选项切换事件
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont, format_select_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)global_id);

    lv_obj_align(cont, LV_ALIGN_CENTER, 0, -10);

    return cont;
}

// 创建一个表盘 (用于界面2)
static lv_obj_t *alarm_info_view(lv_obj_t *parent, const char *src, int global_id)
{
    lv_obj_t *wrapper = lv_obj_create(parent);
    lv_obj_set_size(wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wrapper, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(wrapper, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wrapper, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 让整个组合体 (wrapper) 处理点击事件，增加点击范围
    lv_obj_add_flag(wrapper, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(wrapper, format_select_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)global_id);

    lv_obj_t *alarm_time = lv_img_create(wrapper);
    lv_img_set_src(alarm_time, src);

    lv_obj_t *alarm_icon = lv_img_create(wrapper);
    // 判断初始化时应渲染的状态
    if (global_selected_format == global_id)
    {
        lv_img_set_src(alarm_icon, GET_IMAGE_PATH("icon_select.png"));
    }
    else
    {
        lv_img_set_src(alarm_icon, GET_IMAGE_PATH("icon_unselect.png"));
    }

    // 保存图标对象指针
    current_page_icons[global_id] = alarm_icon;

    lv_obj_set_style_pad_top(alarm_icon, 20, LV_PART_MAIN);

    return wrapper;
}

// ======================= 页面主函数 =======================

void page_time_setting1() // 日期格式
{
    lv_obj_t *init0 = init(0);

    // 传入 ID: 0
    lv_obj_t *seeting1 = txt_info_view(init0, 0);

    lv_obj_t *alarm_icon = lv_img_create(init0);

    // 判断初始化时的选择状态
    if (global_selected_format == 0)
    {
        lv_img_set_src(alarm_icon, GET_IMAGE_PATH("icon_select.png"));
    }
    else
    {
        lv_img_set_src(alarm_icon, GET_IMAGE_PATH("icon_unselect.png"));
    }
    current_page_icons[0] = alarm_icon; // 存入数组便于后续刷新

    lv_obj_align_to(alarm_icon, seeting1, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
}

void page_time_setting2() // 表盘格式
{
    lv_obj_t *init1 = init(1);

    lv_obj_t *cont1 = lv_obj_create(init1);
    lv_obj_set_size(cont1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(cont1, &com_style, LV_PART_MAIN);
    lv_obj_align(cont1, LV_ALIGN_CENTER, 120, 0);

    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont1, 20, LV_PART_MAIN);

    // 分别传入全局 ID: 1, 2, 3
    alarm_info_view(cont1, GET_IMAGE_PATH("icon_dial.png"), 1);
    alarm_info_view(cont1, GET_IMAGE_PATH("icon_dial1.png"), 2);
    alarm_info_view(cont1, GET_IMAGE_PATH("icon_dial2.png"), 3);
}

void page_time_setting3() // 数字格式
{
    lv_obj_t *init2 = init(2); // 修复原代码中的init(1)

    lv_obj_t *label = lv_label_create(init2);
    lv_label_set_text(label, "12:05");
    obj_font_set(label, FONT_TYPE_CN, 60);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);

    // 使时间文字可点击，绑定 ID: 4
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label, format_select_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)4);

    lv_obj_t *alarm_icon = lv_img_create(init2);
    // 判断初始化时的选择状态
    if (global_selected_format == 4)
    {
        lv_img_set_src(alarm_icon, GET_IMAGE_PATH("icon_select.png"));
    }
    else
    {
        lv_img_set_src(alarm_icon, GET_IMAGE_PATH("icon_unselect.png"));
    }
    current_page_icons[4] = alarm_icon; // 存入数组便于后续刷新

    lv_obj_align_to(alarm_icon, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
}