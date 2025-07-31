/*!
    \file       lvgl_main.h
    \brief      LVGL main interface header file
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#ifndef __LVGL_MAIN_H
#define __LVGL_MAIN_H
#include "lvgl.h"
#include "./ADC/adc.h"

extern volatile uint8_t main_menu_page_flag;

/*!
    \brief      LVGL main interface structure
*/
typedef struct
{
    lv_obj_t *main_obj;                     /*!< Main object container */
    lv_obj_t *status_bar_obj;               /*!< Status bar object */
    lv_obj_t *menu_obj;                     /*!< Menu object */
    lv_obj_t *wallpaper;                    /*!< Wallpaper object */
    lv_image_dsc_t wallpaper_dsc;           /*!< Wallpaper image descriptor */
    lv_obj_t *ina226_label;                 /*!< INA226 sensor label */
    lv_obj_t *cpu_info_label;               /*!< CPU information label */
    lv_obj_t *mem_perused_label;            /*!< Memory usage label */
    
    lv_obj_t *wifi_label;                   /*!< WiFi status label */
    lv_obj_t *rtc_data_label;               /*!< RTC data label */
    
    lv_obj_t *ina226_popup_obj;             /*!< INA226 popup object */
    lv_obj_t *ina226_popup_label;           /*!< INA226 popup label */
    lv_obj_t *cpu_info_popup_obj;           /*!< CPU info popup object */
    lv_obj_t *cpu_info_popup_label;         /*!< CPU info popup label */
    lv_obj_t *mem_perused_popup_obj;        /*!< Memory usage popup object */
    lv_obj_t *mem_perused_popup_label;      /*!< Memory usage popup label */
    lv_obj_t *wifi_popup_obj;               /*!< WiFi popup object */
    lv_obj_t *wifi_popup_label;             /*!< WiFi popup label */
    lv_obj_t *rtc_data_popup_obj;           /*!< RTC data popup object */
    lv_obj_t *rtc_data_popup_label;         /*!< RTC data popup label */
    
    lv_obj_t *app_obj;                      /*!< Application object container */
    lv_obj_t *file_manager_app;             /*!< File manager app icon */
    lv_obj_t *file_manager_app_label;       /*!< File manager app label */
    lv_obj_t *setting_app;                  /*!< Settings app icon */
    lv_obj_t *setting_app_label;            /*!< Settings app label */
    lv_obj_t *debugger_app;                 /*!< Debugger app icon */
    lv_obj_t *debugger_app_label;           /*!< Debugger app label */
    lv_obj_t *usart_app;                    /*!< USART app icon */
    lv_obj_t *usart_app_label;              /*!< USART app label */
    lv_obj_t *can_app;                      /*!< CAN app icon */
    lv_obj_t *can_app_label;                /*!< CAN app label */
    
    lv_obj_t *power_obj;                    /*!< Power control object */
    lv_obj_t *power_switch;                 /*!< Power switch */
    lv_obj_t *power_voltage_slider;         /*!< Power voltage slider */
    lv_obj_t *power_voltage_slider_lable;   /*!< Power voltage slider label */
    lv_obj_t *power_current_slider;         /*!< Power current slider */
    lv_obj_t *power_current_slider_lable;   /*!< Power current slider label */
    lv_obj_t *power_voltage_lable;          /*!< Power voltage display label */
    lv_obj_t *power_current_lable;          /*!< Power current display label */
    lv_obj_t *power_power_lable;            /*!< Power display label */
    lv_obj_t *power_chart;                  /*!< Power chart */
    
    lv_obj_t *debugger_obj;                 /*!< Debugger object */
    lv_obj_t *debugger_id_label;            /*!< Debugger ID label */
    lv_obj_t *debugger_connect_led;         /*!< Debugger connect LED */
    lv_obj_t *debugger_running_led;         /*!< Debugger running LED */
    lv_obj_t *debugger_download_count_label; /*!< Debugger download count label */
    lv_obj_t *debugger_download_time_label; /*!< Debugger download time label */
    lv_obj_t *debugger_download_state_label; /*!< Debugger download state label */
    
    lv_obj_t *num_keyboard;                 /*!< Numeric keyboard */
    lv_obj_t *all_keyboard;                 /*!< Full keyboard */
    
    lv_timer_t *timer;                      /*!< Update timer */
}lvgl_main_struct;
extern lvgl_main_struct lvgl_main;

/*!
    \brief      LVGL style structure
*/
typedef struct
{
    lv_style_t general_obj;                 /*!< General object style */
    lv_style_t popup_obj;                   /*!< Popup object style */
    lv_style_t app_icon_obj;                /*!< App icon object style */
}lvgl_style_struct;
extern lvgl_style_struct lvgl_style;

/*!
    \brief      LVGL CPU information structure
*/
typedef struct
{
    uint8_t utilization;                    /*!< CPU utilization percentage */
    adc2_data_struct adc2_data;             /*!< ADC2 data */
}lvgl_cpu_info_struct;

/*!
    \brief      LVGL main state structure
*/
typedef struct
{
    uint8_t wifi;                           /*!< WiFi enable state */
    char wifi_name[32];                     /*!< WiFi network name */
    uint8_t bluetooth;                      /*!< Bluetooth enable state */
}lvgl_main_state_struct;

/*!
    \brief      LVGL main memory usage structure
*/
typedef struct
{
    float sram;                             /*!< SRAM usage percentage */
    float sram01;                           /*!< SRAM01 usage percentage */
    float dtcm;                             /*!< DTCM usage percentage */
    float sdram;                            /*!< SDRAM usage percentage */
}lvgl_main_mem_perused_struct;

/* function declarations */
void lvgl_start(void);                                          /* start LVGL interface */
void lvgl_num_keyboard_create(lv_obj_t *parent);                /* create numeric keyboard */
void lvgl_num_keyboard_delete(void);                            /* delete numeric keyboard */
void lvgl_all_keyboard_create(lv_obj_t *parent);                /* create full keyboard */
void lvgl_all_keyboard_delete(void);                            /* delete full keyboard */
void lvgl_show_error_msgbox_creat(const char *errorInfo);       /* create error message box */
void lvgl_hidden_main_menu(void);                               /* hide main menu */
void lvgl_show_main_menu(void);                                 /* show main menu */

#endif /* __LVGL_MAIN_H */
