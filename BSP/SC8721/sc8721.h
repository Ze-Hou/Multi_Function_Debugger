/*!
    \file       sc8721.h
    \brief      header file for SC8721 step-up converter driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __SC8721_H
#define __SC8721_H
#include <stdint.h>
#include "gd32h7xx_gpio.h"

/* SC8721 device configuration */
#define SC8721_ADDR                 0x62        /*!< SC8721 I2C device address */
#define SC8721_CALVALUE             5.0         /*!< SC8721 calibration reference voltage */

/* SC8721 register definitions */
#define SC8721_CsoReg               0x01        /*!< (R/W) current sense offset register */
#define SC8721_SlopeCompReg         0x02        /*!< (R/W) slope compensation register */
#define SC8721_VoutSetMsbReg        0x03        /*!< (R/W) output voltage set MSB register */
#define SC8721_VoutSetLsbReg        0x04        /*!< (R/W) output voltage set LSB register */
#define SC8721_GlobalCtrlReg        0x05        /*!< (R/W) global control register */
#define SC8721_SysSetReg            0x06        /*!< (R/W) system settings register */
#define SC8721_FreqSetReg           0x08        /*!< (R/W) switching frequency set register */
#define SC8721_StatusReg1           0x09        /*!< (R) status register 1 */
#define SC8721_StatusReg2           0x0A        /*!< (R) status register 2 */

/* SC8721 data structure */
typedef struct
{
    float set_voltage;                          /*!< configured output voltage in volts */
    float limit_current;                        /*!< configured current limit in amperes */
}sc8721_data_struct;

/* external variables */
extern sc8721_data_struct sc8721_data;         /*!< SC8721 device data instance */

/* function declarations */
void sc8721_init(void);                                                     /*!< initialize SC8721 step-up converter */
void sc8721_ce_set(bit_status value);                                       /*!< enable/disable SC8721 chip enable pin */
void sc8721_config(uint8_t pwm_mode, uint8_t dead_time, uint8_t vinreg);    /*!< configure SC8721 system settings */
void sc8721_frequency_set(uint8_t frequency);                               /*!< set SC8721 switching frequency */
void sc8721_slope_comp_set(uint8_t comp);                                   /*!< set SC8721 slope compensation */
void sc8721_standby_set(uint8_t standby);                                   /*!< set SC8721 standby mode */
void sc8721_output_set(float voltage, float limit_current);                 /*!< set SC8721 output voltage and current limit */
#endif
