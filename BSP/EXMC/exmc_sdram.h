/*!
    \file       exmc_sdram.h
    \brief      header file for EXMC SDRAM driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
    \note       Modified from original firmware for GD32H7xx
*/

#ifndef __EXMC_SDRAM_H
#define __EXMC_SDRAM_H
#include <stdint.h>

/* SDRAM device base addresses */
#define SDRAM_DEVICE0_ADDR                         ((uint32_t)0xC0000000U)    /* SDRAM device 0 base address */
#define SDRAM_DEVICE1_ADDR                         ((uint32_t)0xD0000000U)    /* SDRAM device 1 base address */

/* function declarations */
void sdram_init(uint32_t sdram_device);                                       /*!< initialize SDRAM peripheral */
void sdram_writebuffer_8(uint32_t sdram_device, uint8_t *pbuffer, 
                         uint32_t writeaddr, uint32_t numbytetowrite);         /*!< write byte buffer to SDRAM */
void sdram_readbuffer_8(uint32_t sdram_device, uint8_t *pbuffer, 
                        uint32_t readaddr, uint32_t numbytetoread);            /*!< read byte buffer from SDRAM */
#endif /* __EXMC_SDRAM_H */
