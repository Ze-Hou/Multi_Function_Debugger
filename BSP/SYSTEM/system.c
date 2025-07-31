/*!
    \file       system.c
    \brief      system information management and hardware configuration
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./SYSTEM/system.h"
#include "./USART/usart.h"

system_device_struct    system_device_info;

/*!
    \brief      get system device information
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_device_info_get(void)
{
    uint32_t temp=0;
    temp = *((volatile uint32_t *)0x1FF0F7E0);                              /* read memory information */
    system_device_info.memory_flash = (uint16_t)(temp>>16);
    system_device_info.memory_sram = (uint16_t)(temp);
    system_device_info.device_id[0] = *((volatile uint32_t *)0x1FF0F7E8);   /* read id information */
    system_device_info.device_id[1] = *((volatile uint32_t *)0x1FF0F7EC);
    system_device_info.device_id[2] = *((volatile uint32_t *)0x1FF0F7F0);
    fmc_pid_get(&system_device_info.device_pid);
    system_device_info.boot_address = ob_boot_address_get(BOOT_PIN_0);
    temp = *((volatile uint32_t *)(0x52002000+0x1C));                       /* read boot mode */
    system_device_info.boot_scr = (uint8_t)(temp>>16)&0x10;
    system_device_info.boot_spc = (uint8_t)(temp>>8);
    ob_tcm_shared_ram_size_get((uint32_t *)&system_device_info.share_sram_itcm, (uint32_t *)&system_device_info.share_sram_dtcm);
    system_device_info.share_sram_sram = 512 - (system_device_info.share_sram_itcm + system_device_info.share_sram_dtcm);
}

/*!
    \brief      get system RCU clock frequency
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_rcu_clock_freq_get(void)
{
    system_device_info.rcu_clock_freq.sys_ck = rcu_clock_freq_get(CK_SYS);
    system_device_info.rcu_clock_freq.ahb_ck = rcu_clock_freq_get(CK_AHB);
    system_device_info.rcu_clock_freq.apb1_ck = rcu_clock_freq_get(CK_APB1);
    system_device_info.rcu_clock_freq.apb2_ck = rcu_clock_freq_get(CK_APB2);
    system_device_info.rcu_clock_freq.apb3_ck = rcu_clock_freq_get(CK_APB3);
    system_device_info.rcu_clock_freq.apb4_ck = rcu_clock_freq_get(CK_APB4);
}

/*!
    \brief      get complete system information
    \param[in]  none
    \param[out] none
    \retval     none
    \details    Retrieves comprehensive system information including device ID,
                revision, and all clock frequencies for system monitoring.
*/
void system_info_get(void)
{
    system_device_info_get();
    system_rcu_clock_freq_get();
}

/*!
    \brief      print system information to USART
    \param[in]  none
    \param[out] none
    \retval     none
*/
void system_info_print(void)
{
    PRINT_INFO("print system information>>\r\n");
    PRINT_INFO("/*********************************************************************/\r\n");
    PRINT_INFO("系统设备信息：\r\n");
    PRINT_INFO("memory_flash: \t\t\t%u KB(start address: 0x08000000)\r\n", system_device_info.memory_flash);
    PRINT_INFO("memory_sram: \t\t\t%u KB(start address: 0x24000000)\r\n", system_device_info.memory_sram);
    PRINT_INFO("device_id: \t\t\t0x%X%X%X\r\n", system_device_info.device_id[2],\
                                            system_device_info.device_id[1],\
                                            system_device_info.device_id[0]);
    PRINT_INFO("device_pid: \t\t\t0x%X\r\n", system_device_info.device_pid);
    PRINT_INFO("boot_address: \t\t\t0x%08X\r\n", system_device_info.boot_address);
    PRINT_INFO("boot_scr: \t\t\t0x%X\r\n", system_device_info.boot_scr);
    PRINT_INFO("boot_spc: \t\t\t0x%X\r\n", system_device_info.boot_spc);
    PRINT_INFO("boot_scr(0:失能安全模式,1:使能安全模式)\r\n");
    PRINT_INFO("boot_spc(0xAA:无保护状态,0xCC:安全保护等级高,other:安全保护等级低)\r\n");   
    PRINT_INFO("系统共享SRAM大小分配（共512KB）：\r\n");
    PRINT_INFO("itcm: \t\t\t\t%u KB(start address: 0x00000000)\r\n", system_device_info.share_sram_itcm);
    PRINT_INFO("dtcm: \t\t\t\t%u KB(start address: 0x20000000)\r\n", system_device_info.share_sram_dtcm);
    PRINT_INFO("sram: \t\t\t\t%u KB(start address: 0x24080000)\r\n", system_device_info.share_sram_sram);
    PRINT_INFO("\r\n");
    
    PRINT_INFO("系统时钟频率：\r\n");
    PRINT_INFO("sys_ck: \t\t\t%u Hz\r\n", system_device_info.rcu_clock_freq.sys_ck);
    PRINT_INFO("ahb_ck: \t\t\t%u Hz\r\n", system_device_info.rcu_clock_freq.ahb_ck);
    PRINT_INFO("apb1_ck: \t\t\t%u Hz\r\n", system_device_info.rcu_clock_freq.apb1_ck);
    PRINT_INFO("apb2_ck: \t\t\t%u Hz\r\n", system_device_info.rcu_clock_freq.apb2_ck);
    PRINT_INFO("apb3_ck: \t\t\t%u Hz\r\n", system_device_info.rcu_clock_freq.apb3_ck);
    PRINT_INFO("apb4_ck: \t\t\t%u Hz\r\n", system_device_info.rcu_clock_freq.apb4_ck);
    PRINT_INFO("/*********************************************************************/\r\n");
}

/*!
    \brief      enable the CPU cache
    \param[in]  none
    \param[out] none
    \retval     none
*/
void system_cache_enable(void)
{
    SCB_EnableICache();             /* enable i-cache */
    SCB_EnableDCache();             /* enable d-cache */
}

/*!
    \brief      configure NVIC vector table location
    \param[in]  nvic_vict_tab: new vector table base address
    \param[in]  offset: vector table offset
    \param[out] none
    \retval     none
*/
void system_nvic_vector_table_config(uint32_t nvic_vict_tab, uint32_t offset)
{
    memcpy((uint32_t *)nvic_vict_tab, (uint32_t *)FLASH_BASE, 0x400);
    nvic_vector_table_set(nvic_vict_tab, offset);
}

/*!
    \brief      initialize free watchdog timer (FWDGT)
    \param[in]  none
    \param[out] none
    \retval     none
*/
void system_fwdgt_init(void)
{
    /* enable IRC32K */
    rcu_osci_on(RCU_IRC32K);

    /* wait till IRC32K is ready */
    while(SUCCESS != rcu_osci_stab_wait(RCU_IRC32K)) {
    }
    
    fwdgt_config(5 * 500, FWDGT_PSC_DIV64);
    fwdgt_enable();
}

/* DWT (Data Watchpoint and Trace) register definitions */
#define  DEM_CR_TRCENA               (1 << 24)    /* Enable trace and debug block DEMCR.TRCENA */
#define  DWT_CR_CYCCNTENA            (1 <<  0)    /* Enable cycle counter DWT_CR.CYCCNTENA */

/*!
    \brief      initialize DWT (Data Watchpoint and Trace) for cycle counting
    \param[in]  none
    \param[out] none
    \retval     none
*/
void system_dwt_init(void)
{
	DEM_CR         |= (unsigned int)DEM_CR_TRCENA;   /* enable trace and debug block */
	DWT_CYCCNT      = (unsigned int)0U;              /* reset cycle counter */
	DWT_CR         |= (unsigned int)DWT_CR_CYCCNTENA; /* enable cycle counter */
}

/*!
    \brief      configure peripheral clock sources using PLL1 and PLL2
    \param[in]  none
    \param[out] none
    \retval     none
*/
void system_rcu_peripheral_clock_config(void)
{
    /*
        PLL1 Configuration:
        - PLL1P: ADC clock source (130MHz)
        - PLL1R: SDIO clock source (260MHz)
        Formula: CK_PLL1 = HXTAL_VALUE / M * N / P
        Where M=5, N=104, P=4 for PLL1P, P=2 for PLL1R
    */
    /* configure the pll1 input and output clock range */
    rcu_pll_input_output_clock_range_config(IDX_PLL1, RCU_PLL1RNG_4M_8M, RCU_PLL1VCO_192M_836M);
    /* configure the PLL1P clock: CK_PLL1P/CK_PLL1Q/CK_PLL1R = HXTAL_VALUE / 5 * 104 / 4, 130MHz */
    /* configure the PLL1R clock: CK_PLL1P/CK_PLL1Q/CK_PLL1R = HXTAL_VALUE / 5 * 104 / 2, 260MHz */
    rcu_pll1_config(5, 104, 4, 2, 2);
    /* enable PLL1P clock output */
    rcu_pll_clock_output_enable(RCU_PLL1P);
    /* enable PLL1R clock output */
    rcu_pll_clock_output_enable(RCU_PLL1R);
    /* enable PLL1 clock */
    rcu_osci_on(RCU_PLL1_CK);

    /* wait for PLL1 to stabilize */
    if(ERROR == rcu_osci_stab_wait(RCU_PLL1_CK)) {
        return;
    }
    
    /*
        PLL2 Configuration:
        - PLL2R: TLI (LCD-TFT) clock source (48MHz)
        Formula: CK_PLL2R = HXTAL_VALUE / M * N / R
        Where M=25, N=288, R=6
        PLL2R = 25MHz / 25 * 288 / 6 = 48MHz
    */
    rcu_pll_input_output_clock_range_config(IDX_PLL2, RCU_PLL2RNG_1M_2M, RCU_PLL2VCO_192M_836M);
    rcu_pll2_config(25, 288, 2, 2, 6);  /* PLL2R = 25 / 25 * 288 / 6 = 48MHz */
    rcu_pll_clock_output_enable(RCU_PLL2R);
    
    /* enable PLL2 clock */
    rcu_osci_on(RCU_PLL2_CK);

    /* wait for PLL2 to stabilize */
    if(ERROR == rcu_osci_stab_wait(RCU_PLL2_CK))
    {
        return;
    }
}
