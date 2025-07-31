/*!
    \file       trigsel.c
    \brief      TRIGSEL trigger source selection driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./TRIGSEL/trigsel.h"

/*!
    \brief      configure peripheral trigger source
    \param[in]  none
    \param[out] none
    \retval     none
*/
void trigsel_config(void)
{
    /* enable TRIGSEL peripheral clock */
    rcu_periph_clock_enable(RCU_TRIGSEL);
    
    /* reset TRIGSEL configuration */
    trigsel_deinit();
    
    /* configure TIMER40_TRGO0 to trigger ADC2 regular sequence */
    trigsel_init(TRIGSEL_OUTPUT_ADC2_REGTRG, TRIGSEL_INPUT_TIMER40_TRGO0);
    
    /* lock TRIGSEL register configuration */
    trigsel_register_lock_set(TRIGSEL_OUTPUT_ADC2_REGTRG);
}