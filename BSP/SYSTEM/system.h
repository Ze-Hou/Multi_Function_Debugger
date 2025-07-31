/*!
    \file       system.h
    \brief      system information management and hardware configuration header file
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef __SYSTEM_H
#define __SYSTEM_H
#include <stdint.h>

/*!
    \brief      OS support configuration
                0: bare-metal mode
                1: FreeRTOS support mode
*/
#define SYSTEM_SUPPORT_OS 1

/* DWT (Data Watchpoint and Trace) register definitions for precise timing */
#define  DWT_CYCCNT  *(volatile unsigned int *)0xE0001004    /*!< cycle counter register */
#define  DWT_CR      *(volatile unsigned int *)0xE0001000    /*!< control register */
#define  DEM_CR      *(volatile unsigned int *)0xE000EDFC    /*!< debug exception and monitor control register */
#define  DBGMCU_CR   *(volatile unsigned int *)0xE0042004    /*!< debug MCU configuration register */
	

/* TCM RAM base address definitions */
#ifndef ITCMRAM_BASE
    #define ITCMRAM_BASE           ((uint32_t)0x00000000)    /*!< instruction TCM RAM base address */
#endif /* ITCMRAM_BASE */

#ifndef DTCMRAM_BASE
    #define DTCMRAM_BASE           ((uint32_t)0x20000000)    /*!< data TCM RAM base address */
#endif /* DTCMRAM_BASE */

/*!
    \brief RCU clock frequency information structure
*/
typedef struct
{
    uint32_t sys_ck;                                        /*!< system clock frequency */
    uint32_t ahb_ck;                                        /*!< AHB clock frequency */
    uint32_t apb1_ck;                                       /*!< APB1 clock frequency */
    uint32_t apb2_ck;                                       /*!< APB2 clock frequency */
    uint32_t apb3_ck;                                       /*!< APB3 clock frequency */
    uint32_t apb4_ck;                                       /*!< APB4 clock frequency */
} system_rcu_clock_freq_struct;

/*!
    \brief system device information structure
*/
typedef struct
{
    uint16_t memory_flash;                                  /*!< flash memory size in KB */
    uint16_t memory_sram;                                   /*!< SRAM memory size in KB */
    uint32_t device_id[3];                                  /*!< unique device identifier */
    uint32_t device_pid;                                    /*!< product identifier */
    uint32_t boot_address;                                  /*!< boot address */
    uint8_t boot_scr;                                       /*!< boot security configuration */
    uint8_t boot_spc;                                       /*!< boot security protection level */
    uint16_t share_sram_itcm;                               /*!< ITCM shared SRAM size in KB */
    uint16_t share_sram_dtcm;                               /*!< DTCM shared SRAM size in KB */
    uint16_t share_sram_sram;                               /*!< remaining SRAM size in KB */
    system_rcu_clock_freq_struct rcu_clock_freq;            /*!< RCU clock frequency information */
} system_device_struct;

/*! global system device information variable */
extern system_device_struct system_device_info;

/* function declarations */
void system_info_get(void);                                                     /*!< get complete system information */
void system_info_print(void);                                                   /*!< print system information to USART */
void system_cache_enable(void);                                                 /*!< enable the CPU cache */
void system_nvic_vector_table_config(uint32_t nvic_vict_tab, uint32_t offset);  /*!< configure NVIC vector table location */
void system_fwdgt_init(void);                                                   /*!< initialize free watchdog timer */
void system_dwt_init(void);                                                     /*!< initialize DWT for cycle counting */
void system_rcu_peripheral_clock_config(void);                                  /*!< configure peripheral clock sources (PLL1/PLL2) */
#endif
