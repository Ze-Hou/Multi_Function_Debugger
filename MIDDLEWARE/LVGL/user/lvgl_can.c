#include "lvgl_can.h"
#include "freertos_main.h"
#include "lvgl_main.h"
#include "./CAN/can.h"
#include "./MALLOC/malloc.h"
#include <string.h>

lvgl_can_struct lvgl_can;
lvgl_can_state_struct lvgl_can_state;

#define SHOW_LINE_NUM   10
#define BUFFER_TEXTAREA_SIZE (4 * 1024)

static uint8_t connect_status = 0;

/* static function declarations */
static void lvgl_can_delete(void);

/**************************************************************
函数名称 ： timer_cb
功    能 ： 定时器回调
参    数 ： timer
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void timer_cb(lv_timer_t *timer)
{
    uint8_t buffer_len = 0;
    uint8_t *buffer_temp;
    char *buffer_textarea;
    uint16_t i = 0;
    uint32_t cursor_pos, total_line_num;
    lv_obj_t *textarea_label;
    lv_point_t letter_pos;
    
    cursor_pos = lv_textarea_get_cursor_pos(lvgl_can.receive_textarea);
    textarea_label = lv_textarea_get_label(lvgl_can.receive_textarea);

    lv_label_get_letter_pos(textarea_label, cursor_pos, &letter_pos);
    total_line_num = letter_pos.y / 34 + 1;
    if(total_line_num > SHOW_LINE_NUM)
    {
        letter_pos.x = 0;
        letter_pos.y = (total_line_num - SHOW_LINE_NUM) * 34;
        lv_label_cut_text(textarea_label, 0, lv_label_get_letter_on(textarea_label, &letter_pos, true));
    }
    
    can_read_dam_receive_data();
    if(gCanReceiveDataLength)
    {
        buffer_len = snprintf(NULL, 0, "%02X %02X %02X %02X %02X %02X %02X %02X", \
                                        gCanReceiveData[i * 8], gCanReceiveData[i * 8 + 1], gCanReceiveData[i * 8 + 2], gCanReceiveData[i * 8 + 3], \
                                        gCanReceiveData[i * 8 + 4], gCanReceiveData[i * 8 + 5], gCanReceiveData[i * 8 + 6], gCanReceiveData[i * 8 + 7]) + 1;
        buffer_temp = (uint8_t *)mymalloc(SRAMDTCM, buffer_len);
        buffer_textarea = (char *)mymalloc(SRAMDTCM, BUFFER_TEXTAREA_SIZE);
        if((buffer_temp == NULL) || (buffer_textarea == NULL))
        {
            myfree(SRAMDTCM, buffer_temp);
            myfree(SRAMDTCM, buffer_textarea);
            return;
        }
        
        memset((void *)buffer_textarea, 0x00, BUFFER_TEXTAREA_SIZE);
        
        for(i = 0; i < gCanReceiveDataLength / 8; i++)
        {
            snprintf((char *)buffer_temp, buffer_len, "%02X %02X %02X %02X %02X %02X %02X %02X", \
                                                      gCanReceiveData[i * 8], gCanReceiveData[i * 8 + 1], gCanReceiveData[i * 8 + 2], gCanReceiveData[i * 8 + 3], \
                                                      gCanReceiveData[i * 8 + 4], gCanReceiveData[i * 8 + 5], gCanReceiveData[i * 8 + 6], gCanReceiveData[i * 8 + 7]);
            strncat((char *)buffer_textarea, (const char *)buffer_temp, BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
        }
        if(gCanReceiveDataLength % 8)
        {
            for(i = i * 8 ; i < gCanReceiveDataLength - 1; i++)
            {
                snprintf((char *)buffer_temp, buffer_len, "%02X ", gCanReceiveData[i]);
                strncat((char *)buffer_textarea, (const char *)buffer_temp, BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
            }
            snprintf((char *)buffer_temp, buffer_len, "%02X\n", gCanReceiveData[i]);
            strncat((char *)buffer_textarea, (const char *)buffer_temp, BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
        }
        else
        {
            strncat((char *)buffer_textarea, (const char *)"\n", BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
        }
        lv_textarea_add_text(lvgl_can.receive_textarea, (const char *)buffer_textarea);
        myfree(SRAMDTCM, buffer_temp);
        myfree(SRAMDTCM, buffer_textarea);
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
            if(dropdown == lvgl_can.mode_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                        lvgl_can_state.mode = 0;
                        can_operation_mode_enter(CAN1, CAN_LOOPBACK_SILENT_MODE);
                        break;
                    
                    case 1:
                        lvgl_can_state.mode = 1;
                        can_operation_mode_enter(CAN1, CAN_NORMAL_MODE);
                        break;
                    
                    default: break;
                }
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
    char *token;
    char send_text_temp[32];
    uint8_t send_length = 0;
    uint8_t send_data[8];
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(btn == lvgl_can.close_btn)
            {
                lvgl_can_delete();
            }
            else if(btn == lvgl_can.connect_btn)
            {
                if(connect_status == 0)
                {
                    connect_status = 0x01;
                    lv_led_on(lvgl_can.status_led);
                    lv_obj_set_style_bg_color(lvgl_can.connect_btn, lv_palette_main(LV_PALETTE_RED), 0);
                    lv_label_set_text(lvgl_can.connect_btn_label, "断开");
                    
                    lv_textarea_set_text(lvgl_can.receive_textarea, (const char *)"");
                    lvgl_can.timer = lv_timer_create(timer_cb, 50, NULL);
                }
                else if(connect_status == 0x01)
                {
                    connect_status = 0;
                    lv_led_off(lvgl_can.status_led);
                    lv_obj_set_style_bg_color(lvgl_can.connect_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
                    lv_label_set_text(lvgl_can.connect_btn_label, "连接");
                    
                    lv_timer_delete(lvgl_can.timer);
                }
            }
            else if(btn == lvgl_can.clean_receive_btn)
            {
                lv_textarea_set_text(lvgl_can.receive_textarea, (const char *)"");
            }
            else if(btn == lvgl_can.send_btn)
            {
                send_text = lv_textarea_get_text(lvgl_can.send_textarea);
                if(send_text[0] != '\0')
                {
                    memcpy(send_text_temp, send_text, strlen(send_text) + 1);
                    
                    if(strlen(send_text_temp) > 2)
                    {
                        token = strtok(send_text_temp, " ");
                        while(token != NULL && send_length < 8)
                        {
                            send_data[send_length++] = (uint8_t)strtol(token, NULL, 16);
                            token = strtok(NULL, " ");
                        }
                    }
                    else
                    {
                        send_data[send_length++] = (uint8_t)strtol(send_text_temp, NULL, 16);
                    }
                    
                    if(strlen(send_text_temp))
                    {
                        can_transmit_message(0, send_length, send_data, 8);
                    }
                }
            }
            else if(btn == lvgl_can.clean_send_btn)
            {
                lv_textarea_set_text(lvgl_can.send_textarea, (const char *)"");
            }
            break;
        
        default: break;
    }
    
}

/**************************************************************
函数名称 ： setting_label_event_handler
功    能 ： 设置标签事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void setting_label_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            
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
                lvgl_all_keyboard_create(lvgl_can.main_obj);
            }
            lv_keyboard_set_textarea(lvgl_main.all_keyboard, lvgl_can.send_textarea);
            break;

        case LV_EVENT_CANCEL:
            lvgl_all_keyboard_delete();
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： lvgl_can_creat
功    能 ： 创建CAN终端
参    数 ： parent: 父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_can_creat(lv_obj_t *parent)
{
    lv_obj_t *obj1, *obj2, *obj3;
    lv_obj_t *label1, *label2;
    
    lvgl_can.main_obj = lv_obj_create(parent);                 /* 创建容器 */
    lv_obj_set_size(lvgl_can.main_obj, lv_pct(100), lv_pct(100));
    lv_obj_center(lvgl_can.main_obj);
    lv_obj_set_style_pad_top(lvgl_can.main_obj, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_can.main_obj, 5, 0);
    lv_obj_set_style_pad_left(lvgl_can.main_obj, 5, 0);
    lv_obj_set_style_pad_right(lvgl_can.main_obj, 5, 0);
    lv_obj_remove_flag(lvgl_can.main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(lvgl_can.main_obj, true, 0);
    
    /* 创建顶部容器栏 */
    obj1 = lv_obj_create(lvgl_can.main_obj);
    lv_obj_add_style(obj1, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj1, lv_pct(100), lv_pct(15));
    lv_obj_align(obj1, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(obj1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_can.status_led = lv_led_create(obj1);
    lv_obj_set_size(lvgl_can.status_led, 30, 30);
    lv_led_set_color(lvgl_can.status_led, lv_color_hex(0x98FB98));
    lv_obj_align(lvgl_can.status_led, LV_ALIGN_LEFT_MID, 15, 0);
    lv_led_off(lvgl_can.status_led);
    
    lvgl_can.mode_dropdown = lv_dropdown_create(obj1);
    lv_dropdown_set_options(lvgl_can.mode_dropdown, "LOOPBACK\n"
                                                    "NORMAL");
    lv_dropdown_set_selected(lvgl_can.mode_dropdown, lvgl_can_state.mode);
    lv_obj_set_style_text_font(lvgl_can.mode_dropdown, &lv_font_fzst_24, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(lvgl_can.mode_dropdown), &lv_font_fzst_24, 0);
    lv_obj_set_size(lvgl_can.mode_dropdown, lv_pct(25), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(lvgl_can.mode_dropdown, 5, 0);
    lv_obj_set_style_pad_all(lv_dropdown_get_list(lvgl_can.mode_dropdown), 5, 0);
    lv_obj_align_to(lvgl_can.mode_dropdown, lvgl_can.status_led, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_obj_add_event_cb(lvgl_can.mode_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    label1 = lv_label_create(obj1);
    lv_label_set_text(label1, "CAN终端");
    lv_obj_set_style_text_font(label1, &lv_font_fzst_32, 0);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
    
    lvgl_can.close_btn = lv_button_create(obj1);                                              /* 创建关闭按钮 */
    lv_obj_set_size(lvgl_can.close_btn, lv_pct(10), lv_pct(100));
    lv_obj_align(lvgl_can.close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_margin_all(lvgl_can.close_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_can.close_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    label2 = lv_label_create(lvgl_can.close_btn);
    lv_label_set_text(label2, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(label2, &lv_font_fzst_32, 0);
    lv_obj_center(label2);
    
    lvgl_can.setting_label = lv_label_create(obj1);
    lv_label_set_text(lvgl_can.setting_label, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(lvgl_can.setting_label, &lv_font_fzst_32, 0);
    lv_obj_align_to(lvgl_can.setting_label, lvgl_can.close_btn, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_add_flag(lvgl_can.setting_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvgl_can.setting_label, setting_label_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_can.receive_textarea = lv_textarea_create(lvgl_can.main_obj);
    lv_obj_set_size(lvgl_can.receive_textarea, lv_pct(100), lv_pct(65));
    lv_obj_set_style_text_font(lvgl_can.receive_textarea, &lv_font_fzst_32, 0);
    lv_obj_align_to(lvgl_can.receive_textarea, obj1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_state(lvgl_can.receive_textarea, LV_STATE_FOCUSED);
    lv_obj_remove_flag(lvgl_can.receive_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    
    lvgl_can.send_textarea = lv_textarea_create(lvgl_can.main_obj);
    lv_obj_set_size(lvgl_can.send_textarea, lv_pct(60), lv_pct(20));
    lv_textarea_set_placeholder_text(lvgl_can.send_textarea, "send textarea(HEX)");
    lv_textarea_set_accepted_chars(lvgl_can.send_textarea, "0123456789ABCDEF ");
    lv_textarea_set_max_length(lvgl_can.send_textarea, 23);
    lv_obj_set_style_text_font(lvgl_can.send_textarea, &lv_font_fzst_32, 0);
    lv_obj_align(lvgl_can.send_textarea, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_state(lvgl_can.send_textarea, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(lvgl_can.send_textarea, textarea_event_handler, LV_EVENT_ALL, NULL);
    
    obj2 = lv_obj_create(lvgl_can.main_obj);
    lv_obj_add_style(obj2, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj2, lv_pct(20), lv_pct(20));
    lv_obj_align_to(obj2, lvgl_can.send_textarea, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_remove_flag(obj2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(obj2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj2, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lvgl_can.send_btn = lv_button_create(obj2);
    lv_obj_set_size(lvgl_can.send_btn, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_margin_all(lvgl_can.send_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_can.send_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_can.send_btn, 5, 0);

    lvgl_can.send_btn_label = lv_label_create(lvgl_can.send_btn);
    lv_label_set_text(lvgl_can.send_btn_label, "发送");
    lv_obj_set_style_text_font(lvgl_can.send_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_can.send_btn_label);
    
    lvgl_can.clean_send_btn = lv_button_create(obj2);
    lv_obj_set_size(lvgl_can.clean_send_btn, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_margin_all(lvgl_can.clean_send_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_can.clean_send_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_can.clean_send_btn, 5, 0);

    lvgl_can.clean_send_btn_label = lv_label_create(lvgl_can.clean_send_btn);
    lv_label_set_text(lvgl_can.clean_send_btn_label, "清除\n发送");
    lv_obj_set_style_text_font(lvgl_can.clean_send_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_can.clean_send_btn_label);
    
    obj3 = lv_obj_create(lvgl_can.main_obj);
    lv_obj_add_style(obj3, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj3, lv_pct(20), lv_pct(20));
    lv_obj_align(obj3, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_remove_flag(obj3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(obj3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj3, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lvgl_can.connect_btn = lv_button_create(obj3);
    lv_obj_set_size(lvgl_can.connect_btn, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_margin_all(lvgl_can.connect_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_can.connect_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_can.connect_btn, 5, 0);

    lvgl_can.connect_btn_label = lv_label_create(lvgl_can.connect_btn);
    lv_label_set_text(lvgl_can.connect_btn_label, "连接");
    lv_obj_set_style_text_font(lvgl_can.connect_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_can.connect_btn_label);
    
    lvgl_can.clean_receive_btn = lv_button_create(obj3);
    lv_obj_set_size(lvgl_can.clean_receive_btn, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_style_margin_all(lvgl_can.clean_receive_btn, 5, 0);
    lv_obj_add_event_cb(lvgl_can.clean_receive_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_all(lvgl_can.clean_receive_btn, 5, 0);

    lvgl_can.clean_receive_btn_label = lv_label_create(lvgl_can.clean_receive_btn);
    lv_label_set_text(lvgl_can.clean_receive_btn_label, "清除\n接收");
    lv_obj_set_style_text_font(lvgl_can.clean_receive_btn_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_can.clean_receive_btn_label);
    
    main_menu_page_flag |= 0x10;
}

/**************************************************************
函数名称 ： lvgl_can_delete
功    能 ： 删除CAN终端
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_can_delete(void)
{
    if(lvgl_main.all_keyboard != NULL)
    {
        lvgl_all_keyboard_delete();
    }
    
    lv_obj_delete(lvgl_can.main_obj);
    lvgl_can.main_obj = NULL;
    
    if(connect_status == 0x01)
    {
        connect_status = 0;
        lv_timer_delete(lvgl_can.timer);
    }
    
    main_menu_page_flag &= ~0x10;
    if(main_menu_page_flag == 0)
    {
        lvgl_show_main_menu();
    }
}