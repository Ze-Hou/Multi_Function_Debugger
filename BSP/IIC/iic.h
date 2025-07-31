/*!
    \file       iic.h
    \brief      header file for Software I2C driver
    \version    1.0
    \date       2025-07-24
    \author     Ze-Hou
*/

#ifndef __IIC_H
#define __IIC_H
#include "gd32h7xx.h"
#include "gd32h7xx_rcu.h"
#include <stdbool.h>

/* I2C device configuration structure */
typedef __PACKED_STRUCT
{
    rcu_periph_enum periph_scl;        /* SCL pin clock source */
    uint32_t gpio_periph_scl;           /* SCL GPIO port */
    uint32_t pin_scl;                   /* SCL pin number */
    rcu_periph_enum periph_sda;         /* SDA pin clock source */
    uint32_t gpio_periph_sda;           /* SDA GPIO port */
    uint32_t pin_sda;                   /* SDA pin number */
    bool iic_state;                     /* I2C initialization state */
}iicDeviceStruct;

extern iicDeviceStruct iicDevice_1;                 /*!< I2C device instance 1 */
extern iicDeviceStruct iicDevice_2;                 /*!< I2C device instance 2 */

/* function declarations */
void iic_init(iicDeviceStruct *iicDev);             /*!< initialize I2C GPIO configuration */
void iic_start(iicDeviceStruct *iicDev);            /*!< generate I2C start signal */
void iic_stop(iicDeviceStruct *iicDev);             /*!< generate I2C stop signal */
void iic_ack(iicDeviceStruct *iicDev);              /*!< send ACK response signal */
void iic_nack(iicDeviceStruct *iicDev);             /*!< send NACK response signal */
uint8_t iic_wait_ack(iicDeviceStruct *iicDev);      /*!< wait for ACK response signal */
void iic_send_byte(uint8_t data, iicDeviceStruct *iicDev);  /*!< send one byte via I2C */
uint8_t iic_read_byte(uint8_t ack, iicDeviceStruct *iicDev); /*!< receive one byte via I2C */
#endif /* __IIC_H */
