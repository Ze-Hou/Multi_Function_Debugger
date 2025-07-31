/*!
    \file       key.c
    \brief      key driver based on GPIO input detection
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./KEY/key.h"
#include "./DELAY/delay.h"

/*!
    \brief      initialize key GPIO configuration
    \param[in]  none
    \param[out] none
    \retval     none
*/
void key_init(void)
{
    /* enable GPIO clock for WKUP key */
    rcu_periph_clock_enable(WK_UP_RCU);
    
    /* configure as input mode with pull-down resistor */ 
    gpio_mode_set(WK_UP_PORT,GPIO_MODE_INPUT,GPIO_PUPD_PULLDOWN,WK_UP_PIN);  /* key default state is low level, configure as pull-down */
}

/*!
    \brief      key scanning function with debounce processing
    \param[in]  mode: scan mode selection (0 or 1)
                - 0: single-press mode (only first press returns key value when held down,
                     must release before next press is recognized)
                - 1: continuous-press mode (returns key value on every function call
                     when key is held down)
    \param[out] none
    \retval     key value definitions:
                - WKUP_PRES (1): WKUP key pressed
                - 0: no key pressed
*/
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;  /* key release flag */
    uint8_t keyval = 0;

    if (mode) key_up = 1;       /* support continuous press */

    if (key_up && (WK_UP == SET) ) /* key release flag is 1, and any key is pressed */
    {
        delay_xms(20);           /* debounce delay */
        key_up = 0;

        if (WK_UP == SET) keyval = WKUP_PRES;
    }
    else if (WK_UP == RESET)         /* no key pressed, mark key as released */
    {
        key_up = 1;
    }

    return keyval;              /* return key value */
}
