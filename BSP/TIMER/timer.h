/*!
    \file       timer.h
    \brief      header file for comprehensive timer driver implementation
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef __TIMER_H
#define __TIMER_H
#include <stdint.h>

/* function declarations */
void timer_general2_config(void);                                               /*!< configure TIMER2 for general purpose PWM output */
void timer_base5_config(uint16_t psc, uint32_t period);                         /*!< configure TIMER5 for BSP USART timeout detection */
void timer_base6_config(uint16_t psc, uint32_t period);                         /*!< configure TIMER6 for terminal USART timeout detection */
void timer_general14_config(uint16_t psc, uint16_t period);                     /*!< configure TIMER14 for general purpose single pulse mode */
void timer_general15_config(uint16_t psc, uint16_t period);                     /*!< configure TIMER15 for UART4 timeout detection */
void timer_general16_config(uint16_t psc, uint16_t period);                     /*!< configure TIMER16 for automatic watchdog feeding */
void timer_general40_config(uint16_t psc, uint16_t period);                     /*!< configure TIMER40 for trigger output generation */
void timer_general41_config(uint16_t psc, uint16_t period, uint32_t repetitioncounter); /*!< configure TIMER41 for single pulse mode */
void timer_base51_config(uint16_t psc, uint32_t period);                        /*!< configure TIMER51 for single pulse mode */
#endif /* __TIMER_H */
