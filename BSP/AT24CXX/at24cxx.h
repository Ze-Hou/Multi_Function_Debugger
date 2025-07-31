/*!
    \file       at24cxx.h
    \brief      header file for AT24CXX EEPROM driver
    \version    1.0
    \date       2025-07-24
    \author     Ze-Hou
*/

#ifndef __AT24CXX_H
#define __AT24CXX_H
#include <stdint.h>

/* AT24CXX EEPROM size definitions */
#define AT24C01     127      /*!< AT24C01 size: 128 bytes */
#define AT24C02     255      /*!< AT24C02 size: 256 bytes */
#define AT24C04     511      /*!< AT24C04 size: 512 bytes */
#define AT24C08     1023     /*!< AT24C08 size: 1K bytes */
#define AT24C16     2047     /*!< AT24C16 size: 2K bytes */
#define AT24C32     4095     /*!< AT24C32 size: 4K bytes */
#define AT24C64     8191     /*!< AT24C64 size: 8K bytes */
#define AT24C128    16383    /*!< AT24C128 size: 16K bytes */
#define AT24C256    32767    /*!< AT24C256 size: 32K bytes */
#define AT24C512    65535    /*!< AT24C512 size: 64K bytes */

/* EEPROM type selection - Using AT24C512, modify EE_TYPE for other types */
#define EE_TYPE                  AT24C512
#define POWER_ON_STATE_ADDR      AT24C512 - 5                          /*!< 5 bytes, verify_bit + data_bit(4) */

/* AT24CXX device I2C addresses */
#define AT24CXX_DEVICE1_ADDR      0x50    /*!< First AT24CXX device address */
#define AT24CXX_DEVICE2_ADDR      0x51    /*!< Second AT24CXX device address */

/* File system configuration */
#define AT24CXX_FILE_SYSTEM_TOTAL_SIZE            128*1024                /*!< File system total size */
#define AT24CXX_FILE_SYSTEM_PAGE_SIZE             128                     /*!< File system page size */      
#define AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM         9                       /*!< File system info storage pages */  
#define AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM    40                      /*!< File system catalogue storage pages */ 
#define AT24CXX_FILE_SYSTEM_INDEX_PAGE_NUM        15                      /*!< File system index storage pages */ 
#define AT24CXX_FILE_SYSTEM_STORAGE_PAGE_NUM      960                     /*!< File system data storage pages */
#define AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE        32                      /*!< File system catalogue entry size (32 bytes) */ 
#define AT24CXX_FILE_SYSTEM_INDEX_SIZE            2                       /*!< File system index entry size (2 bytes) */ 

/**
 * \brief File system information structure
 */
typedef struct
{
    uint8_t file_system_state;                  /*!< File system state flag */
    uint32_t file_system_total_size;            /*!< Total file system size */
    uint8_t file_system_page_size;              /*!< File system page size */
    uint16_t file_system_info_page_num;         /*!< Info pages count */
    uint16_t file_system_catalogue_page_num;    /*!< Catalogue pages count */
    uint16_t file_system_index_page_num;        /*!< Index pages count */
    uint16_t file_system_storge_page_num;       /*!< Storage pages count */
    uint8_t file_system_catalogue_size;         /*!< Catalogue entry size */
    uint8_t file_system_index_size;             /*!< Index entry size */
    uint32_t file_system_size_used;             /*!< Used space size */
    uint32_t file_system_size_idle;             /*!< Free space size */
}at24cxx_file_system_struct;

extern at24cxx_file_system_struct at24cxx_file_system_info;

/* function declarations */
void at24cxx_init(void);                                                                /*!< initialize AT24CXX EEPROM driver */
uint8_t at24cxx_read_one_byte(uint8_t device, uint16_t addr);                           /*!< read one byte from EEPROM */
void at24cxx_write_one_byte(uint8_t device, uint16_t addr, uint8_t data);               /*!< write one byte to EEPROM */
void at24cxx_read(uint32_t addr, uint8_t *pbuf, uint16_t datalen);                      /*!< read multiple bytes from EEPROM */
void at24cxx_write(uint32_t addr, uint8_t *pbuf, uint16_t datalen);                     /*!< write multiple bytes to EEPROM */
void at24cxx_file_system_format(void);                                                  /*!< format EEPROM file system */
void at24cxx_file_system_info_set(at24cxx_file_system_struct at24cxx_file_system_info); /*!< set file system information */
void at24cxx_file_system_info_print(void);                                              /*!< print file system information */
void at24cxx_file_system_file_get(void);                                                /*!< get and display all files */
uint8_t at24cxx_file_system_file_creat(const uint8_t *file_name);                       /*!< create new file in file system */
uint8_t at24cxx_file_system_file_open(const uint8_t *file_name);                        /*!< open file in file system */
uint8_t at24cxx_file_system_file_write(const uint8_t *file_name, const uint8_t *write_data, const uint16_t file_size); /*!< write data to file */
uint8_t at24cxx_file_system_file_read(const uint8_t *file_name, uint8_t *read_data);    /*!< read data from file */
uint8_t at24cxx_file_system_file_delete(const uint8_t *file_name);                      /*!< delete file from file system */
#endif
