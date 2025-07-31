/*!
    \file       lvgl_setting.h
    \brief      LVGL setting interface header file
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#ifndef __LVGL_SETTING_H
#define __LVGL_SETTING_H
#include "lvgl.h"

/*!
    \brief      LVGL setting interface structure
*/
typedef struct
{
    lv_obj_t *main_obj;                     /*!< Main object container */
    lv_obj_t *brightness_slider;            /*!< Brightness adjustment slider */
    lv_obj_t *brightness_slider_label;      /*!< Brightness slider label */
    lv_obj_t *wifi_switch;                  /*!< WiFi enable/disable switch */
    lv_obj_t *wifi_cont;                    /*!< WiFi container */
    lv_obj_t *wifi_label;                   /*!< WiFi status label */
    lv_obj_t *wifi_scan_cont;               /*!< WiFi scan container */
    lv_obj_t *wifi_scan_btn;                /*!< WiFi scan button */
    lv_obj_t *wifi_scan_list;               /*!< WiFi scan results list */
    lv_obj_t *ntp_section;                  /*!< NTP section container */
    lv_obj_t *ntp_dropdown;                 /*!< NTP server dropdown */
    lv_obj_t *get_ntp_data_btn;             /*!< Get NTP data button */
    lv_obj_t *pd_dropdown;                  /*!< PD voltage dropdown */
    lv_obj_t *sc8721_frequency_dropdown;    /*!< SC8721 frequency dropdown */
    lv_obj_t *sc8721_slope_dropdown;        /*!< SC8721 slope dropdown */
    
    lv_obj_t *possword_textarea;            /*!< Password input textarea */
    
    lv_timer_t *timer;                      /*!< Update timer */
}lvgl_setting_struct;
extern lvgl_setting_struct lvgl_setting;

/*!
    \brief      LVGL setting state structure
*/
typedef struct
{
    uint8_t wifi;                           /*!< WiFi enable state */
    uint8_t buletooth;                      /*!< Bluetooth enable state */
    uint8_t pd_voltage;                     /*!< PD voltage setting */
    uint8_t brightness;                     /*!< Screen brightness level */
    uint8_t sc8721_frequency;               /*!< SC8721 frequency setting */
    uint8_t sc8721_slope;                   /*!< SC8721 slope setting */
}lvgl_setting_state_struct;
extern lvgl_setting_state_struct lvgl_setting_state;

/*!
    \brief      LVGL setting wireless state structure
*/
typedef struct
{
    uint8_t status;                         /*!< Wireless status */
    uint8_t error;                          /*!< Wireless error code */
}lvgl_setting_wireless_state_struct;

/*!
    \brief      LVGL setting wireless scan information structure
*/
typedef struct
{
    uint8_t wifi_num;                       /*!< Number of WiFi networks found */
    char wifi_name[16][32];                 /*!< Store up to 16 WiFi network names */
}lvgl_setting_wireless_scan_info_struct;
extern lvgl_setting_wireless_scan_info_struct lvgl_setting_wireless_scan_info;

/*!
    \brief      LVGL setting wireless connection structure
*/
typedef struct
{
    char wifi_name[32];                     /*!< WiFi network name */
    char wifi_password[32];                 /*!< WiFi network password */
    uint8_t ntp_server;                     /*!< NTP server selection */
}lvgl_setting_wireless_connect_struct;
extern lvgl_setting_wireless_connect_struct lvgl_setting_wireless_connect;

/* function declarations */
void lvgl_setting_creat(lv_obj_t *parent);         /* create LVGL setting interface */
#endif /* __LVGL_SETTING_H */
