/*!
    \file       timer.h
    \brief      header file for comprehensive timer driver implementation
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou

    This file contains:
    - Function declarations for USART communication timeout detection
    - Timer configuration interfaces for system watchdog feeding
    - FreeRTOS runtime statistics timer function declarations
    - Single pulse mode timer setup for timeout applications
    - High-precision timing interfaces for performance monitoring
*/

#ifndef __TIMER_H
#define __TIMER_H
#include <stdint.h>

/* function declarations */
void timer_base5_config(uint16_t psc, uint32_t period);                         /*!< configure TIMER5 for BSP USART timeout detection */
void timer_base6_config(uint16_t psc, uint32_t period);                         /*!< configure TIMER6 for terminal USART timeout detection */
void timer_general15_config(uint16_t psc, uint16_t period);                     /*!< configure TIMER15 for UART4 timeout detection */
void timer_general16_config(uint16_t psc, uint16_t period);                     /*!< configure TIMER16 for automatic watchdog feeding */
#endif /* __TIMER_H */
