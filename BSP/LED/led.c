/*!
    \file       led.c
    \brief      LED driver based on GPIO output control
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./LED/led.h"

/*!
    \brief      initialize LED GPIO configuration
    \param[in]  none
    \param[out] none
    \retval     none
*/
void led_init(void)
{
    /* enable GPIO clock for LED1 and LED2 */
    rcu_periph_clock_enable(RCU_LED1);
    rcu_periph_clock_enable(RCU_LED2);

    /* configure LED1 as output mode with push-pull and high speed */
    gpio_mode_set(PORT_LED1,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_LED1);
    gpio_output_options_set(PORT_LED1,GPIO_OTYPE_PP,GPIO_OSPEED_60MHZ,PIN_LED1);

    /* configure LED2 as output mode with push-pull and high speed */
    gpio_mode_set(PORT_LED2,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_LED2);
    gpio_output_options_set(PORT_LED2,GPIO_OTYPE_PP,GPIO_OSPEED_60MHZ,PIN_LED2);

    /* turn off all LEDs by default */
    gpio_bit_set(PORT_LED1,PIN_LED1);
    gpio_bit_set(PORT_LED2,PIN_LED2);
}

