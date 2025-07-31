/*!
    \file       lvgl_can.h
    \brief      LVGL CAN interface header file
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#ifndef __LVGL_CAN_H
#define __LVGL_CAN_H
#include "lvgl.h"

/*!
    \brief      LVGL CAN interface structure
*/
typedef struct
{
    lv_obj_t *main_obj;                     /*!< Main object container */
    lv_obj_t *status_led;                   /*!< CAN status LED */
    lv_obj_t *mode_dropdown;                /*!< CAN mode dropdown */
    lv_obj_t *setting_label;                /*!< Setting label */
    lv_obj_t *close_btn;                    /*!< Close button */
    lv_obj_t *receive_textarea;             /*!< Receive data textarea */
    lv_obj_t *send_textarea;                /*!< Send data textarea */
    lv_obj_t *send_btn;                     /*!< Send button */
    lv_obj_t *send_btn_label;               /*!< Send button label */
    lv_obj_t *clean_send_btn;               /*!< Clear send data button */
    lv_obj_t *clean_send_btn_label;         /*!< Clear send data button label */
    lv_obj_t *connect_btn;                  /*!< Connect button */
    lv_obj_t *connect_btn_label;            /*!< Connect button label */
    lv_obj_t *clean_receive_btn;            /*!< Clear receive data button */
    lv_obj_t *clean_receive_btn_label;      /*!< Clear receive data button label */
    
    lv_timer_t *timer;                      /*!< Update timer */
}lvgl_can_struct;
extern lvgl_can_struct lvgl_can;

/*!
    \brief      LVGL CAN state structure
*/
typedef struct
{
    uint8_t mode;                           /*!< CAN mode setting */
}lvgl_can_state_struct;
extern lvgl_can_state_struct lvgl_can_state;

/* function declarations */
void lvgl_can_creat(lv_obj_t *parent);
#endif
