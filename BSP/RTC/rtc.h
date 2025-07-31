/*!
    \file       rtc.h
    \brief      header file for RTC real-time clock driver
    \version    1.0
    \date       2025-07-24
    \author     Ze-Hou
*/

#ifndef __RTC_H
#define __RTC_H
#include <stdint.h>

/* RTC configuration and status definitions */
#define RTC_CONFIG_STA      RTC_BKP0    /* 32-bit value: high 16 bits for current century, low 16 bits for RTC initialization check */
#define RTC_CHECK_VALUE     0xAAAA      /* RTC initialization check value */
#define RTC_UPDATE          0xFF        /* RTC force update flag */

/* month name lookup table */
static const char *const rtc_month[] = {
    "",
    "Jan",  /* January */
    "Feb",  /* February */
    "Mar",  /* March */
    "Apr",  /* April */
    "May",  /* May */
    "Jun",  /* June */
    "Jul",  /* July */
    "Aug",  /* August */
    "Sep",  /* September */
    "Oct",  /* October */
    "Nov",  /* November */
    "Dec"   /* December */
};

/* week day name lookup table */
static const char *const rtc_week[] = {
    "",
    "Mon",   /* Monday */
    "Tue",   /* Tuesday */
    "Wed",   /* Wednesday */
    "Thu",   /* Thursday */
    "Fri",   /* Friday */
    "Sat",   /* Saturday */
    "Sun"    /* Sunday */
};

/* RTC data structure with BCD format storage */
typedef struct
{
    uint16_t year;      /* year value in BCD format */
    uint8_t month;      /* month value in BCD format (1-12) */
    uint8_t day;        /* day value in BCD format (1-31) */
    uint8_t week;       /* week day value in BCD format (1-7) */
    uint8_t hour;       /* hour value in BCD format (0-23) */
    uint8_t minute;     /* minute value in BCD format (0-59) */
    uint8_t second;     /* second value in BCD format (0-59) */
    uint8_t century;    /* century value in BCD format */
}rtc_data_struct;

extern rtc_data_struct rtc_data;                            /*!< global RTC data structure */

/* function declarations */
void rtc_config(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint8_t update);  /*!< configure RTC date and time */
void rtc_get_date_time(void);                               /*!< get current RTC date and time */
void rtc_print_date_time(void);                             /*!< print current RTC date and time in formatted output */
#endif /* __RTC_H */
