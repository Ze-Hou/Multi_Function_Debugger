#include "system_config.h"
#include "ff.h"
#include "./MALLOC/malloc.h"
#include "gd32h7xx_libopt.h"
#include "./WIRELESS/wireless.h"
#include "./SC8721/sc8721.h"
#include "./SCREEN/RGBLCD/rgblcd.h"
#include "lvgl_setting.h"
#include "lvgl_usart.h"
#include "lvgl_can.h"
#include <string.h>
#include <stdlib.h>

static const char *const CONFIG_PARAMETER_TABLE_PATH = "C:/SYSTEM/system_config.cfg";

static const char *const CONFIG_PARAMETER_TABLE[] = {
    "WIRELESSBAUDRATE",
    "#WIFI",
    "#BLUETOOTH",
    "#PD",
    "#SC8721FRE",
    "#SC8721SLOPE",
    "#BRIGHTNESS",
    "#USART",
    "#SENDADD",
    "#BAUDRATE",
    "#STOPBIT",
    "#DATABIT",
    "#CHECKBIT",
    "#CANMODE",
};

void system_parameter_init_from_cfg_file(void)
{
    uint16_t i;
    uint32_t file_size, read_bytes;
    FRESULT fresult;
    FIL *file;
    uint8_t *data_buffer;
    char *data_temp[sizeof(CONFIG_PARAMETER_TABLE) / sizeof(char *)];
    char *temp;
    
    file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    if((file == NULL))
    {
        return;
    }
    
    fresult = f_open(file, CONFIG_PARAMETER_TABLE_PATH, FA_READ);
    if(fresult == FR_OK)
    {
        file_size = f_size(file);
    }
    
    data_buffer = (uint8_t *)mymalloc(SRAMIN, file_size);
    if(data_buffer == NULL)
    {
        return;
    }
    
    fresult = f_read(file, data_buffer, file_size, &read_bytes);
    if((fresult == FR_OK) && (read_bytes == file_size))
    {
        for(i = 0; i < sizeof(CONFIG_PARAMETER_TABLE) / sizeof(char *); i++)
        {
            data_temp[i] = strstr((const char *)data_buffer, CONFIG_PARAMETER_TABLE[i]);
        }
        
        /* 将回车换行符替换位空字符 */
        for(i = 0; i < read_bytes - 1; i++)
        {
            if((data_buffer[i] == '\r') && (data_buffer[i + 1] == '\n'))
            {
                data_buffer[i] = '\0';
                data_buffer[i + 1] = '\0';
            }
        }
        
        for(uint16_t i = 0; i < sizeof(CONFIG_PARAMETER_TABLE) / sizeof(char *); i++)
        {
            temp = strchr(data_temp[i], ':');
            switch(i)
            {
                /* #WIRELESSBAUDRATE */
                case 0:
                    wireless_baudrate = atoi(temp + 1);
                    if(wireless_baudrate)
                    {
                        usart_disable(UART4); 
                        usart_baudrate_set(UART4, wireless_baudrate);
                        usart_enable(UART4);
                        uint32_t wireless_baud_rate_temp = wireless_get_baud_rate();

                        if((wireless_baudrate == 115200  || \
                            wireless_baudrate == 921600) && wireless_baud_rate_temp == 0)
                        {
                            usart_disable(UART4); 
                            if(wireless_baudrate == 115200) usart_baudrate_set(UART4, 921600);
                            if(wireless_baudrate == 921600) usart_baudrate_set(UART4, 115200);
                            usart_enable(UART4);
                            
                            wireless_baud_rate_temp = wireless_get_baud_rate();
                        }
                        
                        if(wireless_baud_rate_temp && wireless_baud_rate_temp != wireless_baudrate)
                        {
                            wireless_set_baud_rate(wireless_baudrate);
                            usart_disable(UART4); 
                            usart_baudrate_set(UART4, wireless_baudrate);
                            usart_enable(UART4); 
                            
                        }
                    }
                    break;
                    
                /* #WIFI */
                case 1:
                    if(strcmp(temp + 1, "TRUE") == 0)
                    {
                        lvgl_setting_state.wifi = 1;
                    }
                    else
                    {
                        lvgl_setting_state.wifi = 0;
                    }
                    break;
                    
                /* #BLUETOOTH */
                case 2:
                    if(strcmp(temp + 1, "TRUE") == 0)
                    {
                        lvgl_setting_state.buletooth = 1;
                    }
                    else
                    {
                        lvgl_setting_state.buletooth = 0;
                    }
                    break;
                
                /* #PD */
                case 3:
                    lvgl_setting_state.pd_voltage = atoi(temp + 1) + 1;
                    if(lvgl_setting_state.pd_voltage > 3)
                    {
                        lvgl_setting_state.pd_voltage = 2;
                    }
                    break;
                    
                /* #SC8721FRE */
                case 4:
                    lvgl_setting_state.sc8721_frequency = atoi(temp + 1);
                    if(lvgl_setting_state.sc8721_frequency > 3)
                    {
                        lvgl_setting_state.sc8721_frequency = 3;
                    }
                    sc8721_frequency_set(lvgl_setting_state.sc8721_frequency);
                    break;
                    
                /* #SC8721SLOPE */
                case 5:
                    lvgl_setting_state.sc8721_slope = atoi(temp + 1);
                    if(lvgl_setting_state.sc8721_slope > 3)
                    {
                        lvgl_setting_state.sc8721_slope = 0;
                    }
                    sc8721_slope_comp_set(lvgl_setting_state.sc8721_slope);
                    break;
                
                /* #BRIGHTNESS */
                case 6:
                    lvgl_setting_state.brightness = atoi(temp + 1);
                    if((lvgl_setting_state.brightness == 0) || (lvgl_setting_state.brightness > 100))
                    {
                        lvgl_setting_state.brightness = 10;
                    }
                    rgblcd_blk_set(lvgl_setting_state.brightness);
                    break;
                
                /* #USART */
                case 7:
                    lvgl_usart_state.usart = atoi(temp + 1);
                    if(lvgl_usart_state.usart > 1)
                    {
                        lvgl_usart_state.usart = 0;
                    }
                    break;
                
                /* #SENDADD */
                case 8:
                    lvgl_usart_state.send_add = atoi(temp + 1);
                    if(lvgl_usart_state.send_add > 3)
                    {
                        lvgl_usart_state.send_add = 0;
                    }
                    break;
                
                /* #BAUDRATE */
                case 9:
                    lvgl_usart_state.baud_rate = atoi(temp + 1);
                    if(lvgl_usart_state.baud_rate > 9)
                    {
                        lvgl_usart_state.baud_rate = 3;
                    }
                    break;
                
                /* #STOPBIT */
                case 10:
                    lvgl_usart_state.stop_bit = atoi(temp + 1);
                    if(lvgl_usart_state.stop_bit > 3)
                    {
                        lvgl_usart_state.stop_bit = 1;
                    }
                    break;
                
                /* #DATABIT */
                case 11:
                    lvgl_usart_state.data_bit = atoi(temp + 1);
                    if(lvgl_usart_state.data_bit > 3)
                    {
                        lvgl_usart_state.data_bit = 1;
                    }
                    break;
                
                /* #CHECKBIT */
                case 12:
                    lvgl_usart_state.check_bit = atoi(temp + 1);
                    if(lvgl_usart_state.check_bit > 2)
                    {
                        lvgl_usart_state.check_bit = 0;
                    }
                    break;
                    
                /* #CANMODE */
                case 13:
                    lvgl_can_state.mode = atoi(temp + 1);
                    if(lvgl_can_state.mode > 1)
                    {
                        lvgl_can_state.mode = 0;
                    }
                    break;
                
                default: break;
            }
        }
    }
    f_close(file);
    myfree(SRAMIN, file);
    myfree(SRAMIN, data_buffer);
    
    switch(lvgl_usart_state.usart)
    {
        case 0:
            usart_disable(USART1);
            switch(lvgl_usart_state.baud_rate)
            {
                case 0:
                    usart_baudrate_set(USART1, 9600);
                    break;
                    
                case 1:
                    usart_baudrate_set(USART1, 19200);
                    break;
                
                case 2:
                    usart_baudrate_set(USART1, 56000);
                    break;
                
                case 3:
                    usart_baudrate_set(USART1, 115200);
                    break;
                
                case 4:
                    usart_baudrate_set(USART1, 230400);
                    break;
                
                case 5:
                    usart_baudrate_set(USART1, 460800);
                    break;
                
                case 6:
                    usart_baudrate_set(USART1, 512000);
                    break;
                
                case 7:
                    usart_baudrate_set(USART1, 750000);
                    break;
                
                case 8:
                    usart_baudrate_set(USART1, 921600);
                    break;
                
                case 9:
                    usart_baudrate_set(USART1, 1000000);
                    break;
                
                default: break;
            }
            
            switch(lvgl_usart_state.stop_bit)
            {
                case 0:
                    usart_stop_bit_set(USART1, USART_STB_0_5BIT);
                    break;
                    
                case 1:
                    usart_stop_bit_set(USART1, USART_STB_1BIT);
                    break;
                
                case 2:
                    usart_stop_bit_set(USART1, USART_STB_1_5BIT);
                    break;
                
                case 3:
                    usart_stop_bit_set(USART1, USART_STB_2BIT);
                    break;
                
                default: break;
            }
            
            switch(lvgl_usart_state.data_bit)
            {
                case 0:
                    usart_stop_bit_set(USART1, USART_WL_7BIT);
                    break;
                    
                case 1:
                    usart_stop_bit_set(USART1, USART_WL_8BIT);
                    break;
                
                case 2:
                    usart_stop_bit_set(USART1, USART_WL_9BIT);
                    break;
                
                case 3:
                    usart_stop_bit_set(USART1, USART_WL_10BIT);
                    break;
                
                default: break;
            }
            
            switch(lvgl_usart_state.check_bit)
            {
                case 0:
                    usart_parity_config(USART1, USART_PM_NONE);
                    break;
                    
                case 1:
                    usart_parity_config(USART1, USART_PM_ODD);
                    break;
                
                case 2:
                    usart_parity_config(USART1, USART_PM_EVEN);
                    break;
                
                default: break;
            }
            
            usart_enable(USART1);
            break;
        
        case 1:
            /* 不对wireless的串口做改动 */
            break;
        
        default: break;
    }
    
    switch(lvgl_can_state.mode)
    {
        case 0:
            can_operation_mode_enter(CAN1, CAN_LOOPBACK_SILENT_MODE);
            break;
        
        case 1:
            can_operation_mode_enter(CAN1, CAN_NORMAL_MODE);
            break;
        
        default: break;
    }
}
