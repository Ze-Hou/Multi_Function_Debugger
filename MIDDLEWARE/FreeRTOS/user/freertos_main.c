/*!
    \file       freertos_main.c
    \brief      FreeRTOS main implementation file
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#include "freertos_main.h"
#include "cpu_utils.h"
#include "gd32h7xx_libopt.h"
#include "./USART/usart.h"
#include "./SYSTEM/system.h"
#include "./LED/led.h"
#include "./KEY/key.h"
#include "./INA226/ina226.h"
#include "./RTC/rtc.h"
#include "./WIRELESS/wireless.h"
#include "./ADC/adc.h"

#include "./MALLOC/malloc.h"

#include "./DAP/dap_main.h"

#include "lvgl_main.h"
#include "lvgl_setting.h"
#include "lvgl_debugger.h"
#include "lvgl_usart.h"

#include <string.h>

/******************************************************************************************************/
/* FreeRTOS task configurations */
/* Task definitions */
#define DAP_LINK_TASK_PRIO       4                          /* DAP link task priority */
#define DAP_LINK_STK_SIZE        1024                       /* DAP link task stack size */
TaskHandle_t DAP_LINKTask_Handler;                          /* DAP link task handle */
void dap_link_task(void *pvParameters);                     /* DAP link task function */

#define LVGL_TASK_PRIO          3                           /* LVGL task priority */
#define LVGL_STK_SIZE           2048                        /* LVGL task stack size */
TaskHandle_t LVGLTask_Handler;                              /* LVGL task handle */
void lvgl_task(void *pvParameters);                         /* LVGL task function */

#define OTHER_TASK_PRIO         2                           /* Other task priority */
#define OTHER_STK_SIZE          512                         /* Other task stack size */
TaskHandle_t OTHERTask_Handler;                             /* Other task handle */
void other_task(void *pvParameters);                        /* Other task function */

#define DBUGGER_DOWNLOAD_TASK_PRIO         1                /* Debugger download task priority */
#define DBUGGER_DOWNLOAD_STK_SIZE          512              /* Debugger download task stack size */
TaskHandle_t DBUGGER_DOWNLOADTask_Handler;                  /* Debugger download task handle */
void debugger_download_task(void *pvParameters);            /* Debugger download task function */

#define WIRELESS_TASK_PRIO         1                        /* Wireless task priority */
#define WIRELESS_STK_SIZE          512                      /* Wireless task stack size */
TaskHandle_t WIRELESSTask_Handler;                          /* Wireless task handle */
void wireless_task(void *pvParameters);                     /* Wireless task function */

/* Queue definitions */
QueueHandle_t xQueueIna226Data;                             /* INA226 data queue */
#define xQueueIna226DataLength  1
#define xQueueIna226DataSize    sizeof(ina226_data_struct)
    
QueueHandle_t xQueueCpuInfo;                                /* CPU info queue */
#define xQueueCpuInfoLength  1
#define xQueueCpuInfoSize    sizeof(lvgl_cpu_info_struct)
    
QueueHandle_t xQueueLvglMainState;                          /* LVGL main state queue */
#define xQueueLvglMainStateLength  1
#define xQueueLvglMainStateSize    sizeof(lvgl_main_state_struct)
    
QueueHandle_t xQueueRtcData;                                /* RTC data queue */
#define xQueueRtcDataLength  1
#define xQueueRtcDataSize    sizeof(rtc_data_struct)
    
QueueHandle_t xQueueMEMPerused;                             /* Memory usage queue */
#define xQueueMEMPerusedLength  1
#define xQueueMEMPerusedSize    sizeof(lvgl_main_mem_perused_struct)
 
QueueHandle_t xQueueDebuggerDownload;                       /* Debugger download queue */
#define xQueueDebuggerDownloadLength  1
#define xQueueDebuggerDownloadSize    sizeof(lvgl_debugger_download_struct)
    
QueueHandle_t xQueueWirelseeState;                          /* Wireless state queue */
#define xQueueWirelseeStateLength  1
#define xQueueWirelseeStateSize    sizeof(lvgl_setting_wireless_state_struct)

/* Semaphore definitions */    

/* static variable declarations */

/******************************************************************************************************/

/*!
    \brief      FreeRTOS main initialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void freertos_main(void)
{
    taskENTER_CRITICAL();                   /* Enter critical section */
    
    /* Create queues */
    xQueueIna226Data = xQueueCreate(xQueueIna226DataLength, xQueueIna226DataSize);
    xQueueCpuInfo = xQueueCreate(xQueueCpuInfoLength, xQueueCpuInfoSize);
    xQueueLvglMainState = xQueueCreate(xQueueLvglMainStateLength, xQueueLvglMainStateSize);
    xQueueRtcData = xQueueCreate(xQueueRtcDataLength, xQueueRtcDataSize);
    xQueueMEMPerused = xQueueCreate(xQueueMEMPerusedLength, xQueueMEMPerusedSize);
    xQueueDebuggerDownload = xQueueCreate(xQueueDebuggerDownloadLength, xQueueDebuggerDownloadSize);
    xQueueWirelseeState = xQueueCreate(xQueueWirelseeStateLength, xQueueWirelseeStateSize); 
    
    /* Create DAP link task */
    xTaskCreate((TaskFunction_t)dap_link_task,
                (const char*)"dap_link_task",
                (uint16_t)DAP_LINK_STK_SIZE,
                (void*)NULL,
                (UBaseType_t)DAP_LINK_TASK_PRIO,
                (TaskHandle_t*)&DAP_LINKTask_Handler);
                
    /* Create LVGL task */
    xTaskCreate((TaskFunction_t)lvgl_task,
                (const char*)"lvgl_task",
                (uint16_t)LVGL_STK_SIZE, 
                (void*)NULL,
                (UBaseType_t)LVGL_TASK_PRIO,
                (TaskHandle_t*)&LVGLTask_Handler);
                
    /* Create other task */
    xTaskCreate((TaskFunction_t)other_task,
                (const char*)"other_task",
                (uint16_t)OTHER_STK_SIZE,
                (void*)NULL,
                (UBaseType_t)OTHER_TASK_PRIO,
                (TaskHandle_t*)&OTHERTask_Handler);
    
    /* Create debugger download task */
    xTaskCreate((TaskFunction_t)debugger_download_task,
                (const char*)"debugger_download_task",
                (uint16_t)DBUGGER_DOWNLOAD_STK_SIZE,
                (void*)NULL,
                (UBaseType_t)DBUGGER_DOWNLOAD_TASK_PRIO,
                (TaskHandle_t*)&DBUGGER_DOWNLOADTask_Handler);

    /* Create wireless module processing task */
    xTaskCreate((TaskFunction_t)wireless_task,
                (const char*)"wireless_task",
                (uint16_t)WIRELESS_STK_SIZE,
                (void*)NULL,
                (UBaseType_t)WIRELESS_TASK_PRIO,
                (TaskHandle_t*)&WIRELESSTask_Handler);  
    
    vTaskStartScheduler();                  /* Start task scheduler */
    taskEXIT_CRITICAL();                    /* Exit critical section */              
}

/*!
    \brief      DAP link task
    \param[in]  pvParameters: task parameters
    \param[out] none
    \retval     none
*/
void dap_link_task(void *pvParameters)
{
    pvParameters = pvParameters;
    
    while(1)
    {
        chry_dap_handle();
        vTaskDelay(1);
    }
}

/*!
    \brief      LVGL task
    \param[in]  pvParameters: task parameters
    \param[out] none
    \retval     none
*/
void lvgl_task(void *pvParameters)
{
    pvParameters = pvParameters;
    uint32_t time_till_next = 0;

    lvgl_start();
    
    while(1)
    {
        time_till_next = lv_timer_handler();
        vTaskDelay(time_till_next);
    }
}

/*!
    \brief      Other task for system monitoring
    \param[in]  pvParameters: task parameters
    \param[out] none
    \retval     none
*/
void other_task(void *pvParameters)
{
    pvParameters = pvParameters;
    char *temp;
    uint8_t run_count = 0;
    uint8_t i = 0, adc_count = 0;
    uint32_t adc_value[4];
    lvgl_cpu_info_struct lvgl_cpu_info;
    lvgl_main_mem_perused_struct lvgl_main_mem_perused;
    // char task_info_buf[512];
    
    while(1)
    {
        run_count++;
        
        rtc_get_date_time();
        xQueueOverwrite(xQueueRtcData, &rtc_data);
        
        if(run_count >= 5)
        {
            run_count = 0;
            
            ina226_data_get(INA226_ADDR_1);
            ina226_data_get(INA226_ADDR_2);
            ina226_data_get(INA226_ADDR_3);
            ina226_data_get(INA226_ADDR_4);
            xQueueOverwrite(xQueueIna226Data, &ina226_data);

            lvgl_cpu_info.utilization = osGetCpuUUsage();
            xQueueOverwrite(xQueueCpuInfo, &lvgl_cpu_info);
            
            lvgl_main_mem_perused.sram = (float)my_mem_perused(SRAMIN) / 10;
            lvgl_main_mem_perused.sram01 = (float)my_mem_perused(SRAM0_1) / 10;
            lvgl_main_mem_perused.dtcm = (float)my_mem_perused(SRAMDTCM) / 10;
            lvgl_main_mem_perused.sdram = (float)my_mem_perused(SRAMEX) / 10;
            xQueueOverwrite(xQueueMEMPerused, &lvgl_main_mem_perused);
            
            /* Task statistics usage (debug) */
            // vTaskGetRunTimeStats(task_info_buf);
            // PRINT("%s\r\ninfo length: %u\r\n", task_info_buf, strlen(task_info_buf));         
            // vTaskList(task_info_buf);
            // PRINT("%s\r\ninfo length: %u\r\n", task_info_buf, strlen(task_info_buf));   
            
            LED1_toggle;
        }
        
        switch(adc2_get_internal_channel_data(&lvgl_cpu_info.adc2_data))
        {
            case 1:
                xQueueOverwrite(xQueueCpuInfo, &lvgl_cpu_info);
                break;
            
            case 2:
                xQueueOverwrite(xQueueCpuInfo, &lvgl_cpu_info);
                break;
            
            default: break;
        }

        LED2_toggle;
        vTaskDelay(100);
    }
}

/*!
    \brief      Debugger download task
    \param[in]  pvParameters: task parameters
    \param[out] none
    \retval     none
*/
void debugger_download_task(void *pvParameters)
{
    pvParameters = pvParameters;
    
    uint8_t *buffer;
    uint32_t *buffer_temp;
    
    uint8_t res = 0;
    uint16_t i = 0, j = 0;
    uint32_t notify_val = 0, read_bytes = 0, check_value[2];
    lvgl_debugger_download_struct lvgl_debugger_download;
    
    while(1)
    {
        notify_val = ulTaskNotifyTake((BaseType_t)pdTRUE, (TickType_t)portMAX_DELAY);
        buffer = NULL;
        lvgl_debugger_download.run_count = 0;
        lvgl_debugger_download.status = notify_val;
        lvgl_debugger_download.error = 0;

        switch(notify_val)
        {
            case 1:
                break;
            
            case 2: /* Erase flash sectors */
                for(i = 0; i < (download_file_size / flash_device.sectors[0].szSector); i++)
                {
                    res = target_flash_erase_sector(i * flash_device.sectors[0].szSector);
                    if(res != 0)
                    {
                        lvgl_debugger_download.status = 0;
                        lvgl_debugger_download.error = 2;
                        break;
                    }
                    else
                    {
                        lvgl_debugger_download.run_count++;
                        xQueueOverwrite(xQueueDebuggerDownload, &lvgl_debugger_download);
                    }
                }
                if((download_file_size % flash_device.sectors[0].szSector) && (lvgl_debugger_download.error == 0))
                {
                    res = target_flash_erase_sector(i * flash_device.sectors[0].szSector);
                    if(res != 0)
                    {
                        lvgl_debugger_download.status = 0;
                        lvgl_debugger_download.error = 2;
                    }
                    else
                    {
                        lvgl_debugger_download.run_count++;
                    }
                }
                break;
                
            case 3: /* Program flash pages */
                buffer = (uint8_t *)mymalloc(SRAMIN, flash_device.szPage);
                buffer_temp = (uint32_t *)buffer;
                if(buffer)
                {
                    for(i = 0; i < (download_file_size / flash_device.szPage); i++)
                    {
                        debugger_read_bin_file(i * flash_device.szPage, buffer, flash_device.szPage, &read_bytes);
                        if(flash_device.szPage == read_bytes)
                        {
                            for(j = 0; j < read_bytes / 4; j++)
                            {
                                check_value[0] ^= buffer_temp[j];
                            }
                            res = target_flash_program_page(flash_device.devAdr + i * flash_device.szPage, buffer, read_bytes);
                            if(res != 0)
                            {
                                lvgl_debugger_download.status = 0;
                                lvgl_debugger_download.error = 3;
                                break;
                            }
                            else
                            {
                                lvgl_debugger_download.run_count++;
                                xQueueOverwrite(xQueueDebuggerDownload, &lvgl_debugger_download);
                            }
                        }
                        else
                        {
                            lvgl_debugger_download.status = 0;
                            lvgl_debugger_download.error = 3;
                            break;
                        }
                    }
                    /* Handle remaining bytes */
                    if((download_file_size % flash_device.szPage) && (lvgl_debugger_download.error == 0))
                    {
                        debugger_read_bin_file(i * flash_device.szPage, buffer, download_file_size % flash_device.szPage, &read_bytes);
                        if(download_file_size % flash_device.szPage == read_bytes)
                        {
                            for(j = 0; j < read_bytes / 4; j++)
                            {
                                check_value[0] ^= buffer_temp[j];
                            }
                            res = target_flash_program_page(flash_device.devAdr + i * flash_device.szPage, buffer, read_bytes);
                            if(res != 0)
                            {
                                lvgl_debugger_download.status = 0;
                                lvgl_debugger_download.error = 3;
                            }
                            else
                            {
                                lvgl_debugger_download.run_count++;
                            }
                        }
                        else
                        {
                            lvgl_debugger_download.status = 0;
                            lvgl_debugger_download.error = 3;
                        }
                    }
                    myfree(SRAMIN, buffer);
                }
                else
                {
                    lvgl_debugger_download.status = 0;
                    lvgl_debugger_download.error = 3;
                }
                break;
                
            case 4: /* Verify flash programming */
                buffer = (uint8_t *)mymalloc(SRAMIN, flash_device.szPage);
                buffer_temp = (uint32_t *)buffer;
                if(buffer)
                {
                    for(i = 0; i < (download_file_size / flash_device.szPage); i++)
                    {
                        res = swd_read_memory(flash_device.devAdr + i * flash_device.szPage, buffer, flash_device.szPage);
                        if(res)
                        {
                            for(j = 0; j < flash_device.szPage / 4; j++)
                            {
                                check_value[1] ^= buffer_temp[j];
                            }
                            lvgl_debugger_download.run_count++;
                            xQueueOverwrite(xQueueDebuggerDownload, &lvgl_debugger_download);
                        }
                        else
                        {
                            lvgl_debugger_download.status = 0;
                            lvgl_debugger_download.error = 4;
                            break;                            
                        }
                    }
                    /* Handle remaining bytes and verify checksum */
                    if((download_file_size % flash_device.szPage) && (lvgl_debugger_download.error == 0))
                    {
                        res = swd_read_memory(flash_device.devAdr + i * flash_device.szPage, buffer, download_file_size % flash_device.szPage);
                        if(res)
                        {
                            for(j = 0; j < (download_file_size % flash_device.szPage) / 4; j++)
                            {
                                check_value[1] ^= buffer_temp[j];
                            }
                            if(check_value[0] == check_value[1])
                            {
                                lvgl_debugger_download.run_count++;
                            }
                            else
                            {
                                lvgl_debugger_download.status = 0;
                                lvgl_debugger_download.error = 4;  
                            }
                        }
                        else
                        {
                            lvgl_debugger_download.status = 0;
                            lvgl_debugger_download.error = 4;                          
                        }
                    }
                    myfree(SRAMIN, buffer);
                }
                else
                {
                    lvgl_debugger_download.status = 0;
                    lvgl_debugger_download.error = 4;
                }
                break;
                
            default: break;
        }
        
        if(notify_val)
        {
            xQueueSend(xQueueDebuggerDownload, &lvgl_debugger_download, portMAX_DELAY);
        }
    }
}

/*!
    \brief      Wireless task
    \param[in]  pvParameters: task parameters
    \param[out] none
    \retval     none
*/
void wireless_task(void *pvParameters)
{
    pvParameters = pvParameters;
    uint8_t try_recount, wifi_name_length;
    uint16_t i;
    uint32_t notify_val = 0;
    char *temp1, *temp2;
    lvgl_main_state_struct lvgl_main_state;
    lvgl_setting_wireless_state_struct lvgl_setting_wireless_state;
    
    while(1)
    {
        notify_val = ulTaskNotifyTake((BaseType_t)pdTRUE, (TickType_t)10000);
        lvgl_setting_wireless_state.status = 255;
        lvgl_setting_wireless_state.error = 0;

        switch(notify_val)
        {
            case 0: /* Query WiFi connection status */
                uart4_rx_dma_receive_reset();
                uart4_print_fmt("AT+WJAP?\r\n");
                try_recount = WIRELESS_TRY_RECOUNT;
                while(--try_recount)
                {
                    vTaskDelay(1000);
                    if(g_uart4_recv_complete_flag)
                    {
                        temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                        temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                        if(temp1 || temp2)
                        {
                            break; 
                        }
                    }
                }
                if(try_recount)
                {
                    if(temp1)
                    {
                        temp2 = strchr((const char *)g_uart4_recv_buff, ':');
                        if((*(uint8_t *)(temp2 + 1) - 0x30) == 3)   /* 3 means connected to Wi-Fi and IP obtained */
                        {
                            lvgl_main_state.wifi = 1;
                            temp1 = strchr((const char *)temp2, ',');
                            temp2 = strchr((const char *)(temp1 + 1), ',');
                            wifi_name_length =  temp2 - temp1 - 1;
                            if(wifi_name_length < sizeof(lvgl_main_state.wifi_name))
                            {
                                memcpy(lvgl_main_state.wifi_name, temp1 + 1, wifi_name_length);
                                lvgl_main_state.wifi_name[wifi_name_length] = '\0';
                            }
                        }
                        else
                        {
                            lvgl_main_state.wifi = 0;
                            memcpy(lvgl_main_state.wifi_name, "NULL", 5);
                        }
                        xQueueOverwrite(xQueueLvglMainState, &lvgl_main_state);
                    }
                }
                break;
            
            case 1: /* Scan WiFi networks */
                uart4_rx_dma_receive_reset();
                uart4_print_fmt("AT+WSCAN=,,,-65\r\n");
                try_recount = WIRELESS_TRY_RECOUNT;
                while(--try_recount)
                {
                    vTaskDelay(1000);
                    if(g_uart4_recv_complete_flag)
                    {
                        temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                        temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                        if(temp1 || temp2)
                        {
                            break; 
                        }
                    }
                }
                if(try_recount)
                {
                    if(temp1)
                    {
                        lvgl_setting_wireless_scan_info.wifi_num = 0;
                        temp1 = strchr((const char *)g_uart4_recv_buff, '+');
                        temp2 = strchr((const char *)(temp1 + 1), '+');
                        if(temp1 && temp2)
                        {
                            while(temp1 < temp2)
                            {
                                temp1 = strchr((const char *)temp1, '\n');
                                if(temp1 && ((temp1 + 1) < temp2)) temp1 = strchr((const char *)temp1, ' ');
                                else break;
                                
                                if(temp1)
                                {
                                    temp1 +=1;
                                    if(lvgl_setting_wireless_scan_info.wifi_num < sizeof(lvgl_setting_wireless_scan_info.wifi_name) / sizeof(lvgl_setting_wireless_scan_info.wifi_name[0]))
                                    {
                                        wifi_name_length = strchr((const char *)temp1, ',') - temp1;
                                        if(wifi_name_length < sizeof(lvgl_setting_wireless_scan_info.wifi_name[0]) && wifi_name_length)
                                        {
                                            lvgl_setting_wireless_scan_info.wifi_num++;
                                            memcpy(lvgl_setting_wireless_scan_info.wifi_name[lvgl_setting_wireless_scan_info.wifi_num - 1], temp1, wifi_name_length);
                                            lvgl_setting_wireless_scan_info.wifi_name[lvgl_setting_wireless_scan_info.wifi_num - 1][wifi_name_length] = '\0';
                                        }
                                        lvgl_setting_wireless_state.status = 1;
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        lvgl_setting_wireless_state.error = notify_val;
                    }
                }
                else
                {
                    lvgl_setting_wireless_state.error = notify_val;
                }
                break;
                
            case 2: /* Connect to WiFi */
                uart4_rx_dma_receive_reset();
                uart4_print_fmt("AT+WJAP=%s,%s\r\n", lvgl_setting_wireless_connect.wifi_name, lvgl_setting_wireless_connect.wifi_password);
                try_recount = WIRELESS_TRY_RECOUNT;
                while(--try_recount)
                {
                    vTaskDelay(1000);
                    if(g_uart4_recv_complete_flag)
                    {
                        temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                        temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                        if(temp1 || temp2)
                        {
                            break; 
                        }
                    }
                }
                
                if(try_recount)
                {
                    if(temp1)
                    {
                        vTaskDelay(100);
                        uart4_rx_dma_receive_reset();
                        uart4_print_fmt("AT+WAUTOCONN=1\r\n");  /* 设置自动连接 */
                        try_recount = WIRELESS_TRY_RECOUNT;
                        while(--try_recount)
                        {
                            vTaskDelay(1000);
                            if(g_uart4_recv_complete_flag)
                            {
                                temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                                temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                                if(temp1 || temp2)
                                {
                                    break; 
                                }
                            }
                        }
                        
                        if(try_recount)
                        {
                            if(temp1)
                            {
                                lvgl_main_state.wifi = 1;
                                lvgl_setting_wireless_state.status = 0;
                            }
                            else
                            {
                                lvgl_setting_wireless_state.error = notify_val;
                            }
                        }
                    }
                    else
                    {
                        lvgl_setting_wireless_state.error = notify_val;
                    }
                }
                else
                {
                    lvgl_setting_wireless_state.error = notify_val;
                }
                break;
                
            case 3: /* connect to last connected WiFi */
                if(lvgl_main_state.wifi == 0)
                {
                    if(strlen(lvgl_setting_wireless_connect.wifi_name) && \
                       strlen(lvgl_setting_wireless_connect.wifi_password))
                    {
                        uart4_rx_dma_receive_reset();
                        uart4_print_fmt("AT+WJAP=%s,%s\r\n", lvgl_setting_wireless_connect.wifi_name, lvgl_setting_wireless_connect.wifi_password);
                        try_recount = WIRELESS_TRY_RECOUNT;
                        while(--try_recount)
                        {
                            vTaskDelay(1000);
                            if(g_uart4_recv_complete_flag)
                            {
                                temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                                temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                                if(temp1 || temp2)
                                {
                                    break; 
                                }
                            }
                        }
                        
                        if(try_recount)
                        {
                            if(temp1)
                            {
                                vTaskDelay(100);
                                uart4_rx_dma_receive_reset();
                                uart4_print_fmt("AT+WAUTOCONN=1\r\n");  /* 设置自动连接 */
                                try_recount = WIRELESS_TRY_RECOUNT;
                                while(--try_recount)
                                {
                                    vTaskDelay(1000);
                                    if(g_uart4_recv_complete_flag)
                                    {
                                        temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                                        temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                                        if(temp1 || temp2)
                                        {
                                            break; 
                                        }
                                    }
                                }
                                
                                if(try_recount)
                                {
                                    if(temp1)
                                    {
                                        lvgl_main_state.wifi = 1;
                                        lvgl_setting_wireless_state.status = 0;
                                    }
                                    else
                                    {
                                        lvgl_setting_wireless_state.error = notify_val;
                                    }
                                }
                            }
                            else
                            {
                                lvgl_setting_wireless_state.error = notify_val;
                            }
                        }
                        else
                        {
                            lvgl_setting_wireless_state.error = notify_val;
                        }
                    }
                }
                break;
                
            case 4: /* Disconnect WiFi */
                if(lvgl_main_state.wifi == 1)
                {
                    uart4_rx_dma_receive_reset();
                    uart4_print_fmt("AT+WDISCONNECT\r\n");
                    try_recount = WIRELESS_TRY_RECOUNT;
                    while(--try_recount)
                    {
                        vTaskDelay(1000);
                        if(g_uart4_recv_complete_flag)
                        {
                            temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                            temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                            if(temp1 || temp2)
                            {
                                break; 
                            }
                        }
                    }
                    
                    if(try_recount)
                    {
                        if(temp1)
                        {
                            lvgl_main_state.wifi = 0;
                            lvgl_setting_wireless_state.status = 0;
                            memcpy(lvgl_main_state.wifi_name, "NULL", 5);
                            xQueueOverwrite(xQueueLvglMainState, &lvgl_main_state);
                        }
                        else
                        {
                            lvgl_setting_wireless_state.error = notify_val;
                        }
                    }
                    else
                    {
                        lvgl_setting_wireless_state.error = notify_val;
                    }
                }
                break;
                
            case 5: /* Query TCP/UDP/SSL connection status */
                if(lvgl_main_state.wifi == 1)
                {
                    uart4_rx_dma_receive_reset();
                    uart4_print_fmt("AT+SOCKET?\r\n");
                    try_recount = WIRELESS_TRY_RECOUNT;
                    while(--try_recount)
                    {
                        vTaskDelay(1000);
                        if(g_uart4_recv_complete_flag)
                        {
                            temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                            temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                            if(temp1 || temp2)
                            {
                                break; 
                            }
                        }
                    }
                    if(try_recount)
                    {
                        if(temp1)
                        {
                            temp2 = strstr((const char *)g_uart4_recv_buff, ",");
                            if(temp2)
                            {
                                if((*(temp2 - 1) - 0x30) == 1)
                                {
                                    /* Already connected to server, currently only NTP server is configured */
                                    lvgl_setting_wireless_state.status = 2;
                                }
                                else
                                {
                                    /* Not connected */
                                    lvgl_setting_wireless_state.status = 3;
                                }
                            }
                            else
                            {
                                /* Not connected */
                                lvgl_setting_wireless_state.status = 3;
                            }
                        }
                        else
                        {
                            lvgl_setting_wireless_state.error = notify_val;
                        }
                    }
                    else
                    {
                        lvgl_setting_wireless_state.error = notify_val;
                    }
                }
                break;
                
            case 6: /* Disconnect TCP/UDP/SSL connection */
                if(lvgl_main_state.wifi == 1)
                {
                    uart4_rx_dma_receive_reset();
                    uart4_print_fmt("AT+SOCKETDEL=1\r\n"); /* NTP server conID: 1*/
                    try_recount = WIRELESS_TRY_RECOUNT;
                    while(--try_recount)
                    {
                        vTaskDelay(1000);
                        if(g_uart4_recv_complete_flag)
                        {
                            temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                            temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                            if(temp1 || temp2)
                            {
                                break; 
                            }
                        }
                    }
                    if(try_recount)
                    {
                        if(temp1)
                        {
                            lvgl_setting_wireless_state.status = 254; /* 执行成功，但不处理返回值  */
                        }
                        else 
                        {
                            lvgl_setting_wireless_state.error = notify_val;
                        }
                    }
                    else
                    {
                        lvgl_setting_wireless_state.error = notify_val;
                    }
                }
                break;
                
            case 7: /* Establish TCP/UDP/SSL connection */
                if(lvgl_main_state.wifi == 1)
                {
                    uart4_rx_dma_receive_reset();
                    /* 指定使用conID 1进行连接 */
                    uart4_print_fmt("AT+SOCKET=2,%s,123,,1\r\n", ntp_server[lvgl_setting_wireless_connect.ntp_server]);
                    try_recount = WIRELESS_TRY_RECOUNT;
                    while(--try_recount)
                    {
                        vTaskDelay(1000);
                        if(g_uart4_recv_complete_flag)
                        {
                            temp1 = strstr((const char *)g_uart4_recv_buff, "OK");
                            temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                            if(temp1 || temp2)
                            {
                                break; 
                            }
                        }
                    }
                    if(try_recount)
                    {
                        if(temp1)
                        {
                            lvgl_setting_wireless_state.status = 2;
                        }
                        else
                        {
                            lvgl_setting_wireless_state.error = notify_val;
                        }
                    }
                    else
                    {
                        lvgl_setting_wireless_state.error = notify_val;
                    }
                }
                break;
                
            case 8: /* Send NTP request */
                if(lvgl_main_state.wifi == 1)
                {
                    uart4_rx_dma_receive_reset();
                    uart4_print_fmt("AT+SOCKETSEND=1,48\r\n");
                    try_recount = WIRELESS_TRY_RECOUNT;
                    while(--try_recount)
                    {
                        vTaskDelay(1000);
                        if(g_uart4_recv_complete_flag)
                        {
                            temp1 = strstr((const char *)g_uart4_recv_buff, ">");
                            temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                            if(temp1 || temp2)
                            {
                                break; 
                            }
                        }
                    }
                    if(try_recount)
                    {
                        if(temp1)
                        {
                            uart4_rx_dma_receive_reset();
                            timer_event_software_generate(TIMER14, TIMER_EVENT_SRC_UPG);
                            timer_enable(TIMER14);
                            uart4_print_fmt("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", \
                                            ntp_server_transmit_message[0], ntp_server_transmit_message[1], ntp_server_transmit_message[2], ntp_server_transmit_message[3], \
                                            ntp_server_transmit_message[4], ntp_server_transmit_message[5], ntp_server_transmit_message[6], ntp_server_transmit_message[7], \
                                            ntp_server_transmit_message[8], ntp_server_transmit_message[9], ntp_server_transmit_message[10], ntp_server_transmit_message[11], \
                                            ntp_server_transmit_message[12], ntp_server_transmit_message[13], ntp_server_transmit_message[14], ntp_server_transmit_message[15], \
                                            ntp_server_transmit_message[16], ntp_server_transmit_message[17], ntp_server_transmit_message[18], ntp_server_transmit_message[19], \
                                            ntp_server_transmit_message[20], ntp_server_transmit_message[21], ntp_server_transmit_message[22], ntp_server_transmit_message[23], \
                                            ntp_server_transmit_message[24], ntp_server_transmit_message[25], ntp_server_transmit_message[26], ntp_server_transmit_message[27], \
                                            ntp_server_transmit_message[28], ntp_server_transmit_message[29], ntp_server_transmit_message[30], ntp_server_transmit_message[31], \
                                            ntp_server_transmit_message[32], ntp_server_transmit_message[33], ntp_server_transmit_message[34], ntp_server_transmit_message[35], \
                                            ntp_server_transmit_message[36], ntp_server_transmit_message[37], ntp_server_transmit_message[38], ntp_server_transmit_message[39], \
                                            ntp_server_transmit_message[40], ntp_server_transmit_message[41], ntp_server_transmit_message[42], ntp_server_transmit_message[43], \
                                            ntp_server_transmit_message[44], ntp_server_transmit_message[45], ntp_server_transmit_message[46], ntp_server_transmit_message[47]);
                            try_recount = WIRELESS_TRY_RECOUNT;
                            while(--try_recount)
                            {
                                vTaskDelay(1000);
                                if(g_uart4_recv_complete_flag)
                                {
                                    temp1 = strstr((const char *)g_uart4_recv_buff, "+EVENT:SocketDown,1,48,");
                                    temp2 = strstr((const char *)g_uart4_recv_buff, "ERROR");
                                    if(temp1 || temp2)
                                    {
                                        timer_disable(TIMER14);
                                        break; 
                                    }
                                }
                            }
                            
                            if(try_recount)
                            {
                                if(temp1)
                                {
                                    memcpy(ntp_server_receive_message, temp1 + strlen("+EVENT:SocketDown,1,48,"), 48);
                                    wireless_set_rtc_date_time(ntp_server_receive_message);
                                }
                            }
                        }
                    }
                    else
                    {
                        lvgl_setting_wireless_state.error = notify_val;
                    }
                }
                break;
                
            default: break;
        }
        
        if(notify_val && (notify_val < 0xFF))
        {
            xQueueSend(xQueueWirelseeState, &lvgl_setting_wireless_state, (TickType_t)10000);
        }
    }
}
