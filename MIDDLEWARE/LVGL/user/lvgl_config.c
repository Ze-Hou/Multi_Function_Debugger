/*!
    \file       lvgl_config.c
    \brief      LVGL configuration implementation file
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#include "lvgl_config.h"
#include "./USART/usart.h"
#include "./MALLOC/malloc.h"
#include "./FATFS/fatfs_config.h"
#include "FreeRTOS.h"
#include "lvgl.h"
#include "lvgl_setting.h"
#include "lvgl_usart.h"

/* External function declarations */
extern TickType_t xTaskGetTickCount( void );
extern void lvgl_font_buffer_malloc(void);
extern void lvgl_power_chart_buffer_malloc(void);

/* External input device declarations */
extern lv_indev_t * indev_touchpad;
extern lv_indev_t * indev_mouse;
extern lv_indev_t * indev_keypad;
extern lv_indev_t * indev_encoder;
extern lv_indev_t * indev_button;

#if LV_USE_LOG && (LV_LOG_PRINTF == 0)
/*!
    \brief      LVGL log print callback function
    \param[in]  level: log level
    \param[in]  buf: log message buffer
    \param[out] none
    \retval     none
*/
void lvgl_log_cb(lv_log_level_t level, const char * buf)
{
    printf("%s",buf);
}
#endif

/*!
    \brief      LVGL configuration initialization
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lvgl_config(void)
{
    /* Set tick callback function for LVGL */
    lv_tick_set_cb(xTaskGetTickCount);
    
    /* Register log print callback if logging is enabled */
    #if LV_USE_LOG && (LV_LOG_PRINTF == 0)
        lv_log_register_print_cb(lvgl_log_cb);
    #endif
    
    /* Allocate memory for LVGL font buffer */
    lvgl_font_buffer_malloc();
    
    /* Allocate memory for LVGL power chart buffer */
    lvgl_power_chart_buffer_malloc();
}

/*!
    \brief      Configure LVGL input device read timer
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lvgl_config_indev_read_timer(void)
{
    /* Set touchpad input device read timer period to 2x default refresh period */
    lv_timer_set_period(lv_indev_get_read_timer(indev_touchpad), LV_DEF_REFR_PERIOD * 2);
}
