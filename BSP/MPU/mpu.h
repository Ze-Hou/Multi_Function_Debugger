/*!
    \file       mpu.h
    \brief      header file for memory protection unit (MPU) driver implementation
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef __MPU_H
#define __MPU_H
#include <stdint.h>

/* function declarations */
uint8_t mpu_set_protection(uint32_t baseaddr, uint32_t size, uint32_t rnum, uint8_t de, \
                           uint8_t tex, uint8_t ap, uint8_t sen, uint8_t cen, uint8_t ben);     /*!< configure memory protection region with specified attributes */
void mpu_memory_protection(void);                                                               /*!< configure memory protection for all system memory regions */

#endif /* __MPU_H */


