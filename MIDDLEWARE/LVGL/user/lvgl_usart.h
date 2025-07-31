#ifndef __LVGL_USART_H
#define __LVGL_USART_H
#include "lvgl.h"

typedef struct
{
    lv_obj_t *main_obj;
    lv_obj_t *status_led;
    lv_obj_t *usart_dropdown;
    lv_obj_t *send_add_dropdown;
    lv_obj_t *close_btn;
    lv_obj_t *receive_textarea;
    lv_obj_t *send_textarea;
    lv_obj_t *connect_btn;
    lv_obj_t *connect_btn_label;
    lv_obj_t *baud_rate_dropdown;
    lv_obj_t *stop_bit_dropdown;
    lv_obj_t *data_bit_dropdown;
    lv_obj_t *check_bit_dropdown;
    lv_obj_t *clean_receive_btn;
    lv_obj_t *clean_receive_btn_label;
    lv_obj_t *send_btn;
    lv_obj_t *send_btn_label;
    lv_obj_t *clean_send_btn;
    lv_obj_t *clean_send_btn_label;
    
    lv_timer_t *timer;
}lvgl_usart_struct;
extern lvgl_usart_struct lvgl_usart;

typedef struct
{
    uint8_t usart;
    uint8_t send_add;
    uint8_t baud_rate;
    uint8_t stop_bit;
    uint8_t data_bit;
    uint8_t check_bit;
}lvgl_usart_state_struct;
extern lvgl_usart_state_struct lvgl_usart_state;

/* function declarations */
void lvgl_usart_creat(lv_obj_t *parent);
void lvgl_usart_delete(void);
#endif
