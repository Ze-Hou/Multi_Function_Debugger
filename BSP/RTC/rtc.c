/*!
    \file       rtc.c
    \brief      RTC real-time clock driver with calendar and time management
    \version    1.0
    \date       2025-07-24
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./RTC/rtc.h"
#include "./USART/usart.h"

rtc_data_struct rtc_data;

/*!
    \brief      calculate week day using Zeller's congruence formula
    \param[in]  year: year value
    \param[in]  month: month value (1-12)
    \param[in]  day: day value (1-31)
    \param[out] none
    \retval     week day (1-7: Monday to Sunday)
*/
static uint8_t week_calculate(uint16_t year, uint8_t month, uint8_t day)
{
    if(month < 3)   /* month starts from March, January and February are treated as month 13 and 14 of previous year */
    {
        month += 12;
        year -= 1;
    }

    return (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7 + 1;
}

/*!
    \brief      convert 8-bit unsigned integer to BCD format
    \param[in]  value: 8-bit unsigned integer
    \param[out] none
    \retval     BCD format value
*/
static inline uint8_t integer_to_bcd(uint8_t value)
{
    return (uint8_t)((value /10) << 4) + (value % 10);
}

/*!
    \brief      convert BCD format to 8-bit unsigned integer
    \param[in]  value: BCD format value
    \param[out] none
    \retval     8-bit unsigned integer
*/
static inline uint8_t bcd_to_integer(uint8_t value)
{
    return (uint8_t)((value >> 4) * 10 + (value & 0x0F));
}

/*!
    \brief      configure RTC date and time with optional forced update
    \param[in]  year: year value
    \param[in]  month: month value (1-12)
    \param[in]  day: day value (1-31)
    \param[in]  hour: hour value (0-23)
    \param[in]  minute: minute value (0-59)
    \param[in]  second: second value (0-59)
    \param[in]  update: force update flag (NULL or RTC_UPDATE)
    \param[out] none
    \retval     none
*/
void rtc_config(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint8_t update)
{
    uint16_t max_day;
    uint8_t week;
    
    /* check if parameters are valid */
    if(month > 12 || month == 0) return;
    switch(month)
    {
        case 4: case 6: case 9: case 11:    /* months with 30 days */
            max_day = 30;
            break;
        case 2:                             /* February special handling */
            if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))    /* leap year judgment */
            {
                max_day = 29;
            }
            else
            {
                max_day = 28;
            }
            break;
        default:
            max_day = 31;
            break;
    }
    if(day > max_day || day == 0) return;
    if(hour > 23) return;
    if(minute > 59) return;
    if(second > 59) return;
    
    week = week_calculate(year, month, day);

    if(((RTC_CONFIG_STA & RTC_CHECK_VALUE) != RTC_CHECK_VALUE) || (GET_BITS(RCU_BDCTL, 8, 9) == 0) || (update == RTC_UPDATE)) /* check RTC backup register, clock source and update flag */
    {
        rcu_periph_clock_enable(RCU_PMU);
        
        pmu_backup_write_enable();      /* enable backup domain write */
        RTC_CONFIG_STA = RTC_CHECK_VALUE; /* write RTC initialization flag */
        RTC_CONFIG_STA |= (uint8_t)(integer_to_bcd((year - 1) / 100 + 1)) << 16;       /* write current century value in BCD format */
        year = year % 100;
        if(year == 0)
        {
            RTC_CONFIG_STA |= (uint8_t)0xAA << 24;                         /* current century last year flag */
        }
        
        /* these registers are in backup domain */
        rcu_osci_on(RCU_LXTAL);
        rcu_osci_stab_wait(RCU_LXTAL);
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
        rcu_periph_clock_enable(RCU_RTC);
        
        rtc_parameter_struct  rtc_initpara;
        rtc_initpara.factor_asyn        = 0x7F;                /* asynchronous prescaler 0x7F + 1 */
        rtc_initpara.factor_syn         = 0xFF;                 /* synchronous prescaler 0xFF + 1 */
        rtc_initpara.year               = integer_to_bcd(year);
        rtc_initpara.day_of_week        = integer_to_bcd(week);
        rtc_initpara.month              = integer_to_bcd(month);
        rtc_initpara.date               = integer_to_bcd(day);
        rtc_initpara.display_format     = RTC_24HOUR;
        rtc_initpara.am_pm              = RTC_AM;
        rtc_initpara.hour               = integer_to_bcd(hour);
        rtc_initpara.minute             = integer_to_bcd(minute);
        rtc_initpara.second             = integer_to_bcd(second);
        
        rtc_init(&rtc_initpara);
        pmu_backup_write_disable();                     /* disable backup domain write */
    }
}

/*!
    \brief      get current RTC date and time
    \param[in]  none
    \param[out] none
    \retval     none (updates global rtc_data structure)
*/
void rtc_get_date_time(void)
{
    uint8_t century;
    rtc_parameter_struct  rtc_initpara;
    
    rtc_current_time_get(&rtc_initpara);
    
    if((rtc_initpara.year == 0x00) || (rtc_initpara.year == 0x01))
    {
        pmu_backup_write_enable();      /* enable backup domain write */
        if((rtc_initpara.year == 0x00) && (((RTC_CONFIG_STA >> 24) & 0xAA) != 0xAA))
        {
            RTC_CONFIG_STA |= (uint8_t)0xAA << 24;                                              /* current century last year flag */
        }
        else if((rtc_initpara.year == 0x01) && (((RTC_CONFIG_STA >> 24) & 0xAA) == 0xAA))      /* update century in first year of new century */
        {
            RTC_CONFIG_STA &= ~((uint32_t)(0xFF << 24));                                        /* clear current century last year flag */
            /* update century value */
            century = (uint8_t)(RTC_CONFIG_STA >> 16);
            century = bcd_to_integer(century) + 1;
            century = integer_to_bcd(century);
            RTC_CONFIG_STA |= century << 16;                                                    /* write current century value in BCD format */
        }
        pmu_backup_write_disable();                     /* disable backup domain write */
    }
    
    century = (uint8_t)(RTC_CONFIG_STA >> 16);
    rtc_data.century = century;
    
    if(rtc_initpara.year == 0)
    {
       rtc_data.year = century << 8;
    }
    else
    {
        rtc_data.year = (integer_to_bcd(bcd_to_integer(century) - 1) << 8) | rtc_initpara.year;
    }

    rtc_data.month  = rtc_initpara.month;
    rtc_data.day    = rtc_initpara.date;
    rtc_data.week   = rtc_initpara.day_of_week;
    rtc_data.hour   = rtc_initpara.hour;
    rtc_data.minute = rtc_initpara.minute;
    rtc_data.second = rtc_initpara.second;
}

/*!
    \brief      print current RTC date and time in formatted output
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_print_date_time(void)
{
    rtc_get_date_time();  /* update rtc_data structure with current date and time */

    PRINT_INFO("%04X.%02X.%02X %02X.%02X.%02X\r\n", rtc_data.year, rtc_data.month, rtc_data.day, 
                                                    rtc_data.hour, rtc_data.minute, rtc_data.second);
    switch(rtc_data.week)
    {
        case 1: PRINT_INFO(" %s\r\n", rtc_week[1]); break;   /* Monday */
        case 2: PRINT_INFO(" %s\r\n", rtc_week[2]); break;   /* Tuesday */
        case 3: PRINT_INFO(" %s\r\n", rtc_week[3]); break;   /* Wednesday */
        case 4: PRINT_INFO(" %s\r\n", rtc_week[4]); break;   /* Thursday */
        case 5: PRINT_INFO(" %s\r\n", rtc_week[5]); break;   /* Friday */
        case 6: PRINT_INFO(" %s\r\n", rtc_week[6]); break;   /* Saturday */
        case 7: PRINT_INFO(" %s\r\n", rtc_week[7]); break;   /* Sunday */
        default: PRINT_INFO("\r\n"); break;
    }
}
