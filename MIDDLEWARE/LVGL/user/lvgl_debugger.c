#include "lvgl_debugger.h"
#include "freertos_main.h"
#include "lvgl_main.h"
#include "gd32h7xx_timer.h"
#include "./USART/usart.h"
#include "./MALLOC/malloc.h"

extern void reset_dap_link_state(void);
extern volatile uint8_t gDebuggerOnLineIdleFlag;

#define BUFFER_TEXTAREA_SIZE (8 * 1024)

lvgl_debugger_struct lvgl_debugger;
static lvgl_debugger_download_struct lvgl_debugger_download;

volatile static uint8_t connect_status = 0;
volatile static uint32_t read_address = 0;
volatile static uint16_t read_size = 0;
static char download_file_path[FF_LFN_BUF + 1];
volatile uint32_t download_file_size = 0;
volatile static uint32_t download_count = 0;
volatile static float download_time = 0.0;
volatile static uint16_t erase_count_check = 0;
volatile static uint16_t download_count_check = 0;
volatile static uint16_t verify_count_check = 0;

/* static function declarations */
static void lvgl_flm_select_msgbox_creat(void);
static void flm_file_select(lv_obj_t *parent);
static void bin_file_select(lv_obj_t *parent);
static uint32_t get_debugger_bin_file_size(void);

/**************************************************************
函数名称 ： flm_prase_callback
功    能 ： 用于释放存储解析信息的内存
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void flm_prase_callback(void)
{
    myfree(SRAMIN, flash_algo.algo_blob);
    myfree(SRAMIN, flash_device.sectors);
}

/**************************************************************
函数名称 ： timer_cb
功    能 ： 定时器回调
参    数 ： timer
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void timer_cb(lv_timer_t *timer)
{
    if(xQueueReceive(xQueueDebuggerDownload, &lvgl_debugger_download, 0) == pdTRUE)
    {
        switch(lvgl_debugger_download.status)
        {
            case 0:
                switch(lvgl_debugger_download.error)
                {
                    case 1:
                        break;
                    
                    case 2:
                        lv_label_set_text(lvgl_debugger.download_update_label, "E ERS.");
                        break;
                    
                    case 3:
                        lv_label_set_text(lvgl_debugger.download_update_label, "E DWN.");
                        break;
                    
                    case 4:
                        lv_label_set_text(lvgl_debugger.download_update_label, "E VFY.");
                        break;
                    
                    default: break;
                }
                if(lvgl_debugger_download.error)    /* 只要有错误发送就终止当前下载 */
                {
                    lv_led_off(lvgl_debugger.download_led);
                    lv_timer_delete(lvgl_debugger.timer);
                    lv_obj_add_flag(lvgl_debugger.download_btn, LV_OBJ_FLAG_CLICKABLE);
                }
                break;
                
            case 1:
                /* 后台下载任务应答成功 */
                lv_led_on(lvgl_debugger.download_led);
                lv_label_set_text(lvgl_debugger.download_update_label, "start");
                xTaskNotify((TaskHandle_t)DBUGGER_DOWNLOADTask_Handler, (uint32_t)2, (eNotifyAction)eSetValueWithOverwrite); /* 通知任务进入状态二 */
                timer_counter_value_config(TIMER51, 0);
                timer_enable(TIMER51);
                break;
                
            case 2:
                lv_label_set_text_fmt(lvgl_debugger.download_update_label, "E %.1f%%", ((float)lvgl_debugger_download.run_count / erase_count_check * 100));
                if(lvgl_debugger_download.run_count == erase_count_check)
                {
                    xTaskNotify((TaskHandle_t)DBUGGER_DOWNLOADTask_Handler, (uint32_t)3, (eNotifyAction)eSetValueWithOverwrite); /* 通知任务进入状态三 */
                }
                break;
                
            case 3:
                lv_label_set_text_fmt(lvgl_debugger.download_update_label, "D %.1f%%", ((float)lvgl_debugger_download.run_count / download_count_check * 100));
                if(lvgl_debugger_download.run_count == download_count_check)
                {
                   xTaskNotify((TaskHandle_t)DBUGGER_DOWNLOADTask_Handler, (uint32_t)4, (eNotifyAction)eSetValueWithOverwrite); /* 通知任务进入状态三 */ 
                }
                break;
                
            case 4:
                lv_label_set_text_fmt(lvgl_debugger.download_update_label, "V %.1f%%", ((float)lvgl_debugger_download.run_count / verify_count_check * 100));
                if(lvgl_debugger_download.run_count == verify_count_check)
                {
                    timer_disable(TIMER51);
                    download_time = (float)timer_counter_read(TIMER51) / 10;
                    download_count++;
                    lv_label_set_text_fmt(lvgl_debugger.download_label, "%uc, %.1fms", download_count, download_time);
                    lv_led_off(lvgl_debugger.download_led);
                    lv_timer_delete(lvgl_debugger.timer);
                    lv_obj_add_flag(lvgl_debugger.download_btn, LV_OBJ_FLAG_CLICKABLE);
                }
                break;
                
            default: break;
        }
    }
}

/**************************************************************
函数名称 ： msgbox_close_btn_event_cb
功    能 ： 消息框关闭按钮事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void msgbox_close_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            lv_msgbox_close(lvgl_debugger.msgbox);
            break;
        
        default: break;
    }
    
}

/**************************************************************
函数名称 ： msgbox_ack_btn_event_cb
功    能 ： 消息框确认按钮事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void msgbox_ack_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            lv_msgbox_close(lvgl_debugger.msgbox);
            flm_prase_callback();
            flm_file_select(lvgl_debugger.main_obj);
            break;
        
        default: break;
    }
    
}

/**************************************************************
函数名称 ： closebtn_event_handler
功    能 ： 关闭按钮事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void closebtn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            lvgl_debugger_off_line_delete();
            flm_prase_callback();
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： flm_file_select_event_handler
功    能 ： FLM文件选择按钮事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void flm_file_select_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            lvgl_flm_select_msgbox_creat();
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
    uint8_t buffer_len = 0;
    uint8_t *buffer_temp;
    uint8_t *buffer_read;
    char *buffer_textarea;
    uint16_t i = 0;
    uint32_t debugger_id = 0;
    error_t error_code = 0;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(obj == lvgl_debugger.connect_btn)
            {
                if(connect_status == 0)
                {
                    if(gDebuggerOnLineIdleFlag == 0)
                    {
                        if(strcmp(lv_label_get_text(lvgl_debugger.flm_file_label), "NULL") != 0)
                        {
                            error_code = target_flash_init(flash_device.devAdr);
                            if(error_code == ERROR_SUCCESS)
                            {
                                lv_obj_set_style_bg_color(lvgl_debugger.connect_btn, lv_palette_main(LV_PALETTE_RED), 0);
                                lv_label_set_text(lvgl_debugger.connect_label, "断开");
                                if(swd_read_idcode(&debugger_id))
                                {
                                    lv_label_set_text_fmt(lvgl_debugger.id_label, "DBG ID: 0x%08X", debugger_id);
                                }
                                connect_status = 0x01;
                            }
                            else
                            {
                                lv_label_set_text_fmt(lvgl_debugger.connect_label, "重新连接(%d)", error_code);
                            }
                        }
                        else
                        {
                            lvgl_show_error_msgbox_creat("请先选择相应的算法文件！");
                        }
                    }
                    else
                    {
                        lvgl_show_error_msgbox_creat("在线调试器与离线调试器不能同时使用！");
                    }
                }
                else if(connect_status == 0x01)
                {
                    error_code = target_flash_uninit();
                    if(error_code == ERROR_SUCCESS)
                    {
                        lv_obj_set_style_bg_color(lvgl_debugger.connect_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
                        lv_label_set_text(lvgl_debugger.connect_label, "连接");
                        lv_label_set_text(lvgl_debugger.id_label, "DBG ID: NULL");
                        connect_status = 0x00;
                    }
                    else
                    {
                        lv_label_set_text_fmt(lvgl_debugger.connect_label, "重新断开(%d)", error_code);
                    }
                }
            }
            else if(obj == lvgl_debugger.read_btn)
            {
                if(connect_status == 0x01)
                {
                    read_address = atoi(lv_textarea_get_text(lvgl_debugger.address_textarea));
                    read_size = atoi(lv_textarea_get_text(lvgl_debugger.size_textarea));
                    if(((read_address + read_size) <= flash_device.szDev) && (read_size <= 1024) && read_size)
                    {
                        buffer_read = (uint8_t *)mymalloc(SRAMDTCM, read_size);
                        buffer_textarea = (char *)mymalloc(SRAMDTCM, BUFFER_TEXTAREA_SIZE);
                        if((buffer_read == NULL) || (buffer_textarea == NULL))
                        {
                            myfree(SRAMDTCM, buffer_read);
                            myfree(SRAMDTCM, (void *)buffer_textarea);
                            return;
                        }
                        memset((void *)buffer_textarea, 0x00, BUFFER_TEXTAREA_SIZE);
                        swd_read_memory(flash_device.devAdr + read_address, buffer_read, read_size);
                        for(i = 0; i < (read_size / 8); i++)
                        {
                            buffer_len = snprintf(NULL, 0, "00000000->  %02X %02X %02X %02X %02X %02X %02X %02X\n", \
                                                                      buffer_read[i * 8], buffer_read[i * 8 + 1], buffer_read[i * 8 + 2], buffer_read[i * 8 + 3], \
                                                                      buffer_read[i * 8 + 4], buffer_read[i * 8 + 5], buffer_read[i * 8 + 6], buffer_read[i * 8 + 7]) + 1;

                            buffer_temp = (uint8_t *)mymalloc(SRAMDTCM, buffer_len);
                            if(buffer_temp != NULL)
                            {
                                snprintf((char *)buffer_temp, buffer_len, "%08d->  %02X %02X %02X %02X %02X %02X %02X %02X\n", i * 8 + read_address, \
                                                                      buffer_read[i * 8], buffer_read[i * 8 + 1], buffer_read[i * 8 + 2], buffer_read[i * 8 + 3], \
                                                                      buffer_read[i * 8 + 4], buffer_read[i * 8 + 5], buffer_read[i * 8 + 6], buffer_read[i * 8 + 7]);
                                strncat((char *)buffer_textarea, (const char *)buffer_temp, BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
                                myfree(SRAMDTCM, buffer_temp);
                            }
                        }
                        if(read_size % 8)
                        {
                            buffer_len = snprintf(NULL, 0, "00000000->  ") + 1;
                            buffer_temp = (uint8_t *)mymalloc(SRAMDTCM, buffer_len);
                            if(buffer_temp != NULL)
                            {
                                snprintf((char *)buffer_temp, buffer_len, "%08d->  ", i * 8  + read_address);
                                strncat((char *)buffer_textarea, (const char *)buffer_temp, BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
                                buffer_len = snprintf(NULL, 0, "%02X ", buffer_read[i * 8]) + 1;
                                for(i = i * 8 ; i < read_size - 1; i++)
                                {
                                    snprintf((char *)buffer_temp, buffer_len, "%02X ", buffer_read[i]);
                                    strncat((char *)buffer_textarea, (const char *)buffer_temp, BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
                                }
                                snprintf((char *)buffer_temp, buffer_len, "%02X", buffer_read[i]);
                                strncat((char *)buffer_textarea, (const char *)buffer_temp, BUFFER_TEXTAREA_SIZE - strlen(buffer_textarea));
                                myfree(SRAMDTCM, buffer_temp);
                            }
                        }
                        lv_textarea_set_text(lvgl_debugger.textarea, (const char *)buffer_textarea);
                        myfree(SRAMDTCM, buffer_read);
                        myfree(SRAMDTCM, buffer_textarea);
                    }
                    else
                    {
                        lvgl_show_error_msgbox_creat("读地址或者读大小错误，请重新设置！");
                    }
                }
            }
            else if(obj == lvgl_debugger.download_btn)
            {
                if(connect_status == 0x01)
                {
                    memset(download_file_path, 0x00, FF_LFN_BUF + 1);
                    memcpy(download_file_path, lv_label_get_text(lvgl_debugger.download_bin_path_label), strlen(lv_label_get_text(lvgl_debugger.download_bin_path_label)) + 1);
                    if(download_file_path[0] != '\0')
                    {
                        download_file_size = get_debugger_bin_file_size();
                        erase_count_check = (download_file_size % flash_device.sectors[0].szSector) ? \
                                            ((download_file_size / flash_device.sectors[0].szSector) + 1) : (download_file_size / flash_device.sectors[0].szSector);
                        download_count_check = (download_file_size % flash_device.szPage) ? \
                                               ((download_file_size / flash_device.szPage) + 1) : (download_file_size / flash_device.szPage);
                        verify_count_check = download_count_check;
                        if((download_file_size <= flash_device.szDev) && download_file_size)
                        {
                            lv_obj_remove_flag(lvgl_debugger.download_btn, LV_OBJ_FLAG_CLICKABLE);
                            lvgl_debugger.timer = lv_timer_create(timer_cb, 100, NULL);
                            if(DBUGGER_DOWNLOADTask_Handler != NULL)
                            {
                                xTaskNotify((TaskHandle_t)DBUGGER_DOWNLOADTask_Handler, (uint32_t)1, (eNotifyAction)eSetValueWithOverwrite); /* 通知任务进入状态一 */
                            }
                        }
                        else
                        {
                            lvgl_show_error_msgbox_creat("下载出错，请检查bin文件！");
                        }
                    }
                }
                else
                {
                    lvgl_show_error_msgbox_creat("请先连接目标设备！");
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
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            if(lvgl_main.num_keyboard == NULL)
            {
                lvgl_num_keyboard_create(lvgl_debugger.main_obj);
            }
            
            if(textarea == lvgl_debugger.address_textarea)
            {
                lv_keyboard_set_textarea(lvgl_main.num_keyboard, lvgl_debugger.address_textarea);
            }
            else if(textarea == lvgl_debugger.size_textarea)
            {
                lv_keyboard_set_textarea(lvgl_main.num_keyboard, lvgl_debugger.size_textarea);
            }
            break;
            
        case LV_EVENT_READY:
            if(textarea == lvgl_debugger.address_textarea)
            {
                read_address = atoi(lv_textarea_get_text(textarea));
            }
            else if(textarea == lvgl_debugger.size_textarea)
            {
                read_size = atoi(lv_textarea_get_text(textarea));
            }
            break;
            
        case LV_EVENT_CANCEL:
            lvgl_num_keyboard_delete();
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： bin_file_select_event_handler
功    能 ： BIN文件选择按钮事件回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void bin_file_select_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            bin_file_select(lvgl_debugger.main_obj);
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： get_debugger_bin_file_size
功    能 ： 获取bin文件大小
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static uint32_t get_debugger_bin_file_size(void)
{
    uint32_t file_size;
    FRESULT fresult;
    FIL *file;
    
    file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    if((file == NULL))
    {
        return 0;
    }
    
    fresult = f_open(file, download_file_path, FA_READ);
    if(fresult == FR_OK)
    {
        file_size = f_size(file);
    }

    f_close(file);
    myfree(SRAMIN, file);
    
    return file_size;
}

/**************************************************************
函数名称 ： debugger_read_bin_file
功    能 ： 调试器读取bin文件数据
参    数 ： offse: 偏移, buf: 数据缓冲区
            size: 预期读大小, read_bytes: 实际读大小
返 回 值 ： 读取结果
作    者 ： ZeHou
**************************************************************/
int debugger_read_bin_file(uint32_t offset, void* buf, uint32_t size, uint32_t *read_bytes)
{
    FRESULT fresult;
    FIL *file;
    
    file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    if(file == NULL)return -1;
    
    fresult = f_open(file, download_file_path, FA_READ);
    if(fresult != FR_OK)
    {
        goto __exit;
    }
    fresult = f_lseek(file, offset);
    if(fresult != FR_OK)
    {
        goto __exit;
    }
    fresult = f_read(file, buf, size, read_bytes);
    if(fresult != FR_OK)
    {
        goto __exit;
    }
    
__exit:
    f_close(file);
    myfree(SRAMIN, file);
    return fresult;
}

/**************************************************************
函数名称 ： lvgl_flm_select_msgbox_creat
功    能 ： 创建FLM文件选取消息框
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_flm_select_msgbox_creat(void)
{
    lv_obj_t *btn1, *btn2;
    
    lvgl_debugger.msgbox = lv_msgbox_create(NULL);
    lv_obj_set_style_text_font(lvgl_debugger.msgbox, &lv_font_fzst_24, 0);
    lv_msgbox_add_title(lvgl_debugger.msgbox, "FLM算法选择");
    lv_msgbox_add_text(lvgl_debugger.msgbox, "要进行离线调试，请先选择相应的算法文件");
    
    btn1 = lv_msgbox_add_header_button(lvgl_debugger.msgbox, LV_SYMBOL_CLOSE);
    lv_obj_add_event_cb(btn1, msgbox_close_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    btn2 = lv_msgbox_add_footer_button(lvgl_debugger.msgbox, "确定");
    lv_obj_add_event_cb(btn2, msgbox_ack_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

/**************************************************************
函数名称 ： flm_file_select
功    能 ： FLM文件选取
参    数 ： parent: 父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void flm_file_select(lv_obj_t *parent)
{
    lvgl_file_manager.file_selector = FILE_SELECTOR_FLM;    /* 需要选择FLM下载算法文件 */
    memcpy(lvgl_file_manager.file_ext, file_selector_file_ext[FILE_SELECTOR_FLM], strlen(file_selector_file_ext[FILE_SELECTOR_FLM]) + 1);   /* 文件扩展名 */
    lvgl_file_manager_creat(parent);            /* 创建文件管理器，并且进入文件选择模式 */
}

/**************************************************************
函数名称 ： bin_file_select
功    能 ： BIN文件选取
参    数 ： parent: 父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void bin_file_select(lv_obj_t *parent)
{
    lvgl_file_manager.file_selector = FILE_SELECTOR_BIN;    /* 需要选择BIN文件 */
    memcpy(lvgl_file_manager.file_ext, file_selector_file_ext[FILE_SELECTOR_BIN], strlen(file_selector_file_ext[FILE_SELECTOR_BIN]) + 1);   /* 文件扩展名 */
    lvgl_file_manager_creat(parent);            /* 创建文件管理器，并且进入文件选择模式 */
}

/**************************************************************
函数名称 ： lvgl_debugger_off_line_creat
功    能 ： 创建离线调试器
参    数 ： parent: 父母
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_debugger_off_line_creat(lv_obj_t *parent)
{
    lv_obj_t *obj1, *obj2, *obj3, *obj4, *obj5, *obj6;
    lv_obj_t *closebtn;
    lv_obj_t *label1, *label2, *label3, *titlelabel, *closebtnlabel;
    
    lvgl_debugger.main_obj = lv_obj_create(parent);                 /* 创建容器 */
    lv_obj_set_size(lvgl_debugger.main_obj, lv_pct(100), lv_pct(100));
    lv_obj_center(lvgl_debugger.main_obj);
    lv_obj_set_style_pad_top(lvgl_debugger.main_obj, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_debugger.main_obj, 5, 0);
    lv_obj_set_style_pad_left(lvgl_debugger.main_obj, 5, 0);
    lv_obj_set_style_pad_right(lvgl_debugger.main_obj, 5, 0);
    lv_obj_remove_flag(lvgl_debugger.main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(lvgl_debugger.main_obj, true, 0);
    
    obj1 = lv_obj_create(lvgl_debugger.main_obj);
    lv_obj_add_style(obj1, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj1, lv_pct(100), lv_pct(15));
    lv_obj_align(obj1, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(obj1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    label1 = lv_label_create(obj1);
    lv_label_set_text(label1, "FLM:");
    lv_obj_set_style_text_font(label1, &lv_font_fzst_24, 0);
    lv_obj_align(label1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(label1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label1, flm_file_select_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_debugger.flm_file_label = lv_label_create(obj1);
    lv_obj_set_size(lvgl_debugger.flm_file_label, lv_pct(30), LV_SIZE_CONTENT);
    lv_label_set_text(lvgl_debugger.flm_file_label, "NULL");
    lv_label_set_long_mode(lvgl_debugger.flm_file_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(lvgl_debugger.flm_file_label, &lv_font_fzst_24, 0);
    lv_obj_align_to(lvgl_debugger.flm_file_label, label1, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    
    titlelabel = lv_label_create(obj1);
    lv_label_set_text(titlelabel, "离线调试器");
    lv_obj_set_style_text_font(titlelabel, &lv_font_fzst_32, 0);
    lv_obj_center(titlelabel);
    
    closebtn = lv_button_create(obj1);                                              /* 创建关闭按钮 */
    lv_obj_add_event_cb(closebtn, closebtn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(closebtn, lv_pct(10), lv_pct(100));
    lv_obj_align(closebtn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_margin_all(closebtn, 5, 0);
    lv_obj_update_layout(closebtn);

    closebtnlabel = lv_label_create(closebtn);
    lv_label_set_text(closebtnlabel, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(closebtnlabel, &lv_font_fzst_32, 0);
    lv_obj_center(closebtnlabel);
    
    lvgl_debugger.id_label = lv_label_create(obj1);
    lv_label_set_text(lvgl_debugger.id_label, "DBG ID: NULL");
    lv_obj_set_style_text_font(lvgl_debugger.id_label, &lv_font_fzst_24, 0);
    lv_obj_align(lvgl_debugger.id_label, LV_ALIGN_RIGHT_MID, -(lv_obj_get_width(closebtn) + 5), 0);

    lvgl_debugger.textarea = lv_textarea_create(lvgl_debugger.main_obj);
    lv_obj_set_size(lvgl_debugger.textarea, lv_pct(75), lv_pct(85));
    lv_obj_set_style_text_font(lvgl_debugger.textarea, &lv_font_simsun_32, 0);
    lv_obj_align(lvgl_debugger.textarea, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_remove_state(lvgl_debugger.textarea, LV_STATE_FOCUSED);
    lv_obj_remove_flag(lvgl_debugger.textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    
    obj2 = lv_obj_create(lvgl_debugger.main_obj);
    lv_obj_add_style(obj2, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj2, lv_pct(25), lv_pct(45));
    lv_obj_align_to(obj2, obj1, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
    lv_obj_remove_flag(obj2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(obj2, 5, 0);
    lv_obj_set_flex_flow(obj2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj2, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lvgl_debugger.connect_btn = lv_button_create(obj2);
    lv_obj_set_size(lvgl_debugger.connect_btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_event_cb(lvgl_debugger.connect_btn, btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_top(lvgl_debugger.connect_btn, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_debugger.connect_btn, 5, 0);
    lv_obj_set_style_pad_left(lvgl_debugger.connect_btn, 5, 0);
    lv_obj_set_style_pad_right(lvgl_debugger.connect_btn, 5, 0);
    
    lvgl_debugger.connect_label = lv_label_create(lvgl_debugger.connect_btn);
    lv_label_set_text(lvgl_debugger.connect_label, "连接");
    lv_obj_set_style_text_font(lvgl_debugger.connect_label, &lv_font_fzst_24, 0);
    lv_obj_center(lvgl_debugger.connect_label);

    lvgl_debugger.address_textarea = lv_textarea_create(obj2);
    lv_obj_set_size(lvgl_debugger.address_textarea, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(lvgl_debugger.address_textarea, &lv_font_fzst_24, 0);
    lv_textarea_set_one_line(lvgl_debugger.address_textarea, true);
    lv_textarea_set_accepted_chars(lvgl_debugger.address_textarea, "0123456789");
    lv_textarea_set_placeholder_text(lvgl_debugger.address_textarea, "addr offset");
    lv_obj_add_event_cb(lvgl_debugger.address_textarea, textarea_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_pad_top(lvgl_debugger.address_textarea, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_debugger.address_textarea, 5, 0);
    lv_obj_set_style_pad_left(lvgl_debugger.address_textarea, 5, 0);
    lv_obj_set_style_pad_right(lvgl_debugger.address_textarea, 5, 0);
    
    lvgl_debugger.size_textarea = lv_textarea_create(obj2);
    lv_obj_set_size(lvgl_debugger.size_textarea, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(lvgl_debugger.size_textarea, &lv_font_fzst_24, 0);
    lv_textarea_set_one_line(lvgl_debugger.size_textarea, true);
    lv_textarea_set_accepted_chars(lvgl_debugger.size_textarea, "0123456789");
    lv_textarea_set_placeholder_text(lvgl_debugger.size_textarea, "rd size max(1024)");
    lv_obj_add_event_cb(lvgl_debugger.size_textarea, textarea_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_pad_top(lvgl_debugger.size_textarea, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_debugger.size_textarea, 5, 0);
    lv_obj_set_style_pad_left(lvgl_debugger.size_textarea, 5, 0);
    lv_obj_set_style_pad_right(lvgl_debugger.size_textarea, 5, 0);
    
    lvgl_debugger.read_btn = lv_button_create(obj2);
    lv_obj_set_size(lvgl_debugger.read_btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_event_cb(lvgl_debugger.read_btn, btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_top(lvgl_debugger.read_btn, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_debugger.read_btn, 5, 0);
    lv_obj_set_style_pad_left(lvgl_debugger.read_btn, 5, 0);
    lv_obj_set_style_pad_right(lvgl_debugger.read_btn, 5, 0);
    
    label2 = lv_label_create(lvgl_debugger.read_btn);
    lv_label_set_text(label2, "读取");
    lv_obj_set_style_text_font(label2, &lv_font_fzst_24, 0);
    lv_obj_center(label2);

    obj3 = lv_obj_create(lvgl_debugger.main_obj);
    lv_obj_add_style(obj3, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj3, lv_pct(25), lv_pct(40));
    lv_obj_align(obj3, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_remove_flag(obj3, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_row(obj3, 5, 0);
    lv_obj_set_flex_flow(obj3, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj3, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    obj4 = lv_obj_create(obj3);
    lv_obj_add_style(obj4, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj4, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align(obj4, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(obj4, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_debugger.download_led = lv_led_create(obj4);
    lv_obj_set_size(lvgl_debugger.download_led, 20, 20);
    lv_led_set_color(lvgl_debugger.download_led, lv_color_hex(0xFFB6C1));
    lv_obj_align(lvgl_debugger.download_led, LV_ALIGN_LEFT_MID, 10, 0);
    lv_led_off(lvgl_debugger.download_led);
    
    lvgl_debugger.download_update_label = lv_label_create(obj4);
    lv_label_set_text(lvgl_debugger.download_update_label, "NULL");
    lv_obj_set_style_text_font(lvgl_debugger.download_update_label, &lv_font_fzst_24, 0);
    lv_obj_align(lvgl_debugger.download_update_label, LV_ALIGN_RIGHT_MID, -10, 0);
    
    obj5 = lv_obj_create(obj3);
    lv_obj_add_style(obj5, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj5, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align_to(obj5, obj4, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_remove_flag(obj5, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    
    lvgl_debugger.download_label = lv_label_create(obj5);
    lv_label_set_text_fmt(lvgl_debugger.download_label, "%uc, %.1fms", download_count, download_time);
    lv_obj_set_style_text_font(lvgl_debugger.download_label, &lv_font_fzst_24, 0);
    lv_obj_align(lvgl_debugger.download_label, LV_ALIGN_CENTER, 0, 0);
    
    obj6 = lv_obj_create(obj3);
    lv_obj_add_style(obj6, &lvgl_style.general_obj, 0);
    lv_obj_set_size(obj6, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align_to(obj6, obj5, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_remove_flag(obj6, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(obj6, bin_file_select_event_handler, LV_EVENT_CLICKED, NULL);
    
    lvgl_debugger.download_bin_path_label = lv_label_create(obj6);
    lv_obj_set_size(lvgl_debugger.download_bin_path_label, lv_pct(100), LV_SIZE_CONTENT);
    lv_label_set_text(lvgl_debugger.download_bin_path_label, "bin file path");
    lv_label_set_long_mode(lvgl_debugger.download_bin_path_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(lvgl_debugger.download_bin_path_label, &lv_font_fzst_24, 0);
    lv_obj_align(lvgl_debugger.download_bin_path_label, LV_ALIGN_CENTER, 0, 0);
    
    lvgl_debugger.download_btn = lv_button_create(obj3);
    lv_obj_set_size(lvgl_debugger.download_btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_event_cb(lvgl_debugger.download_btn, btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_pad_top(lvgl_debugger.download_btn, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_debugger.download_btn, 5, 0);
    lv_obj_set_style_pad_left(lvgl_debugger.download_btn, 5, 0);
    lv_obj_set_style_pad_right(lvgl_debugger.download_btn, 5, 0);
    
    label3 = lv_label_create(lvgl_debugger.download_btn);
    lv_label_set_text(label3, "烧录");
    lv_obj_set_style_text_font(label3, &lv_font_fzst_24, 0);
    lv_obj_center(label3);
    
    lvgl_flm_select_msgbox_creat();
    main_menu_page_flag |= 0x04;
    vTaskSuspend(DAP_LINKTask_Handler);             /* 进入离线调试器后挂起在线调试器任务 */
}

/**************************************************************
函数名称 ： lvgl_flm_prase
功    能 ： 解析flm下载算法文件
            note: 该函数在lvgl_file_manager_select_file中回调
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_flm_prase(const char *fpath)
{
    if(flm_prase(fpath) != 0)   /* 解析失败 */
    {
        lv_label_set_text(lvgl_debugger.flm_file_label, "NULL");
        return;
    }
    lv_label_set_text(lvgl_debugger.flm_file_label, flash_device.devName);
}

/**************************************************************
函数名称 ： bin_file_select_callback
功    能 ： BIN文件选取回调
参    数 ： fpath: BIN文件路径
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void bin_file_select_callback(const char *fpath)
{
    lv_label_set_text(lvgl_debugger.download_bin_path_label, fpath);
}

/**************************************************************
函数名称 ： lvgl_debugger_off_line_delete
功    能 ： 删除离线调试器
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_debugger_off_line_delete(void)
{
    if(lvgl_main.num_keyboard != NULL)
    {
        lvgl_num_keyboard_delete();
    }
    lv_obj_delete(lvgl_debugger.main_obj);
    lvgl_debugger.main_obj = NULL;
    main_menu_page_flag &= ~0x04;
    if(connect_status == 0x01)
    {
        target_flash_uninit();
    }
    reset_dap_link_state();
    vTaskResume(DAP_LINKTask_Handler);                  /* 成功断开连接后恢复在线调试器任务 */
    connect_status = 0;
    if(main_menu_page_flag == 0)
    {
        lvgl_show_main_menu();
    }
}
