#include "lvgl_main.h"
#include "freertos_main.h"
#include "gd32h7xx_timer.h"
#include "./USART/usart.h"
#include "./TIMER/timer.h"
#include "./INA226/ina226.h"
#include "./SC8721/sc8721.h"
#include "./RTC/rtc.h"
#include "./ADC/adc.h"
#include "./MALLOC/malloc.h"
#include "lvgl_file_manager.h"
#include "lvgl_setting.h"
#include "lvgl_debugger.h"
#include "lvgl_usart.h"
#include "lvgl_can.h"

lvgl_main_struct lvgl_main;
lvgl_style_struct lvgl_style;
static ina226_data_struct lvgl_ina226_data;
static rtc_data_struct lvgl_rtc_data;
static lvgl_cpu_info_struct lvgl_cpu_info;
static lvgl_main_mem_perused_struct lvgl_main_mem_perused;
lvgl_main_state_struct lvgl_main_state;

static __ALIGNED(4) uint8_t wallpaper_buffer[1024 * 1024] __attribute__((section(".bss.ARM.__at_0XC0100000")));
volatile uint8_t main_menu_page_flag = 0;
volatile static uint8_t timer_run_count = 0;
    
/* 数字电源部分开始 */
static float power_voltage_value = 5.00;    /* 单位V */
static float power_current_value = 0.5;   /* 单位A */
#define POWER_POINT_COUNT   100
static int32_t *power_chart;
/* 数字电源部分结束 */

/* 在线调试器部分开始 */
volatile uint8_t gDebuggerOnLineIdleFlag = 0;
volatile uint8_t debugger_connect_status = 0;
volatile uint8_t debugger_running_status = 0;
volatile uint8_t debugger_id_update_flag = 0;
volatile uint32_t debugger_id = 0;
volatile uint32_t debugger_download_count = 0;
volatile float debugger_download_time = 0;   /* 单位ms */
/* 在线调试器部分结束 */

/* static function declarations */
static void lvgl_main_status_bar(void);
static void lvgl_main_menu(void);
static void lvgl_show_ina226_data(void);
static void lvgl_show_cpu_info(void);
static void lvgl_show_mem_perused(void);
static void lvgl_show_wifi_name(void);
static void lvgl_show_rtc_data(void);
static void lvgl_style_init(void);
static uint8_t lvgl_wallpaper_load(lv_img_dsc_t *image, const char *path);

static void lvgl_power_creat(uint16_t width, uint16_t height);
static void lvgl_debugger_on_line_creat(uint16_t width, uint16_t height);

/**************************************************************
函数名称 ： timer_cb
功    能 ： 定时器回调
参    数 ： timer
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void timer_cb(lv_timer_t *timer)
{
    timer_run_count++;
    
    xQueueReceive(xQueueIna226Data, &lvgl_ina226_data, 0);
    xQueueReceive(xQueueCpuInfo, &lvgl_cpu_info, 0);
    xQueueReceive(xQueueRtcData, &lvgl_rtc_data, 0);
    xQueueReceive(xQueueMEMPerused, &lvgl_main_mem_perused, 0);
    xQueueReceive(xQueueLvglMainState, &lvgl_main_state, 0);
    

    lv_label_set_text_fmt(lvgl_main.ina226_label, "PD: %.2fV", lvgl_ina226_data.bus_voltage[3]);
    lv_label_set_text_fmt(lvgl_main.cpu_info_label, "CPU: %u%% %.2f℃", lvgl_cpu_info.utilization, lvgl_cpu_info.adc2_data.high_precision_cpu_temperature);
    lv_label_set_text_fmt(lvgl_main.mem_perused_label, "DTCM: %.1f%%", lvgl_main_mem_perused.dtcm);
    lv_label_set_text_fmt(lvgl_main.rtc_data_label, "%02X:%02X:%02X", lvgl_rtc_data.hour, lvgl_rtc_data.minute, lvgl_rtc_data.second);
    
    if(lvgl_main.ina226_popup_obj != NULL)
    {
        lv_label_set_text_fmt(lvgl_main.ina226_popup_label, "PD: %.2fV %.2fA %.2fW\n5V: %.2fV %.2fA %.2fW\nSIN: %.2fV %.2fA %.2fW\nSOUT: %.2fV %.2fA %.2fW", \
                                                                 lvgl_ina226_data.bus_voltage[3], (float)lvgl_ina226_data.current[3] / 1000, (float)lvgl_ina226_data.power[3] / 1000, \
                                                                 lvgl_ina226_data.bus_voltage[2], (float)lvgl_ina226_data.current[2] / 1000, (float)lvgl_ina226_data.power[2] / 1000, \
                                                                 lvgl_ina226_data.bus_voltage[1], (float)lvgl_ina226_data.current[1] / 1000, (float)lvgl_ina226_data.power[1] / 1000, \
                                                                 lvgl_ina226_data.bus_voltage[0], (float)lvgl_ina226_data.current[0] / 1000, (float)lvgl_ina226_data.power[0] / 1000);
    }
    
    if(lvgl_main.cpu_info_popup_obj != NULL)
    {
        lv_label_set_text_fmt(lvgl_main.cpu_info_popup_label, "UTIL: %u%%\nTEMP: %.2f℃\nHTEMP: %.2f℃\nVBAT: %.2fV\nINVREF: %.2fV", lvgl_cpu_info.utilization, \
                                                               lvgl_cpu_info.adc2_data.cpu_temperature, lvgl_cpu_info.adc2_data.high_precision_cpu_temperature, \
                                                               lvgl_cpu_info.adc2_data.vbat, lvgl_cpu_info.adc2_data.internal_vref);
    }
    
    if(lvgl_main.mem_perused_popup_obj != NULL)
    {
        lv_label_set_text_fmt(lvgl_main.mem_perused_popup_label, "SRAM: %.1f%%\nSRAM0_1: %.1f%%\nDTCM: %.1f%%\nSDRAM: %.1f%%", lvgl_main_mem_perused.sram, lvgl_main_mem_perused.sram01, \
                                                                                                                               lvgl_main_mem_perused.dtcm, lvgl_main_mem_perused.sdram);
    }
    
    if(lvgl_main.rtc_data_popup_obj != NULL)
    {
        lv_label_set_text_fmt(lvgl_main.rtc_data_popup_label, "%04X.%X.%X %s\n%02X:%02X:%02X", lvgl_rtc_data.year, lvgl_rtc_data.month, lvgl_rtc_data.day, rtc_week[lvgl_rtc_data.week],  \
                                                                                                    lvgl_rtc_data.hour, lvgl_rtc_data.minute, lvgl_rtc_data.second);
    }
    
    if(lvgl_main_state.wifi)
    {
        lv_obj_remove_flag(lvgl_main.wifi_label, LV_OBJ_FLAG_HIDDEN);
        if(lvgl_main.wifi_popup_obj != NULL)
        {
            lv_label_set_text_static(lvgl_main.wifi_popup_label, lvgl_main_state.wifi_name);
        }
    }
    else
    {
        lv_obj_add_flag(lvgl_main.wifi_label, LV_OBJ_FLAG_HIDDEN);
        if(lvgl_main.wifi_popup_obj != NULL)
        {
            lv_obj_delete(lvgl_main.wifi_popup_obj);
            lvgl_main.wifi_popup_obj = NULL;
        }
    }
    
    if(timer_run_count >= 2)
    {
        timer_run_count = 0;
        
        /* 数字电源部分值更新 */
        memmove(power_chart + 1, power_chart, (POWER_POINT_COUNT - 1) * sizeof(int32_t));
        memmove(power_chart + POWER_POINT_COUNT + 1, power_chart + POWER_POINT_COUNT, (POWER_POINT_COUNT - 1) * sizeof(int32_t));
        *power_chart = (int32_t)(lvgl_ina226_data.bus_voltage[0] * 10);
        *(power_chart + POWER_POINT_COUNT) = (int32_t)((float)lvgl_ina226_data.current[0] / 100);
        
        if(main_menu_page_flag == 0)
        {
            /* 数字电源部分 */
            lv_label_set_text_fmt(lvgl_main.power_voltage_lable, "%.2fV", lvgl_ina226_data.bus_voltage[0]);
            lv_label_set_text_fmt(lvgl_main.power_current_lable, "%.2fA", (float)lvgl_ina226_data.current[0] / 1000);
            lv_label_set_text_fmt(lvgl_main.power_power_lable, "%.1fW", (float)lvgl_ina226_data.power[0] / 1000);
            lv_chart_refresh(lvgl_main.power_chart);
            
            /* 在线调试部分 */
            if(debugger_connect_status == 1)
            {
                gDebuggerOnLineIdleFlag = 1;
                if(debugger_id_update_flag == 0XAA)
                {
                    if(debugger_id)
                    {
                        lv_label_set_text_fmt(lvgl_main.debugger_id_label, "DBG ID: 0x%08X", debugger_id);
                    }
                    else
                    {
                        lv_label_set_text(lvgl_main.debugger_id_label, "DBG ID: NULL");
                    }
                    debugger_id_update_flag = 0;
                }
                
                lv_led_on(lvgl_main.debugger_connect_led);
                lv_label_set_text(lvgl_main.debugger_download_state_label, "程序下载中...");
                debugger_download_count += 1;
                lv_label_set_text_fmt(lvgl_main.debugger_download_count_label, "%d", debugger_download_count);
                timer_counter_value_config(TIMER51, 0);
                timer_enable(TIMER51);
                debugger_connect_status = 0xFF;
            }
            else if(debugger_connect_status == 0)
            {
                gDebuggerOnLineIdleFlag = 0;
                lv_led_off(lvgl_main.debugger_connect_led);
                lv_label_set_text(lvgl_main.debugger_download_state_label, "空闲");
                timer_disable(TIMER51);
                debugger_download_time = (float)timer_counter_read(TIMER51) / 10;
                lv_label_set_text_fmt(lvgl_main.debugger_download_time_label, "%.1fms", debugger_download_time);
                debugger_connect_status = 0xFF;
            }
            
            if(debugger_running_status == 1)
            {
                lv_led_on(lvgl_main.debugger_running_led);
                lv_label_set_text(lvgl_main.debugger_download_state_label, "调试运行中...");
                debugger_running_status = 0xFF;
            }
            else if(debugger_running_status == 0)
            {
                lv_led_off(lvgl_main.debugger_running_led);
                lv_label_set_text(lvgl_main.debugger_download_state_label, "空闲");
                debugger_running_status = 0xFF;
            }
        }
    }
}

/**************************************************************
函数名称 ： set_width
功    能 ： 设置宽度回调
参    数 ： var, v
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void set_width(void * var, int32_t v)
{
    lv_obj_set_width(var, v);
}

/**************************************************************
函数名称 ： set_height
功    能 ： 设置高度回调
参    数 ： var, v
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void set_height(void * var, int32_t v)
{
    lv_obj_set_height(var, v);
}

/**************************************************************
函数名称 ： anim_completed_handler
功    能 ： 动画完成回调
参    数 ： a
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void anim_completed_handler(lv_anim_t *a)
{
    if(a->var == lvgl_main.main_obj)
    {
        lvgl_main_status_bar();
        lvgl_main_menu();
    }
    else if(a->var == lvgl_main.can_app)
    {
        lvgl_main.file_manager_app_label = lv_label_create(lvgl_main.menu_obj);
        lv_label_set_text(lvgl_main.file_manager_app_label, "文件管理");
        lv_obj_set_style_text_font(lvgl_main.file_manager_app_label, &lv_font_fzst_24, 0);
        lv_obj_align_to(lvgl_main.file_manager_app_label, lvgl_main.file_manager_app, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        
        lvgl_main.setting_app_label = lv_label_create(lvgl_main.menu_obj);
        lv_label_set_text(lvgl_main.setting_app_label, "设置");
        lv_obj_set_style_text_font(lvgl_main.setting_app_label, &lv_font_fzst_24, 0);
        lv_obj_align_to(lvgl_main.setting_app_label, lvgl_main.setting_app, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        
        lvgl_main.debugger_app_label = lv_label_create(lvgl_main.menu_obj);
        lv_label_set_text(lvgl_main.debugger_app_label, "离线调试器");
        lv_obj_set_style_text_font(lvgl_main.debugger_app_label, &lv_font_fzst_24, 0);
        lv_obj_align_to(lvgl_main.debugger_app_label, lvgl_main.debugger_app, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        
        lvgl_main.usart_app_label = lv_label_create(lvgl_main.menu_obj);
        lv_label_set_text(lvgl_main.usart_app_label, "串口终端");
        lv_obj_set_style_text_font(lvgl_main.usart_app_label, &lv_font_fzst_24, 0);
        lv_obj_align_to(lvgl_main.usart_app_label, lvgl_main.usart_app, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        
        lvgl_main.can_app_label = lv_label_create(lvgl_main.menu_obj);
        lv_label_set_text(lvgl_main.can_app_label, "CAN终端");
        lv_obj_set_style_text_font(lvgl_main.can_app_label, &lv_font_fzst_24, 0);
        lv_obj_align_to(lvgl_main.can_app_label, lvgl_main.can_app, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        
        /* 动画完成后创建定时器刷新数据 */
        lvgl_main.timer = lv_timer_create(timer_cb, 250, NULL);
        xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)0, (eNotifyAction)eSetValueWithOverwrite);
    }
    else
    {
        
    }
}

/**************************************************************
函数名称 ： status_bar_event_handler
功    能 ： 状态栏事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void status_bar_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(obj == lvgl_main.ina226_label)
            {
                if(lvgl_main.ina226_popup_obj == NULL)
                {
                    lvgl_show_ina226_data();
                }
                else
                {
                    lv_obj_delete(lvgl_main.ina226_popup_obj);
                    lvgl_main.ina226_popup_obj = NULL;
                }
            }
            else if(obj == lvgl_main.cpu_info_label)
            {
                if(lvgl_main.cpu_info_popup_obj == NULL)
                {
                    lvgl_show_cpu_info();
                }
                else
                {
                    lv_obj_delete(lvgl_main.cpu_info_popup_obj);
                    lvgl_main.cpu_info_popup_obj = NULL;
                }
            }
            else if(obj == lvgl_main.mem_perused_label)
            {
                if(lvgl_main.mem_perused_popup_obj == NULL)
                {
                    lvgl_show_mem_perused();
                }
                else
                {
                    lv_obj_delete(lvgl_main.mem_perused_popup_obj);
                    lvgl_main.mem_perused_popup_obj = NULL;
                }
            }
            else if(obj == lvgl_main.wifi_label)
            {
                if(lvgl_main.wifi_popup_obj == NULL)
                {
                    lvgl_show_wifi_name();
                }
                else
                {
                    lv_obj_delete(lvgl_main.wifi_popup_obj);
                    lvgl_main.wifi_popup_obj = NULL;
                }
            }
            else if(obj == lvgl_main.rtc_data_label)
            {
                if(lvgl_main.rtc_data_popup_obj == NULL)
                {
                    lvgl_show_rtc_data();
                }
                else
                {
                    lv_obj_delete(lvgl_main.rtc_data_popup_obj);
                    lvgl_main.rtc_data_popup_obj = NULL;
                }
            }

            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： app_event_handler
功    能 ： app事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void app_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    switch(code)
    {
        case LV_EVENT_CLICKED:
            /* main_menu_page_flag |= 0x01 */
            if(obj == lvgl_main.file_manager_app)
            {
                lvgl_file_manager_creat(lvgl_main.menu_obj);
            }
            /* main_menu_page_flag |= 0x02 */
            else if(obj == lvgl_main.setting_app)
            {
                lvgl_setting_creat(lvgl_main.menu_obj);
            }
            /* main_menu_page_flag |= 0x04 */
            else if(obj == lvgl_main.debugger_app)
            {
                lvgl_debugger_off_line_creat(lvgl_main.menu_obj);
            }
            /* main_menu_page_flag |= 0x08 */
            else if(obj == lvgl_main.usart_app)
            {
                lvgl_usart_creat(lvgl_main.menu_obj);
            }
            /* main_menu_page_flag |= 0x10 */
            else if(obj == lvgl_main.can_app)
            {
                lvgl_can_creat(lvgl_main.menu_obj);
            }
            lvgl_hidden_main_menu();
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： lvgl_start
功    能 ： 绘制主界面
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_start(void)
{
    lv_anim_t a1;
    lv_anim_t a2;

    lvgl_style_init();
    lvgl_wallpaper_load(&lvgl_main.wallpaper_dsc, "C:/SYSTEM/wallpaper/wallpaper.bin");
    
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    
    lvgl_main.main_obj = lv_obj_create(lv_screen_active());
    lv_obj_set_size(lvgl_main.main_obj, lv_display_get_horizontal_resolution(lv_display_get_default()), lv_display_get_vertical_resolution(lv_display_get_default()));
    lv_obj_center(lvgl_main.main_obj);
    lv_obj_set_style_radius(lvgl_main.main_obj, 8, 0);
    lv_obj_set_style_pad_top(lvgl_main.main_obj, 0, 0);
    lv_obj_set_style_pad_bottom(lvgl_main.main_obj, 0, 0);
    lv_obj_set_style_pad_left(lvgl_main.main_obj, 0, 0);
    lv_obj_set_style_pad_right(lvgl_main.main_obj, 0, 0);
    lv_obj_set_style_border_width(lvgl_main.main_obj, 0, 0);
    lv_obj_set_style_outline_width(lvgl_main.main_obj, 0, 0);
    lv_obj_remove_flag(lvgl_main.main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(lvgl_main.main_obj, true, 0);

    lvgl_main.wallpaper = lv_image_create(lvgl_main.main_obj);
    lv_image_set_src(lvgl_main.wallpaper, &lvgl_main.wallpaper_dsc);
    lv_obj_center(lvgl_main.wallpaper);
    
    lvgl_main.status_bar_obj = lv_obj_create(lvgl_main.main_obj);
    lv_obj_set_size(lvgl_main.status_bar_obj, lv_pct(100), lv_pct(6));
    lv_obj_align(lvgl_main.status_bar_obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(lvgl_main.status_bar_obj, 0, 0);
    lv_obj_set_style_pad_top(lvgl_main.status_bar_obj, 0, 0);
    lv_obj_set_style_pad_bottom(lvgl_main.status_bar_obj, 0, 0);
    lv_obj_set_style_pad_left(lvgl_main.status_bar_obj, 0, 0);
    lv_obj_set_style_pad_right(lvgl_main.status_bar_obj, 0, 0);
    lv_obj_set_style_border_width(lvgl_main.status_bar_obj, 0, 0);
    lv_obj_set_style_outline_width(lvgl_main.status_bar_obj, 0, 0);
    lv_obj_remove_flag(lvgl_main.status_bar_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lvgl_main.status_bar_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lvgl_main.status_bar_obj, LV_OPA_40, 0);
    
    lvgl_main.menu_obj = lv_obj_create(lvgl_main.main_obj);
    lv_obj_set_size(lvgl_main.menu_obj, lv_pct(100), lv_pct(94));
    lv_obj_align(lvgl_main.menu_obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(lvgl_main.menu_obj, 0, 0);
    lv_obj_set_style_pad_top(lvgl_main.menu_obj, 0, 0);
    lv_obj_set_style_pad_bottom(lvgl_main.menu_obj, 0, 0);
    lv_obj_set_style_pad_left(lvgl_main.menu_obj, 0, 0);
    lv_obj_set_style_pad_right(lvgl_main.menu_obj, 0, 0);
    lv_obj_set_style_border_width(lvgl_main.menu_obj, 0, 0);
    lv_obj_set_style_outline_width(lvgl_main.menu_obj, 0, 0);
    lv_obj_remove_flag(lvgl_main.menu_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(lvgl_main.menu_obj, LV_OPA_TRANSP, 0);

    lv_anim_init(&a1);
    lv_anim_set_var(&a1, lvgl_main.main_obj);
    lv_anim_set_values(&a1, 0, lv_display_get_horizontal_resolution(lv_display_get_default()));
    lv_anim_set_exec_cb(&a1, set_width);
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_in_out);
    lv_anim_set_duration(&a1, 240);

    lv_anim_init(&a2);
    lv_anim_set_var(&a2, lvgl_main.main_obj);
    lv_anim_set_values(&a2, 0, lv_display_get_vertical_resolution(lv_display_get_default()));
    lv_anim_set_exec_cb(&a2, set_height);
    lv_anim_set_path_cb(&a2, lv_anim_path_ease_in_out);
    lv_anim_set_duration(&a2, 240);

    lv_anim_set_completed_cb(&a2, anim_completed_handler);
    lv_anim_start(&a1);
    lv_anim_start(&a2);
}

/**************************************************************
函数名称 ： lvgl_main_status_bar
功    能 ： 绘制状态栏
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_main_status_bar(void)
{
    lv_obj_t *obj1, *obj2;
    
    obj1 = lv_obj_create(lvgl_main.status_bar_obj);
    lv_obj_set_size(obj1, lv_pct(70), lv_pct(100));
    lv_obj_align(obj1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_top(obj1, 0, 0);
    lv_obj_set_style_pad_bottom(obj1, 0, 0);
    lv_obj_set_style_pad_left(obj1, 5, 0);
    lv_obj_set_style_pad_right(obj1, 5, 0);
    lv_obj_remove_flag(obj1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(obj1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_side(obj1, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_flex_flow(obj1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lvgl_main.ina226_label = lv_label_create(obj1);
    lv_label_set_text_fmt(lvgl_main.ina226_label, "PD: %.2fV", lvgl_ina226_data.bus_voltage[3]);
    lv_obj_set_style_text_font(lvgl_main.ina226_label, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.ina226_label, lv_color_white(), 0);
    lv_obj_add_flag(lvgl_main.ina226_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvgl_main.ina226_label, status_bar_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_main.cpu_info_label = lv_label_create(obj1);
    lv_label_set_text_fmt(lvgl_main.cpu_info_label, "CPU: %u%% %.2f℃", lvgl_cpu_info.utilization, lvgl_cpu_info.adc2_data.high_precision_cpu_temperature);
    lv_obj_set_style_text_font(lvgl_main.cpu_info_label, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.cpu_info_label, lv_color_white(), 0);
    lv_obj_add_flag(lvgl_main.cpu_info_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvgl_main.cpu_info_label, status_bar_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_main.mem_perused_label = lv_label_create(obj1);
    lv_label_set_text_fmt(lvgl_main.mem_perused_label, "DTCM: %.1f%%", lvgl_main_mem_perused.dtcm);
    lv_obj_set_style_text_font(lvgl_main.mem_perused_label, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.mem_perused_label, lv_color_white(), 0);
    lv_obj_add_flag(lvgl_main.mem_perused_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvgl_main.mem_perused_label, status_bar_event_handler, LV_EVENT_CLICKED, NULL);

    obj2 = lv_obj_create(lvgl_main.status_bar_obj);
    lv_obj_set_size(obj2, lv_pct(30), lv_pct(100));
    lv_obj_align(obj2, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_pad_top(obj2, 0, 0);
    lv_obj_set_style_pad_bottom(obj2, 0, 0);
    lv_obj_set_style_pad_left(obj2, 5, 0);
    lv_obj_set_style_pad_right(obj2, 5, 0);
    lv_obj_remove_flag(obj2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(obj2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_side(obj2, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_flex_flow(obj2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj2, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lvgl_main.wifi_label = lv_label_create(obj2);
    lv_label_set_text(lvgl_main.wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(lvgl_main.wifi_label, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.wifi_label, lv_color_white(), 0);
    lv_obj_add_flag(lvgl_main.wifi_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.wifi_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvgl_main.wifi_label, status_bar_event_handler, LV_EVENT_CLICKED, NULL);

    lvgl_main.rtc_data_label = lv_label_create(obj2);
    lv_label_set_text_fmt(lvgl_main.rtc_data_label, "%02X:%02X:%02X", lvgl_rtc_data.hour, lvgl_rtc_data.minute, lvgl_rtc_data.second);
    lv_obj_set_style_text_font(lvgl_main.rtc_data_label, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.rtc_data_label, lv_color_white(), 0);
    lv_obj_add_flag(lvgl_main.rtc_data_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvgl_main.rtc_data_label, status_bar_event_handler, LV_EVENT_CLICKED, NULL);
}

/**************************************************************
函数名称 ： lvgl_main_menu
功    能 ： 绘制菜单
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_main_menu(void)
{
    lv_anim_t a1_1;
    lv_anim_t a1_2;
    lv_anim_t a2_1;
    lv_anim_t a2_2;
    lv_anim_t a3_1;
    lv_anim_t a3_2;
    lv_anim_t a4_1;
    lv_anim_t a4_2;
    lv_anim_t a5_1;
    lv_anim_t a5_2;
    lv_anim_timeline_t *anim_timeline;
    lv_obj_t *obj;
    
    lvgl_main.app_obj = lv_obj_create(lvgl_main.menu_obj);
    lv_obj_set_size(lvgl_main.app_obj, lv_pct(80), 100);
    lv_obj_align(lvgl_main.app_obj, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_pad_top(lvgl_main.app_obj, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_main.app_obj, 5, 0);
    lv_obj_set_style_pad_left(lvgl_main.app_obj, 5, 0);
    lv_obj_set_style_pad_right(lvgl_main.app_obj, 5, 0);
    lv_obj_remove_flag(lvgl_main.app_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(lvgl_main.app_obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_side(lvgl_main.app_obj, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_flex_flow(lvgl_main.app_obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lvgl_main.app_obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lvgl_main.file_manager_app = lv_obj_create(lvgl_main.app_obj);
    lv_obj_add_style(lvgl_main.file_manager_app, &lvgl_style.app_icon_obj, 0);
    lv_obj_add_event_cb(lvgl_main.file_manager_app, app_event_handler, LV_EVENT_CLICKED, NULL);

    lvgl_main.setting_app = lv_obj_create(lvgl_main.app_obj);
    lv_obj_add_style(lvgl_main.setting_app, &lvgl_style.app_icon_obj, 0);
    lv_obj_add_event_cb(lvgl_main.setting_app, app_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_main.debugger_app = lv_obj_create(lvgl_main.app_obj);
    lv_obj_add_style(lvgl_main.debugger_app, &lvgl_style.app_icon_obj, 0);
    lv_obj_add_event_cb(lvgl_main.debugger_app, app_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_main.usart_app = lv_obj_create(lvgl_main.app_obj);
    lv_obj_add_style(lvgl_main.usart_app, &lvgl_style.app_icon_obj, 0);
    lv_obj_add_event_cb(lvgl_main.usart_app, app_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_main.can_app = lv_obj_create(lvgl_main.app_obj);
    lv_obj_add_style(lvgl_main.can_app, &lvgl_style.app_icon_obj, 0);
    lv_obj_add_event_cb(lvgl_main.can_app, app_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_power_creat(390, 300);
    lvgl_debugger_on_line_creat(390, 300);
    
    anim_timeline = lv_anim_timeline_create();
    lv_anim_init(&a1_1);
    lv_anim_set_var(&a1_1, lvgl_main.file_manager_app);
    lv_anim_set_values(&a1_1, 0, 80);
    lv_anim_set_exec_cb(&a1_1, set_width);
    lv_anim_set_path_cb(&a1_1, lv_anim_path_ease_out);
    lv_anim_set_duration(&a1_1, 480);
    
    lv_anim_init(&a1_2);
    lv_anim_set_var(&a1_2, lvgl_main.file_manager_app);
    lv_anim_set_values(&a1_2, 0, 80);
    lv_anim_set_exec_cb(&a1_2, set_height);
    lv_anim_set_path_cb(&a1_2, lv_anim_path_ease_out);
    lv_anim_set_duration(&a1_2, 480);
    
    lv_anim_init(&a2_1);
    lv_anim_set_var(&a2_1, lvgl_main.setting_app);
    lv_anim_set_values(&a2_1, 0, 80);
    lv_anim_set_exec_cb(&a2_1, set_width);
    lv_anim_set_path_cb(&a2_1, lv_anim_path_ease_out);
    lv_anim_set_duration(&a2_1, 480);
    
    lv_anim_init(&a2_2);
    lv_anim_set_var(&a2_2, lvgl_main.setting_app);
    lv_anim_set_values(&a2_2, 0, 80);
    lv_anim_set_exec_cb(&a2_2, set_height);
    lv_anim_set_path_cb(&a2_2, lv_anim_path_ease_out);
    lv_anim_set_duration(&a2_2, 480);
    
    lv_anim_init(&a3_1);
    lv_anim_set_var(&a3_1, lvgl_main.debugger_app);
    lv_anim_set_values(&a3_1, 0, 80);
    lv_anim_set_exec_cb(&a3_1, set_width);
    lv_anim_set_path_cb(&a3_1, lv_anim_path_ease_out);
    lv_anim_set_duration(&a3_1, 480);
    
    lv_anim_init(&a3_2);
    lv_anim_set_var(&a3_2, lvgl_main.debugger_app);
    lv_anim_set_values(&a3_2, 0, 80);
    lv_anim_set_exec_cb(&a3_2, set_height);
    lv_anim_set_path_cb(&a3_2, lv_anim_path_ease_out);
    lv_anim_set_duration(&a3_2, 480);
    
    lv_anim_init(&a4_1);
    lv_anim_set_var(&a4_1, lvgl_main.usart_app);
    lv_anim_set_values(&a4_1, 0, 80);
    lv_anim_set_exec_cb(&a4_1, set_width);
    lv_anim_set_path_cb(&a4_1, lv_anim_path_ease_out);
    lv_anim_set_duration(&a4_1, 480);
    
    lv_anim_init(&a4_2);
    lv_anim_set_var(&a4_2, lvgl_main.usart_app);
    lv_anim_set_values(&a4_2, 0, 80);
    lv_anim_set_exec_cb(&a4_2, set_height);
    lv_anim_set_path_cb(&a4_2, lv_anim_path_ease_out);
    lv_anim_set_duration(&a4_2, 480);
    
    lv_anim_init(&a5_1);
    lv_anim_set_var(&a5_1, lvgl_main.can_app);
    lv_anim_set_values(&a5_1, 0, 80);
    lv_anim_set_exec_cb(&a5_1, set_width);
    lv_anim_set_path_cb(&a5_1, lv_anim_path_ease_out);
    lv_anim_set_duration(&a5_1, 480);
    
    lv_anim_init(&a5_2);
    lv_anim_set_var(&a5_2, lvgl_main.can_app);
    lv_anim_set_values(&a5_2, 0, 80);
    lv_anim_set_exec_cb(&a5_2, set_height);
    lv_anim_set_path_cb(&a5_2, lv_anim_path_ease_out);
    lv_anim_set_duration(&a5_2, 480);
    lv_anim_set_completed_cb(&a5_2, anim_completed_handler);

    lv_anim_timeline_add(anim_timeline, 0, &a1_1);
    lv_anim_timeline_add(anim_timeline, 0, &a1_2);
    lv_anim_timeline_add(anim_timeline, 240, &a2_1);
    lv_anim_timeline_add(anim_timeline, 240, &a2_2);
    lv_anim_timeline_add(anim_timeline, 480, &a3_1);
    lv_anim_timeline_add(anim_timeline, 480, &a3_2);
    lv_anim_timeline_add(anim_timeline, 720, &a4_1);
    lv_anim_timeline_add(anim_timeline, 720, &a4_2);
    lv_anim_timeline_add(anim_timeline, 960, &a5_1);
    lv_anim_timeline_add(anim_timeline, 960, &a5_2);
    
    lv_anim_timeline_start(anim_timeline);
}

/**************************************************************
函数名称 ： lvgl_show_ina226_data
功    能 ： 显示INA226数据
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_show_ina226_data(void)
{
    lvgl_main.ina226_popup_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(lvgl_main.ina226_popup_obj, &lvgl_style.popup_obj, 0);
    lv_obj_set_size(lvgl_main.ina226_popup_obj, 300, 150);
    lv_obj_align_to(lvgl_main.ina226_popup_obj, lvgl_main.status_bar_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 5);

    lvgl_main.ina226_popup_label = lv_label_create(lvgl_main.ina226_popup_obj);
    lv_label_set_text_fmt(lvgl_main.ina226_popup_label, "PD: %.2fV %.2fA %.2fW\n5V: %.2fV %.2fA %.2fW\nSIN: %.2fV %.2fA %.2fW\nSOUT: %.2fV %.2fA %.2fW", \
                                                             lvgl_ina226_data.bus_voltage[3], (float)lvgl_ina226_data.current[3] / 1000, (float)lvgl_ina226_data.power[3] / 1000, \
                                                             lvgl_ina226_data.bus_voltage[2], (float)lvgl_ina226_data.current[2] / 1000, (float)lvgl_ina226_data.power[2] / 1000, \
                                                             lvgl_ina226_data.bus_voltage[1], (float)lvgl_ina226_data.current[1] / 1000, (float)lvgl_ina226_data.power[1] / 1000, \
                                                             lvgl_ina226_data.bus_voltage[0], (float)lvgl_ina226_data.current[0] / 1000, (float)lvgl_ina226_data.power[0] / 1000);
    lv_obj_set_style_text_font(lvgl_main.ina226_popup_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_main.ina226_popup_label);
}

/**************************************************************
函数名称 ： lvgl_show_cpu_run_time_data
功    能 ： 显示CPU运行时间数据
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_show_cpu_info(void)
{
    lvgl_main.cpu_info_popup_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(lvgl_main.cpu_info_popup_obj, &lvgl_style.popup_obj, 0);
    lv_obj_set_size(lvgl_main.cpu_info_popup_obj, 300, 150);
    lv_obj_align_to(lvgl_main.cpu_info_popup_obj, lvgl_main.status_bar_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 5);

    lvgl_main.cpu_info_popup_label = lv_label_create(lvgl_main.cpu_info_popup_obj);
    lv_label_set_text_fmt(lvgl_main.cpu_info_popup_label, "UTIL: %u%%\nTEMP: %.2f℃\nHTEMP: %.2f℃\nVBAT: %.2fV\nINVREF: %.2fV", lvgl_cpu_info.utilization, \
                                                           lvgl_cpu_info.adc2_data.cpu_temperature, lvgl_cpu_info.adc2_data.high_precision_cpu_temperature, \
                                                           lvgl_cpu_info.adc2_data.vbat, lvgl_cpu_info.adc2_data.internal_vref);
    lv_obj_set_style_text_font(lvgl_main.cpu_info_popup_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_main.cpu_info_popup_label);
}

/**************************************************************
函数名称 ： lvgl_show_mem_perused
功    能 ： 显示内存使用率
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_show_mem_perused(void)
{
    lvgl_main.mem_perused_popup_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(lvgl_main.mem_perused_popup_obj, &lvgl_style.popup_obj, 0);
    lv_obj_set_size(lvgl_main.mem_perused_popup_obj, 300, 150);
    lv_obj_align_to(lvgl_main.mem_perused_popup_obj, lvgl_main.status_bar_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 5);

    lvgl_main.mem_perused_popup_label = lv_label_create(lvgl_main.mem_perused_popup_obj);
    lv_label_set_text_fmt(lvgl_main.mem_perused_popup_label, "SRAM: %.1f%%\nSRAM0_1: %.1f%%\nDTCM: %.1f%%\nSDRAM: %.1f%%", lvgl_main_mem_perused.sram, lvgl_main_mem_perused.sram01, \
                                                                                                                           lvgl_main_mem_perused.dtcm, lvgl_main_mem_perused.sdram);
    lv_obj_set_style_text_font(lvgl_main.mem_perused_popup_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_main.mem_perused_popup_label);
}

/**************************************************************
函数名称 ： lvgl_show_wifi_name
功    能 ： 显示WIFI名称
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_show_wifi_name(void)
{
    lvgl_main.wifi_popup_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(lvgl_main.wifi_popup_obj, &lvgl_style.popup_obj, 0);
    lv_obj_set_size(lvgl_main.wifi_popup_obj, 180, 90);
    lv_obj_align_to(lvgl_main.wifi_popup_obj, lvgl_main.status_bar_obj, LV_ALIGN_OUT_BOTTOM_RIGHT, -5, 5);

    lvgl_main.wifi_popup_label = lv_label_create(lvgl_main.wifi_popup_obj);
    lv_label_set_text_static(lvgl_main.wifi_popup_label, lvgl_main_state.wifi_name);
    lv_obj_set_style_text_font(lvgl_main.wifi_popup_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_main.wifi_popup_label);
}

/**************************************************************
函数名称 ： lvgl_show_rtc_data
功    能 ： 显示RTC数据
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_show_rtc_data(void)
{
    lvgl_main.rtc_data_popup_obj = lv_obj_create(lv_layer_top());
    lv_obj_add_style(lvgl_main.rtc_data_popup_obj, &lvgl_style.popup_obj, 0);
    lv_obj_set_size(lvgl_main.rtc_data_popup_obj, 180, 90);
    lv_obj_align_to(lvgl_main.rtc_data_popup_obj, lvgl_main.status_bar_obj, LV_ALIGN_OUT_BOTTOM_RIGHT, -5, 5);

    lvgl_main.rtc_data_popup_label = lv_label_create(lvgl_main.rtc_data_popup_obj);
    lv_label_set_text_fmt(lvgl_main.rtc_data_popup_label, "%04X.%X.%X %s\n%02X:%02X:%02X", lvgl_rtc_data.year, lvgl_rtc_data.month, lvgl_rtc_data.day, rtc_week[lvgl_rtc_data.week],  \
                                                                                                lvgl_rtc_data.hour, lvgl_rtc_data.minute, lvgl_rtc_data.second);
    lv_obj_set_style_text_font(lvgl_main.rtc_data_popup_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_main.rtc_data_popup_label);
}

/**************************************************************
函数名称 ： lvgl_style_init
功    能 ： 样式初始化
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_style_init(void)
{
    lv_style_init(&lvgl_style.general_obj);
    lv_style_set_radius(&lvgl_style.general_obj, 8);
    lv_style_set_pad_top(&lvgl_style.general_obj, 2);
    lv_style_set_pad_bottom(&lvgl_style.general_obj, 2);
    lv_style_set_pad_left(&lvgl_style.general_obj, 2);
    lv_style_set_pad_right(&lvgl_style.general_obj, 2);
    lv_style_set_bg_opa(&lvgl_style.general_obj, LV_OPA_COVER);
    
    lv_style_init(&lvgl_style.popup_obj);
    lv_style_set_radius(&lvgl_style.popup_obj, 8);
    lv_style_set_pad_top(&lvgl_style.popup_obj, 5);
    lv_style_set_pad_bottom(&lvgl_style.popup_obj, 5);
    lv_style_set_pad_left(&lvgl_style.popup_obj, 5);
    lv_style_set_pad_right(&lvgl_style.popup_obj, 5);
    lv_style_set_shadow_width(&lvgl_style.popup_obj, 10);
    lv_style_set_shadow_offset_x(&lvgl_style.popup_obj, -2);
    lv_style_set_shadow_offset_y(&lvgl_style.popup_obj, 2);
    lv_style_set_shadow_opa(&lvgl_style.popup_obj, LV_OPA_80);
    lv_style_set_bg_opa(&lvgl_style.popup_obj, LV_OPA_80);
    
    lv_style_init(&lvgl_style.app_icon_obj);
    lv_style_set_size(&lvgl_style.app_icon_obj, 80, 80);
    lv_style_set_radius(&lvgl_style.app_icon_obj, 16);
    lv_style_set_pad_top(&lvgl_style.app_icon_obj, 5);
    lv_style_set_pad_bottom(&lvgl_style.app_icon_obj, 5);
    lv_style_set_pad_left(&lvgl_style.app_icon_obj, 5);
    lv_style_set_pad_right(&lvgl_style.app_icon_obj, 5);
    lv_style_set_shadow_width(&lvgl_style.app_icon_obj, 10);
    lv_style_set_shadow_offset_x(&lvgl_style.app_icon_obj, -2);
    lv_style_set_shadow_offset_y(&lvgl_style.app_icon_obj, 2);
    lv_style_set_shadow_opa(&lvgl_style.app_icon_obj, LV_OPA_COVER);
    lv_style_set_bg_color(&lvgl_style.app_icon_obj, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_bg_opa(&lvgl_style.app_icon_obj, LV_OPA_COVER);
    lv_style_set_bg_grad_color(&lvgl_style.app_icon_obj, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_grad_dir(&lvgl_style.app_icon_obj, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_opa(&lvgl_style.app_icon_obj, LV_OPA_80);
}

/**************************************************************
函数名称 ： lvgl_wallpaper_load
功    能 ： 加载壁纸数据到SDRAM
参    数 ： image: 图像数据描述结构体, path: 壁纸路径
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static uint8_t lvgl_wallpaper_load(lv_img_dsc_t *image, const char *path)
{
    /* header size 12 byte
    +----------------------------------------------------------+
    | magic |  cf  | flags |   w   |   h   | stride | reserved |
    +----------------------------------------------------------+
    |  8bit | 8bit | 16bit | 16bit | 16bit | 16bit  |  16bit   |
    +----------------------------------------------------------+
    */
    uint32_t read_bytes, file_size;
    FRESULT fresult;
    FIL *file;
    uint8_t *pbuffer;
    
    pbuffer = wallpaper_buffer;

    file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    if((file == NULL))
    {
        return 1;
    }

    fresult = f_open(file, path, FA_READ);
    if(fresult != FR_OK)
    {
        return fresult;
    }
    
    file_size = f_size(file);
    if(file_size - 12 > 800 * 480 * 2)
    {
        return 2;
    }
    
    fresult = f_read(file, pbuffer, file_size, &read_bytes);
    if(fresult != FR_OK || read_bytes != file_size)
    {
        return fresult;
    }
 
    image->header.magic         = *(uint8_t *)((uint8_t *)pbuffer + 0);
    image->header.cf            = *(uint8_t *)((uint8_t *)pbuffer + 1);
    image->header.flags         = *(uint16_t *)((uint8_t *)pbuffer + 2);
    image->header.w             = *(uint16_t *)((uint8_t *)pbuffer + 4);
    image->header.h             = *(uint16_t *)((uint8_t *)pbuffer + 6);
    image->header.stride        = *(uint16_t *)((uint8_t *)pbuffer + 8);
    image->header.reserved_2    = *(uint16_t *)((uint8_t *)pbuffer + 10);
    image->data_size            = file_size - 12;
    image->data                 = (uint8_t *)pbuffer + 12;

    f_close(file);
    myfree(SRAMIN, file);
    
    return 0;
}



/* 数字电源部分开始 */

/**************************************************************
函数名称 ： lvgl_power_chart_buffer_malloc
功    能 ： 为power_chart申请内存
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_power_chart_buffer_malloc(void)
{
    power_chart = (int32_t *)mymalloc(SRAMDTCM, POWER_POINT_COUNT * sizeof(int32_t) * 2);
    if(power_chart)
    {
        memset(power_chart, 0x00, POWER_POINT_COUNT * sizeof(int32_t) * 2);
        PRINT_INFO("apply %d * sizeof(int32_t) * 2 byte memory successfully in lvgl_power_chart_buffer_malloc, address: 0x%08X\r\n", POWER_POINT_COUNT, (uint32_t)power_chart);
    }
    else
    {
        PRINT_ERROR("apply %d * sizeof(int32_t) * 2 byte memory unsuccessfully in lvgl_power_chart_buffer_malloc\r\n", POWER_POINT_COUNT);
    }
}

/**************************************************************
函数名称 ： power_switch_event_handler
功    能 ： 数字电源开关事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void power_switch_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *sw = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_VALUE_CHANGED:
            if(sw == lvgl_main.power_switch)
            {
                sc8721_standby_set((lv_obj_has_state(sw, LV_STATE_CHECKED) ? 0 : 1));              /* 打开/关闭输出 */
            }
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： power_slider_event_handler
功    能 ： 数字电源滑动条事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void power_slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *slider = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_VALUE_CHANGED:
            if(slider == lvgl_main.power_voltage_slider)
            {
                power_voltage_value = (float)lv_slider_get_value(slider) / 10;
                lv_label_set_text_fmt(lvgl_main.power_voltage_slider_lable, "%.1fV", power_voltage_value);
            }
            else if(slider == lvgl_main.power_current_slider)
            {
                power_current_value = (float)lv_slider_get_value(slider) / 10;
                lv_label_set_text_fmt(lvgl_main.power_current_slider_lable, "%.1fA", power_current_value);
            }
            break;
            
        case LV_EVENT_RELEASED:
            if((slider == lvgl_main.power_voltage_slider) || (slider == lvgl_main.power_current_slider))
            {
                sc8721_output_set(power_voltage_value, power_current_value);
            }
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： lvgl_power_creat
功    能 ： 创建数字电源
参    数 ： width: 宽, height: 高
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_power_creat(uint16_t width, uint16_t height)
{
    lv_obj_t *obj1, *obj2, *obj3, *obj4;
    lv_obj_t *label1, *label2;
    lv_obj_t *scale_left, *scale_right;
    
    lvgl_main.power_obj = lv_obj_create(lvgl_main.menu_obj);
    lv_obj_add_style(lvgl_main.power_obj, &lvgl_style.popup_obj, 0);
    lv_obj_set_size(lvgl_main.power_obj, width, height);
    lv_obj_align(lvgl_main.power_obj, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_remove_flag(lvgl_main.power_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    /* 电压调节与限制电流调节滑动条，输出开关 */
    obj1 = lv_obj_create(lvgl_main.power_obj);
    lv_obj_add_style(obj1, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj1, lv_pct(100), lv_pct(20));
    lv_obj_align(obj1, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(obj1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_main.power_switch = lv_switch_create(obj1);
    lv_obj_align(lvgl_main.power_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_state(lvgl_main.power_switch, 0);
    lv_obj_add_event_cb(lvgl_main.power_switch, power_switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    label1 = lv_label_create(obj1);
    lv_label_set_text(label1, "V");
    lv_obj_set_style_text_font(label1, &lv_font_fzst_24, 0);
    lv_obj_align(label1, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lvgl_main.power_voltage_slider = lv_slider_create(obj1);
    lv_obj_set_size(lvgl_main.power_voltage_slider, 260, 10);
    lv_obj_set_style_bg_color(lvgl_main.power_voltage_slider, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(lvgl_main.power_voltage_slider, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(lvgl_main.power_voltage_slider, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(lvgl_main.power_voltage_slider, lv_color_hex(0xA8B5C4), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_color(lvgl_main.power_voltage_slider, lv_color_hex(0x58595B), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(lvgl_main.power_voltage_slider, LV_GRAD_DIR_RADIAL, LV_PART_KNOB);
    lv_obj_align_to(lvgl_main.power_voltage_slider, label1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_slider_set_range(lvgl_main.power_voltage_slider, 30, 200);
    lv_slider_set_value(lvgl_main.power_voltage_slider, (uint16_t)(power_voltage_value * 10), LV_ANIM_OFF);
    lv_obj_add_event_cb(lvgl_main.power_voltage_slider, power_slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(lvgl_main.power_voltage_slider, power_slider_event_handler, LV_EVENT_RELEASED, NULL);
    
    label2 = lv_label_create(obj1);
    lv_label_set_text(label2, "C");
    lv_obj_set_style_text_font(label2, &lv_font_fzst_24, 0);
    lv_obj_align(label2, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    lvgl_main.power_current_slider = lv_slider_create(obj1);
    lv_obj_set_size(lvgl_main.power_current_slider, 260, 10);
    lv_obj_set_style_bg_color(lvgl_main.power_current_slider, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(lvgl_main.power_current_slider, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(lvgl_main.power_current_slider, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(lvgl_main.power_current_slider, lv_color_hex(0xA8B5C4), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_color(lvgl_main.power_current_slider, lv_color_hex(0x58595B), LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(lvgl_main.power_current_slider, LV_GRAD_DIR_RADIAL, LV_PART_KNOB);
    lv_obj_align_to(lvgl_main.power_current_slider, label2, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_slider_set_range(lvgl_main.power_current_slider, 0, 50);
    lv_slider_set_value(lvgl_main.power_current_slider, (uint16_t)(power_current_value * 10), LV_ANIM_OFF);
    lv_obj_add_event_cb(lvgl_main.power_current_slider, power_slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(lvgl_main.power_current_slider, power_slider_event_handler, LV_EVENT_RELEASED, NULL);
    
    /* 电压设置与限制电流设置值 */
    obj2 = lv_obj_create(lvgl_main.power_obj);
    lv_obj_add_style(obj2, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj2, lv_pct(20), lv_pct(30));
    lv_obj_align_to(obj2, obj1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_flag(obj2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lvgl_main.power_voltage_slider_lable = lv_label_create(obj2);
    lv_label_set_text_fmt(lvgl_main.power_voltage_slider_lable, "%.1fV", power_voltage_value);
    lv_obj_set_style_text_font(lvgl_main.power_voltage_slider_lable, &lv_font_fzst_24, 0);
    lv_obj_align(lvgl_main.power_voltage_slider_lable, LV_ALIGN_TOP_MID, 0, 5);
    
    lvgl_main.power_current_slider_lable = lv_label_create(obj2);
    lv_label_set_text_fmt(lvgl_main.power_current_slider_lable, "%.1fA", power_current_value);
    lv_obj_set_style_text_font(lvgl_main.power_current_slider_lable, &lv_font_fzst_24, 0);
    lv_obj_align(lvgl_main.power_current_slider_lable, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    /* 电压，电流，功率值 */
    obj3 = lv_obj_create(lvgl_main.power_obj);
    lv_obj_add_style(obj3, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj3, lv_pct(20), lv_pct(50));
    lv_obj_align_to(obj3, obj2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_flag(obj3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lvgl_main.power_voltage_lable = lv_label_create(obj3);
    lv_label_set_text_fmt(lvgl_main.power_voltage_lable, "%.2fV", lvgl_ina226_data.bus_voltage[0]);
    lv_obj_set_style_text_font(lvgl_main.power_voltage_lable, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.power_voltage_lable, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(lvgl_main.power_voltage_lable, LV_ALIGN_TOP_MID, 0, 5);
    
    lvgl_main.power_current_lable = lv_label_create(obj3);
    lv_label_set_text_fmt(lvgl_main.power_current_lable, "%.2fA", (float)lvgl_ina226_data.current[0] / 1000);
    lv_obj_set_style_text_font(lvgl_main.power_current_lable, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.power_current_lable, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(lvgl_main.power_current_lable, LV_ALIGN_CENTER, 0, 0);
    
    lvgl_main.power_power_lable = lv_label_create(obj3);
    lv_label_set_text_fmt(lvgl_main.power_power_lable, "%.1fW", (float)lvgl_ina226_data.power[0] / 1000);
    lv_obj_set_style_text_font(lvgl_main.power_power_lable, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_color(lvgl_main.power_power_lable, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_align(lvgl_main.power_power_lable, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    /* 电压与电流曲线图 */
    obj4 = lv_obj_create(lvgl_main.power_obj);
    lv_obj_add_style(obj4, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj4, lv_pct(80), lv_pct(80));
    lv_obj_align(obj4, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_remove_flag(obj4, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_main.power_chart = lv_chart_create(obj4);
    lv_obj_set_size(lvgl_main.power_chart, lv_pct(100), lv_pct(100));
    lv_chart_set_div_line_count(lvgl_main.power_chart, 10, 10);
    lv_obj_center(lvgl_main.power_chart);
    lv_obj_set_style_bg_color(lvgl_main.power_chart, lv_palette_lighten(LV_PALETTE_GREY, 5), 0);
    lv_obj_remove_flag(lvgl_main.power_chart, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    scale_left = lv_scale_create(lvgl_main.power_chart);
    lv_obj_set_size(scale_left, 5, lv_pct(100));
    lv_scale_set_mode(scale_left, LV_SCALE_MODE_VERTICAL_RIGHT);
    lv_obj_align(scale_left, LV_ALIGN_LEFT_MID, -lv_obj_get_style_pad_left(lvgl_main.power_chart, 0), 0);
    lv_scale_set_label_show(scale_left, true);
    lv_scale_set_total_tick_count(scale_left, 21);
    lv_scale_set_major_tick_every(scale_left, 5);
    lv_obj_set_style_length(scale_left, 2, LV_PART_ITEMS);
    lv_obj_set_style_length(scale_left, 5, LV_PART_INDICATOR);
    lv_scale_set_range(scale_left, 0, 20);
    lv_obj_set_style_line_color(scale_left, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_set_style_line_opa(scale_left, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_line_color(scale_left, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_obj_set_style_line_opa(scale_left, LV_OPA_50, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale_left, lv_palette_main(LV_PALETTE_RED), LV_PART_ITEMS);
    lv_obj_set_style_line_opa(scale_left, LV_OPA_50, LV_PART_ITEMS);
    lv_obj_set_style_text_color(scale_left, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_opa(scale_left, LV_OPA_50, 0);
    
    scale_right = lv_scale_create(lvgl_main.power_chart);
    lv_obj_set_size(scale_right, 5, lv_pct(100));
    lv_scale_set_mode(scale_right, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_obj_align(scale_right, LV_ALIGN_RIGHT_MID, lv_obj_get_style_pad_right(lvgl_main.power_chart, 0), 0);
    lv_scale_set_label_show(scale_right, true);
    lv_scale_set_total_tick_count(scale_right, 26);
    lv_scale_set_major_tick_every(scale_right, 5);
    lv_obj_set_style_length(scale_right, 2, LV_PART_ITEMS);
    lv_obj_set_style_length(scale_right, 5, LV_PART_INDICATOR);
    lv_scale_set_range(scale_right, 0, 5);
    lv_obj_set_style_line_color(scale_right, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
    lv_obj_set_style_line_opa(scale_right, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_line_color(scale_right, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_line_opa(scale_right, LV_OPA_50, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale_right, lv_palette_main(LV_PALETTE_BLUE), LV_PART_ITEMS);
    lv_obj_set_style_line_opa(scale_right, LV_OPA_50, LV_PART_ITEMS);
    lv_obj_set_style_text_color(scale_right, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_text_opa(scale_right, LV_OPA_50, 0);
    
    lv_chart_set_point_count(lvgl_main.power_chart, POWER_POINT_COUNT);
    lv_chart_series_t *series_voltage = lv_chart_add_series(lvgl_main.power_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *series_current = lv_chart_add_series(lvgl_main.power_chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);
    lv_chart_set_range(lvgl_main.power_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 200);
    lv_chart_set_range(lvgl_main.power_chart, LV_CHART_AXIS_SECONDARY_Y, 0, 50);
    lv_chart_set_ext_y_array(lvgl_main.power_chart, series_voltage, (int32_t *)power_chart);
    lv_chart_set_ext_y_array(lvgl_main.power_chart, series_current, (int32_t *)power_chart + POWER_POINT_COUNT);
}

/* 数字电源部分结束 */

/* 在线调试器部分开始 */

/**************************************************************
函数名称 ： lvgl_debugger_on_line_creat
功    能 ： 在线调试器创建
参    数 ： width: 宽, height: 高
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_debugger_on_line_creat(uint16_t width, uint16_t height)
{
    lv_obj_t *obj1, *obj2, *obj3;
    lv_obj_t *label1, *label2;
    
    lvgl_main.debugger_obj = lv_obj_create(lvgl_main.menu_obj);
    lv_obj_add_style(lvgl_main.debugger_obj, &lvgl_style.popup_obj, 0);
    lv_obj_set_size(lvgl_main.debugger_obj, width, height);
    lv_obj_align(lvgl_main.debugger_obj, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_remove_flag(lvgl_main.debugger_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    obj1 = lv_obj_create(lvgl_main.debugger_obj);
    lv_obj_add_style(obj1, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj1, lv_pct(100), lv_pct(20));
    lv_obj_align(obj1, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(obj1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_main.debugger_id_label = lv_label_create(obj1);
    lv_label_set_text(lvgl_main.debugger_id_label, "DBG ID: NULL");
    lv_obj_set_style_text_font(lvgl_main.debugger_id_label, &lv_font_fzst_24, 0);
    lv_obj_align(lvgl_main.debugger_id_label, LV_ALIGN_LEFT_MID, 5, 0);
    
    lvgl_main.debugger_connect_led = lv_led_create(obj1);
    lv_obj_set_size(lvgl_main.debugger_connect_led, 20, 20);
    lv_led_set_color(lvgl_main.debugger_connect_led, lv_color_hex(0xFFB6C1));
    lv_obj_align(lvgl_main.debugger_connect_led, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_led_off(lvgl_main.debugger_connect_led);
    
    lvgl_main.debugger_running_led = lv_led_create(obj1);
    lv_obj_set_size(lvgl_main.debugger_running_led, 20, 20);
    lv_led_set_color(lvgl_main.debugger_running_led, lv_color_hex(0xAFEEEE));
    lv_obj_align_to(lvgl_main.debugger_running_led, lvgl_main.debugger_connect_led, LV_ALIGN_OUT_LEFT_MID, -15, 0);
    lv_led_off(lvgl_main.debugger_running_led);
    
    
    obj2 = lv_obj_create(lvgl_main.debugger_obj);
    lv_obj_add_style(obj2, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj2, lv_pct(50), lv_pct(80));
    lv_obj_align(obj2, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_flag(obj2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    label1 = lv_label_create(obj2);
    lv_label_set_text(label1, "下载/调试次数：");
    lv_obj_set_style_text_font(label1, &lv_font_fzst_24, 0);
    lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 20);
    
    lvgl_main.debugger_download_count_label = lv_label_create(obj2);
    lv_label_set_text_fmt(lvgl_main.debugger_download_count_label, "%d", debugger_download_count);
    lv_obj_set_style_text_font(lvgl_main.debugger_download_count_label, &lv_font_fzst_32, 0);
    lv_obj_set_style_text_color(lvgl_main.debugger_download_count_label, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(lvgl_main.debugger_download_count_label, LV_ALIGN_CENTER, 0, -40);
    
    label2 = lv_label_create(obj2);
    lv_label_set_text(label2, "下载/调试耗时：");
    lv_obj_set_style_text_font(label2, &lv_font_fzst_24, 0);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, 20);
    
    lvgl_main.debugger_download_time_label = lv_label_create(obj2);
    lv_label_set_text_fmt(lvgl_main.debugger_download_time_label, "%.1fms", debugger_download_time);
    lv_obj_set_style_text_font(lvgl_main.debugger_download_time_label, &lv_font_fzst_32, 0);
    lv_obj_set_style_text_color(lvgl_main.debugger_download_time_label, lv_palette_main(LV_PALETTE_DEEP_ORANGE), 0);
    lv_obj_align(lvgl_main.debugger_download_time_label, LV_ALIGN_BOTTOM_MID, 0, -40);
    
    
    obj3 = lv_obj_create(lvgl_main.debugger_obj);
    lv_obj_add_style(obj3, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj3, lv_pct(50), lv_pct(80));
    lv_obj_align(obj3, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_remove_flag(obj3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_main.debugger_download_state_label = lv_label_create(obj3);
    lv_label_set_text(lvgl_main.debugger_download_state_label, "空闲");
    lv_obj_set_style_text_font(lvgl_main.debugger_download_state_label, &lv_font_fzst_32, 0);
    lv_obj_align(lvgl_main.debugger_download_state_label, LV_ALIGN_CENTER, 0, 0);
}
/* 在线调试器部分结束 */

/**************************************************************
函数名称 ： drag_event_handler
功    能 ： 拖动事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void drag_event_handler(lv_event_t * e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL)  return;

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    int32_t x = lv_obj_get_x_aligned(obj) + vect.x;
    int32_t y = lv_obj_get_y_aligned(obj) + vect.y;
    lv_obj_set_pos(obj, x, y);
}

/**************************************************************
函数名称 ： lvgl_num_keyboard_create
功    能 ： 创建数字键盘
参    数 ： parent: 父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_num_keyboard_create(lv_obj_t *parent)
{
    lv_obj_t *obj;
    lv_obj_t *label;
    
    obj = lv_obj_create(parent);
    lv_obj_add_style(obj, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj, lv_pct(50), lv_pct(60));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(obj, drag_event_handler, LV_EVENT_PRESSING, NULL);
    
    label = lv_label_create(obj);
    lv_label_set_text(label, "按压拖动");
    lv_obj_set_style_text_font(label, &lv_font_fzst_16, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_opa(label, LV_OPA_30, 0);
    
    lvgl_main.num_keyboard = lv_keyboard_create(obj);
    lv_obj_set_size(lvgl_main.num_keyboard, lv_pct(100), lv_pct(90));
    lv_keyboard_set_mode(lvgl_main.num_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_style_text_font(lvgl_main.num_keyboard, &lv_font_montserrat_24, 0);
    lv_obj_align(lvgl_main.num_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/**************************************************************
函数名称 ： lvgl_num_keyboard_delete
功    能 ： 删除数字键盘
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_num_keyboard_delete(void)
{
    lv_obj_delete(lv_obj_get_parent(lvgl_main.num_keyboard));
    lvgl_main.num_keyboard = NULL;
}

/**************************************************************
函数名称 ： lvgl_all_keyboard_create
功    能 ： 创建全键盘
参    数 ： parent: 父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_all_keyboard_create(lv_obj_t *parent)
{
    lv_obj_t *obj;
    lv_obj_t *label;
    
    obj = lv_obj_create(parent);
    lv_obj_add_style(obj, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj, lv_pct(75), lv_pct(60));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(obj, drag_event_handler, LV_EVENT_PRESSING, NULL);
    
    label = lv_label_create(obj);
    lv_label_set_text(label, "按压拖动");
    lv_obj_set_style_text_font(label, &lv_font_fzst_16, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_opa(label, LV_OPA_30, 0);
    
    lvgl_main.all_keyboard = lv_keyboard_create(obj);
    lv_obj_set_size(lvgl_main.all_keyboard, lv_pct(100), lv_pct(90));
    lv_keyboard_set_popovers(lvgl_main.all_keyboard, true);
    lv_obj_set_style_text_font(lvgl_main.all_keyboard, &lv_font_montserrat_24, 0);
    lv_obj_align(lvgl_main.all_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/**************************************************************
函数名称 ： lvgl_all_keyboard_delete
功    能 ： 删除全键盘
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_all_keyboard_delete(void)
{
    lv_obj_delete(lv_obj_get_parent(lvgl_main.all_keyboard));
    lvgl_main.all_keyboard = NULL;
}

/**************************************************************
函数名称 ： lvgl_show_error_msgbox_creat
功    能 ： 创建错误操作消息提示框
参    数 ： errorInfo: 错误信息
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_show_error_msgbox_creat(const char *errorInfo)
{
    lv_obj_t *msgbox;
    
    msgbox = lv_msgbox_create(NULL);
    lv_obj_set_style_text_font(msgbox, &lv_font_fzst_24, 0);
    lv_msgbox_add_title(msgbox, "错误操作");
    lv_msgbox_add_text(msgbox, errorInfo);
    lv_msgbox_add_close_button(msgbox);
}

/**************************************************************
函数名称 ： lvgl_hidden_main_menu
功    能 ： 隐藏主菜单
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_hidden_main_menu(void)
{
    lv_obj_add_flag(lvgl_main.power_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.debugger_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.app_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.file_manager_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.setting_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.debugger_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.usart_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvgl_main.can_app_label, LV_OBJ_FLAG_HIDDEN);
}

/**************************************************************
函数名称 ： lvgl_hidden_main_menu
功    能 ： 显示主菜单
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_show_main_menu(void)
{
    lv_obj_remove_flag(lvgl_main.power_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvgl_main.debugger_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvgl_main.app_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvgl_main.file_manager_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvgl_main.setting_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvgl_main.debugger_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvgl_main.usart_app_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvgl_main.can_app_label, LV_OBJ_FLAG_HIDDEN);
}
