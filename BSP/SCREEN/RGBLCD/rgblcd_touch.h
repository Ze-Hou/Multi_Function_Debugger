/*!
    \file       rgblcd_touch.h
    \brief      header file for RGB LCD touch screen driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef _RGB_LCD_TOUCH_H
#define _RGB_LCD_TOUCH_H
#include "./SCREEN/RGBLCD/rgblcd.h"

#if (RGBLCD_USING_TOUCH != 0)

#define RGBLCD_TOUCH_PID0            "1158"

/* RGB LCD touch screen error codes */
typedef enum
{
    RGBLCD_TOUCH_OK = 0x00,                     /* 0: RGB LCD touch operation successful */
    RGBLCD_TOUCH_ERROR,                         /* 1: an error occurred */
}rgblcd_touch_error_enum;

/* maximum touch points supported by RGB LCD module */
#define RGBLCD_TOUCH_TP_MAX             5

/* RGB LCD touch screen register definitions */
#define RGBLCD_TOUCH_REG_CTRL           0x8040  /* control register */
#define RGBLCD_TOUCH_REG_PID            0x8140  /* product ID register */
#define RGBLCD_TOUCH_REG_TPINFO         0x814E  /* touch status register */
#define RGBLCD_TOUCH_REG_TP1            0x8150  /* touch point 1 data register */
#define RGBLCD_TOUCH_REG_TP2            0x8158  /* touch point 2 data register */
#define RGBLCD_TOUCH_REG_TP3            0x8160  /* touch point 3 data register */
#define RGBLCD_TOUCH_REG_TP4            0x8168  /* touch point 4 data register */
#define RGBLCD_TOUCH_REG_TP5            0x8170  /* touch point 5 data register */

/* touch status register masks */
#define RGBLCD_TOUCH_TPINFO_MASK_STA    0x80
#define RGBLCD_TOUCH_TPINFO_MASK_CNT    0x0F

/* GPIO pin definitions for touch screen interface */
#define RGBLCD_TOUCH_RST_GPIO_RCU           RCU_GPIOB
#define RGBLCD_TOUCH_RST_GPIO_PORT          GPIOB
#define RGBLCD_TOUCH_RST_GPIO_PIN           GPIO_PIN_0
#define RGBLCD_TOUCH_INT_GPIO_RCU           RCU_GPIOB
#define RGBLCD_TOUCH_INT_GPIO_PORT          GPIOB
#define RGBLCD_TOUCH_INT_GPIO_PIN           GPIO_PIN_1

/* RGB LCD touch screen I2C communication address enumeration */
typedef enum
{
    RGBLCD_TOUCH_IIC_ADDR_14 = 0x14,    /* 0x14 GT1151 */
}rgblcd_touch_iic_addr_enum;

/* touch point coordinate data structure */
typedef struct
{
    uint16_t x;     /* touch point X coordinate */
    uint16_t y;     /* touch point Y coordinate */
    uint16_t size;  /* touch point size */
}rgblcd_touch_point_struct;

static rgblcd_touch_point_struct rgblcd_touch_point[RGBLCD_TOUCH_TP_MAX];

/* function declarations */
uint8_t rgblcd_touch_init(rgblcd_touch_enum type);                           /*!< initialize RGB LCD touch screen */
uint8_t rgblcd_touch_scan(rgblcd_touch_point_struct *point, uint8_t cnt);    /*!< scan RGB LCD touch screen */
#endif
#endif
