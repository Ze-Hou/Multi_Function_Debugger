/*!
    \file       delay.h
    \brief      header file for system delay driver
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef __DELAY_H
#define __DELAY_H
#include <stdint.h>

/* function declarations */
void delay_init(void);                                                          /*!< initialize delay function */
void delay_us(uint32_t nus);                                                    /*!< microsecond precision delay (0~7158278) */
void delay_ms(uint16_t nms);                                                    /*!< intelligent millisecond delay (0~65535) */
void delay_xms(uint16_t nms);                                                   /*!< force busy-wait millisecond delay (0~65535) */
#endif /* __DELAY_H */
