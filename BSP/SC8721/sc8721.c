/*!
    \file       sc8721.c
    \brief      SC8721 step-up converter driver with I2C control
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "./SC8721/sc8721.h"
#include "./IIC/iic.h"
#include "./DELAY/delay.h"
#include "./USART/usart.h"
#include <stdbool.h>
#include <math.h>

/* SC8721 device data instance */
sc8721_data_struct sc8721_data;

/* static function declarations */
static void sc8721_gpio_config(void);                      /*!< configure GPIO for SC8721 chip enable pin */
static uint8_t sc8721_read_reg(uint8_t reg);               /*!< read SC8721 register via I2C */
static void sc8721_write_reg(uint8_t reg, uint8_t value);  /*!< write SC8721 register via I2C */

/*!
    \brief      configure GPIO for SC8721 chip enable pin
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void sc8721_gpio_config(void)
{
    /* enable GPIOC peripheral clock */
    rcu_periph_clock_enable(RCU_GPIOC);
    
    /* configure PC4 as output pin for SC8721 chip enable */
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_4);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, GPIO_PIN_4);
}

/*!
    \brief      initialize SC8721 step-up converter
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sc8721_init(void)
{
    /* configure GPIO for chip enable control */
    sc8721_gpio_config();
    
    /* initialize I2C communication if not already initialized */
    if(iicDevice_1.iic_state != true)
    {
        iic_init(&iicDevice_1);
    }
    
    /* enable SC8721 and configure default settings */
    sc8721_ce_set(SET);                                   /* enable SC8721 chip */
    delay_xms(10);
    sc8721_standby_set(1);                              /* set standby mode */
    sc8721_config(0, 0, 1);          /* auto PWM mode, 20ns dead time, enable input regulation */
    sc8721_frequency_set(3);                          /* set switching frequency to 920kHz */
    sc8721_slope_comp_set(0);                              /* disable slope compensation */
    sc8721_output_set(5.0, 0.5);         /* set output voltage to 5.0V with 0.5A current limit */
}

/*!
    \brief      set SC8721 chip enable pin state
    \param[in]  value: enable state (SET or RESET)
    \param[out] none
    \retval     none
*/
void sc8721_ce_set(bit_status value)
{
    gpio_bit_write(GPIOC, GPIO_PIN_4, value);
}

/*!
    \brief      read SC8721 register via I2C
    \param[in]  reg: register address to read
    \param[out] none
    \retval     register value
*/
static uint8_t sc8721_read_reg(uint8_t reg)
{
    uint8_t value;
    
    /* start I2C communication and send device address for write */
    iic_start(&iicDevice_1);
    iic_send_byte(((SC8721_ADDR << 1) & 0xFE), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    
    /* send register address */
    iic_send_byte(reg, &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    
    /* restart and send device address for read */
    iic_start(&iicDevice_1);
    iic_send_byte(((SC8721_ADDR << 1) | 0x01), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    
    /* read register value and send NACK */
    value = iic_read_byte(0, &iicDevice_1);
    iic_stop(&iicDevice_1);
    
    return value;
}

/*!
    \brief      write SC8721 register via I2C
    \param[in]  reg: register address to write
    \param[in]  value: register value to write
    \param[out] none
    \retval     none
*/
static void sc8721_write_reg(uint8_t reg, uint8_t value)
{
    /* start I2C communication and send device address for write */
    iic_start(&iicDevice_1);
    iic_send_byte(((SC8721_ADDR << 1) & 0xFE), &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    
    /* send register address */
    iic_send_byte(reg, &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    
    /* send register value */
    iic_send_byte(value, &iicDevice_1);
    iic_wait_ack(&iicDevice_1);
    iic_stop(&iicDevice_1);
}

/*!
    \brief      configure SC8721 system settings
    \param[in]  pwm_mode: PWM mode selection (0=auto PWM mode, 1=force PWM mode)
    \param[in]  dead_time: dead time setting (0=20ns, 1=40ns)
    \param[in]  vinreg: input voltage regulation (0=disable, 1=enable)
    \param[out] none
    \retval     none
*/
void sc8721_config(uint8_t pwm_mode, uint8_t dead_time, uint8_t vinreg)
{
    uint8_t temp;
    
    /* validate input parameters */
    if((pwm_mode > 1) || (dead_time > 1) || (vinreg > 1))
    {
        return;
    }
    
    /* read current system settings register */
    temp = sc8721_read_reg(SC8721_SysSetReg);
    
    /* clear configuration bits and set new values */
    temp &= 0x2F;
    temp |= (pwm_mode << 7) | (dead_time << 6) | (vinreg << 4);
    sc8721_write_reg(SC8721_SysSetReg, temp);
    
    /* verify configuration was written successfully */
    if(temp != sc8721_read_reg(SC8721_SysSetReg))
    {
        PRINT_ERROR("configure sc8721 unsuccessfully\r\n");
    }
    else
    {
        PRINT_INFO("configure sc8721 successfully\r\n");
    }
}

/*!
    \brief      set SC8721 switching frequency
    \param[in]  frequency: switching frequency selection (0=260kHz, 1=500kHz, 2=720kHz, 3=920kHz)
    \param[out] none
    \retval     none
*/
void sc8721_frequency_set(uint8_t frequency)
{
    uint8_t temp;
    
    /* validate input parameter */
    if(frequency > 3)
    {
        return;
    }
    
    /* read current frequency register and update frequency bits */
    temp = sc8721_read_reg(SC8721_FreqSetReg);
    temp &= 0xFC;
    temp |= frequency;
    sc8721_write_reg(SC8721_FreqSetReg, temp);
    
    /* verify frequency was set successfully */
    if((sc8721_read_reg(SC8721_FreqSetReg) & ~(0xFC)) != frequency)
    {
        PRINT_ERROR("set sc8721 frequency unsuccessfully\r\n");
    }
    else
    {
        PRINT_INFO("set sc8721 frequency successfully\r\n");
    }
}

/*!
    \brief      set SC8721 slope compensation
    \param[in]  comp: slope compensation setting (0=none, 1=50mV/A, 2=100mV/A, 3=150mV/A)
    \param[out] none
    \retval     none
*/
void sc8721_slope_comp_set(uint8_t comp)
{
    uint8_t temp;
    
    /* validate input parameter */
    if(comp > 3)
    {
        return;
    }
    
    /* read current slope compensation register and update */
    temp = sc8721_read_reg(SC8721_SlopeCompReg);
    temp &= 0xFC;
    temp |= comp;
    
    sc8721_write_reg(SC8721_SlopeCompReg, temp);
    
    /* verify slope compensation was set successfully */
    if((sc8721_read_reg(SC8721_SlopeCompReg) & ~(0xFC)) != comp)
    {
        PRINT_ERROR("set sc8721 slope compensation unsuccessfully\r\n");
    }
    else
    {
        PRINT_INFO("set sc8721 slope compensation successfully\r\n");
    }
}

/*!
    \brief      set SC8721 standby mode
    \param[in]  standby: standby mode setting (0=disable, 1=enable)
    \param[out] none
    \retval     none
*/
void sc8721_standby_set(uint8_t standby)
{
    uint8_t temp;
    
    /* validate input parameter */
    if(standby > 1)
    {
        return;
    }
    
    /* read current global control register and update standby bit */
    temp = sc8721_read_reg(SC8721_GlobalCtrlReg);
    temp &= 0xFB;
    temp |= standby << 2;
    
    sc8721_write_reg(SC8721_GlobalCtrlReg, temp);
    
    /* verify standby mode was set successfully */
    if((sc8721_read_reg(SC8721_GlobalCtrlReg) & ~(0xFB)) != standby << 2)
    {
        PRINT_ERROR("set sc8721 standby mode unsuccessfully\r\n");
    }
    else
    {
        PRINT_INFO("set sc8721 standby mode successfully\r\n");
    }
}

/*!
    \brief      set SC8721 output voltage and current limit
    \param[in]  voltage: desired output voltage in volts (2.7V to 22V)
    \param[in]  limit_current: desired current limit in amperes (0A to 8.5A)
    \param[out] none
    \retval     none
*/
void sc8721_output_set(float voltage, float limit_current)
{
    uint8_t temp;
    uint8_t current_temp;
    uint16_t voltage_temp;
    float voltage_offset;

    /* validate input parameters */
    if((voltage < 2.7) || (voltage > 22) || (limit_current < 0) || (limit_current > 8.5))
    {
        return;
    }
    
    /* calculate voltage offset from calibration value */
    voltage_offset = fabs(voltage - SC8721_CALVALUE);
    
    /* configure voltage setting LSB register */
    temp = sc8721_read_reg(SC8721_VoutSetLsbReg);
    temp &= 0xE0;
    temp |= (1 << 4) | (1 << 3);                   /* enable voltage setting */
    
    /* set direction bit based on voltage relative to calibration value */
    if(voltage < SC8721_CALVALUE)
    {
        temp |= (1 << 2);                           /* voltage below calibration value */
    }
    
    /* calculate and set voltage offset value */
    voltage_temp = round(voltage_offset * 1000 / 20);   /* 20mV resolution */
    temp |= (uint8_t)(voltage_temp & 0x03);             /* lower 2 bits */
    sc8721_write_reg(SC8721_VoutSetLsbReg, temp);
    
    /* set voltage MSB register */
    temp = (uint8_t)(voltage_temp >> 2);                /* upper 8 bits */
    sc8721_write_reg(SC8721_VoutSetMsbReg, temp);
    
    /* calculate and set current limit */
    if(round(limit_current * 10 * 24 / 4 / 2) >= 1)
    {
        current_temp = round(limit_current * 10 * 24 / 4 / 2) - 1;
    }
    else
    {
        current_temp = 0;
    }
    
    sc8721_write_reg(SC8721_CsoReg, current_temp);
    
    /* calculate actual configured values for data structure */
    if(voltage >= SC8721_CALVALUE)
    {
        sc8721_data.set_voltage = (float)((float)voltage_temp * 20 / 1000 + SC8721_CALVALUE);
    }
    else
    {
        sc8721_data.set_voltage = (float)(SC8721_CALVALUE - (float)voltage_temp * 20 / 1000);
    }
    sc8721_data.limit_current = (float)(((float)current_temp + 1) * 4 * 2 / 10 / 24);
    
    /* enable output by clearing standby bit */
    temp = sc8721_read_reg(SC8721_GlobalCtrlReg);
    temp &= 0xFD;
    temp |= (1 << 1);                               /* enable output */
    sc8721_write_reg(SC8721_GlobalCtrlReg, temp);

    /* provide feedback about configured values */
    PRINT_INFO("sc8721 set voltage:%.4f V, limit current: %.4f A\r\n", sc8721_data.set_voltage, sc8721_data.limit_current);
}
