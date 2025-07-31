/*!
    \file       wireless.c
    \brief      wireless communication module driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./WIRELESS/wireless.h"
#include "./DELAY/delay.h"
#include "./USART/usart.h"
#include "./TIMER/timer.h"
#include "./RTC/rtc.h"
#include "./MALLOC/malloc.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

ntp_packet_struct ntp_packet;

/*!
    \brief      initialize wireless module GPIO configuration
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void wireless_gpio_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GPIO_PIN_5);
}

/*!
    \brief      enable or disable wireless module
    \param[in]  value: the status of wireless module
      \arg        SET: enable wireless module
      \arg        RESET: disable wireless module
    \param[out] none
    \retval     none
*/
void wireless_en(bit_status value)
{
    gpio_bit_write(GPIOC, GPIO_PIN_5, value);
}

/*!
    \brief      reset wireless module
    \param[in]  none
    \param[out] none
    \retval     none
*/
bool wireless_reset(void)
{
    uint8_t count = WIRELESS_TRY_RECOUNT;
    uint32_t timeout = WIRELESS_TIMEOUT;
    char *ptr;
    
    uart4_rx_dma_receive_reset();
    uart4_print_fmt("AT+RST\r\n");
    while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
    delay_xms(5000);

    return true;
}

/*!
    \brief      initialize wireless module
    \param[in]  baud_rate: the baud rate of wireless module
    \param[out] none
    \retval     none
*/
void wireless_init(uint32_t baud_rate)
{
    uart4_init(baud_rate);
    wireless_gpio_init();
    wireless_en(SET);
    delay_xms(2000);        /* wait for module to initialize */
    wireless_reset();
}

/*!
    \brief      get wireless module baud rate
    \param[in]  none
    \param[out] none
    \retval     baud rate of wireless module (0 if failed)
*/
uint32_t wireless_get_baud_rate(void)
{
    uint32_t timeout = WIRELESS_TIMEOUT;
    char *ptr;

    uart4_rx_dma_receive_reset();
    uart4_print_fmt("AT+UARTCFG?\r\n");
    while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
    
    if(g_uart4_recv_complete_flag)
    {
        if(strstr((const char *)g_uart4_recv_buff, "OK"))
        {
            ptr = strstr((const char *)g_uart4_recv_buff, "+UARTCFG:");
            if(ptr) return atoi((const char *)(ptr + strlen("+UARTCFG:")));
        }
    }

    return 0;
}

/*!
    \brief      set wireless module baud rate
    \param[in]  baud_rate: the baud rate to be set
    \param[out] none
    \retval     operation result (0: failed, 1: successful)
*/
uint8_t wireless_set_baud_rate(uint32_t baud_rate)
{
    uint8_t count = WIRELESS_TRY_RECOUNT;
    uint32_t timeout = WIRELESS_TIMEOUT;
    char *ptr;
    
    uart4_rx_dma_receive_reset();
    uart4_print_fmt("AT+UARTCFG=%u,8,1,0\r\n", baud_rate);
    while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
    while(--count)
    {
        if(g_uart4_recv_complete_flag)
        {
            if(strstr((const char *)g_uart4_recv_buff, "OK"))
            {
                break;
            }
        }
        delay_xms(100);
    }
    
    return 0;
}

/*!
    \brief      configure wireless module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void wireless_config(void)
{
    uint8_t count = WIRELESS_TRY_RECOUNT;
    uint32_t timeout = WIRELESS_TIMEOUT;
    
    uart4_rx_dma_receive_reset();
    uart4_print_fmt("AT\r\n");
    while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
    while(--count)
    {
        if(g_uart4_recv_complete_flag)
        {
            if(strstr((const char *)g_uart4_recv_buff, "OK"))
            {
                break;
            }
        }
        delay_xms(100);
    }
    
    if(count == 0)
    {
        PRINT_ERROR("Wireless module: No response to AT\r\n");
        return;
    }
    
    count = WIRELESS_TRY_RECOUNT;
    
    uart4_rx_dma_receive_reset();
    uart4_print_fmt("ATE0\r\n");
    timeout = WIRELESS_TIMEOUT;
    while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
    while(--count)
    {
        if(g_uart4_recv_complete_flag)
        {
            if(strstr((const char *)g_uart4_recv_buff, "OK"))
            {
                break;
            }
        }
        delay_xms(100);
    }
    
    if(count == 0)
    {
        PRINT_ERROR("Wireless moudule: No response to ATE0\r\n");
        return;
    }

    /* set to station mode */
    count = WIRELESS_TRY_RECOUNT;
    timeout = WIRELESS_TIMEOUT;
    uart4_rx_dma_receive_reset();
    uart4_print_fmt("AT+WMODE?\r\n");
    while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
    while(--count)
    {
        if(strstr((const char *)g_uart4_recv_buff, "OK"))
        {
            if((*(uint8_t *)(strchr((const char *)g_uart4_recv_buff, ':') + 1) - 0x30) != 1)
            {
                uart4_rx_dma_receive_reset();
                uart4_print_fmt("AT+WMODE=1,1\r\n");
                timeout = WIRELESS_TIMEOUT;
                while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
                if(strstr((const char *)g_uart4_recv_buff, "OK"))
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        delay_xms(100);
    }
    
    if(count == 0)
    {
        PRINT_ERROR("Wireless module: No response to AT+WMODE\r\n");
        return;
    }

    /* set socket receive mode */
    count = WIRELESS_TRY_RECOUNT;
    timeout = WIRELESS_TIMEOUT;
    uart4_rx_dma_receive_reset();
    uart4_print_fmt("AT+SOCKETRECVCFG=1\r\n");
    while((!g_uart4_recv_complete_flag) && (timeout--))__NOP();
    while(--count)
    {
        if(strstr((const char *)g_uart4_recv_buff, "OK"))
        {
            break;
        }
        delay_xms(100);
    }
    
    if(count == 0)
    {
        PRINT_ERROR("Wireless module: No response to AT+SOCKETRECVCFG\r\n");
        return;
    }
}

/*!
    \brief      convert big endian to little endian format
    \param[in]  big_endian: 32-bit big endian value
    \param[out] none
    \retval     32-bit little endian value
*/
static inline uint32_t big_to_little_endian(uint32_t big_endian)
{
    return ((big_endian >> 24) & 0x000000FF) |          /* move highest byte to lowest */
           ((big_endian >> 8)  & 0x0000FF00) |          /* move second highest byte to second lowest */
           ((big_endian << 8)  & 0x00FF0000) |          /* move second lowest byte to second highest */
           ((big_endian << 24) & 0xFF000000);           /* move lowest byte to highest */
}

/*!
    \brief      set RTC date and time from NTP data
    \param[in]  ntp_data: pointer to NTP packet data
    \param[out] none
    \retval     none
*/
void wireless_set_rtc_date_time(void *ntp_data)
{
    time_t unix = 0;
    
    memcpy((uint8_t *)&ntp_packet, ntp_data, 4);
    ntp_packet.root_delay = big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 4));
    ntp_packet.root_disp = big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 8));
    ntp_packet.ref_id = big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 12));
    ntp_packet.ref_ts = (uint64_t)((uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 16)) << 32) | (uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 20));
    ntp_packet.orig_ts = (uint64_t)((uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 24)) << 32) | (uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 28));
    ntp_packet.recv_ts = (uint64_t)((uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 32)) << 32) | (uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 36));
    ntp_packet.trans_ts = (uint64_t)((uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 40)) << 32) | (uint64_t)big_to_little_endian(*(uint32_t *)((uint8_t *)ntp_data + 44));
    
    unix = (time_t)(ntp_packet.trans_ts >> 32) - 2208988800 + 8 * 60 *60 + (uint32_t)round((float)(0xFFFFFFFF - timer_runtime_repetition_value_read(TIMER14)) / 1000);
    struct tm *local_time = localtime(&unix);
    
    rtc_config(local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec, RTC_UPDATE);
}
