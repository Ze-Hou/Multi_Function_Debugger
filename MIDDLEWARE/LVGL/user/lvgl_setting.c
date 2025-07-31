#include "lvgl_setting.h"
#include "lvgl_main.h"
#include "./USART/usart.h"
#include "./SYSTEM/system.h"
#include "./CH224K/ch224k.h"
#include "./SCREEN/RGBLCD/rgblcd.h"
#include "./SC8721/sc8721.h"
#include "./MALLOC/malloc.h"
#include "freertos_main.h"

lvgl_setting_struct lvgl_setting;
lvgl_setting_state_struct lvgl_setting_state;
extern lvgl_main_state_struct lvgl_main_state;

static lvgl_setting_wireless_state_struct lvgl_setting_wireless_state;
lvgl_setting_wireless_scan_info_struct lvgl_setting_wireless_scan_info;
lvgl_setting_wireless_connect_struct lvgl_setting_wireless_connect;

typedef enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
} lv_menu_builder_variant_t;

/* static function declarations */
static lv_obj_t *menu_create_text(lv_obj_t *parent, const char *icon, const char *txt, lv_menu_builder_variant_t builder_variant);
static lv_obj_t *menu_create_slider(lv_obj_t *parent, const char *icon, const char *txt, int32_t min, int32_t max, int32_t val);
static lv_obj_t *menu_create_switch(lv_obj_t *parent, const char *icon, const char *txt, bool chk);
static lv_obj_t *menu_create_btn(lv_obj_t *parent, const char *icon, const char *txt, const char *btn_txt);
static lv_obj_t *menu_create_dropdown(lv_obj_t *parent, const char *icon, const char *txt, const char *option, uint8_t option_id);
static void lvgl_setting_delete(void);
static void lvgl_setting_scan_wifi_list_creat(void);
static void lvgl_setting_scan_wifi_list_delete(void);

/**************************************************************
函数名称 ： timer_cb
功    能 ： 定时器回调
参    数 ： timer
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void timer_cb(lv_timer_t *timer)
{
    if(xQueueReceive(xQueueWirelseeState, &lvgl_setting_wireless_state, 0) == pdTRUE)
    {
        switch(lvgl_setting_wireless_state.status)
        {
            case 0:
                xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)0, (eNotifyAction)eSetValueWithOverwrite);
                break;
            
            case 1:
                lvgl_setting_scan_wifi_list_creat();
                break;
            
            case 2:
                xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)8, (eNotifyAction)eSetValueWithOverwrite);
                break;
            
            case 3:
                xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)7, (eNotifyAction)eSetValueWithOverwrite);
                break;
            
            case 4:
                break;
            
            /* 正确响应， 但不做处理返回值 */
            case 254:
                break;
            
            /* 错误处理 */
            case 255:
                switch(lvgl_setting_wireless_state.error)
                {
                    case 1:
                        lvgl_show_error_msgbox_creat("扫描WIFI出错，请重新尝试！");
                        break;
                    
                    case 2:
                        lvgl_show_error_msgbox_creat("连接WIFI出错，请重新尝试！");
                        break;
                    
                    case 3:
                        lvgl_show_error_msgbox_creat("连接上次连接的WIFI出错，请重新尝试！");
                        break;
                    
                    case 4:
                        lvgl_show_error_msgbox_creat("断开WIFI出错，请重新尝试！");
                        break;
                    
                    case 5:
                        lvgl_show_error_msgbox_creat("查询TCP/UDP/SSL连接状态和信息出错，请重新尝试！");
                        break;
                    
                    case 6:
                        lvgl_show_error_msgbox_creat("断开TCP/UDP/SSL连接出错，请重新尝试！");
                        break;
                    
                    case 7:
                        lvgl_show_error_msgbox_creat("建立TCP/UDP/SSL连接出错，请重新尝试！");
                        break;
                    
                    case 8:
                        lvgl_show_error_msgbox_creat("访问NTP服务器出错，请重新尝试！");
                        break;
                    
                    default: break;
                }
                break;
            
            default: break;
        }
    }
    
    if(lvgl_setting_state.wifi)
    {
        lv_label_set_text(lvgl_setting.wifi_label, lvgl_main_state.wifi_name);
    }
}

/**************************************************************
函数名称 ： back_event_handler
功    能 ： 返回按钮事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void back_event_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *menu = lv_event_get_user_data(e);

    if(lv_menu_back_button_is_root(menu, obj)) {
        lvgl_setting_delete();
    }
}

/**************************************************************
函数名称 ： slider_event_handler
功    能 ： 滑动条事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void slider_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *slider = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_VALUE_CHANGED:
            if(slider == lvgl_setting.brightness_slider)
            {
                lvgl_setting_state.brightness = (uint8_t)lv_slider_get_value(slider);
                lv_label_set_text_fmt(lvgl_setting.brightness_slider_label, "%u%%", lvgl_setting_state.brightness);
                rgblcd_blk_set(lvgl_setting_state.brightness);
            }
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： switch_event_handler
功    能 ： 开关事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void switch_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *sw = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_VALUE_CHANGED:
            if(sw == lvgl_setting.wifi_switch)
            {
                lvgl_setting_state.wifi = lv_obj_has_state(sw, LV_STATE_CHECKED) ? 1 : 0;
                
                if(lvgl_setting_state.wifi)
                {
                    lv_obj_remove_flag(lvgl_setting.wifi_cont, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(lvgl_setting.wifi_scan_cont, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(lvgl_setting.ntp_section, LV_OBJ_FLAG_HIDDEN);
                    xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)3, (eNotifyAction)eSetValueWithOverwrite);
                }
                else
                {
                    lv_obj_add_flag(lvgl_setting.wifi_cont, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(lvgl_setting.wifi_scan_cont, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(lvgl_setting.ntp_section, LV_OBJ_FLAG_HIDDEN);
                    xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)4, (eNotifyAction)eSetValueWithOverwrite);
                }
            }
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： cont_event_handler
功    能 ： 菜单容器事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void cont_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cont = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(cont == lvgl_setting.wifi_cont)
            {
                if(strcmp(lv_label_get_text(lvgl_setting.wifi_label), "NULL") != 0)
                {
                    xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)4, (eNotifyAction)eSetValueWithOverwrite);
                }
                else
                {
                    xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)3, (eNotifyAction)eSetValueWithOverwrite);
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
static void btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(btn == lvgl_setting.wifi_scan_btn)
            {
                xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)1, (eNotifyAction)eSetValueWithOverwrite);
            }
            else if(btn == lvgl_setting.get_ntp_data_btn)
            {
                xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)5, (eNotifyAction)eSetValueWithOverwrite);
            }
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
            if(dropdown == lvgl_setting.pd_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        lvgl_setting_state.pd_voltage = option_id + 1;
                        ch224k_config(lvgl_setting_state.pd_voltage);
                        break;
                    
                    default: break;
                }
            }
            else if(dropdown == lvgl_setting.ntp_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                    case 1:
                        lvgl_setting_wireless_connect.ntp_server = option_id;
                        xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)6, (eNotifyAction)eSetValueWithOverwrite);
                        break;
                    
                    default: break;
                }
                
            }
            else if(dropdown == lvgl_setting.sc8721_frequency_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        lvgl_setting_state.sc8721_frequency = option_id;
                        sc8721_frequency_set(lvgl_setting_state.sc8721_frequency);
                        break;
                    
                    default: break;
                }
            }
            else if(dropdown == lvgl_setting.sc8721_slope_dropdown)
            {
                switch(option_id)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        lvgl_setting_state.sc8721_slope = option_id;
                        sc8721_slope_comp_set(lvgl_setting_state.sc8721_slope);
                        break;
                    
                    default: break;
                }
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
    const char *temp;
    uint8_t copy_length = 0;
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(lvgl_main.all_keyboard == NULL)
            {
                lvgl_all_keyboard_create(lvgl_setting.main_obj);
            }
            lv_keyboard_set_textarea(lvgl_main.all_keyboard, textarea);
            break;
            
        case LV_EVENT_READY:
            temp = lv_textarea_get_text(textarea);
            copy_length = strlen(temp) + 1;
            memset(lvgl_setting_wireless_connect.wifi_password, 0x00, sizeof(lvgl_setting_wireless_connect.wifi_password));
            memcpy(lvgl_setting_wireless_connect.wifi_password, temp, copy_length > 32 ? 32 : copy_length);
            xTaskNotify((TaskHandle_t)WIRELESSTask_Handler, (uint32_t)2, (eNotifyAction)eSetValueWithOverwrite);
            lv_obj_send_event(textarea, LV_EVENT_CANCEL, NULL);
            break;
            
        case LV_EVENT_CANCEL:
            lv_textarea_set_text(textarea, "");
            lv_obj_remove_state(textarea, LV_STATE_FOCUSED);
            lv_obj_add_flag(textarea, LV_OBJ_FLAG_HIDDEN);
            lvgl_all_keyboard_delete();
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： lvgl_setting_creat
功    能 ： 创建设置
参    数 ： parent: 设置的父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_setting_creat(lv_obj_t *parent)
{
    lv_obj_t *menu;
    lv_obj_t *root_page;
    lv_obj_t *about_page, *display_brightness_page, *wifi_page, *bluetooth_page, *power_page;
    lv_obj_t *section;
    lv_obj_t *cont;
    
    uint8_t buffer_len;
    char data_buffer[256];
    
    lvgl_setting.main_obj = lv_obj_create(parent);
    lv_obj_set_size(lvgl_setting.main_obj, lv_pct(100), lv_pct(100));
    lv_obj_center(lvgl_setting.main_obj);
    lv_obj_set_style_pad_top(lvgl_setting.main_obj, 0, 0);
    lv_obj_set_style_pad_bottom(lvgl_setting.main_obj, 0, 0);
    lv_obj_set_style_pad_left(lvgl_setting.main_obj, 0, 0);
    lv_obj_set_style_pad_right(lvgl_setting.main_obj, 0, 0);
    lv_obj_remove_flag(lvgl_setting.main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(lvgl_setting.main_obj, true, 0);
    
    menu = lv_menu_create(lvgl_setting.main_obj);
    lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 10), 0);
    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
    lv_obj_set_size(menu, lv_pct(100), lv_pct(100));
    lv_obj_center(menu);
    lv_obj_set_style_text_font(menu, &lv_font_fzst_24, 0);
    
    root_page = lv_menu_page_create(menu, "设置");
    lv_obj_set_style_pad_hor(root_page, 10, 0);
    lv_obj_set_style_pad_row(root_page, 10, 0);
    lv_menu_set_sidebar_page(menu, root_page);
    lv_obj_set_style_text_font(lv_menu_get_sidebar_header(menu), &lv_font_fzst_32, 0);
    
    /* sub_page 1 about page */
    about_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(about_page, 10, 0);
    lv_obj_set_style_pad_row(about_page, 10, 0);
    lv_menu_separator_create(about_page);
    section = lv_menu_section_create(about_page);
    cont = menu_create_text(section, NULL, "设备名称：多功能调试器", LV_MENU_ITEM_BUILDER_VARIANT_1);
    cont = menu_create_text(section, NULL, "Powered by ZEHOU", LV_MENU_ITEM_BUILDER_VARIANT_1);         /* 请勿修改 */
    
    section = lv_menu_section_create(about_page);
    cont = menu_create_text(section, NULL, "CPU: GD32H759IMK6", LV_MENU_ITEM_BUILDER_VARIANT_1);
    buffer_len = snprintf(NULL, 0, "SN: %08X%08X%08X", system_device_info.device_id[2], system_device_info.device_id[1], system_device_info.device_id[0]) + 1;
    snprintf(data_buffer, buffer_len, "SN: %08X%08X%08X", system_device_info.device_id[2], system_device_info.device_id[1], system_device_info.device_id[0]);
    cont = menu_create_text(section, NULL, data_buffer, LV_MENU_ITEM_BUILDER_VARIANT_1);
    buffer_len = snprintf(NULL, 0, "PID: %08X", system_device_info.device_pid) + 1;
    snprintf(data_buffer, buffer_len, "PID: %08X", system_device_info.device_pid);
    cont = menu_create_text(section, NULL, data_buffer, LV_MENU_ITEM_BUILDER_VARIANT_1);
    buffer_len = snprintf(NULL, 0, "ROM: %u KB", system_device_info.memory_flash) + 1;
    snprintf(data_buffer, buffer_len, "ROM: %u KB", system_device_info.memory_flash);
    cont = menu_create_text(section, NULL, data_buffer, LV_MENU_ITEM_BUILDER_VARIANT_1);
    buffer_len = snprintf(NULL, 0, "RAM: %u KB(ITCM: %u, DTCM: %u, SRAM: %u)", system_device_info.memory_sram, system_device_info.share_sram_itcm, system_device_info.share_sram_dtcm, \
                                                                               system_device_info.memory_sram - system_device_info.share_sram_itcm - system_device_info.share_sram_dtcm) + 1;
    snprintf(data_buffer, buffer_len, "RAM: %u KB(ITCM: %u, DTCM: %u, SRAM: %u)", system_device_info.memory_sram, system_device_info.share_sram_itcm, system_device_info.share_sram_dtcm, \
                                                                                  system_device_info.memory_sram - system_device_info.share_sram_itcm - system_device_info.share_sram_dtcm);
    cont = menu_create_text(section, NULL, data_buffer, LV_MENU_ITEM_BUILDER_VARIANT_1);

    section = lv_menu_section_create(root_page);
    cont = menu_create_text(section, NULL, "关于此设备", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, about_page);
    
    /* sub_page 2 display and brightness page */
    display_brightness_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(display_brightness_page, 10, 0);
    lv_obj_set_style_pad_row(display_brightness_page, 10, 0);
    lv_menu_separator_create(display_brightness_page);
    section = lv_menu_section_create(display_brightness_page);
    cont = menu_create_slider(section, LV_SYMBOL_EYE_OPEN, "亮度", 0, 100, lvgl_setting_state.brightness);
    lvgl_setting.brightness_slider = lv_obj_get_child(cont, 2);
    lvgl_setting.brightness_slider_label = lv_obj_get_child(cont, 3);
    lv_obj_add_event_cb(lvgl_setting.brightness_slider, slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    section = lv_menu_section_create(display_brightness_page);
    buffer_len = snprintf(NULL, 0, "屏幕分辨率: %ux%u", lv_display_get_horizontal_resolution(lv_display_get_default()), \
                                                        lv_display_get_vertical_resolution(lv_display_get_default())) + 1;
    snprintf(data_buffer, buffer_len, "屏幕分辨率: %ux%u", lv_display_get_horizontal_resolution(lv_display_get_default()), \
                                                           lv_display_get_vertical_resolution(lv_display_get_default()));
    cont = menu_create_text(section, NULL, data_buffer, LV_MENU_ITEM_BUILDER_VARIANT_1);
    
    section = lv_menu_section_create(root_page);
    cont = menu_create_text(section, NULL, "显示与亮度", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, display_brightness_page);
    
    /* sub_page 3 wifi page */
    wifi_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(wifi_page, 10, 0);
    lv_obj_set_style_pad_row(wifi_page, 10, 0);
    lv_menu_separator_create(wifi_page);
    section = lv_menu_section_create(wifi_page);
    cont = menu_create_switch(section, NULL, "WIFI", lvgl_setting_state.wifi ? true : false);
    lvgl_setting.wifi_switch = lv_obj_get_child(cont, 1);
    lv_obj_add_event_cb(lvgl_setting.wifi_switch, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    cont = menu_create_text(section, LV_SYMBOL_WIFI, "NULL", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lvgl_setting.wifi_cont = cont;
    lvgl_setting.wifi_label = lv_obj_get_child(cont, 1);
    lv_obj_add_flag(lvgl_setting.wifi_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lvgl_setting.wifi_cont, cont_event_handler, LV_EVENT_CLICKED, NULL);
    
    section = lv_menu_section_create(wifi_page);
    cont = menu_create_btn(section, NULL, "选取附近wifi", "扫描");
    lvgl_setting.wifi_scan_cont = cont;
    lvgl_setting.wifi_scan_btn = lv_obj_get_child(cont, 1);
    lv_obj_add_event_cb(lvgl_setting.wifi_scan_btn, btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    section = lv_menu_section_create(wifi_page);
    lvgl_setting.ntp_section = section;
    cont = menu_create_dropdown(lvgl_setting.ntp_section, NULL, "NTP服务器", "ntp.aliyun.com\n"
                                                            "ntp.sjtu.edu.cn", lvgl_setting_wireless_connect.ntp_server);
    lvgl_setting.ntp_dropdown = lv_obj_get_child(cont, 1);
    lv_obj_add_event_cb(lvgl_setting.ntp_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    cont = menu_create_btn(lvgl_setting.ntp_section, NULL, "从NTP服务器同步时间", "同步");
    lvgl_setting.get_ntp_data_btn = lv_obj_get_child(cont, 1);
    lv_obj_add_event_cb(lvgl_setting.get_ntp_data_btn, btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    if(!lvgl_setting_state.wifi)
    {
        lv_obj_add_flag(lvgl_setting.wifi_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lvgl_setting.wifi_scan_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lvgl_setting.ntp_section, LV_OBJ_FLAG_HIDDEN);
    }

    /* sub_page 4 bluetooth page */
    bluetooth_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(bluetooth_page, 10, 0);
    lv_obj_set_style_pad_row(bluetooth_page, 10, 0);
    lv_menu_separator_create(bluetooth_page);
    section = lv_menu_section_create(bluetooth_page);
    cont = menu_create_text(section, NULL, "等待硬件升级", LV_MENU_ITEM_BUILDER_VARIANT_1);
    
    section = lv_menu_section_create(root_page);
    cont = menu_create_text(section, LV_SYMBOL_WIFI, "WIFI", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, wifi_page);
    cont = menu_create_text(section, LV_SYMBOL_BLUETOOTH, "蓝牙", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, bluetooth_page);
    
    /* sub_page 5 power page */
    power_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(power_page, 10, 0);
    lv_obj_set_style_pad_row(power_page, 10, 0);
    lv_menu_separator_create(power_page);
    section = lv_menu_section_create(power_page);
    cont = menu_create_dropdown(section, NULL, "设置PD协议电压", "POWER_9V\n"
                                                                 "POWER_12V\n"
                                                                 "POWER_15V\n"
                                                                 "POWER_20V", lvgl_setting_state.pd_voltage - 1);
    lvgl_setting.pd_dropdown = lv_obj_get_child(cont, 1);
    lv_obj_add_event_cb(lvgl_setting.pd_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    section = lv_menu_section_create(power_page);
    cont = menu_create_dropdown(section, NULL, "设置数字电源开关频率", "260KHz\n"
                                                                       "500KHz\n"
                                                                       "720KHz\n"
                                                                       "900KHz", lvgl_setting_state.sc8721_frequency);
    lvgl_setting.sc8721_frequency_dropdown = lv_obj_get_child(cont, 1);
    lv_obj_add_event_cb(lvgl_setting.sc8721_frequency_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    cont = menu_create_dropdown(section, NULL, "设置数字电源斜率补偿", "NULL\n"
                                                                       "50mV/A\n"
                                                                       "100mV/A\n"
                                                                       "150mV/A", lvgl_setting_state.sc8721_slope);
    lvgl_setting.sc8721_slope_dropdown = lv_obj_get_child(cont, 1);
    lv_obj_add_event_cb(lvgl_setting.sc8721_slope_dropdown, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    section = lv_menu_section_create(root_page);
    cont = menu_create_text(section, NULL, "电源", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, power_page);
    
    lv_obj_send_event(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED, NULL);
    
    lvgl_setting.possword_textarea = lv_textarea_create(lvgl_setting.main_obj);
    lv_obj_set_size(lvgl_setting.possword_textarea, lv_pct(50), LV_SIZE_CONTENT);
    lv_obj_set_align(lvgl_setting.possword_textarea, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_font(lvgl_setting.possword_textarea, &lv_font_fzst_24, 0);
    lv_textarea_set_one_line(lvgl_setting.possword_textarea, true);
    lv_textarea_set_max_length(lvgl_setting.possword_textarea, 32);
    lv_textarea_set_placeholder_text(lvgl_setting.possword_textarea, "possword");
    lv_obj_add_event_cb(lvgl_setting.possword_textarea, textarea_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(lvgl_setting.possword_textarea, LV_OBJ_FLAG_HIDDEN);
    
    lvgl_setting.timer = lv_timer_create(timer_cb, 1000, NULL);
    
    main_menu_page_flag |= 0x02;
}

/**************************************************************
函数名称 ： menu_create_text
功    能 ： 在菜单创建文本
参    数 ： parent: 父母, icon: 图标, txt: 文本, 
            builder_variant: 是否换行
返 回 值 ： 容器指针
作    者 ： ZeHou
**************************************************************/
static lv_obj_t *menu_create_text(lv_obj_t *parent, const char *icon, const char *txt, lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t *obj = lv_menu_cont_create(parent);

    lv_obj_t *img = NULL;
    lv_obj_t *label = NULL;

    if(icon)
    {
        img = lv_image_create(obj);
        lv_image_set_src(img, icon);
    }

    if(txt)
    {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt)
    {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

/**************************************************************
函数名称 ： menu_create_slider
功    能 ： 在菜单创建滑动条
参    数 ： parent: 父母, icon: 图标, txt: 文本, 
            min: 最小值, max: 最大值, val: 值
返 回 值 ： 容器指针
作    者 ： ZeHou
**************************************************************/
static lv_obj_t *menu_create_slider(lv_obj_t *parent, const char *icon, const char *txt, int32_t min, int32_t max, int32_t val)
{
    lv_obj_t *obj = menu_create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t *slider = lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    if(icon == NULL)
    {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }
    
    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text_fmt(label, "%u%%", val);

    return obj;
}

/**************************************************************
函数名称 ： menu_create_switch
功    能 ： 在菜单创建开关
参    数 ： parent: 父母, icon: 图标, txt: 文本, 
            chk: 开关状态
返 回 值 ： 容器指针
作    者 ： ZeHou
**************************************************************/
static lv_obj_t *menu_create_switch(lv_obj_t *parent, const char *icon, const char *txt, bool chk)
{
    lv_obj_t *obj = menu_create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t *sw = lv_switch_create(obj);
    lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

    return obj;
}

/**************************************************************
函数名称 ： menu_create_btn
功    能 ： 在菜单创建按钮
参    数 ： parent: 父母, icon: 图标, txt: 文本, 
            btn_txt: 按钮文本
返 回 值 ： 容器指针
作    者 ： ZeHou
**************************************************************/
static lv_obj_t *menu_create_btn(lv_obj_t *parent, const char *icon, const char *txt, const char *btn_txt)
{
    lv_obj_t *obj = menu_create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t *btn = lv_button_create(obj);
    lv_obj_set_style_pad_all(btn, 5, 0);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, btn_txt);
    lv_obj_center(label);

    return obj;
}

/**************************************************************
函数名称 ： menu_create_dropdown
功    能 ： 在菜单创建下拉列表
参    数 ： parent: 父母, icon: 图标, txt: 文本, 
            option: 列表选项, option_id: 默认选项
返 回 值 ： 容器指针
作    者 ： ZeHou
**************************************************************/
static lv_obj_t *menu_create_dropdown(lv_obj_t *parent, const char *icon, const char *txt, const char *option, uint8_t option_id)
{
    lv_obj_t *obj = menu_create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t *dropdown = lv_dropdown_create(obj);
    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    lv_obj_set_style_text_font(list, lv_obj_get_style_text_font(obj, 0), 0);
    lv_dropdown_set_options(dropdown, option);
    lv_dropdown_set_selected(dropdown, option_id);
    lv_obj_set_flex_grow(dropdown, 1);

    return obj;
}

/**************************************************************
函数名称 ： lvgl_setting_delete
功    能 ： 删除设置
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_setting_delete(void)
{
    if(lvgl_main.all_keyboard != NULL)
    {
        lvgl_all_keyboard_delete();
    }
    
    lv_timer_delete(lvgl_setting.timer);
    if(lvgl_setting.wifi_scan_list != NULL)
    {
        lvgl_setting_scan_wifi_list_delete();
    }
    lv_obj_delete(lvgl_setting.main_obj);
    lvgl_setting.main_obj = NULL;
    main_menu_page_flag &= ~0x02;
    if(main_menu_page_flag == 0)
    {
        lvgl_show_main_menu();
    }
}

/**************************************************************
函数名称 ： listbtn_event_handler
功    能 ： 列表事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void listbtn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const char *temp;
    uint8_t copy_length = 0;

    switch(code)
    {
        case LV_EVENT_CLICKED:
            temp = lv_list_get_button_text(lvgl_setting.wifi_scan_list, obj);
            copy_length = strlen(temp) + 1;
            memset(lvgl_setting_wireless_connect.wifi_name, 0x00, sizeof(lvgl_setting_wireless_connect.wifi_name));
            memcpy(lvgl_setting_wireless_connect.wifi_name, temp, copy_length > 32 ? 32 : copy_length);
            lv_obj_remove_flag(lvgl_setting.possword_textarea, LV_OBJ_FLAG_HIDDEN);
            lv_obj_send_event(lvgl_setting.possword_textarea, LV_EVENT_CLICKED, NULL);
            break;
        
        default: break;       
    }
}

/**************************************************************
函数名称 ： lvgl_setting_scan_wifi_list_creat
功    能 ： 创建WIFI扫描列表
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_setting_scan_wifi_list_creat(void)
{
    lv_obj_t * listbtn;
    
    if(lvgl_setting.wifi_scan_list != NULL)
    {
        lvgl_setting_scan_wifi_list_delete();
    }
    
    lvgl_setting.wifi_scan_list = lv_list_create(lvgl_setting.wifi_scan_cont);
    lv_obj_set_style_pad_row(lvgl_setting.wifi_scan_list, 0, 0);
    lv_obj_set_flex_grow(lvgl_setting.wifi_scan_list, 1);
    lv_obj_add_flag(lvgl_setting.wifi_scan_list, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    
    for(uint8_t i = 0; i < lvgl_setting_wireless_scan_info.wifi_num; i++)
    {
        listbtn = lv_list_add_button(lvgl_setting.wifi_scan_list, LV_SYMBOL_WIFI, lvgl_setting_wireless_scan_info.wifi_name[i]);
        lv_obj_add_event_cb(listbtn, listbtn_event_handler, LV_EVENT_CLICKED, NULL);
    }
    lvgl_setting_wireless_scan_info.wifi_num = 0;
}

/**************************************************************
函数名称 ： lvgl_setting_scan_wifi_list_delete
功    能 ： 删除WIFI扫描列表
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_setting_scan_wifi_list_delete(void)
{
    lv_obj_delete(lvgl_setting.wifi_scan_list);
    lvgl_setting.wifi_scan_list = NULL;
}