/*!
    \file       rgblcd.h
    \brief      RGB LCD display driver header file
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __RGBLCD_H
#define __RGBLCD_H
#include <stdint.h>
#include "gd32h7xx_gpio.h"
#include "./SCREEN/color.h"
#include "./SCREEN/font.h"
#include <stdbool.h>

#define RGBLCD_PID_4384         0x4384      /*!< RGB LCD 4.3" 800x480 product ID */
#define RGBLCD_USING_TOUCH      1           /*!< enable touch screen support */

/* RGB LCD error flags */
typedef enum
{
    RGBLCD_OK = 0x00,                   /*!< 0: RGB LCD operation successful */
    RGBLCD_ERROR,                       /*!< 1: an error occurred */
    RGBLCD_PARAMETER_INVALID,           /*!< 2: the parameter is invalid */
}rgblcd_error_enum;

/* RGB LCD display direction enumeration */
typedef enum
{
    RGBLCD_DISP_DIR_0 = 0x00,   /*!< LCD clockwise rotation 0° display direction */
    RGBLCD_DISP_DIR_90,         /*!< LCD clockwise rotation 90° display direction */
    RGBLCD_DISP_DIR_180,        /*!< LCD clockwise rotation 180° display direction */
    RGBLCD_DISP_DIR_270,        /*!< LCD clockwise rotation 270° display direction */
}rgblcd_disp_dir_enum;

/* RGB LCD font size enumeration */
typedef enum
{
    RGBLCD_FONT_12 = 0x00,      /*!< 12 point font */
    RGBLCD_FONT_16,             /*!< 16 point font */
    RGBLCD_FONT_24,             /*!< 24 point font */
    RGBLCD_FONT_32,             /*!< 32 point font */
}rgblcd_font_enum;

/* RGB LCD number display mode enumeration */
typedef enum
{
  RGBLCD_NUM_SHOW_NOZERO = 0x00,  /*!< leading zeros not displayed */
  RGBLCD_NUM_SHOW_ZERO,           /*!< leading zeros displayed */
} rgblcd_num_mode_enum;

/* RGB LCD timing structure */
typedef struct
{
    uint32_t clock_freq;            /*!< pixel clock frequency */
    uint8_t  pixel_clock_polarity;  /*!< pixel clock polarity */
    uint16_t hactive;               /*!< horizontal active pixels */
    uint16_t hback_porch;           /*!< horizontal back porch */
    uint16_t hfront_porch;          /*!< horizontal front porch */
    uint16_t hsync_len;             /*!< horizontal sync length */
    uint16_t vactive;               /*!< vertical active lines */
    uint16_t vback_porch;           /*!< vertical back porch */
    uint16_t vfront_porch;          /*!< vertical front porch */
    uint16_t vsync_len;             /*!< vertical sync length */
}rgblcd_timing_struct;

/* RGB LCD touch IC type enumeration */
typedef enum
{
    TOUCH_GTXX = 0x00,              /*!< Goodix touch controller */
}rgblcd_touch_enum;

/* RGB LCD parameter structure */
typedef struct
{
    uint8_t id;                     /*!< LCD ID */
    uint16_t pid;                   /*!< product ID */
    uint16_t width;                 /*!< LCD width in pixels */
    uint16_t height;                /*!< LCD height in pixels */
    rgblcd_timing_struct timing;    /*!< LCD timing parameters */
    rgblcd_touch_enum touch_type;   /*!< touch controller type */
}rgblcd_param_struct;

/* RGB LCD parameter matching table */
static const rgblcd_param_struct rgblcd_param[] = {
    {0, RGBLCD_PID_4384,  800, 480, {24000000, 0, 800, 8, 8, 4, 480, 8, 8, 4}, TOUCH_GTXX}, /*!< RGB_LCD-4384, 4.3" 800x480 */
};

/* RGB LCD status data structure */
typedef struct
{
    const rgblcd_param_struct *param; /*!< LCD parameter pointer */
    rgblcd_disp_dir_enum disp_dir;    /*!< display direction */
    uint16_t width;                   /*!< current display width */
    uint16_t height;                  /*!< current display height */
    void *fb;                         /*!< frame buffer pointer */
    uint16_t back_color;              /*!< background color */
    bool rgblcd_state;             /*!< RGB LCD initialization flag */
}rgblcd_status_struct;
extern rgblcd_status_struct rgblcd_status;

/* function declarations */
rgblcd_error_enum rgblcd_init(void);                                                                  /*!< initialize RGB LCD display */
void rgblcd_info_print(void);                                                                         /*!< print RGB LCD information */
void rgblcd_display_on(void);                                                                         /*!< turn on RGB LCD display */
void rgblcd_display_off(void);                                                                        /*!< turn off RGB LCD display */
void rgblcd_pwr_set(bit_status value);                                                                /*!< set RGB LCD power status */
void rgblcd_blk_set(uint8_t value);                                                                   /*!< set RGB LCD backlight brightness */
rgblcd_error_enum rgblcd_set_disp_dir(rgblcd_disp_dir_enum disp_dir);                                 /*!< set RGB LCD display direction */
void rgblcd_ipa_fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);             /*!< fill rectangular area with solid color using IPA */
void rgblcd_ipa_color_fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, void *color_buffer);   /*!< fill rectangular area with color buffer using IPA */
void rgblcd_clear(uint16_t color);                                                                     /*!< clear screen with specified color */
void rgblcd_draw_point(uint16_t x, uint16_t y, uint16_t color);                                        /*!< draw a point at specified coordinates */
uint16_t rgblcd_read_point(uint16_t x, uint16_t y);                                                    /*!< read color value at specified coordinates */
void rgblcd_draw_line(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);             /*!< draw line between two points */
void rgblcd_draw_rect(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);             /*!< draw rectangle outline */
void rgblcd_draw_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);                           /*!< draw circle outline */
void rgblcd_show_char(uint16_t x, uint16_t y, char ch, rgblcd_font_enum font, uint16_t color);         /*!< display single character */
void rgblcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, rgblcd_font_enum font, uint16_t color); /*!< display string with automatic wrapping */
void rgblcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, rgblcd_num_mode_enum mode, rgblcd_font_enum font, uint16_t color); /*!< display number with leading zero control */
void rgblcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, rgblcd_font_enum font, uint16_t color); /*!< display number without leading zeros */
#endif
