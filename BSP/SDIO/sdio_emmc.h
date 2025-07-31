/*!
    \file       sdio_emmc.h
    \brief      SDIO eMMC memory card driver header file
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __SDIO_EMMC_H
#define __SDIO_EMMC_H
#include <stdint.h>
#include "gd32h7xx_sdio.h"

/* configure emmc sdio */
#define SDIO_EMMC           SDIO1

/* config SDIO bus mode */
#define SDR_BUSMODE_1BIT    0
#define SDR_BUSMODE_4BIT    0
#define SDR_BUSMODE_8BIT    0
#define DDR_BUSMODE_4BIT    0
#define DDR_BUSMODE_8BIT    1

/* config data transfer mode */
#define EMMC_DMA_MODE       1

/* emmc error flags */
typedef enum
{
    EMMC_OK = 0,                            /*!< 0: no error occurred */
    
    EMMC_OUT_OF_RANGE,                      /*!< 1: command's argument was out of range */
    EMMC_ADDRESS_ERROR,                     /*!< 2: misaligned address which did not match the block length */
    EMMC_BLOCK_LEN_ERROR,                   /*!< 3: transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
    EMMC_ERASE_SEQ_ERROR,                   /*!< 4: an error in the sequence of erase command occurs */
    EMMC_ERASE_PARAM,                       /*!< 5: an invalid selection of write-blocks for erase occurred */
    EMMC_WP_VIOLATION,                      /*!< 6: attempt to program a write protect block or permanent write protected card */
    EMMC_LOCK_UNLOCK_FAILED,                /*!< 7: sequence or password error has been detected in lock/unlock card command */
    EMMC_COM_CRC_ERROR,                     /*!< 8: CRC check of the previous command failed */
    EMMC_ILLEGAL_COMMAND,                   /*!< 9: command not legal for the card state */
    EMMC_CARD_ECC_FAILED,                   /*!< 10: card internal ECC was applied but failed to correct the data */
    EMMC_CC_ERROR,                          /*!< 11: internal card controller error */
    EMMC_GENERAL_ERROR,                     /*!< 12: A generic card error related to the (and detected during) execution of the last host command */
    EMMC_UNDERRUN,                          /*!< 13: The card could not sustain data transfer in stream read mode */
    EMMC_OVERRUN,                           /*!< 14: The card could not sustain data programming in stream write mode */
    EMMC_CSD_OVERWRITE,                     /*!< 15: read only section of the CSD does not match the card content or an attempt to reverse the copy or permanent WP bits was made */
    EMMC_WP_ERASE_SKIP,                     /*!< 16: only partial address space was erased or the temporary or permanent write protected card was erased */
    EMMC_ERASE_RESET,                       /*!< 17: erase sequence was cleared before executing because an out of erase sequence command was received */
    EMMC_SWITCH_ERROR,                      /*!< 18: If set, the card did not switch to the expected mode as requested by the SWITCH command */

    EMMC_CMD_CRC_ERROR,                     /*!< 19: command response received (CRC check failed) */
    EMMC_DATA_CRC_ERROR,                    /*!< 20: data block sent/received (CRC check failed) */
    EMMC_CMD_RESP_TIMEOUT,                  /*!< 21: command response timeout */
    EMMC_DATA_TIMEOUT,                      /*!< 22: data timeout */
    EMMC_TX_UNDERRUN_ERROR,                 /*!< 23: transmit FIFO underrun error occurs */
    EMMC_RX_OVERRUN_ERROR,                  /*!< 24: received FIFO overrun error occurs */
    EMMC_START_BIT_ERROR,                   /*!< 25: start bit error in the bus */

    EMMC_VOLTRANGE_INVALID,                 /*!< 26: the voltage range is invalid */
    EMMC_PARAMETER_INVALID,                 /*!< 27: the parameter is invalid */
    EMMC_OPERATION_IMPROPER,                /*!< 28: the operation is improper */
    EMMC_FUNCTION_UNSUPPORTED,              /*!< 29: the function is unsupported */
    EMMC_LOCKED_STSTE,                      /*!< 30: card state is locked */
    EMMC_ERROR,                             /*!< 31: an error occurred */
    EMMC_DMA_ERROR,                         /*!< 32: an error when idma transfer */
}emmc_error_enum;

/* emmc information struct */
/* \note
        card_state: card state
        card_locked_state: 0 unlocked, 1 locked
        card_ready_state: 0 not ready, 1 ready
        ocr: [6:0] reserved, [7] voltage(1.70-1.95V), [14:8] voltage(2.0-2.6V), [23:15] voltage(2.7-3.6V),
             [30:29] access mode(00b: byte mode, 10b: sector mode), [31] card power up status bit (busy)
        cid: see JESD84-A441 for detials
        csd: see JESD84-A441 for detials
        ext_csd: see JESD84-A441 for detials  
*/
typedef struct
{
    uint8_t card_state;
    uint8_t card_locked_state;
    uint8_t card_ready_state;
    uint8_t emmc_init_state;
    uint32_t ocr;
    uint32_t cid[4];
    uint32_t csd[4];
    uint32_t ext_csd[128];
}emmc_info_struct;

extern emmc_info_struct emmc_info; /* emmc information struct */

/* function declarations */
emmc_error_enum emmc_init(void);                                                             /* initialize eMMC card and configure SDIO interface */
void emmc_info_print(void);                                                                  /* print eMMC card information */
emmc_error_enum emmc_card_select_deselect(uint16_t cardrca);                                /* select or deselect eMMC card */
emmc_error_enum emmc_card_state_get(void);                                                  /* get eMMC card current state */
emmc_error_enum emmc_bus_mode_config(uint32_t busmode, uint32_t datarate);                 /* configure eMMC bus mode and data rate */
emmc_error_enum emmc_read_disk(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber); /* read data from eMMC disk */
emmc_error_enum emmc_write_disk(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber); /* write data to eMMC disk */
#endif
