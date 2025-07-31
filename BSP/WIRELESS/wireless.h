/*!
    \file       wireless.h
    \brief      header file for wireless communication module driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __WIRELESS_H
#define __WIRELESS_H
#include <stdint.h>
#include <stdbool.h>
#include "gd32h7xx_gpio.h"

#define WIRELESS_TRY_RECOUNT        60
#define WIRELESS_TIMEOUT            100000000

static uint32_t wireless_baudrate = 0;

/* NTP server list */
static const char *const ntp_server[] = {
    "ntp.aliyun.com",   /* 203.107.6.88 */
    "ntp.sjtu.edu.cn",   /* 17.253.116.125 */
};

static const uint8_t ntp_server_transmit_message[48] = {
    0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8_t ntp_server_receive_message[48] = {0};

/* NTP packet structure for network byte order storage, conversion to little endian required when used */
typedef struct
{
    uint8_t li_vn_mode;             /* leap indicator, version number, and mode */
    uint8_t stratum;                /* stratum level */
    uint8_t poll;                   /* poll interval */
    uint8_t precision;              /* clock precision */
    uint32_t root_delay;            /* root delay, total delay to reference clock, format: 16-bit integer + 16-bit fraction */
    uint32_t root_disp;             /* root dispersion, total dispersion to reference clock, same format */
    uint32_t ref_id;                /* reference identifier */
    uint64_t ref_ts;                /* reference timestamp, last calibration time */
    uint64_t orig_ts;               /* originate timestamp, client transmission time */
    uint64_t recv_ts;               /* receive timestamp, server reception time */
    uint64_t trans_ts;              /* transmit timestamp, server transmission time */
}ntp_packet_struct;
extern ntp_packet_struct ntp_packet;

/* function declarations */
void wireless_en(bit_status value);                            /*!< enable or disable wireless module */
bool wireless_reset(void);                                     /*!< reset wireless module */
void wireless_init(uint32_t baud_rate);                        /*!< initialize wireless module */
uint32_t wireless_get_baud_rate(void);                         /*!< get wireless module baud rate */
uint8_t wireless_set_baud_rate(uint32_t baud_rate);            /*!< set wireless module baud rate */
void wireless_config(void);                                    /*!< configure wireless module */
void wireless_set_rtc_date_time(void *ntp_data);               /*!< set RTC date and time from NTP data */
#endif
