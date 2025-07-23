/*!
    \file       mpu.h
    \brief      header file for memory protection unit (MPU) driver implementation
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou

    This file contains:
    - Function declarations for MPU region configuration and protection setup
    - Memory protection interfaces for ITCM, DTCM, AXI SRAM, SRAM0-1, and SDRAM
    - MPU attribute configuration functions for cache and buffer optimization
    - ARM Cortex-M7 MPU hardware abstraction layer declarations
    - Type extension field (TEX) and memory attribute control interfaces
    - Shareable, cacheable, and bufferable memory configuration functions
*/

#ifndef __MPU_H
#define __MPU_H
#include <stdint.h>

/* function declarations */
uint8_t mpu_set_protection(uint32_t baseaddr, uint32_t size, uint32_t rnum, uint8_t de, \
                           uint8_t tex, uint8_t ap, uint8_t sen, uint8_t cen, uint8_t ben);     /*!< configure memory protection region with specified attributes */
void mpu_memory_protection(void);                                                               /*!< configure memory protection for all system memory regions */

#endif /* __MPU_H */


