/*!
    \file       adc.h
    \brief      header file for ADC2 internal channel driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __ADC_H
#define __ADC_H
#include <stdint.h>

/* ADC2 register definitions */
#define ADC2_RDATA  (ADC2 + 0x64)                          /*!< ADC2 regular data register address */
#define ADC2_INTERNAL_CHANNEL_DATA_LENGTH   100             /*!< ADC2 internal channel data buffer length */

/* temperature calibration values from factory */
#define ADC_TEMP_CALIBRATION_VALUE_25                (REG16(0x1FF0F7C0) & 0x0FFF)     /*!< temperature sensor calibration value at 25°C */
#define ADC_TEMP_CALIBRATION_VALUE_MINUS40           (REG16(0x1FF0F7C2) & 0x0FFF)     /*!< temperature sensor calibration value at -40°C */
#define ADC_HIGH_TEMP_CALIBRATION_VALUE_25           (REG16(0x1FF0F7C4) & 0x0FFF)     /*!< high precision temperature sensor calibration value at 25°C */

/* ADC2 internal channel data structure */
typedef struct
{
    float vbat;                                             /*!< battery voltage in volts */
    float cpu_temperature;                                  /*!< CPU temperature in Celsius */
    float internal_vref;                                    /*!< internal reference voltage in volts */
    float high_precision_cpu_temperature;                   /*!< high precision CPU temperature in Celsius */
}adc2_data_struct;

/* function declarations */
void adc2_internal_channel_config(void);                                /*!< configure ADC2 for internal channel sampling */
uint8_t adc2_get_internal_channel_data(adc2_data_struct *adc2_data);    /*!< get ADC2 internal channel data */
void adc2_print_internal_channel_data(void);                            /*!< print ADC2 internal channel data */
#endif
