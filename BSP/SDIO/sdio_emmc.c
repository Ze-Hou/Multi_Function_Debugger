/*!
    \file       sdio_emmc.c
    \brief      SDIO eMMC memory card driver implementation
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./SDIO/sdio_emmc.h"
#include "./USART/usart.h"
#include "./DELAY/delay.h"

/* emmc memory card bus commands index */
/* class 0 (basic) */
#define EMMC_CMD_GO_IDLE_STATE                  ((uint8_t)0)   /* CMD0, GO_IDLE_STATE */
#define EMMC_CMD_SEND_OP_COND                   ((uint8_t)1)   /* CMD1, SEND_OP_COND */
#define EMMC_CMD_ALL_SEND_CID                   ((uint8_t)2)   /* CMD2, ALL_SEND_CID */
#define EMMC_CMD_SEND_RELATIVE_ADDR             ((uint8_t)3)   /* CMD3, SEND_RELATIVE_ADDR */
#define EMMC_CMD_SET_DSR                        ((uint8_t)4)   /* CMD4, SET_DSR */
#define EMMC_CMD_SWITCH_FUNC                    ((uint8_t)6)   /* CMD6, SWITCH */
#define EMMC_CMD_SELECT_DESELECT_CARD           ((uint8_t)7)   /* CMD7, SELECT_DESELECT_CARD */
#define EMMC_CMD_SEND_EXT_CSD                   ((uint8_t)8)   /* CMD8, SEND_EXT_CSD */
#define EMMC_CMD_SEND_CSD                       ((uint8_t)9)   /* CMD9, SEND_CSD */
#define EMMC_CMD_SEND_CID                       ((uint8_t)10)  /* CMD10, SEND_CID */
#define EMMC_CMD_STOP_TRANSMISSION              ((uint8_t)12)  /* CMD12, STOP_TRANSMISSION */
#define EMMC_CMD_SEND_STATUS                    ((uint8_t)13)  /* CMD13, SEND_STATUS */
#define EMMC_CMD_GO_INACTIVE_STATE              ((uint8_t)15)  /* CMD15, GO_INACTIVE_STATE */
/* class 2 (block read) */
#define EMMC_CMD_SET_BLOCKLEN                   ((uint8_t)16)  /* CMD16, SET_BLOCKLEN */
#define EMMC_CMD_READ_SINGLE_BLOCK              ((uint8_t)17)  /* CMD17, READ_SINGLE_BLOCK */
#define EMMC_CMD_READ_MULTIPLE_BLOCK            ((uint8_t)18)  /* CMD18, READ_MULTIPLE_BLOCK */
#define EMMC_SET_BLOCK_COUNT                    ((uint8_t)23)  /* CMD23, SET_BLOCK_COUNT */
/* class 4 (block write) */
#define EMMC_CMD_WRITE_BLOCK                    ((uint8_t)24)  /* CMD24, WRITE_BLOCK */
#define EMMC_CMD_WRITE_MULTIPLE_BLOCK           ((uint8_t)25)  /* CMD25, WRITE_MULTIPLE_BLOCK */
#define EMMC_CMD_PROG_CSD                       ((uint8_t)27)  /* CMD27, PROG_CSD */
/* class 5 (erase) */
#define EMMC_CMD_ERASE_GROUP_START              ((uint8_t)35)  /* CMD35, ERASE_GROUP_START */
#define EMMC_CMD_ERASE_GROUP_END                ((uint8_t)36)  /* CMD36, ERASE_GROUP_END */
#define EMMC_CMD_ERASE                          ((uint8_t)38)  /* CMD38, ERASE */
/* class 6 (write protection) */
#define EMMC_CMD_SET_WRITE_PROT                 ((uint8_t)28)  /* CMD28, SET_WRITE_PROT */
#define EMMC_CMD_CLR_WRITE_PROT                 ((uint8_t)29)  /* CMD29, CLR_WRITE_PROT */
#define EMMC_CMD_SEND_WRITE_PROT                ((uint8_t)30)  /* CMD30, SEND_WRITE_PROT */
#define EMMC_CMD_SEND_WRITE_PROT_TYPE           ((uint8_t)30)  /* CMD30, SEND_WRITE_PROT_TYPE */
/* class 7 (lock card) */
#define EMMC_CMD_LOCK_UNLOCK                    ((uint8_t)42)  /* CMD42, LOCK_UNLOCK */

/* emmc clock division */
#define EMMC_CLK_DIV_INIT                     ((uint16_t)0x0289)        /* eMMC clock division in initilization phase, 400kHz */
#define EMMC_CLK_DIV_TRANS_DSPEED             ((uint16_t)0x0009)        /* eMMC clock division in default speed transmission phase, 26MHz */
#define EMMC_CLK_DIV_TRANS_HSPEED             ((uint16_t)0x0004)        /* eMMC clock division in high speed transmission phase, 52MHz  */

/* the maximum length of data */
#define EMMC_MAX_DATA_LENGTH                  ((uint32_t)0x01FFFFFFU)   /* the maximum length of data */

/* mask flags */
#define SDIO_MASK_INTC_FLAGS                  ((uint32_t)0x1FE00FFF)    /* mask flags of SDIO_INTC */
#define SDIO_MASK_CMD_FLAGS                   ((uint32_t)0x002000C5)    /* mask flags of CMD FLAGS */
#define SDIO_MASK_DATA_FLAGS                  ((uint32_t)0x08000F3A)    /* mask flags of DATA FLAGS */

/* emmc r1 no error flags */
#define EMMC_ALLZERO                          ((uint32_t)0x00000000U)   /* all zero */

/* emmc rac */
#define EMMC_RCA                              ((uint16_t)0x0001U)        /* rac bits */
#define EMMC_RCA_SHIFT                        ((uint8_t)0x10U)           /* rac shift bits */

/* emmc data */
#define EMMC_DATATIMEOUT                      ((uint32_t)0xFFFFFFFFU)    /* DSM data timeout */

/* sdio fifo data*/
#define SDIO_FIFO_ADDR                        SDIO_FIFO(SDIO)            /* address of SDIO_FIFO */
#define EMMC_FIFOHALF_WORDS                   ((uint32_t)0x00000008U)    /* words of FIFO half full/empty */
#define EMMC_FIFOHALF_BYTES                   ((uint32_t)0x00000020U)    /* bytes of FIFO half full/empty */

/* card status of R1 definitions (JESD84-A441) */
#define EMMC_R1_OUT_OF_RANGE                  BIT(31)                   /* command's argument was out of the allowed range */
#define EMMC_R1_ADDRESS_ERROR                 BIT(30)                   /* misaligned address which did not match the block length */
#define EMMC_R1_BLOCK_LEN_ERROR               BIT(29)                   /* transferred block length is not allowed */
#define EMMC_R1_ERASE_SEQ_ERROR               BIT(28)                   /* an error in the sequence of erase commands occurred */
#define EMMC_R1_ERASE_PARAM                   BIT(27)                   /* an invalid selection of write-blocks for erase occurred */
#define EMMC_R1_WP_VIOLATION                  BIT(26)                   /* the host attempts to write to a protected block or to the temporary or permanent write protected card */
#define EMMC_R1_CARD_IS_LOCKED                BIT(25)                   /* (status) the card is locked by the host */
#define EMMC_R1_LOCK_UNLOCK_FAILED            BIT(24)                   /* a sequence or password error has been detected in lock/unlock card command */
#define EMMC_R1_COM_CRC_ERROR                 BIT(23)                   /* CRC check of the previous command failed */
#define EMMC_R1_ILLEGAL_COMMAND               BIT(22)                   /* command not legal for the card state */
#define EMMC_R1_CARD_ECC_FAILED               BIT(21)                   /* card internal ECC was applied but failed to correct the data */
#define EMMC_R1_CC_ERROR                      BIT(20)                   /* internal card controller error */
#define EMMC_R1_GENERAL_ERROR                 BIT(19)                   /* A generic card error related to the (and detected during) execution of the last host command */
#define EMMC_R1_UNDERRUN                      BIT(18)                   /* The card could not sustain data transfer in stream read mode */
#define EMMC_R1_OVERRUN                       BIT(17)                   /* The card could not sustain data programming in stream write mode */
#define EMMC_R1_CSD_OVERWRITE                 BIT(16)                   /* read only section of the CSD does not match or attempt to reverse the copy or permanent WP bits */
#define EMMC_R1_WP_ERASE_SKIP                 BIT(15)                   /* partial address space was erased */
                                                                        /* bit 14 reserved */
#define EMMC_R1_ERASE_RESET                   BIT(13)                   /* an erase sequence was cleared before executing */
#define EMMC_R1_CURRENT_STATE                 BITS(9, 12)               /* (status) The state of the card when receiving the command. If the command execution causes a state change,
                                                                           it will be visible to the host in the response on the next command. */
#define EMMC_R1_READY_FOR_DATA                BIT(8)                    /* (status) correspond to buffer empty signaling on the bus */
#define EMMC_R1_SWITCH_ERROR                  BIT(7)                    /* If set, the card did not switch to the expected mode as requested by the SWITCH command */
#define EMMC_R1_URGENT_BKOPS                  BIT(6)                    /* (status) If set, device needs to perform background operations urgently,
                                                                           Host can check EXT_CSD field BKOPS_STATUS for the detailed level*/
#define EMMC_R1_APP_CMD                       BIT(5)                    /* (status) card will expect ACMD,or indication that the command has been interpreted as ACMD*/
                                                                        /* bit 4:0 reserved */
#define EMMC_R1_ERROR_BITS                    ((uint32_t)0xFDFFA080U)       /* all the R1 error bits */

/* card state (JESD84-A441) */
#define EMMC_CARDSTATE_IDLE                   ((uint8_t)0x00)           /* eMMC is in idle state */
#define EMMC_CARDSTATE_READY                  ((uint8_t)0x01)           /* eMMC is in ready state */
#define EMMC_CARDSTATE_IDENTIFICAT            ((uint8_t)0x02)           /* eMMC is in identificat state */
#define EMMC_CARDSTATE_STANDBY                ((uint8_t)0x03)           /* eMMC is in standby state */
#define EMMC_CARDSTATE_TRANSFER               ((uint8_t)0x04)           /* eMMC is in transfer state */
#define EMMC_CARDSTATE_DATA                   ((uint8_t)0x05)           /* eMMC is in data sending state */
#define EMMC_CARDSTATE_RECEIVING              ((uint8_t)0x06)           /* eMMC is in receiving state */
#define EMMC_CARDSTATE_PROGRAMMING            ((uint8_t)0x07)           /* eMMC is in programming state */
#define EMMC_CARDSTATE_DISCONNECT             ((uint8_t)0x08)           /* eMMC is in disconnect state */
#define EMMC_CARDSTATE_BUSTEST                ((uint8_t)0x09)           /* eMMC is in bus test state */
#define EMMC_CARDSTATE_SLEEP                  ((uint8_t)0x0A)           /* eMMC is in sleep state */

/* static function declarations */
static emmc_error_enum emmc_block_read(uint32_t *preadbuffer, uint32_t blockaddr);             /* read single block from eMMC */
static emmc_error_enum emmc_multiblocks_read(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber); /* read multiple blocks from eMMC */
static emmc_error_enum emmc_block_write(uint32_t *pbuffer, uint32_t blockaddr);                /* write single block to eMMC */
static emmc_error_enum emmc_multiblocks_write(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber); /* write multiple blocks to eMMC */
static emmc_error_enum emmc_power_on(void);                                                   /* power on eMMC card */
static emmc_error_enum emmc_power_off(void);                                                  /* power off eMMC card */
static emmc_error_enum emmc_card_init(void);                                                  /* initialize eMMC card */
static emmc_error_enum emmc_card_extcsd_get(void);                                           /* get extended CSD register */
static emmc_error_enum emmc_enter_high_speed_mode(void);                                     /* enter high speed mode */
static emmc_error_enum emmc_transfer_stop(void);                                             /* stop data transfer */
static emmc_error_enum cmdsent_error_check(void);                                            /* check if command sent error occurs */
static emmc_error_enum r1_error_check(uint8_t cmdindex);                                     /* check if error occurs for R1 response */
static emmc_error_enum r1_error_type_check(uint32_t resp);                                   /* check error type for R1 response */
static emmc_error_enum r2_error_check(void);                                                 /* check if error occurs for R2 response */
static emmc_error_enum r3_error_check(void);                                                 /* check if error occurs for R3 response */

/* define emmc information struct */
emmc_info_struct emmc_info={0}; /* emmc information struct */

/*!
    \brief      configure SDIO pins and clock for eMMC interface
    \param[in]  none
    \param[out] none
    \retval     0: configuration successful, 1: configuration failed
*/
static uint8_t sdio_emmc_config(void)
{
    rcu_sdio_clock_config(IDX_SDIO1, RCU_SDIO1SRC_PLL1R);
    rcu_periph_clock_enable(RCU_SDIO1);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOG);
    
    sdio_deinit(SDIO1);
    
    /* configure the SDIO_CLK(PC1), SDIO_CMD(PD7),
                     SDIO_DAT0(PG9), SDIO_DAT1(PG10),
                     SDIO_DAT2(PG11), SDIO_DAT3(PG12),
                     SDIO_DAT4(PB8), SDIO_DAT5(PB9),
                     SDIO_DAT6(PG13), SDIO_DAT7(PG14) */
    gpio_af_set(GPIOB, GPIO_AF_10, GPIO_PIN_8 | GPIO_PIN_9);
    gpio_af_set(GPIOC, GPIO_AF_9, GPIO_PIN_1);
    gpio_af_set(GPIOD, GPIO_AF_11, GPIO_PIN_7);
    gpio_af_set(GPIOG, GPIO_AF_11, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_af_set(GPIOG, GPIO_AF_10, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14);

    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8 | GPIO_PIN_9);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_8 | GPIO_PIN_9);

    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ,  GPIO_PIN_1);

    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_7);
    
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
                                                         GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
                                                                          GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14);
    
    return 0;
}

/*!
    \brief      initialize eMMC card and configure SDIO interface
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
emmc_error_enum emmc_init(void)
{
    emmc_error_enum status = EMMC_OK;

    sdio_emmc_config();
    
    status = emmc_power_on(); /* configure the initial clock and work voltage */
    
    if(status != EMMC_OK)
    {
        PRINT_ERROR("emmc_power_on(%d)\r\n", status);
        
        return status;
    }
    
    status = emmc_card_init(); /* initialize the card and get CID and CSD of the card */
    
    if(status != EMMC_OK)
    {
        PRINT_ERROR("emmc_card_init(%d)\r\n", status);
        
        return status;
    }
    
    status = emmc_card_select_deselect(EMMC_RCA); /* select deselect a card */
    
    if(status != EMMC_OK)
    {
        PRINT_ERROR("emmc_card_select_deselect(%d)\r\n", status);
        
        return status;
    }
    
    status = emmc_card_extcsd_get(); /* get ext_csd of a emmc card */
    
    if(status != EMMC_OK)
    {
        PRINT_ERROR("emmc_card_extcsd_get(%d)\r\n", status);
        
        return status;
    }

    if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 196)) > 0x01) /* check partition support */
    {
        status = emmc_enter_high_speed_mode(); /* enter high speed mode */
        
        if(status != EMMC_OK)
        {
            PRINT_ERROR("emmc_enter_high_speed_mode(%d)\r\n", status);
            
            return status;
        }
        
        sdio_clock_config(SDIO_EMMC, SDIO_SDIOCLKEDGE_FALLING, SDIO_CLOCKPWRSAVE_DISABLE, EMMC_CLK_DIV_TRANS_HSPEED);
    }
    else
    {
        sdio_clock_config(SDIO_EMMC, SDIO_SDIOCLKEDGE_FALLING, SDIO_CLOCKPWRSAVE_DISABLE, EMMC_CLK_DIV_TRANS_DSPEED);
    }
    
    delay_xms(10);
    
    #if(SDR_BUSMODE_1BIT)
        status = emmc_bus_mode_config(SDIO_BUSMODE_1BIT, SDIO_DATA_RATE_SDR);
    #elif(SDR_BUSMODE_4BIT)
        status = emmc_bus_mode_config(SDIO_BUSMODE_4BIT, SDIO_DATA_RATE_SDR);
    #elif(SDR_BUSMODE_8BIT)
        status = emmc_bus_mode_config(SDIO_BUSMODE_8BIT, SDIO_DATA_RATE_SDR);
    #elif(DDR_BUSMODE_4BIT)
        status = emmc_bus_mode_config(SDIO_BUSMODE_4BIT, SDIO_DATA_RATE_DDR);
    #elif(DDR_BUSMODE_8BIT)
        status = emmc_bus_mode_config(SDIO_BUSMODE_8BIT, SDIO_DATA_RATE_DDR);
    #endif
    
    if(status != EMMC_OK)
    {
        PRINT_ERROR("emmc_bus_mode_config(%d)\r\n", status);
        
        return status;
    }

    status = emmc_card_extcsd_get(); /* update ext_csd of a emmc card */
    
    if(status != EMMC_OK)
    {
        PRINT_ERROR("emmc_card_extcsd_get(%d)\r\n", status);
        
        return status;
    }
    
    emmc_info.emmc_init_state = 0xAA; /* card already initialized */
    
    emmc_info_print();
    
    return status;
}

/*!
    \brief      print eMMC card information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void emmc_info_print(void)
{
    uint8_t temp[7];
    uint8_t i = 0;
    
    PRINT_INFO("print emmc information>>\r\n");
    PRINT_INFO("/*********************************************************************/\r\n");
    PRINT_INFO("emmc ocr: 0x%08X\r\n", emmc_info.ocr);
    PRINT_INFO("emmc cid: 0x%08X%08X%08X%08X\r\n", emmc_info.cid[3], emmc_info.cid[2], \
                                               emmc_info.cid[1], emmc_info.cid[0]);
    PRINT_INFO("emmc csd: 0x%08X%08X%08X%08X\r\n", emmc_info.csd[3], emmc_info.csd[2], \
                                               emmc_info.csd[1], emmc_info.csd[0]);
    
    if(emmc_info.ocr & (uint32_t)(1 << 7))
    {
        PRINT_INFO("emmc supports voltage(1.70-1.95V)\r\n");
    }
    
    if(emmc_info.ocr & (uint32_t)(0x7F << 8))
    {
        PRINT_INFO("emmc supports voltage(2.0-2.6V)\r\n");
    }
    
    if(emmc_info.ocr & (uint32_t)(0x1FF << 15))
    {
        PRINT_INFO("emmc supports voltage(2.7-3.6V)\r\n");
    }
    
    if((emmc_info.ocr & (uint32_t)(0x3 << 29)) == 0)
    {
        PRINT_INFO("emmc supports byte access mode\r\n");
    }
    else if((emmc_info.ocr & (uint32_t)(0x3 << 29)) == (uint32_t)(0x2 << 29))
    {
        PRINT_INFO("emmc supports sector access mode\r\n");
    }
    
    PRINT_INFO("emmc device id: 0x%02X\r\n", (uint8_t)(emmc_info.cid[3]>>24));
    
    temp[0] = (uint8_t)emmc_info.cid[3];
    temp[1] = (uint8_t)(emmc_info.cid[2] >> 24);
    temp[2] = (uint8_t)(emmc_info.cid[2] >> 16);
    temp[3] = (uint8_t)(emmc_info.cid[2] >> 8);
    temp[4] = (uint8_t)emmc_info.cid[2];
    temp[5] = (uint8_t)(emmc_info.cid[1] >> 24);
    temp[6] = '\0';

    PRINT_INFO("emmc name: %s\r\n", temp);
    PRINT_INFO("emmc serial number: 0x%08X\r\n", *(uint32_t *)((uint8_t *)&emmc_info.cid[0]+2));
    PRINT_INFO("emmc supports command class: class ");
    
    for(i = 0; i < 12; i++)
    {
        if((emmc_info.csd[2] >> (20 + i)) & 0x1)
        {
            if(i != 0)
            {
                PRINT(", ");
            }
            PRINT("%d", i);
        }
    }
    
    PRINT("\r\n");
    PRINT_INFO("emmc read data block length: %d byte\r\n", 1 << ((uint8_t)(emmc_info.csd[2] >> 16) & 0x0F));
    PRINT_INFO("emmc write data block length: %d byte\r\n", 1 << ((uint8_t)(emmc_info.csd[0] >> 22) & 0x0F));
    
    if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd+503)) & 0x1)
    {
        PRINT_INFO("emmc supports HPI\r\n");
        
        if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd+503)) & 0x2)
        {
             PRINT_INFO("emmc HPI mechanism implementation based on CMD12\r\n");
        }
        else
        {
            PRINT_INFO("emmc HPI mechanism implementation based on CMD13\r\n");
        }
    }
    
    if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd+502)) & 0x1)
    {
        PRINT_INFO("emmc supports background operations (level: %d)\r\n", (*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 246)) & 0x3);
    }
    
    PRINT_INFO("emmc sector number programmed correctly by the last CMD25: %u\r\n", (*(uint32_t *)((uint8_t *)&emmc_info.ext_csd + 242)));
    PRINT_INFO("emmc boot information: 0x%02X\r\n", (*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 228)));
    
    if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 228)) & 0x2)
    {
        PRINT_INFO("emmc supports dual data rate during boot\r\n");
    }
    
    if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 228)) & 0x4)
    {
        PRINT_INFO("emmc supports high speed timing during boot\r\n");
    }
    
    PRINT_INFO("emmc sector number: %u\r\n", (*(uint32_t *)((uint8_t *)&emmc_info.ext_csd + 212)));
    PRINT_INFO("emmc capacity: %.2f GB\r\n", (double)(*(uint32_t *)((uint8_t *)&emmc_info.ext_csd + 212)) *512 /1024 /1024 /1024);
    
    if(*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 196))
    {
        PRINT_INFO("emmc card type: High-Speed MultiMediaCard");
        
        if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 196)) & 0x1)PRINT(" @ 26MHz");
        if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 196)) & 0x2)PRINT(" @ 52MHz");
        if((*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 196)) & 0x4)PRINT(" @ 52MHz(DDR)");
        
        PRINT("\r\n");
    }
    
    if(*(uint8_t *)((uint8_t *)&emmc_info.ext_csd + 185))
    {
        PRINT_INFO("emmc card in high speed mode\r\n");
    }
  
    PRINT_INFO("/*********************************************************************/\r\n");
}

/*!
    \brief      select or deselect eMMC card
    \param[in]  cardrca: card relative address (address 0 deselects the card)
    \param[out] none
    \retval     emmc_error_enum: error status
*/
emmc_error_enum emmc_card_select_deselect(uint16_t cardrca)
{
    emmc_error_enum status = EMMC_OK;
    
    status = emmc_card_state_get();
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    if(emmc_info.card_locked_state)
    {
        return EMMC_LOCKED_STSTE;
    }
    
    /* send CMD7(SELECT/DESELECT_CARD) to select or deselect the card */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_SELECT_DESELECT_CARD, (uint32_t)cardrca << EMMC_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC);

    status = r1_error_check(EMMC_CMD_SELECT_DESELECT_CARD);
    
    if(EMMC_OK != status)
    {
        return status;
    }
       
    if(cardrca)
    {
        while(status == EMMC_OK)
        {
            status = emmc_card_state_get();
            
            if((emmc_info.card_state == EMMC_CARDSTATE_TRANSFER) || (emmc_info.card_state == EMMC_CARDSTATE_PROGRAMMING))
            {
                break;
            }
        }
    }
    
    return status;
}

/*!
    \brief      get eMMC card current state
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
emmc_error_enum emmc_card_state_get(void)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t state = 0;
    
    /* send CMD13(SEND_STATUS), addressed card sends its status registers */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_SEND_STATUS, (uint32_t)EMMC_RCA << EMMC_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC);
    
    status = r1_error_check(EMMC_CMD_SEND_STATUS); /* check if some error occurs */
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    state = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE0);
    emmc_info.card_state = (uint8_t)(state  >> 9);
    emmc_info.card_locked_state = (state & EMMC_R1_CARD_IS_LOCKED) ? 0x01 : 0x00;
    emmc_info.card_ready_state = (state & EMMC_R1_READY_FOR_DATA) ? 0x01 : 0x00;
  
    return status;
}

/*!
    \brief      configure eMMC bus mode and data rate
    \param[in]  busmode: bus mode (SDIO_BUSMODE_1BIT/4BIT/8BIT)
    \param[in]  datarate: data rate mode (SDIO_DATA_RATE_SDR/DDR)
    \param[out] none
    \retval     emmc_error_enum: error status
*/
emmc_error_enum emmc_bus_mode_config(uint32_t busmode, uint32_t datarate)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t cmd_argument = 0x03B70000;
    uint32_t sdio_bus_speed = 0;
    uint32_t sdio_receive_clock = 0;
    
    status = emmc_card_state_get();
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    if(emmc_info.card_locked_state)
    {
        return EMMC_LOCKED_STSTE;
    }

    if(busmode == SDIO_BUSMODE_1BIT)
    {
        cmd_argument = 0x03B70000;
        sdio_bus_speed = SDIO_BUSSPEED_LOW;
        sdio_receive_clock = SDIO_RECEIVECLOCK_INCLK;
    }
    else if((busmode == SDIO_BUSMODE_4BIT) && (datarate == SDIO_DATA_RATE_SDR))
    {
        cmd_argument = 0x03B70100;
        sdio_bus_speed = SDIO_BUSSPEED_HIGH;
        sdio_receive_clock = SDIO_RECEIVECLOCK_INCLK;
    }
    else if((busmode == SDIO_BUSMODE_8BIT) && (datarate == SDIO_DATA_RATE_SDR))
    {
        cmd_argument = 0x03B70200;
        sdio_bus_speed = SDIO_BUSSPEED_HIGH;
        sdio_receive_clock = SDIO_RECEIVECLOCK_FBCLK;
    }
    else if((busmode == SDIO_BUSMODE_4BIT) && (datarate == SDIO_DATA_RATE_DDR))
    {
        cmd_argument = 0x03B70500;
        sdio_bus_speed = SDIO_BUSSPEED_HIGH;
        sdio_receive_clock = SDIO_RECEIVECLOCK_FBCLK;
    }
    else if((busmode == SDIO_BUSMODE_8BIT) && (datarate == SDIO_DATA_RATE_DDR))
    {
        cmd_argument = 0x03B70600;
        sdio_bus_speed = SDIO_BUSSPEED_HIGH;
        sdio_receive_clock = SDIO_RECEIVECLOCK_FBCLK;
    }
    else
    {
        status = EMMC_PARAMETER_INVALID;
        
        return status;
    }
    
    /* send CMD6(EMMC_CMD_SWITCH) to set the width */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_SWITCH_FUNC, (uint32_t)cmd_argument, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC); /* enable the CSM */
 
    status = r1_error_check(EMMC_CMD_SWITCH_FUNC); /* check if some error occurs */
    
    if(EMMC_OK != status)     
    {
        return status;
    }
    
    sdio_bus_mode_set(SDIO_EMMC, busmode);
    sdio_data_rate_set(SDIO_EMMC, datarate);
    sdio_bus_speed_set(SDIO_EMMC, sdio_bus_speed);
    sdio_clock_receive_set(SDIO_EMMC, sdio_receive_clock);
    sdio_hardware_clock_enable(SDIO_EMMC);

    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_INTC_FLAGS);
    
    status = emmc_card_state_get();
    
    while((status == EMMC_OK) && (emmc_info.card_state == EMMC_CARDSTATE_PROGRAMMING))
    {
        status = emmc_card_state_get();
    }
    
    return status;
}

/*!
    \brief      read data from eMMC disk
    \param[in]  pbuffer: pointer to buffer for data reading
    \param[in]  blockaddr: start block address
    \param[in]  blocksnumber: number of blocks to read
    \param[out] none
    \retval     emmc_error_enum: error status
*/
emmc_error_enum emmc_read_disk(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber)
{
    emmc_error_enum status = EMMC_OK;
    
    if((pbuffer == NULL) || ((uint32_t)pbuffer % 4)) /* buffer address invalid or not 4-byte aligned */
    {
        status = EMMC_PARAMETER_INVALID;
        
        return status;
    }
    
    if(blocksnumber == 1)
    {
        status = emmc_block_read(pbuffer, blockaddr);
    }
    else
    {
         status = emmc_multiblocks_read(pbuffer, blockaddr, blocksnumber);
    }
    
    return status;
}

/*!
    \brief      write data to eMMC disk
    \param[in]  pbuffer: pointer to buffer containing data to write
    \param[in]  blockaddr: start block address
    \param[in]  blocksnumber: number of blocks to write
    \param[out] none
    \retval     emmc_error_enum: error status
*/
emmc_error_enum emmc_write_disk(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber)
{
    emmc_error_enum status = EMMC_OK;
    
    if((pbuffer == NULL) || ((uint32_t)pbuffer % 4)) /* buffer address invalid or not 4-byte aligned */
    {
        status = EMMC_PARAMETER_INVALID;
        
        return status;
    }
    
    if(blocksnumber == 1)
    {
        status = emmc_block_write(pbuffer, blockaddr);
    }
    else
    {
         status = emmc_multiblocks_write(pbuffer, blockaddr, blocksnumber);
    }
    return status;
}

/*!
    \brief      read single block from eMMC card
    \param[in]  pbuffer: pointer to buffer for data reading
    \param[in]  blockaddr: block address to read
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_block_read(uint32_t *pbuffer, uint32_t blockaddr)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t count = 0U;

    if(NULL == pbuffer)
    {
        status = EMMC_PARAMETER_INVALID;
        
        return status;
    }
   
    status = emmc_card_state_get(); /* check whether the card is locked */
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    if(emmc_info.card_locked_state)
    {
        return EMMC_LOCKED_STSTE;
    }
    
    /* configure SDIO data transmisson */
    sdio_data_config(SDIO_EMMC, EMMC_DATATIMEOUT, 512, SDIO_DATABLOCKSIZE_512BYTES);
    sdio_data_transfer_config(SDIO_EMMC, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
    sdio_trans_start_enable(SDIO_EMMC);
    
    #if(EMMC_DMA_MODE)
        sdio_idma_set(SDIO_EMMC, SDIO_IDMA_SINGLE_BUFFER, (uint32_t)(512 >> 5));
        sdio_idma_buffer0_address_set(SDIO_EMMC, (uint32_t)pbuffer);
        sdio_idma_enable(SDIO_EMMC);
    
        /* send CMD17(READ_SINGLE_BLOCK) to read a block */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_READ_SINGLE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_READ_SINGLE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }
        
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTEND));
        
        sdio_idma_disable(SDIO_EMMC);
        sdio_trans_start_disable(SDIO_EMMC);
        
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RXORE))
        {
            status = EMMC_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_RXORE);
            
            return status;
        }
        else
        {
            /* if else end */
        }
        
    #else
        /* send CMD17(READ_SINGLE_BLOCK) to read a block */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_READ_SINGLE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_READ_SINGLE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }

        /* polling mode */
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTEND))
        {
            if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RFH))
            {
                for(count = 0U; count < EMMC_FIFOHALF_WORDS; count++)/* at least 8 words can be read in the FIFO */
                {
                    *(pbuffer + count) = sdio_data_read(SDIO_EMMC);
                }
                pbuffer += EMMC_FIFOHALF_WORDS;
            }
        }
        
        sdio_trans_start_disable(SDIO_EMMC);
        
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RXORE))
        {
            status = EMMC_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_RXORE);
            
            return status;
        }
        else
        {
            /* if else end */
        }
        
        while((SET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RFE)) && (SET == sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DATSTA)))
        {
            *pbuffer = sdio_data_read(SDIO_EMMC);
            ++pbuffer;
        }
    #endif

    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_DATA_FLAGS); /* clear the DATA_FLAGS flags */
        
    return status;
}

/*!
    \brief      read multiple blocks from eMMC card
    \param[in]  pbuffer: pointer to buffer for data reading
    \param[in]  blockaddr: start block address
    \param[in]  blocksnumber: number of blocks to read
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_multiblocks_read(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t count = 0U;

    if((NULL == pbuffer) || (blocksnumber * 512 > EMMC_MAX_DATA_LENGTH))
    {
        status = EMMC_PARAMETER_INVALID;
        
        return status;
    }
   
    status = emmc_card_state_get(); /* check whether the card is locked */
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    if(emmc_info.card_locked_state)
    {
        return EMMC_LOCKED_STSTE;
    }
    
    /* configure SDIO data transmisson */
    sdio_data_config(SDIO_EMMC, EMMC_DATATIMEOUT, blocksnumber * 512, SDIO_DATABLOCKSIZE_512BYTES);
    sdio_data_transfer_config(SDIO_EMMC, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
    sdio_trans_start_enable(SDIO_EMMC);
    
    #if(EMMC_DMA_MODE)
        sdio_idma_set(SDIO_EMMC, SDIO_IDMA_SINGLE_BUFFER, (uint32_t)(512 >> 5));
        sdio_idma_buffer0_address_set(SDIO_EMMC, (uint32_t)pbuffer);
        sdio_idma_enable(SDIO_EMMC);

        /* send CMD18(READ_MULTIPLE_BLOCK) to read multiple blocks */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_READ_MULTIPLE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_READ_MULTIPLE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }
        
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTEND));
        
        sdio_idma_disable(SDIO_EMMC);
        sdio_trans_start_disable(SDIO_EMMC);
                
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RXORE))
        {
            status = EMMC_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_RXORE);
            
            return status;
        }
        else
        {
            /* if else end */
        }

    #else
        /* send CMD18(READ_MULTIPLE_BLOCK) to read multiple blocks */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_READ_MULTIPLE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_READ_MULTIPLE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }
        
        /* polling mode */
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_DTEND))
        {
            if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RFH))
            {
                for(count = 0U; count < EMMC_FIFOHALF_WORDS; count++)/* at least 8 words can be read in the FIFO */
                {
                    *(pbuffer + count) = sdio_data_read(SDIO_EMMC);  
                }
                pbuffer += EMMC_FIFOHALF_WORDS;
            }
        }

        sdio_trans_start_disable(SDIO_EMMC);
        
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RXORE))
        {
            status = EMMC_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_RXORE);
            
            return status;
        }
        else
        {
            /* if else end */
        }
        
        while((SET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RFE)) && (SET == sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DATSTA)))
        {
            *pbuffer = sdio_data_read(SDIO_EMMC);
            ++pbuffer;
        }
    #endif
    
    status = emmc_transfer_stop();
        
    if(EMMC_OK != status)
    {
        return status;
    }

    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_DATA_FLAGS); /* clear the DATA_FLAGS flags */
        
    return status;
}

/*!
    \brief      write single block to eMMC card
    \param[in]  pbuffer: pointer to buffer containing data to write
    \param[in]  blockaddr: block address to write
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_block_write(uint32_t *pbuffer, uint32_t blockaddr)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t count = 0U, timeout = 100000U;

    if(NULL == pbuffer)
    {
        status = EMMC_PARAMETER_INVALID;
        
        return status;
    }
   
    status = emmc_card_state_get(); /* check whether the card is locked */
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    if(emmc_info.card_locked_state)
    {
        return EMMC_LOCKED_STSTE;
    }
    
    while((0U == emmc_info.card_ready_state) && (timeout > 0U))
    {
        --timeout;
        status = emmc_card_state_get(); /* check whether the card is locked */
    
        if(EMMC_OK != status)
        {
            return status;
        }
    }
    
    if(0U == timeout)
    {
        return EMMC_ERROR;
    }
    
    /* configure SDIO data transmisson */
    sdio_data_config(SDIO_EMMC, EMMC_DATATIMEOUT, 512, SDIO_DATABLOCKSIZE_512BYTES);
    sdio_data_transfer_config(SDIO_EMMC, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_trans_start_enable(SDIO_EMMC);
    
    #if(EMMC_DMA_MODE)
        sdio_idma_set(SDIO_EMMC, SDIO_IDMA_SINGLE_BUFFER, (uint32_t)(512 >> 5));
        sdio_idma_buffer0_address_set(SDIO_EMMC, (uint32_t)pbuffer);
        sdio_idma_enable(SDIO_EMMC);

        /* send CMD24(WRITE_BLOCK) to write a block */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_WRITE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_WRITE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }
        
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTEND));
        
        sdio_idma_disable(SDIO_EMMC);
        sdio_trans_start_disable(SDIO_EMMC);
        
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RXORE))
        {
            status = EMMC_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_RXORE);
            
            return status;
        }
        else
        {
            /* if else end */
        }
        
    #else
        /* send CMD24(WRITE_BLOCK) to write a block */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_WRITE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_WRITE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }

        /* polling mode */
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_TXURE | SDIO_FLAG_DTEND))
        {
            if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_TFH))
            {
                for(count = 0U; count < EMMC_FIFOHALF_WORDS; count++)/* at least 8 words can be read in the FIFO */
                {
                    sdio_data_write(SDIO_EMMC, *(pbuffer + count));
                }
                pbuffer += EMMC_FIFOHALF_WORDS;
            }
        }
        
        sdio_trans_start_disable(SDIO_EMMC);
        
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_TXURE))
        {
            status = EMMC_TX_UNDERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_TXURE);
            
            return status;
        }
        else
        {
            /* if else end */
        }

    #endif

    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_DATA_FLAGS); /* clear the DATA_FLAGS flags */
    
    status = emmc_card_state_get();
    
    while((status == EMMC_OK) && ((emmc_info.card_state == EMMC_CARDSTATE_PROGRAMMING) || (emmc_info.card_state == EMMC_CARDSTATE_RECEIVING)))
    {
        status = emmc_card_state_get();
    }
        
    return status;
}

/*!
    \brief      write multiple blocks to eMMC card
    \param[in]  pbuffer: pointer to buffer containing data to write
    \param[in]  blockaddr: start block address
    \param[in]  blocksnumber: number of blocks to write
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_multiblocks_write(uint32_t *pbuffer, uint32_t blockaddr, uint32_t blocksnumber)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t count = 0U, timeout = 100000U;

    if((NULL == pbuffer) || (blocksnumber * 512 > EMMC_MAX_DATA_LENGTH))
    {
        status = EMMC_PARAMETER_INVALID;
        
        return status;
    }
   
    status = emmc_card_state_get(); /* check whether the card is locked */
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    if(emmc_info.card_locked_state)
    {
        return EMMC_LOCKED_STSTE;
    }
    
    while((0U == emmc_info.card_ready_state) && (timeout > 0U))
    {
        --timeout;
        status = emmc_card_state_get(); /* check whether the card is locked */
    
        if(EMMC_OK != status)
        {
            return status;
        }
    }
    
    if(0U == timeout)
    {
        return EMMC_ERROR;
    }
    
    /* send CMD23(EMMC_SET_BLOCK_COUNT) to set the number of write blocks to be preerased before writing */
    sdio_command_response_config(SDIO_EMMC, EMMC_SET_BLOCK_COUNT, blocksnumber, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC);
    
    status = r1_error_check(EMMC_SET_BLOCK_COUNT); /* check if some error occurs */
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    /* configure SDIO data transmisson */
    sdio_data_config(SDIO_EMMC, EMMC_DATATIMEOUT, blocksnumber * 512, SDIO_DATABLOCKSIZE_512BYTES);
    sdio_data_transfer_config(SDIO_EMMC, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOCARD);
    sdio_trans_start_enable(SDIO_EMMC);
    
    #if(EMMC_DMA_MODE)
        sdio_idma_set(SDIO_EMMC, SDIO_IDMA_SINGLE_BUFFER, (uint32_t)(512 >> 5));
        sdio_idma_buffer0_address_set(SDIO_EMMC, (uint32_t)pbuffer);
        sdio_idma_enable(SDIO_EMMC);

        /* send CMD25(WRITE_MULTIPLE_BLOCK) to continuously write blocks of data */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_WRITE_MULTIPLE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_WRITE_MULTIPLE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }
        
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTEND));
        
        sdio_idma_disable(SDIO_EMMC);
        sdio_trans_start_disable(SDIO_EMMC);
        
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RXORE))
        {
            status = EMMC_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_RXORE);
            
            return status;
        }
        else
        {
            /* if else end */
        }
        
    #else
        /* send CMD25(WRITE_MULTIPLE_BLOCK) to continuously write blocks of data */
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_WRITE_MULTIPLE_BLOCK, (uint32_t)blockaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC);
        
        status = r1_error_check(EMMC_CMD_WRITE_MULTIPLE_BLOCK); /* check if some error occurs */
    
        if(EMMC_OK != status)
        {
            return status;
        }

        /* polling mode */
        while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_TXURE | SDIO_FLAG_DTEND))
        {
            if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_TFH))
            {
                for(count = 0U; count < EMMC_FIFOHALF_WORDS; count++)/* at least 8 words can be read in the FIFO */
                {
                    sdio_data_write(SDIO_EMMC, *(pbuffer + count));
                }
                pbuffer += EMMC_FIFOHALF_WORDS;
            }
        }
        
        sdio_trans_start_disable(SDIO_EMMC);
        
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR))/* whether some error occurs and return it */
        {
            status = EMMC_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
        {
            status = EMMC_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
            
            return status;
        }
        else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_TXURE))
        {
            status = EMMC_TX_UNDERRUN_ERROR;
            sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_TXURE);
            
            return status;
        }
        else
        {
            /* if else end */
        }

    #endif

    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_DATA_FLAGS); /* clear the DATA_FLAGS flags */
    
    status = emmc_card_state_get();
    
    while((status == EMMC_OK) && ((emmc_info.card_state == EMMC_CARDSTATE_PROGRAMMING) || (emmc_info.card_state == EMMC_CARDSTATE_RECEIVING)))
    {
        status = emmc_card_state_get();
    }
        
    return status;
}

/*!
    \brief      power on eMMC card and initialize interface
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_power_on(void)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t response = 0U;
    uint32_t timedelay = 0U;

    /* configure the SDIO peripheral */
    sdio_clock_config(SDIO_EMMC, SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKPWRSAVE_DISABLE, EMMC_CLK_DIV_INIT);
    sdio_bus_mode_set(SDIO_EMMC, SDIO_BUSMODE_1BIT);
    sdio_hardware_clock_disable(SDIO_EMMC);
    sdio_power_state_set(SDIO_EMMC, SDIO_POWER_ON);

    timedelay = 500U; /* time delay for power up*/
    while(timedelay > 0U)
    {
        timedelay--;
    }

    /* send CMD0(GO_IDLE_STATE) to reset the card */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_GO_IDLE_STATE, (uint32_t)0x0, SDIO_RESPONSETYPE_NO);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC); /* enable the CSM */

    status = cmdsent_error_check();
    
    if(EMMC_OK != status) /* check if command sent error occurs */
    {
        return status;
    }

    timedelay = 0x4000U;
    
    while(timedelay--)
    {
        sdio_command_response_config(SDIO_EMMC, EMMC_CMD_SEND_OP_COND, (uint32_t)0x40FF8000, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
        sdio_csm_enable(SDIO_EMMC); /* enable the CSM */

        status = r3_error_check();
        
        if(EMMC_OK != status) /* check if some error occurs */
        {
            return status;
        }
        
        if(!timedelay)
        {
            status = EMMC_ERROR;
            
            return status;
        }

        response = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE0); /* get the response and check card power up status bit(busy) */
        
        if(0x80000000 == (response & 0x80000000))   /* OCR register bit 31 will be set when the device is ready */
        {
            break;
        }
        else
        {
            continue;
        }
    }
    
    emmc_info.ocr = response; /* store R3 response */
    
    return status;
}

/*!
    \brief      power off eMMC card and disable SDIO interface
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_power_off(void)
{
    emmc_error_enum status = EMMC_OK;
    
    sdio_power_state_set(SDIO_EMMC, SDIO_POWER_OFF);
    
    return status;
}

/*!
    \brief      initialize eMMC card and get CID and CSD information
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_card_init(void)
{
    emmc_error_enum status = EMMC_OK;

    if(SDIO_POWER_OFF == sdio_power_state_get(SDIO_EMMC))
    {
        status = EMMC_OPERATION_IMPROPER;
        
        return status;
    }

    /* send CMD2(EMMC_CMD_ALL_SEND_CID) to get the CID numbers */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_ALL_SEND_CID, (uint32_t)0x0, SDIO_RESPONSETYPE_LONG);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC); /* enable the CSM */
    
    status = r2_error_check(); /* check if some error occurs */

    if(EMMC_OK != status)
    {
        return status;
    }

    /* store the CID numbers */
    emmc_info.cid[3] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE0); /* MSB */
    emmc_info.cid[2] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE1);
    emmc_info.cid[1] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE2);
    emmc_info.cid[0] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE3); /* LSB */

    /* send CMD3(SEND_RELATIVE_ADDR) to ask the eMMC to publish a new relative address (RCA) */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_SEND_RELATIVE_ADDR, (uint32_t)(EMMC_RCA << EMMC_RCA_SHIFT), SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC); /* enable the CSM */
    
    status = r1_error_check(EMMC_CMD_SEND_RELATIVE_ADDR); /* check if some error occurs */
    
    if(EMMC_OK != status)
    {
        return status;
    }

    /* send CMD9(SEND_CSD) to get the addressed card's card-specific data (CSD) */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_SEND_CSD, (uint32_t)(EMMC_RCA << EMMC_RCA_SHIFT), SDIO_RESPONSETYPE_LONG);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC); /* enable the CSM */
    
    status = r2_error_check();/* check if some error occurs */
    
    if(EMMC_OK != status)
    {
        return status;
    }
    
    /* store the card-specific data (CSD) */
    emmc_info.csd[3] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE0); /* MSB */
    emmc_info.csd[2] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE1);
    emmc_info.csd[1] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE2);
    emmc_info.csd[0] = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE3); /* LSB */

    return status;
}

/*!
    \brief      get eMMC extended CSD register information
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_card_extcsd_get(void)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t count = 0U;
    uint32_t *ptempbuff = emmc_info.ext_csd;

    /* configure SDIO data transmisson */
    sdio_data_config(SDIO_EMMC, EMMC_DATATIMEOUT, (uint32_t)512, SDIO_DATABLOCKSIZE_512BYTES);
    sdio_data_transfer_config(SDIO_EMMC, SDIO_TRANSMODE_BLOCKCOUNT, SDIO_TRANSDIRECTION_TOSDIO);
    sdio_trans_start_enable(SDIO_EMMC);

    /* send CMD8(READ_SINGLE_BLOCK) to read a block */
    sdio_command_response_config(SDIO_EMMC, (uint32_t)EMMC_CMD_SEND_EXT_CSD, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC);
    
    status = r1_error_check(EMMC_CMD_SEND_EXT_CSD); /* check if some error occurs */
    
    if(EMMC_OK != status)
    {
        return status;
    }

    while(!sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_DTEND))/* polling mode */
    {
        if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RFH)) /* at least 8 words can be read in the FIFO */
        {     
            for(count = 0U; count < EMMC_FIFOHALF_WORDS; count++)
            {
                *(ptempbuff + count) = sdio_data_read(SDIO_EMMC); /*get EXT_CSD register data */
            }
            
            ptempbuff += EMMC_FIFOHALF_WORDS;
        }
    }
    
    sdio_trans_start_disable(SDIO_EMMC);
    
    if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTCRCERR)) /* whether some error occurs and return it */
    {
        status = EMMC_DATA_CRC_ERROR;      
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTCRCERR);
        
        return status;
    }
    else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_DTTMOUT))
    {
        status = EMMC_DATA_TIMEOUT;
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_DTTMOUT);
        
        return status;

    }
    else if(RESET != sdio_flag_get(SDIO_EMMC, SDIO_FLAG_RXORE))
    {
        status = EMMC_RX_OVERRUN_ERROR;
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_RXORE);
        
        return status;
    }
    
    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_INTC_FLAGS); /* clear the SDIO_INTC flags */
    
    return status;
}

/*!
    \brief      enter eMMC high speed mode
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_enter_high_speed_mode(void)
{
    emmc_error_enum status = EMMC_OK;

    /* send CMD6(EMMC_CMD_SWITCH) to switch to high-speed mode */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_SWITCH_FUNC, (uint32_t)0x03B90100, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_csm_enable(SDIO_EMMC); /* enable the CSM */

    status = r1_error_check(EMMC_CMD_SWITCH_FUNC); /* check if some error occurs */
    
    if(EMMC_OK != status)
    {
        return status;
    }

    return status;
}

/*!
    \brief      stop ongoing data transfer
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum emmc_transfer_stop(void)
{
    emmc_error_enum status = EMMC_OK;
    
    /* send CMD12(STOP_TRANSMISSION) to stop transmission */
    sdio_command_response_config(SDIO_EMMC, EMMC_CMD_STOP_TRANSMISSION, (uint32_t)0x0U, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_EMMC, SDIO_WAITTYPE_NO);
    sdio_trans_stop_enable(SDIO_EMMC);
    sdio_csm_enable(SDIO_EMMC); /* enable the CSM */
    
    status = r1_error_check(EMMC_CMD_STOP_TRANSMISSION); /* check if some error occurs */
    sdio_trans_stop_disable(SDIO_EMMC);
    
    return status;
}

/*!
    \brief      check if command sent error occurs
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum cmdsent_error_check(void)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t timeout = 100000U;
    
    while((RESET == sdio_flag_get(SDIO_EMMC, SDIO_FLAG_CMDSEND)) && (timeout > 0U)) /* check command sent flag */
    {
        --timeout;
    }
    
    if(0U == timeout) /* command response is timeout */
    {
        status = EMMC_CMD_RESP_TIMEOUT;
        
        return status;
    }
    
    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_CMD_FLAGS); /* if the command is sent, clear the CMD_FLAGS flags */
    
    return status;
}

/*!
    \brief      check if error occurs for R1 response
    \param[in]  cmdindex: command index
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum r1_error_check(uint8_t cmdindex)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t reg_status = 0U, resp_r1 = 0U;

    reg_status = SDIO_STAT(SDIO_EMMC); /* store the content of SDIO_STAT */
    
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO_EMMC);
    }
    
    if(reg_status & SDIO_FLAG_CCRCERR) /* check whether an error or timeout occurs or command response received */
    {
        status = EMMC_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_CCRCERR);
        
        return status;
    }
    else if(reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = EMMC_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_CMDTMOUT);
        
        return status;
    }
    else
    {
        /* if else end */
    }

    if(sdio_command_index_get(SDIO_EMMC) != cmdindex) /* check whether the last response command index is the desired one */
    {
        status = EMMC_ILLEGAL_COMMAND;
        
        return status;
    }
    
    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_CMD_FLAGS); /* clear all the CMD_FLAGS flags */
    
    resp_r1 = sdio_response_get(SDIO_EMMC, SDIO_RESPONSE0); /* get the SDIO response register 0 for checking */
    
    if(EMMC_ALLZERO == (resp_r1 & EMMC_R1_ERROR_BITS)) /* no error occurs, return EMMC_OK */
    {
        status = EMMC_OK;
        
        return status;
    }

    status = r1_error_type_check(resp_r1);/* if some error occurs, return the error type */
    
    return status;
}

/*!
    \brief      check error type for R1 response
    \param[in]  resp: R1 response value
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum r1_error_type_check(uint32_t resp)
{
    emmc_error_enum status = EMMC_ERROR;
    
    if(resp & EMMC_R1_OUT_OF_RANGE)/* check which error occurs */
    {
        status = EMMC_OUT_OF_RANGE;
    }
    else if(resp & EMMC_R1_ADDRESS_ERROR)
    {
        status = EMMC_ADDRESS_ERROR;
    }
    else if(resp & EMMC_R1_BLOCK_LEN_ERROR)
    {
        status = EMMC_BLOCK_LEN_ERROR;
    }
    else if(resp & EMMC_R1_ERASE_SEQ_ERROR)
    {
        status = EMMC_ERASE_SEQ_ERROR;
    }
    else if(resp & EMMC_R1_ERASE_PARAM)
    {
        status = EMMC_ERASE_PARAM;
    }
    else if(resp & EMMC_R1_WP_VIOLATION)
    {
        status = EMMC_WP_VIOLATION;
    }
    else if(resp & EMMC_R1_LOCK_UNLOCK_FAILED)
    {
        status = EMMC_LOCK_UNLOCK_FAILED;
    }
    else if(resp & EMMC_R1_COM_CRC_ERROR)
    {
        status = EMMC_COM_CRC_ERROR;
    }
    else if(resp & EMMC_R1_ILLEGAL_COMMAND)
    {
        status = EMMC_ILLEGAL_COMMAND;
    }
    else if(resp & EMMC_R1_CARD_ECC_FAILED)
    {
        status = EMMC_CARD_ECC_FAILED;
    }
    else if(resp & EMMC_R1_CC_ERROR)
    {
        status = EMMC_CC_ERROR;
    }
    else if(resp & EMMC_R1_GENERAL_ERROR)
    {
        status = EMMC_GENERAL_ERROR;
    }
    else if(resp & EMMC_R1_UNDERRUN)
    {
        status = EMMC_UNDERRUN;
    } 
    else if(resp & EMMC_R1_OVERRUN)
    {
        status = EMMC_OVERRUN; 
    }
    else if(resp & EMMC_R1_CSD_OVERWRITE)
    {
        status = EMMC_CSD_OVERWRITE;
    }
    else if(resp & EMMC_R1_WP_ERASE_SKIP)
    {
        status = EMMC_WP_ERASE_SKIP;
    }
    else if(resp & EMMC_R1_ERASE_RESET)
    {
        status = EMMC_ERASE_RESET;
    }
    else if(resp & EMMC_R1_SWITCH_ERROR)
    {
        status = EMMC_SWITCH_ERROR;
    }
    else
    {
        /*no todo,  if else end */
    }
    return status;
}

/*!
    \brief      check if error occurs for R2 response
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum r2_error_check(void)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t reg_status = 0U;

    reg_status = SDIO_STAT(SDIO_EMMC); /* store the content of SDIO_STAT */
    
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO_EMMC);
    }
    
    if(reg_status & SDIO_FLAG_CCRCERR) /* check whether an error or timeout occurs or command response received */
    {
        status = EMMC_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_CCRCERR);
        
        return status;
    }
    else if(reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = EMMC_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_CMDTMOUT);
        
        return status;
    }
    else
    {
        /* if else end */
    }
    
    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_CMD_FLAGS);/* clear all the CMD_FLAGS flags */

    return status;
}

/*!
    \brief      check if error occurs for R3 response
    \param[in]  none
    \param[out] none
    \retval     emmc_error_enum: error status
*/
static emmc_error_enum r3_error_check(void)
{
    emmc_error_enum status = EMMC_OK;
    uint32_t reg_status = 0U;

    reg_status = SDIO_STAT(SDIO_EMMC); /* store the content of SDIO_STAT */
    
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
    {
        reg_status = SDIO_STAT(SDIO_EMMC);
    }
    
    if(reg_status & SDIO_FLAG_CMDTMOUT)
    {
        status = EMMC_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_EMMC, SDIO_FLAG_CMDTMOUT);
        
        return status;
    }
    
    sdio_flag_clear(SDIO_EMMC, SDIO_MASK_CMD_FLAGS); /* clear all the CMD_FLAGS flags */
    
    return status;
}























