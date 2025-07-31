/*!
    \file       ina226.h
    \brief      INA226 power monitoring IC driver header file
    \author     ZeHou
    \version    1.0
    \date       2025-07-24
*/

#ifndef __INA226_H
#define __INA226_H
#include <stdint.h>

/* ina226 compensation */
#define INA226_COMPENSATION_1       0
#define INA226_COMPENSATION_2       0
#define INA226_COMPENSATION_3       -0.0145
#define INA226_COMPENSATION_4       0

/* ina226 register */
#define INA226_ConfigReg            0x00        /* (R/W)ina226 configuration register */
#define INA226_ShuntVoltageReg      0x01        /* (R)ina226 shunt voltage register */
#define INA226_BusVoltageReg        0x02        /* (R)ina226 bus voltage register */
#define INA226_PowereReg            0x03        /* (R)ina226 power register */
#define INA226_CurrentReg           0x04        /* (R)ina226 current register */
#define INA226_CalibrationReg       0x05        /* (R/W)ina226 calibration register */
#define INA226_MaskEnableReg        0x06        /* (R/W)ina226 mask/enable register */
#define INA226_AlertLimitReg        0x07        /* (R/W)ina226 alert limit register */
#define INA226_ManufacturerIDReg    0xFE        /* (R)ina226 manufacturer ID register (0x5449) */
#define INA226_DeviceIDReg          0xFF        /* (R)ina226 device ID register (0x2260) */

/* INA226 device address enumeration */
typedef enum
{
    INA226_ADDR_1 = 0x40,                       /*!< SC8721 power out management IC */
    INA226_ADDR_2 = 0x41,                       /*!< SC8721 power in management IC */
    INA226_ADDR_3 = 0x44,                       /*!< 5V power supply monitor */
    INA226_ADDR_4 = 0x45,                       /*!< Type-C protocol IC power monitor */
}ina226_address_enum;

/* INA226 averaging mode enumeration */
typedef enum
{
    INA226_AVERAGE_MODE_0 = 0x00,               /*!< Number of averages: 1 */
    INA226_AVERAGE_MODE_1,                      /*!< Number of averages: 4 */
    INA226_AVERAGE_MODE_2,                      /*!< Number of averages: 16 */
    INA226_AVERAGE_MODE_3,                      /*!< Number of averages: 64 */
    INA226_AVERAGE_MODE_4,                      /*!< Number of averages: 128 */
    INA226_AVERAGE_MODE_5,                      /*!< Number of averages: 256 */
    INA226_AVERAGE_MODE_6,                      /*!< Number of averages: 512 */
    INA226_AVERAGE_MODE_7,                      /*!< Number of averages: 1024 */
}ina226_average_mode_enum;

/* INA226 bus/shunt voltage conversion time enumeration */
typedef enum
{
    INA226_CONVERSION_TIME_0 = 0x00,            /*!< Conversion time: 140µs */
    INA226_CONVERSION_TIME_1,                   /*!< Conversion time: 204µs */
    INA226_CONVERSION_TIME_2,                   /*!< Conversion time: 332µs */
    INA226_CONVERSION_TIME_3,                   /*!< Conversion time: 588µs */
    INA226_CONVERSION_TIME_4,                   /*!< Conversion time: 1100µs */
    INA226_CONVERSION_TIME_5,                   /*!< Conversion time: 2116µs */
    INA226_CONVERSION_TIME_6,                   /*!< Conversion time: 4156µs */
    INA226_CONVERSION_TIME_7,                   /*!< Conversion time: 8244µs */
}ina226_conversion_time_enum;

/* INA226 measurement data structure */
typedef struct
{
    float calibration_value[4];                /*!< Calibration values: current LSB (mA), power LSB = calibration_value*25 (mW) */
    float bus_voltage[4];                      /*!< Bus voltage measurements in volts (V) */
    float shunt_voltage[4];                    /*!< Shunt voltage measurements in microvolts (µV) */
    float current[4];                          /*!< Current measurements in milliamperes (mA) */
    float power[4];                            /*!< Power measurements in milliwatts (mW) */
}ina226_data_struct;
extern ina226_data_struct ina226_data;

/* Function declarations */
void ina226_init(void);                                                                        /*!< \brief initialize INA226 power monitoring system */
void ina226_info_print(void);                                                                  /*!< \brief print INA226 system information and calibration values */
uint32_t ina226_read_id(ina226_address_enum address);                                          /*!< \brief read INA226 device identification */
void ina226_sw_reset(void);                                                                    /*!< \brief perform software reset of all INA226 devices */
void ina226_config(ina226_address_enum address, ina226_average_mode_enum mode, ina226_conversion_time_enum time); /*!< \brief configure INA226 measurement parameters */
void ina226_set_calibration(ina226_address_enum address, uint16_t current);                    /*!< \brief set INA226 calibration for current/power calculations */
void ina226_data_get(ina226_address_enum address);                                             /*!< \brief read and process INA226 measurement data */
void ina226_print_data(void);                                                                  /*!< \brief print INA226 measurement data in engineering units */
#endif
