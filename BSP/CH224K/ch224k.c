/*!
    \file       ch224k.c
    \brief      CH224K power delivery controller driver
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./CH224K/ch224k.h"
#include "./USART/usart.h"

/*!
    \brief      initialize CH224K GPIO configuration with default 12V output
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ch224k_init(void)
{
    /* enable GPIO clock for CH224K configuration pins */
    rcu_periph_clock_enable(CH224K_CFG1_RCU);
    rcu_periph_clock_enable(CH224K_CFG2_RCU);
    rcu_periph_clock_enable(CH224K_CFG3_RCU);
    
    /* configure as output mode with appropriate pull resistors */
    gpio_mode_set(CH224K_CFG1_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLDOWN,CH224K_CFG1_PIN);
    gpio_mode_set(CH224K_CFG2_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLDOWN,CH224K_CFG2_PIN);
    gpio_mode_set(CH224K_CFG3_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLUP,CH224K_CFG3_PIN);
    
    /* configure as push-pull output with 12MHz speed */
    gpio_output_options_set(CH224K_CFG1_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_12MHZ,CH224K_CFG1_PIN);
    gpio_output_options_set(CH224K_CFG2_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_12MHZ,CH224K_CFG2_PIN);
    gpio_output_options_set(CH224K_CFG3_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_12MHZ,CH224K_CFG3_PIN);
    
    /* set default output voltage to 12V */
    ch224k_config(POWER_12V);
}

/*!
    \brief      configure CH224K output voltage
    \param[in]  power_enum: target voltage selection
                - POWER_5V: 5V output
                - POWER_9V: 9V output  
                - POWER_12V: 12V output
                - POWER_15V: 15V output
                - POWER_20V: 20V output
    \param[out] none
    \retval     none
*/
void ch224k_config(ch224k_power_type_enum power_enum)
{
    switch(power_enum)
    {
        case POWER_5V:
            gpio_bit_write(CH224K_CFG1_PORT, CH224K_CFG1_PIN, SET);
            PRINT_INFO("SET POWER_5V\r\n");
            break;

        case POWER_9V:
            gpio_bit_write(CH224K_CFG1_PORT, CH224K_CFG1_PIN, RESET);
            gpio_bit_write(CH224K_CFG2_PORT, CH224K_CFG2_PIN, RESET);
            gpio_bit_write(CH224K_CFG3_PORT, CH224K_CFG3_PIN, RESET);
            PRINT_INFO("SET POWER_9V\r\n");
            break;

        case POWER_12V:
            gpio_bit_write(CH224K_CFG1_PORT, CH224K_CFG1_PIN, RESET);
            gpio_bit_write(CH224K_CFG2_PORT, CH224K_CFG2_PIN, RESET);
            gpio_bit_write(CH224K_CFG3_PORT, CH224K_CFG3_PIN, SET);
            PRINT_INFO("SET POWER_12V\r\n");
            break;

        case POWER_15V:
            gpio_bit_write(CH224K_CFG1_PORT, CH224K_CFG1_PIN, RESET);
            gpio_bit_write(CH224K_CFG2_PORT, CH224K_CFG2_PIN, SET);
            gpio_bit_write(CH224K_CFG3_PORT, CH224K_CFG3_PIN, SET);
            PRINT_INFO("SET POWER_15V\r\n");
            break;

        case POWER_20V:
            gpio_bit_write(CH224K_CFG1_PORT, CH224K_CFG1_PIN, RESET);
            gpio_bit_write(CH224K_CFG2_PORT, CH224K_CFG2_PIN, SET);
            gpio_bit_write(CH224K_CFG3_PORT, CH224K_CFG3_PIN, RESET);
            PRINT_INFO("SET POWER_20V\r\n");
            break;

        default:
            /* default to 12V if invalid parameter */
            gpio_bit_write(CH224K_CFG1_PORT, CH224K_CFG1_PIN, RESET);
            gpio_bit_write(CH224K_CFG2_PORT, CH224K_CFG2_PIN, RESET);
            gpio_bit_write(CH224K_CFG3_PORT, CH224K_CFG3_PIN, SET);
            PRINT_INFO("SET POWER_12V\r\n");
            break;
    }
}
