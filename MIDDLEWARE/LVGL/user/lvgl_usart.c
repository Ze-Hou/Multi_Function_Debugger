#include "lvgl_usart.h"
#include "freertos_main.h"
#include "lvgl_main.h"
#include "gd32h7xx_usart.h"
#include "./USART/usart.h"
#include "./WIRELESS/wireless.h"
#include "./MALLOC/malloc.h"

lvgl_usart_struct lvgl_usart;
lvgl_usart_state_struct lvgl_usart_state;

#define SHOW_LINE_NUM   10

static uint8_t connect_status = 0;

/* static function declarations */
static void lvgl_usart_baudrate_set(uint32_t baudrate, uint8_t usart);
static void lvgl_usart_stop_bit_set(uint32_t stop_bit, uint8_t usart);
static void lvgl_usart_word_length_set(uint32_t word_length, uint8_t usart);
static void lvgl_usart_parity_config(uint32_t parity, uint8_t usart);

/**************************************************************
函数名称 ： timer_cb
功    能 ： 定时器回调
参    数 ： timer
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void timer_cb(lv_timer_t *timer)
{
    uint32_t cursor_pos, total_line_num;
    lv_obj_t *textarea_label;
    lv_point_t letter_pos;
    
    switch(lvgl_usart_state.usart)
    {
        case 0:
            if(g_usart_terminal_recv_complete_flag)
            {
                cursor_pos = lv_textarea_get_cursor_pos(lvgl_usart.receive_textarea);
                textarea_label = lv_textarea_get_label(lvgl_usart.receive_textarea);

                lv_label_get_letter_pos(textarea_label, cursor_pos, &letter_pos);
                total_line_num = letter_pos.y / 34 + 1;
                if(total_line_num > SHOW_LINE_NUM)
                {
                    letter_pos.x = 0;
                    letter_pos.y = (total_line_num - SHOW_LINE_NUM) * 34;
                    lv_label_cut_text(textarea_label, 0, lv_label_get_letter_on(textarea_label, &letter_pos, true));
                }
                lv_textarea_add_text(lvgl_usart.receive_textarea, (const char *)g_usart_terminal_recv_buff);
                usart_terminal_rx_dma_receive_reset();
            }
            break;
            
        case 1:
            if(g_uart4_recv_complete_flag)
            {
                cursor_pos = lv_textarea_get_cursor_pos(lvgl_usart.receive_textarea);
                textarea_label = lv_textarea_get_label(lvgl_usart.receive_textarea);

                lv_label_get_letter_pos(textarea_label, cursor_pos, &letter_pos);
                total_line_num = letter_pos.y / 34 + 1;
                if(total_line_num > SHOW_LINE_NUM)
                {
                    letter_pos.x = 0;
                    letter_pos.y = (total_line_num - SHOW_LINE_NUM) * 34;
                    lv_label_cut_text(textarea_label, 0, lv_label_get_letter_on(textarea_label, &letter_pos, true));
                }
                lv_textarea_add_text(lvgl_usart.receive_textarea, (const char *)g_uart4_recv_buff);
                uart4_rx_dma_receive_reset();
            }
            break;
        
        default: break;
    }
    
}

/**************************************************************
函数名称 ： btn_event_handler
功    能 ： 按钮事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const char *send_text;
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(btn == lvgl_usart.close_btn)
            {
                lvgl_usart_delete();
            }
            else if(btn == lvgl_usart.connect_btn)
            {
                if(connect_status == 0)
                {
                    connect_status = 0x01;
                    lv_led_on(lvgl_usart.status_led);
                    lv_obj_set_style_bg_color(lvgl_usart.connect_btn, lv_palette_main(LV_PALETTE_RED), 0);
                    lv_label_set_text(lvgl_usart.connect_btn_label, "断开");
                    
                    lvgl_usart.timer = lv_timer_create(timer_cb, 50, NULL);
                    usart_terminal_rx_dma_receive_reset();
                }
                else if(connect_status == 0x01)
                {
                    connect_status = 0;
                    lv_led_off(lvgl_usart.status_led);
                    lv_obj_set_style_bg_color(lvgl_usart.connect_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
                    lv_label_set_text(lvgl_usart.connect_btn_label, "连接");
                    
                    lv_timer_delete(lvgl_usart.timer);
                }
            }
            else if(btn == lvgl_usart.clean_receive_btn)
            {
                lv_textarea_set_text(lvgl_usart.receive_textarea, "");
            }
            else if(btn == lvgl_usart.send_btn)
            {
                send_text = lv_textarea_get_text(lvgl_usart.send_textarea);
                if(send_text[0] != '\0')
                {
                    switch(lvgl_usart_state.usart)
                    {
                        case 0:
                            switch(lvgl_usart_state.send_add)
                            {
                                case 0:
                                    usart_terminal_print_fmt("%s", send_text);
                                    break;
                                
                                case 1:
                                    usart_terminal_print_fmt("%s\r", send_text);
                                    break;
                                
                                case 2:
                                    usart_terminal_print_fmt("%s\n", send_text);
                                    break;
                                
                                case 3:
                                    usart_terminal_print_fmt("%s\r\n", send_text);
                                    break;
                                
                                default: break;
                            }
                            break;
                            
                        case 1:
                            switch(lvgl_usart_state.send_add)
                            {
                                case 0:
                                    uart4_print_fmt("%s", send_text);
                                    break;
                                
                                case 1:
                                    uart4_print_fmt("%s\r", send_text);
                                    break;
                                
                                case 2:
                                    uart4_print_fmt("%s\n", send_text);
                                    break;
                                
                                case 3:
                                    uart4_print_fmt("%s\r\n", send_text);
                                    break;
                                
                                default: break;
                            }
                            break;
                            
                        default: break;
                    }
                }
            }
            else if(btn == lvgl_usart.clean_send_btn)
            {
                lv_textarea_set_text(lvgl_usart.send_textarea, "");
            }
            break;
        
        default: break;
    } 
}

/**************************************************************
函数名称 ： textarea_event_handler
功    能 ： 文本框事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void textarea_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *textarea = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(lvgl_main.all_keyboard == NULL)
            {
                lvgl_all_keyboard_create(lvgl_usart.main_obj);
            }
            lv_keyboard_set_textarea(lvgl_main.all_keyboard, lvgl_usart.send_textarea);
            break;

        case LV_EVENT_CANCEL:
            lvgl_all_keyboard_delete();
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： dropdown_event_handler
功    能 ： 下拉列表事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void dropdown_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *dropdown = lv_event_get_target(e);
    uint8_t option_id = 0;

    switch(code)
    {
        case LV_EVENT_VALUE_CHANGED:
            option_id = lv_dropdown_get_selected(dropdown);
            if(dropdown == lvgl_usart.usart_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                        lvgl_usart_state.usart = 0;
                        break;
                    
                    case 1:
                        lvgl_usart_state.usart = 1;
                        break;
                    
                    default: break;
                }
                if(lvgl_usart_state.usart == 1)
                {
                    if((lvgl_usart_state.stop_bit == 0) || (lvgl_usart_state.stop_bit == 2))
                    {
                        lvgl_usart_state.stop_bit = 1;
                    }
                }
                lv_dropdown_set_selected(lvgl_usart.baud_rate_dropdown, lvgl_usart_state.baud_rate);
                lv_dropdown_set_selected(lvgl_usart.stop_bit_dropdown, lvgl_usart_state.stop_bit);
                lv_dropdown_set_selected(lvgl_usart.data_bit_dropdown, lvgl_usart_state.data_bit);
                lv_dropdown_set_selected(lvgl_usart.check_bit_dropdown, lvgl_usart_state.check_bit);
                lv_obj_send_event(lvgl_usart.baud_rate_dropdown, LV_EVENT_VALUE_CHANGED, NULL);
                lv_obj_send_event(lvgl_usart.stop_bit_dropdown, LV_EVENT_VALUE_CHANGED, NULL);
                lv_obj_send_event(lvgl_usart.data_bit_dropdown, LV_EVENT_VALUE_CHANGED, NULL);
                lv_obj_send_event(lvgl_usart.check_bit_dropdown, LV_EVENT_VALUE_CHANGED, NULL);
            }
            else if(dropdown == lvgl_usart.send_add_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                        lvgl_usart_state.send_add = 0;
                        break;
                    
                    case 1:
                        lvgl_usart_state.send_add = 1;
                        break;
                    
                    case 2:
                        lvgl_usart_state.send_add = 2;
                        break;
                    
                    case 3:
                        lvgl_usart_state.send_add = 3;
                        break;
                    
                    default: break;
                }
            }
            else if(dropdown == lvgl_usart.baud_rate_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                        lvgl_usart_baudrate_set(9600, lvgl_usart_state.usart);
                        break;
                    
                    case 1:
                        lvgl_usart_baudrate_set(19200, lvgl_usart_state.usart);
                        break;
                    
                    case 2:
                        lvgl_usart_baudrate_set(56000, lvgl_usart_state.usart);
                        break;
                    
                    case 3:
                        lvgl_usart_baudrate_set(115200, lvgl_usart_state.usart);
                        break;
                    
                    case 4:
                        lvgl_usart_baudrate_set(230400, lvgl_usart_state.usart);
                        break;
                    
                    case 5:
                        lvgl_usart_baudrate_set(460800, lvgl_usart_state.usart);
                        break;
                    
                    case 6:
                        lvgl_usart_baudrate_set(512000, lvgl_usart_state.usart);
                        break;
                    
                    case 7:
                        lvgl_usart_baudrate_set(750000, lvgl_usart_state.usart);
                        break;
                    
                    case 8:
                        lvgl_usart_baudrate_set(921600, lvgl_usart_state.usart);
                        break;
                    
                    case 9:
                        lvgl_usart_baudrate_set(1000000, lvgl_usart_state.usart);
                        break;
                    
                    default: break;
                }
                lvgl_usart_state.baud_rate = option_id;
            }
            else if(dropdown == lvgl_usart.stop_bit_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                        if(lvgl_usart_state.usart == 1)
                        {
                            lv_dropdown_set_selected(lvgl_usart.stop_bit_dropdown, lvgl_usart_state.stop_bit);
                            option_id = lvgl_usart_state.stop_bit;
                        }
                        else
                        {
                            lvgl_usart_stop_bit_set(USART_STB_0_5BIT, lvgl_usart_state.usart);
                        }
                        
                        break;
                    
                    case 1:
                        lvgl_usart_stop_bit_set(USART_STB_1BIT, lvgl_usart_state.usart);
                        break;
                    
                    case 2:
                        if(lvgl_usart_state.usart == 1)
                        {
                            lv_dropdown_set_selected(lvgl_usart.stop_bit_dropdown, lvgl_usart_state.stop_bit);
                            option_id = lvgl_usart_state.stop_bit;
                        }
                        else
                        {
                            lvgl_usart_stop_bit_set(USART_STB_1_5BIT, lvgl_usart_state.usart);
                        }
                        break;
                    
                    case 3:
                        lvgl_usart_stop_bit_set(USART_STB_2BIT, lvgl_usart_state.usart);
                        break;
                    
                    default: break;
                }
                lvgl_usart_state.stop_bit = option_id;
            }
            else if(dropdown == lvgl_usart.data_bit_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                        lvgl_usart_word_length_set(USART_WL_7BIT, lvgl_usart_state.usart);
                        break;
                    
                    case 1:
                        lvgl_usart_word_length_set(USART_WL_8BIT, lvgl_usart_state.usart);
                        break;
                    
                    case 2:
                        lvgl_usart_word_length_set(USART_WL_9BIT, lvgl_usart_state.usart);
                        break;
                    
                    case 3:
                        lvgl_usart_word_length_set(USART_WL_10BIT, lvgl_usart_state.usart);
                        break;
                    
                    default: break;
                }
                lvgl_usart_state.data_bit = option_id;
            }
            else if(dropdown == lvgl_usart.check_bit_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                        lvgl_usart_parity_config(USART_PM_NONE, lvgl_usart_state.usart);
                        break;
                    
                    case 1:
                        lvgl_usart_parity_config(USART_PM_ODD, lvgl_usart_state.usart);
                        break;
                    
                    case 2:
                        lvgl_usart_parity_config(USART_PM_EVEN, lvgl_usart_state.usart);
                        break;
                    
                    default: break;
                }
                lvgl_usart_state.check_bit = option_id;
            }
            else
            {
                
            }
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： lvgl_usart_creat
功    能 ： 创建USART终端
参    数 ： parent: 父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_usart_creat(lv_obj_t *parent)
{
    lv_obj_t *obj1, *obj2, *obj3, *obj4;
    lv_obj_t *label1, *label2, *label3, *label4, *label5;
    
    lvgl_usart.main_obj = lv_obj_create(parent);                 /* 创建容器 */
    lv_obj_set_size(lvgl_usart.main_obj, lv_pct(100), lv_pct(100));
    lv_obj_center(lvgl_usart.main_obj);
    lv_obj_set_style_pad_top(lvgl_usart.main_obj, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_usart.main_obj, 5, 0);
    lv_obj_set_style_pad_left(lvgl_usart.main_obj, 5, 0);
    lv_obj_set_style_pad_right(lvgl_usart.main_obj, 5, 0);
    lv_obj_remove_flag(lvgl_usart.main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(lvgl_usart.main_obj, true, 0);
    
    obj1 = lv_obj_create(lvgl_usart.main_obj);
    lv_obj_add_style(obj1, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj1, lv_pct(100), lv_pct(15));
    lv_obj_align(obj1, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(obj1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_usart.status_led = lv_led_create(obj1);
    lv_obj_set_size(lvgl_usart.status_led, 30, 30);
    lv_led_set_color(lvgl_usart.status_led, lv_color_hex(0x98FB98));
    lv_obj_align(lvgl_usart.status_led, LV_ALIGN_LEFT_MID, 15, 0);
    lv_led_off(lvgl_usart.status_led);
    
    lvgl_usart.usart_dropdown = lv_dropdown_create(obj1);
    lv_dropdown_set_options(lvgl_usart.usart_dropdown, "USART0\n"
                                                       "USART1");
    lv_dropdown_set_selected(lvgl_usart.usart_dropdown, lvgl_usart_state.usart);
    lv_obj_set_style_text_font(lvgl_usart.usart_dropdown, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(lvgl_usart.usart_dropdown), &lv_font_fzst_24, 0);
    lv_obj_set_size(lvgl_usart.usart_dropdown, lv_pct(20), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(lvgl_usart.usart_dropdown, 5, 0);
    lv_obj_set_style_pad_all(lv_dropdown_get_list(lvgl_usart.usart_dropdown), 5, 0);
    lv_obj_align_to(lvgl_usart.usart_dropdown, lvgl_usart.status_led, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_obj_add_event_cb(lvgl_usart.usart_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    label1 = lv_label_create(obj1);
    lv_label_set_text(label1, "串口终端");
    lv_obj_set_style_text_font(label1, &lv_font_fzst_32, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
    
    lvgl_usart.close_btn = lv_button_create(obj1);                                              /* 创建关闭按钮 */
    lv_obj_set_size(lvgl_usart.close_btn, lv_pct(10), lv_pct(100));
    lv_obj_align(lvgl_usart.close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_margin_all(lvgl_usart.close_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_usart.close_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    label2 = lv_label_create(lvgl_usart.close_btn);
    lv_label_set_text(label2, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(label2, &lv_font_fzst_32, 0);
    lv_obj_center(label2);
    
    lvgl_usart.send_add_dropdown = lv_dropdown_create(obj1);
    lv_dropdown_set_options(lvgl_usart.send_add_dropdown, "NULL\n"
                                                          "\\r\n"
                                                          "\\n\n"
                                                          "\\r\\n");
    lv_dropdown_set_selected(lvgl_usart.send_add_dropdown, lvgl_usart_state.send_add);
    lv_obj_set_style_text_font(lvgl_usart.send_add_dropdown, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(lvgl_usart.send_add_dropdown), &lv_font_fzst_24, 0);
    lv_obj_set_size(lvgl_usart.send_add_dropdown, lv_pct(20), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(lvgl_usart.send_add_dropdown, 5, 0);
    lv_obj_set_style_pad_all(lv_dropdown_get_list(lvgl_usart.send_add_dropdown), 5, 0);
    lv_obj_align_to(lvgl_usart.send_add_dropdown, lvgl_usart.close_btn, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_add_event_cb(lvgl_usart.send_add_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    lvgl_usart.receive_textarea = lv_textarea_create(lvgl_usart.main_obj);
    lv_obj_set_size(lvgl_usart.receive_textarea, lv_pct(75), lv_pct(65));
    lv_obj_set_style_text_font(lvgl_usart.receive_textarea, &lv_font_fzst_32, 0);
    lv_obj_align_to(lvgl_usart.receive_textarea, obj1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_state(lvgl_usart.receive_textarea, LV_STATE_FOCUSED);
    lv_obj_remove_flag(lvgl_usart.receive_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    
    lvgl_usart.send_textarea = lv_textarea_create(lvgl_usart.main_obj);
    lv_obj_set_size(lvgl_usart.send_textarea, lv_pct(55), lv_pct(20));
    lv_textarea_set_placeholder_text(lvgl_usart.send_textarea, "send textarea");
    lv_obj_set_style_text_font(lvgl_usart.send_textarea, &lv_font_fzst_32, 0);
    lv_obj_align(lvgl_usart.send_textarea, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_state(lvgl_usart.send_textarea, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(lvgl_usart.send_textarea, textarea_event_handler, LV_EVENT_ALL, NULL);
    
    obj2 = lv_obj_create(lvgl_usart.main_obj);
    lv_obj_add_style(obj2, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj2, lv_pct(25), lv_pct(85));
    lv_obj_align_to(obj2, obj1, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
    lv_obj_remove_flag(obj2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(obj2, 5, 0);
    lv_obj_set_flex_flow(obj2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj2, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    
    lvgl_usart.connect_btn = lv_button_create(obj2);
    lv_obj_set_size(lvgl_usart.connect_btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_all(lvgl_usart.connect_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_usart.connect_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_usart.connect_btn, 5, 0);
    
    lvgl_usart.connect_btn_label = lv_label_create(lvgl_usart.connect_btn);
    lv_label_set_text(lvgl_usart.connect_btn_label, "连接");
    lv_obj_set_style_text_font(lvgl_usart.connect_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_usart.connect_btn_label);
    
    label2 = lv_label_create(obj2);
    lv_label_set_text(label2, "波特率：");
    lv_obj_set_style_text_font(label2, &lv_font_fzst_16, 0);
    lv_obj_center(label2);
    
    lvgl_usart.baud_rate_dropdown = lv_dropdown_create(obj2);
    lv_dropdown_set_options(lvgl_usart.baud_rate_dropdown, "9600\n"
                                                           "19200\n"
                                                           "56000\n"
                                                           "115200\n"
                                                           "230400\n"
                                                           "460800\n"
                                                           "512000\n"
                                                           "750000\n"
                                                           "921600\n"
                                                           "1000000");
    lv_dropdown_set_selected(lvgl_usart.baud_rate_dropdown, lvgl_usart_state.baud_rate);
    lv_obj_set_style_text_font(lvgl_usart.baud_rate_dropdown, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(lvgl_usart.baud_rate_dropdown), &lv_font_fzst_24, 0);
    lv_obj_set_size(lvgl_usart.baud_rate_dropdown, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(lvgl_usart.baud_rate_dropdown, 5, 0);
    lv_obj_set_style_pad_all(lv_dropdown_get_list(lvgl_usart.baud_rate_dropdown), 5, 0);
    lv_obj_add_event_cb(lvgl_usart.baud_rate_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    label3 = lv_label_create(obj2);
    lv_label_set_text(label3, "停止位：");
    lv_obj_set_style_text_font(label3, &lv_font_fzst_16, 0);
    lv_obj_center(label3);
    
    lvgl_usart.stop_bit_dropdown = lv_dropdown_create(obj2);
    lv_dropdown_set_options(lvgl_usart.stop_bit_dropdown, "0.5\n"
                                                          "1\n"
                                                          "1.5\n"
                                                          "2");
    lv_dropdown_set_selected(lvgl_usart.stop_bit_dropdown, lvgl_usart_state.stop_bit);
    lv_obj_set_style_text_font(lvgl_usart.stop_bit_dropdown, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(lvgl_usart.stop_bit_dropdown), &lv_font_fzst_24, 0);
    lv_obj_set_size(lvgl_usart.stop_bit_dropdown, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(lvgl_usart.stop_bit_dropdown, 5, 0);
    lv_obj_set_style_pad_all(lv_dropdown_get_list(lvgl_usart.stop_bit_dropdown), 5, 0);
    lv_obj_add_event_cb(lvgl_usart.stop_bit_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    label4 = lv_label_create(obj2);
    lv_label_set_text(label4, "数据位：");
    lv_obj_set_style_text_font(label4, &lv_font_fzst_16, 0);
    lv_obj_center(label4);
    
    lvgl_usart.data_bit_dropdown = lv_dropdown_create(obj2);
    lv_dropdown_set_options(lvgl_usart.data_bit_dropdown, "7\n"
                                                          "8\n"
                                                          "9\n"
                                                          "10");
    lv_dropdown_set_selected(lvgl_usart.data_bit_dropdown, lvgl_usart_state.data_bit);
    lv_obj_set_style_text_font(lvgl_usart.data_bit_dropdown, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(lvgl_usart.data_bit_dropdown), &lv_font_fzst_24, 0);
    lv_obj_set_size(lvgl_usart.data_bit_dropdown, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(lvgl_usart.data_bit_dropdown, 5, 0);
    lv_obj_set_style_pad_all(lv_dropdown_get_list(lvgl_usart.data_bit_dropdown), 5, 0);
    lv_obj_add_event_cb(lvgl_usart.data_bit_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    label5 = lv_label_create(obj2);
    lv_label_set_text(label5, "校验位：");
    lv_obj_set_style_text_font(label5, &lv_font_fzst_16, 0);
    lv_obj_center(label5);
    
    lvgl_usart.check_bit_dropdown = lv_dropdown_create(obj2);
    lv_dropdown_set_options(lvgl_usart.check_bit_dropdown, "None\n"
                                                           "Odd\n"
                                                           "Even");
    lv_dropdown_set_selected(lvgl_usart.check_bit_dropdown, lvgl_usart_state.check_bit);
    lv_obj_set_style_text_font(lvgl_usart.check_bit_dropdown, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(lvgl_usart.check_bit_dropdown), &lv_font_fzst_24, 0);
    lv_obj_set_size(lvgl_usart.check_bit_dropdown, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(lvgl_usart.check_bit_dropdown, 5, 0);
    lv_obj_set_style_pad_all(lv_dropdown_get_list(lvgl_usart.check_bit_dropdown), 5, 0);
    lv_obj_add_event_cb(lvgl_usart.check_bit_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    lvgl_usart.clean_receive_btn = lv_button_create(obj2);
    lv_obj_set_size(lvgl_usart.clean_receive_btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_all(lvgl_usart.clean_receive_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_usart.clean_receive_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_usart.clean_receive_btn, 5, 0);
    
    lvgl_usart.clean_receive_btn_label = lv_label_create(lvgl_usart.clean_receive_btn);
    lv_label_set_text(lvgl_usart.clean_receive_btn_label, "清除接收");
    lv_obj_set_style_text_font(lvgl_usart.clean_receive_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_usart.clean_receive_btn_label);
    
    obj3 = lv_obj_create(lvgl_usart.main_obj);
    lv_obj_add_style(obj3, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj3, lv_pct(20), lv_pct(20));
    lv_obj_align_to(obj3, lvgl_usart.send_textarea, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_remove_flag(obj3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(obj3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj3, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lvgl_usart.send_btn = lv_button_create(obj3);
    lv_obj_set_size(lvgl_usart.send_btn, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_margin_all(lvgl_usart.send_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_usart.send_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_usart.send_btn, 5, 0);

    lvgl_usart.send_btn_label = lv_label_create(lvgl_usart.send_btn);
    lv_label_set_text(lvgl_usart.send_btn_label, "发送");
    lv_obj_set_style_text_font(lvgl_usart.send_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_usart.send_btn_label);
    
    lvgl_usart.clean_send_btn = lv_button_create(obj3);
    lv_obj_set_size(lvgl_usart.clean_send_btn, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_margin_all(lvgl_usart.clean_send_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_usart.clean_send_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_usart.clean_send_btn, 5, 0);

    lvgl_usart.clean_send_btn_label = lv_label_create(lvgl_usart.clean_send_btn);
    lv_label_set_text(lvgl_usart.clean_send_btn_label, "清除\n发送");
    lv_obj_set_style_text_font(lvgl_usart.clean_send_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_usart.clean_send_btn_label);
    
    main_menu_page_flag |= 0x08;
}

/**************************************************************
函数名称 ： lvgl_usart_delete
功    能 ： 删除USART终端
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_usart_delete(void)
{
    if(lvgl_main.all_keyboard != NULL)
    {
        lvgl_all_keyboard_delete();
    }
    
    lv_obj_delete(lvgl_usart.main_obj);
    lvgl_usart.main_obj = NULL;
    if(connect_status == 0x01)
    {
        connect_status = 0;
        lv_timer_delete(lvgl_usart.timer);
    }
    
    main_menu_page_flag &= ~0x08;
    if(main_menu_page_flag == 0)
    {
        lvgl_show_main_menu();
    }
}

/**************************************************************
函数名称 ： lvgl_usart_baudrate_set
功    能 ： 设置USART波特率
参    数 ： baudrate: 波特率, usart: 串口
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_usart_baudrate_set(uint32_t baudrate, uint8_t usart)
{
    switch(usart)
    {
        case 0:
            usart_disable(USART1); 
            usart_baudrate_set(USART1, baudrate);
            usart_enable(USART1); 
            break;
        
        case 1:
            break;
        
        default: break;
    }
    
}

/**************************************************************
函数名称 ： lvgl_usart_stop_bit_set
功    能 ： 设置USART停止位
参    数 ： stop_bit: 停止位, usart: 串口
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_usart_stop_bit_set(uint32_t stop_bit, uint8_t usart)
{
    switch(usart)
    {
        case 0:
            usart_disable(USART1); 
            usart_stop_bit_set(USART1, stop_bit);
            usart_enable(USART1); 
            break;
        
        case 1:
            break;
        
        default: break;
    }
    
}

/**************************************************************
函数名称 ： lvgl_usart_word_length_set
功    能 ： 设置USART字长
参    数 ： word_length: 字长, usart: 串口
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_usart_word_length_set(uint32_t word_length, uint8_t usart)
{
    switch(usart)
    {
        case 0:
            usart_disable(USART1); 
            usart_word_length_set(USART1, word_length);
            usart_enable(USART1); 
            break;
        
        case 1:
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： lvgl_usart_parity_config
功    能 ： 设置USART校验
参    数 ： parity: 校验, usart: 串口
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_usart_parity_config(uint32_t parity, uint8_t usart)
{
    switch(usart)
    {
        case 0:
            usart_disable(USART1); 
            usart_parity_config(USART1, parity);
            usart_enable(USART1); 
            break;
        
        case 1:
            break;
        
        default: break;
    }
}
