/*!
    \file       ina226.c
    \brief      INA226 power monitor IC driver implementation
    \version    1.0
    \date       2025-07-24
    \author     Ze-Hou
*/

#include "./INA226/ina226.h"
#include "./IIC/iic.h"
#include "./DELAY/delay.h"
#include "./USART/usart.h"
#include <stdbool.h>

ina226_data_struct ina226_data; /* INA226 measurement data structure */

/*!
    \brief      initialize INA226 power monitor devices
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ina226_init(void)
{
    if(iicDevice_1.iic_state != true)
    {
        iic_init(&iicDevice_1);
    }
    ina226_sw_reset();
    delay_xms(500);
    ina226_config(INA226_ADDR_1, INA226_AVERAGE_MODE_2, INA226_CONVERSION_TIME_7);  /* Set conversion time 8.244ms, averaging 16 times, approximately 8.244 * 16 * 2ms for one complete measurement */
    ina226_config(INA226_ADDR_2, INA226_AVERAGE_MODE_2, INA226_CONVERSION_TIME_7);
    ina226_config(INA226_ADDR_3, INA226_AVERAGE_MODE_2, INA226_CONVERSION_TIME_7);
    ina226_config(INA226_ADDR_4, INA226_AVERAGE_MODE_2, INA226_CONVERSION_TIME_7);
    ina226_set_calibration(INA226_ADDR_1, 4096);
    ina226_set_calibration(INA226_ADDR_2, 4096);
    ina226_set_calibration(INA226_ADDR_3, 2048);
    ina226_set_calibration(INA226_ADDR_4, 4096);
    delay_xms(500);
    ina226_data_get(INA226_ADDR_1);
    ina226_data_get(INA226_ADDR_2);
    ina226_data_get(INA226_ADDR_3);
    ina226_data_get(INA226_ADDR_4);
    ina226_info_print();
}

/*!
    \brief      print INA226 device information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ina226_info_print(void)
{
    PRINT_INFO("print ina226 information>>\r\n");
    PRINT_INFO("/*********************************************************************/\r\n");
    PRINT_INFO("ina226(1-4) address: 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n", INA226_ADDR_1, INA226_ADDR_2, \
                                                                      INA226_ADDR_3, INA226_ADDR_4);
    PRINT_INFO("ina226_1 id: 0x%08X\r\n", ina226_read_id(INA226_ADDR_1));
    PRINT_INFO("ina226_2 id: 0x%08X\r\n", ina226_read_id(INA226_ADDR_2));
    PRINT_INFO("ina226_3 id: 0x%08X\r\n", ina226_read_id(INA226_ADDR_3));
    PRINT_INFO("ina226_4 id: 0x%08X\r\n", ina226_read_id(INA226_ADDR_4));
    PRINT_INFO("ina226_1 calibration_value: %f LSB\r\n", ina226_data.calibration_value[0]);
    PRINT_INFO("ina226_2 calibration_value: %f LSB\r\n", ina226_data.calibration_value[1]);
    PRINT_INFO("ina226_3 calibration_value: %f LSB\r\n", ina226_data.calibration_value[2]);
    PRINT_INFO("ina226_4 calibration_value: %f LSB\r\n", ina226_data.calibration_value[3]);
    PRINT_INFO("/*********************************************************************/\r\n");
}

/*!
    \brief      read INA226 register value
    \param[in]  address: INA226 device address
    \param[in]  reg: register address to read
    \param[out] none
    \retval     16-bit register value
*/
static uint16_t ina226_read_reg(ina226_address_enum address, uint8_t reg)
{
    uint16_t value;
    
    iic_start(&iicDevice_1);    /* Send I2C start signal */
    iic_send_byte(((address << 1) & 0xFE), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    iic_send_byte(reg, &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    iic_start(&iicDevice_1);
    iic_send_byte(((address << 1) | 0x01), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    value = iic_read_byte(1, &iicDevice_1) << 8;
    value |= iic_read_byte(0, &iicDevice_1);
    iic_stop(&iicDevice_1);
    
    return value;
}

/*!
    \brief      write value to INA226 register
    \param[in]  address: INA226 device address
    \param[in]  reg: register address to write
    \param[in]  value: 16-bit data to write to register
    \param[out] none
    \retval     none
*/
static void ina226_write_reg(ina226_address_enum address, uint8_t reg, uint16_t value)
{
    iic_start(&iicDevice_1);    /* Send I2C start signal */
    iic_send_byte(((address << 1) & 0xFE), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    iic_send_byte(reg, &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    iic_send_byte((uint8_t)(value >> 8), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    iic_send_byte((uint8_t)(value), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    iic_stop(&iicDevice_1);
}

/*!
    \brief      read INA226 device ID
    \param[in]  address: INA226 device address
    \param[out] none
    \retval     32-bit device ID (manufacturer ID + device ID)
*/
uint32_t ina226_read_id(ina226_address_enum address)
{
    uint32_t id;
    id = ina226_read_reg(address, INA226_ManufacturerIDReg) << 16;
    id |= ina226_read_reg(address, INA226_DeviceIDReg);
    
    return id;
}

/*!
    \brief      perform software reset of all INA226 devices
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ina226_sw_reset(void)
{
    uint16_t temp;
    
    temp = ina226_read_reg(INA226_ADDR_1, INA226_ConfigReg);
    temp &= 0x7FFF;
    temp |= 0x8000;
    ina226_write_reg(INA226_ADDR_1, INA226_ConfigReg, temp);
    
    temp = ina226_read_reg(INA226_ADDR_2, INA226_ConfigReg);
    temp &= 0x7FFF;
    temp |= 0x8000;
    ina226_write_reg(INA226_ADDR_2, INA226_ConfigReg, temp);
    
    temp = ina226_read_reg(INA226_ADDR_3, INA226_ConfigReg);
    temp &= 0x7FFF;
    temp |= 0x8000;
    ina226_write_reg(INA226_ADDR_3, INA226_ConfigReg, temp);
    
    temp = ina226_read_reg(INA226_ADDR_4, INA226_ConfigReg);
    temp &= 0x7FFF;
    temp |= 0x8000;
    ina226_write_reg(INA226_ADDR_4, INA226_ConfigReg, temp);
    
    delay_xms(10);
}

/*!
    \brief      configure INA226 device settings
    \param[in]  address: INA226 device address
    \param[in]  mode: averaging mode for measurements
    \param[in]  time: conversion time for measurements
    \param[out] none
    \retval     none
*/
void ina226_config(ina226_address_enum address, ina226_average_mode_enum mode, ina226_conversion_time_enum time)
{
    uint16_t temp;
    
    temp = ina226_read_reg(address, INA226_ConfigReg);
    temp &= 0x4000;
    temp |= (mode << 9) | (time << 6) | (time << 3) | 0x7;
    ina226_write_reg(address, INA226_ConfigReg, temp);
}

/*!
    \brief      set INA226 calibration register value
    \param[in]  address: INA226 device address
    \param[in]  current: maximum expected current in mA
    \param[out] none
    \retval     none
*/
void ina226_set_calibration(ina226_address_enum address, uint16_t current)
{
    double temp;
    uint16_t value;
    
    temp = (double)current / (1 << 15);
    
    switch(address)
    {
        case INA226_ADDR_1:
            ina226_data.calibration_value[0] = temp + INA226_COMPENSATION_1;
            break;
        case INA226_ADDR_2:
            ina226_data.calibration_value[1] = temp + INA226_COMPENSATION_2;
            break;
        case INA226_ADDR_3:
            ina226_data.calibration_value[2] = temp + INA226_COMPENSATION_3;
            break;
        case INA226_ADDR_4:
            ina226_data.calibration_value[3] = temp + INA226_COMPENSATION_4;
            break;
        default: break;
    }
    
    temp = temp / 1000 * 10 / 1000;
    temp = 0.00512 / temp;
    value = temp;
  
    ina226_write_reg(address, INA226_CalibrationReg, value);
    if(ina226_read_reg(address, INA226_CalibrationReg) != value)
    {
        PRINT_INFO("error: set ina226 calibration register unsuccessfully!\r\n");
    }
    else
    {
        PRINT_INFO("log: set ina226 calibration register(value: %d) successfully!\r\n", value);
    }
}

/*!
    \brief      read and process INA226 measurement data
    \param[in]  address: INA226 device address
    \param[out] none
    \retval     none
*/
void ina226_data_get(ina226_address_enum address)
{
    uint16_t temp;
    uint16_t value_bus, value_power;
    int16_t value_shunt, value_current;
    
    temp = ina226_read_reg(address, INA226_MaskEnableReg);
    
    if((temp & (1 << 3)) && (!(temp & (1 << 2))))   /* Check INA226 conversion ready flag */
    {
        value_bus = ina226_read_reg(address, INA226_BusVoltageReg);
        value_shunt = ina226_read_reg(address, INA226_ShuntVoltageReg);
        value_current = ina226_read_reg(address, INA226_CurrentReg);
        value_power = ina226_read_reg(address, INA226_PowereReg);
        switch(address)
        {
            case INA226_ADDR_1:
                ina226_data.bus_voltage[0] = (float)value_bus * 1.25 / 1000; /* Bus voltage, unit: V */
                ina226_data.shunt_voltage[0] = (float)value_shunt * 2.5; /* Shunt voltage, unit: uV */
                ina226_data.current[0] = (float)value_current * ina226_data.calibration_value[0]; /* Current, unit: mA */
                ina226_data.power[0] = (float)value_power * ina226_data.calibration_value[0] * 25; /* Power, unit: mW */
                break;
            case INA226_ADDR_2:
                ina226_data.bus_voltage[1] = (float)value_bus * 1.25 / 1000; /* Bus voltage, unit: V */
                ina226_data.shunt_voltage[1] = (float)value_shunt * 2.5; /* Shunt voltage, unit: uV */
                ina226_data.current[1] = (float)value_current * ina226_data.calibration_value[1]; /* Current, unit: mA */
                ina226_data.power[1] = (float)value_power * ina226_data.calibration_value[1] * 25; /* Power, unit: mW */
                break;
            case INA226_ADDR_3:
                ina226_data.bus_voltage[2] = (float)value_bus * 1.25 / 1000; /* Bus voltage, unit: V */
                ina226_data.shunt_voltage[2] = (float)value_shunt * 2.5; /* Shunt voltage, unit: uV */
                ina226_data.current[2] = (float)value_current * ina226_data.calibration_value[2]; /* Current, unit: mA */
                ina226_data.power[2] = (float)value_power * ina226_data.calibration_value[2] * 25; /* Power, unit: mW */
                break;
            case INA226_ADDR_4:
                ina226_data.bus_voltage[3] = (float)value_bus * 1.25 / 1000; /* Bus voltage, unit: V */
                ina226_data.shunt_voltage[3] = (float)value_shunt * 2.5; /* Shunt voltage, unit: uV */
                ina226_data.current[3] = (float)value_current * ina226_data.calibration_value[3]; /* Current, unit: mA */
                ina226_data.power[3] = (float)value_power * ina226_data.calibration_value[3] * 25; /* Power, unit: mW */
                break;
            default: break;
        } 
    }
}

/*!
    \brief      print measurement data from all INA226 devices
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ina226_print_data(void)
{
    /* Get fresh measurement data from all INA226 devices */
    ina226_data_get(INA226_ADDR_1);
    ina226_data_get(INA226_ADDR_2);
    ina226_data_get(INA226_ADDR_3);
    ina226_data_get(INA226_ADDR_4);

    /* Display SC8721 input power measurements */
    PRINT_INFO("INA226 SC8721 OUT: %.4fV, %.4fA, %.4fW\r\n", ina226_data.bus_voltage[0], ina226_data.current[0] / 1000, \
                                                             ina226_data.power[0] / 1000);
    /* Display SC8721 output power measurements */
    PRINT_INFO("INA226 SC8721 IN: %.4fV, %.4fA, %.4fW\r\n", ina226_data.bus_voltage[1], ina226_data.current[1] / 1000, \
                                                            ina226_data.power[1] / 1000);
    /* Display 5V rail power measurements */
    PRINT_INFO("INA226 5V: %.4fV, %.4fA, %.4fW\r\n", ina226_data.bus_voltage[2], ina226_data.current[2] / 1000, \
                                                     ina226_data.power[2] / 1000);
    /* Display PD (Power Delivery) power measurements */
    PRINT_INFO("INA226 PD: %.4fV, %.4fA, %.4fW\r\n", ina226_data.bus_voltage[3], ina226_data.current[3] / 1000, \
                                                     ina226_data.power[3] / 1000);
}
