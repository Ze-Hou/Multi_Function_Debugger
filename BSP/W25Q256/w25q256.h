/*!
    \file       w25q256.h
    \brief      W25Q256 SPI flash memory driver header file
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __W25Q256_H
#define __W25Q256_H
#include <stdint.h>

#define W25Q256                         0xEF4019      /*!< W25Q256 device ID */
#define GD25Q256                        0xC84019      /*!< GD25Q256 device ID */

/* W25Q256 instruction set */
#define W25Q_WriteEnable                0x06        /*!< write enable instruction */
#define W25Q_WriteEnableForVolatileSR   0x50        /*!< write enable for volatile status register instruction */
#define W25Q_WriteDisable               0x04        /*!< write disable instruction */
#define W25Q_ReadStatusReg1             0x05        /*!< read status register-1 instruction */
#define W25Q_ReadStatusReg2             0x35        /*!< read status register-2 instruction */
#define W25Q_ReadStatusReg3             0x15        /*!< read status register-3 instruction */
#define W25Q_WriteStatusReg1            0x01        /*!< write status register-1 instruction */
#define W25Q_WriteStatusReg2            0x31        /*!< write status register-2 instruction */
#define W25Q_WriteStatusReg3            0x11        /*!< write status register-3 instruction */
#define W25Q_ResetEnable                0x66        /*!< reset enable instruction */
#define W25Q_ResetDevice                0x99        /*!< reset device instruction */
#define W25Q_Enter4ByteAddressMode      0xB7        /*!< enter 4-byte address mode instruction */
#define W25Q_Exit4ByteAddressMode       0xE9        /*!< exit 4-byte address mode instruction */
#define W25Q_DeviceID                   0x9F        /*!< read device ID instruction */
#define W25Q_UniqueID                   0x4B        /*!< read unique ID instruction */
#define W25Q_FastReadQuad               0xEC        /*!< fast read quad I/O with 4-byte address instruction */
#define W25Q_SectorErase                0x21        /*!< erase sector instruction */
#define W25Q_ChipErase                  0xC7        /*!< erase chip instruction */
#define W25Q_WritePageQuad              0x34        /*!< quad input page program with 4-byte address instruction */
#define W25Q_ReadSecurityReg            0x48        /*!< read security register instruction */
#define W25Q_SecurityRegErase           0x44        /*!< erase security register instruction */
#define W25Q_WriteSecurityReg           0x42        /*!< write security register instruction */

/* function declarations */
uint8_t w25q256_init(void);                                                                                         /* initialize W25Q256 device and configure OSPI interface */
void w25q256_info_print(void);                                                                                      /* print W25Q256 device information */
uint32_t w25q256_read_device_id(void);                                                                              /* read device ID */
uint64_t w25q256_read_unique_id(void);                                                                              /* read unique ID */
uint8_t w25q256_read_data(uint32_t address, uint8_t *pbuffer, uint16_t read_size);                                  /* read data from W25Q256 */
void w25q256_erase_sector(uint16_t sector);                                                                         /* erase sector */
void w25q256_erase_chip(uint16_t sector);                                                                           /* erase entire chip */
uint8_t w25q256_write_data(uint32_t address, uint8_t *pbuffer, uint16_t write_size);                                /* write data to W25Q256 */
uint8_t w25q256_read_security_register(uint8_t reg, uint8_t address, uint8_t *pbuffer, uint16_t read_size);         /* read security register */
uint8_t w25q256_erase_security_register(uint8_t reg);                                                               /* erase security register */
uint8_t w25q256_write_security_register(uint8_t reg, uint8_t address, uint8_t *pbuffer, uint16_t write_size);       /* write security register */
#endif
