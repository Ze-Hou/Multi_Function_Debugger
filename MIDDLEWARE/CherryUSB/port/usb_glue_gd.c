/*!
    \file       usb_glue_gd.c
    \brief      USB glue layer for GigaDevice microcontrollers
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
    \note       Ported and modified from CherryUSB
*/

#include "usb_config.h"
#include "usb_dwc2_reg.h"
#include "gd32h7xx_libopt.h"
#include "./DELAY/delay.h"

/* you can find this config in function:usb_core_init, file:drv_usb_core.c, for example:
 *
 *  usb_regs->gr->GCCFG |= GCCFG_PWRON | GCCFG_VBUSACEN | GCCFG_VBUSBCEN;
 *
*/

/*!
    \brief      get USB device DWC2 GCCFG configuration
    \param[in]  reg_base: register base address
    \param[out] none
    \retval     GCCFG configuration value
*/
uint32_t usbd_get_dwc2_gccfg_conf(uint32_t reg_base)
{
    return (1 << 16);
}

/*!
    \brief      get USB host DWC2 GCCFG configuration
    \param[in]  reg_base: register base address
    \param[out] none
    \retval     GCCFG configuration value
*/
uint32_t usbh_get_dwc2_gccfg_conf(uint32_t reg_base)
{
    return (1 << 16);
}

/*!
    \brief      USB device DWC2 delay function
    \param[in]  ms: delay time in milliseconds
    \param[out] none
    \retval     none
*/
void usbd_dwc2_delay_ms(uint8_t ms)
{
    delay_xms(ms);
}

/*!
    \brief      configure USB interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_intr_config(void)
{
    #ifdef USE_USBHS0
        nvic_irq_enable((uint8_t)USBHS0_IRQn, 3U, 0U);
    #endif /* USE_USBHS0 */

    #ifdef USE_USBHS1
        nvic_irq_enable((uint8_t)USBHS1_IRQn, 3U, 0U);
    #endif /* USE_USBHS0 */ 
}

/*!
    \brief      configure PLL for USB high speed clock
    \param[in]  usb_periph: USB peripheral (USBHS0 or USBHS1)
    \param[out] none
    \retval     none
*/
void pllusb_rcu_config(uint32_t usb_periph)
{
    if(USBHS0 == usb_periph) {
        rcu_pllusb0_config(RCU_PLLUSBHSPRE_HXTAL, RCU_PLLUSBHSPRE_DIV5, RCU_PLLUSBHS_MUL96, RCU_USBHS_DIV8);
        RCU_ADDCTL1 |= RCU_ADDCTL1_PLLUSBHS0EN;
        while((RCU_ADDCTL1 & RCU_ADDCTL1_PLLUSBHS0STB) == 0U) {
        }

        rcu_usbhs_clock_selection_enable(IDX_USBHS0);
        rcu_usb48m_clock_config(IDX_USBHS0, RCU_USB48MSRC_PLLUSBHS);
        rcu_usbhs_clock_config(IDX_USBHS0, RCU_USBHSSEL_60M);
    } else {
        rcu_pllusb1_config(RCU_PLLUSBHSPRE_HXTAL, RCU_PLLUSBHSPRE_DIV5, RCU_PLLUSBHS_MUL96, RCU_USBHS_DIV8);
        RCU_ADDCTL1 |= RCU_ADDCTL1_PLLUSBHS1EN;
        while((RCU_ADDCTL1 & RCU_ADDCTL1_PLLUSBHS1STB) == 0U) {
        }

        rcu_usbhs_clock_selection_enable(IDX_USBHS1);
        rcu_usb48m_clock_config(IDX_USBHS1, RCU_USB48MSRC_PLLUSBHS);
        rcu_usbhs_clock_config(IDX_USBHS1, RCU_USBHSSEL_60M);
    }
}

/*!
    \brief      configure USB RCU (Reset and Clock Unit)
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_rcu_config(void)
{
    rcu_periph_clock_enable(RCU_PMU);       /* enable the power module clock */
//    pmu_usb_regulator_enable();           /* enable when using USB internal LDO */
    pmu_usb_voltage_detector_enable();
    while (pmu_flag_get(PMU_FLAG_USB33RF) != SET) {
    }

    #ifdef CONFIG_USB_HS
    #else
        /* enable IRC48M clock */
        rcu_osci_on(RCU_IRC48M);

        /* wait till IRC48M is ready */
        while (SUCCESS != rcu_osci_stab_wait(RCU_IRC48M)) {
        }
        
        #ifdef USE_USBHS0
            rcu_usb48m_clock_config(IDX_USBHS0, RCU_USB48MSRC_IRC48M);
        #endif /* USE_USBHS0 */

        #ifdef USE_USBHS1
            rcu_usb48m_clock_config(IDX_USBHS1, RCU_USB48MSRC_IRC48M);
        #endif /* USE_USBHS1 */
    
    #endif
    #ifdef USE_USBHS0
        rcu_periph_clock_enable(RCU_USBHS0);
    #endif /* USE_USBHS0 */

    #ifdef USE_USBHS1
        rcu_periph_clock_enable(RCU_USBHS1);
    #endif /* USE_USBHS1 */
}

/*!
    \brief      initialize USB device controller low level hardware
    \param[in]  busid: USB bus ID
    \param[out] none
    \retval     none
*/
void usb_dc_low_level_init(uint8_t busid)
{
    usb_rcu_config();
    usb_intr_config();
}

/*!
    \brief      deinitialize USB device controller low level hardware
    \param[in]  busid: USB bus ID
    \param[out] none
    \retval     none
*/
void usb_dc_low_level_deinit(uint8_t busid)
{
    #ifdef USE_USBHS0
        rcu_periph_reset_enable(RCU_USBHS0RST);
        rcu_periph_reset_disable(RCU_USBHS0RST);
        nvic_irq_disable((uint8_t)USBHS0_IRQn);
        rcu_periph_clock_disable(RCU_USBHS0);
    #endif /* USE_USBHS0 */
    
    #ifdef USE_USBHS1
        rcu_periph_reset_enable(RCU_USBHS1RST);
        rcu_periph_reset_disable(RCU_USBHS1RST);
        nvic_irq_disable((uint8_t)USBHS1_IRQn);
        rcu_periph_clock_disable(RCU_USBHS1);
    #endif /* USE_USBHS1 */
}

#ifdef USE_USBHS0
/*!
    \brief      USB high speed 0 interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USBHS0_IRQHandler()
{
    extern void USBD_IRQHandler(uint8_t busid);
    USBD_IRQHandler(0);
}
#endif /* USE_USBHS0 */

#ifdef USE_USBHS1
/*!
    \brief      USB high speed 1 interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USBHS1_IRQHandler()
{
    extern void USBD_IRQHandler(uint8_t busid);
    USBD_IRQHandler(0);
}
#endif /* USE_USBHS1 */
