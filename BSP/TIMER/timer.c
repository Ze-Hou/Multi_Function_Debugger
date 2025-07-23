/*!
    \file       timer.c
    \brief      comprehensive timer driver implementation for GD32H7xx microcontroller
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou

    This file provides functions for:
    - USART communication timeout detection using TIMER5, TIMER6, and TIMER15
    - System watchdog automatic feeding mechanism via TIMER16
    - FreeRTOS runtime statistics collection using high-precision TIMER50
    - Timer interrupt service routines for communication timeout handling
    - Single pulse mode configuration for timeout detection applications
    - Conditional compilation support for OS and non-OS environments
    - High-resolution 64-bit timer counting for performance monitoring
    - Automatic DMA reception completion detection and flag management
*/

#include "gd32h7xx_libopt.h"
#include "./TIMER/timer.h"
#include "./USART/usart.h"

/*!
    \brief      configure TIMER5 for USART timeout detection
    \param[in]  psc: prescaler value
    \param[in]  period: timer period value
    \param[out] none
    \retval     none
*/
void timer_base5_config(uint16_t psc, uint32_t period)
{
    timer_parameter_struct timer_initpara;

    /* enable TIMER5 clock */
    rcu_periph_clock_enable(RCU_TIMER5);

    /* reset TIMER5 */
    timer_deinit(TIMER5);

    /* configure TIMER5 parameters */
    timer_initpara.prescaler = psc - 1;
    timer_initpara.period = (uint32_t)(period - 1);
    timer_init(TIMER5, &timer_initpara);
    
    /* configure single pulse mode */
    timer_single_pulse_mode_config(TIMER5, TIMER_SP_MODE_SINGLE);
    timer_interrupt_flag_clear(TIMER5, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER5, TIMER_INT_UP);
    nvic_irq_enable(TIMER5_DAC_UDR_IRQn, 4, 0); 
}

/*!
    \brief      configure TIMER6 for terminal USART timeout detection
    \param[in]  psc: prescaler value
    \param[in]  period: timer period value
    \param[out] none
    \retval     none
*/
void timer_base6_config(uint16_t psc, uint32_t period)
{
    timer_parameter_struct timer_initpara;

    /* enable TIMER6 clock */
    rcu_periph_clock_enable(RCU_TIMER6);

    /* reset TIMER6 */
    timer_deinit(TIMER6);

    /* configure TIMER6 parameters */
    timer_initpara.prescaler = psc - 1;
    timer_initpara.period = (uint32_t)(period - 1);
    timer_init(TIMER6, &timer_initpara);
    
    /* configure single pulse mode */
    timer_single_pulse_mode_config(TIMER6, TIMER_SP_MODE_SINGLE);
    timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER6, TIMER_INT_UP);
    nvic_irq_enable(TIMER6_IRQn, 4, 0); 
}

/*!
    \brief      configure TIMER15 for UART4 timeout detection
    \param[in]  psc: prescaler value
    \param[in]  period: timer period value
    \param[out] none
    \retval     none
*/
void timer_general15_config(uint16_t psc, uint16_t period)
{
    timer_parameter_struct timer_initpara;
    
    /* enable TIMER15 clock */
    rcu_periph_clock_enable(RCU_TIMER15);
    
    /* reset TIMER15 */
    timer_deinit(TIMER15);
    
    /* configure TIMER15 parameters */
    timer_initpara.prescaler         = psc - 1;
    timer_initpara.period            = (uint16_t)(period - 1);
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER15, &timer_initpara);
    
    /* configure single pulse mode and interrupt */
    timer_single_pulse_mode_config(TIMER15, TIMER_SP_MODE_SINGLE);
    timer_interrupt_flag_clear(TIMER15, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER15, TIMER_INT_UP);
    nvic_irq_enable(TIMER15_IRQn, 4, 0); 
}

/*!
    \brief      configure TIMER16 for watchdog feeding
    \param[in]  psc: prescaler value
    \param[in]  period: timer period value
    \param[out] none
    \retval     none
*/
void timer_general16_config(uint16_t psc, uint16_t period)
{
    timer_parameter_struct timer_initpara;
    
    /* enable TIMER16 clock */
    rcu_periph_clock_enable(RCU_TIMER16);
    
    /* reset TIMER16 */
    timer_deinit(TIMER16);
    
    /* configure TIMER16 parameters */
    timer_initpara.prescaler         = psc - 1;
    timer_initpara.period            = (uint16_t)(period - 1);
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER16, &timer_initpara);

    /* configure interrupt and enable timer */
    timer_interrupt_flag_clear(TIMER16, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER16, TIMER_INT_UP);
    nvic_irq_enable(TIMER16_IRQn, 2, 0); 
    
    timer_auto_reload_shadow_enable(TIMER16);
    timer_enable(TIMER16);
}

#if SYSTEM_SUPPORT_OS
/*!
    \brief      configure TIMER50 for FreeRTOS runtime statistics
    \param[in]  psc: prescaler value
    \param[out] none
    \retval     none
*/
static void timer_base50_config(uint16_t psc)
{
    timer_parameter_struct timer_initpara;
    
    /* enable TIMER50 clock */
    rcu_periph_clock_enable(RCU_TIMER50);
    
    /* reset TIMER50 */
    timer_deinit(TIMER50);
    
    /* configure TIMER50 parameters */
    timer_initpara.prescaler = psc - 1;
    timer_initpara.period = (uint64_t)(0xFFFFFFFFFFFFFFFF);
    timer_init(TIMER50, &timer_initpara);
    timer_counter_value_config(TIMER50, 0);
    timer_enable(TIMER50);
}

/*!
    \brief      configure timer for FreeRTOS runtime statistics
    \param[in]  none
    \param[out] none
    \retval     none
    \note       This function is used by FreeRTOS to initialize the timer for runtime statistics collection.
                It configures TIMER50 with a prescaler of 3000 to provide high-resolution timing.
                The timer runs continuously and provides a time base that is at least 10x faster than
                the FreeRTOS tick frequency. This function should be called by FreeRTOS kernel during
                initialization when configGENERATE_RUN_TIME_STATS is enabled in FreeRTOSConfig.h.
                External declaration required: extern void ConfigureTimeForRunTimeStats(void);
*/
void ConfigureTimeForRunTimeStats(void)
{
    timer_base50_config(3000);    /* initialize TIMER50 with prescaler 3000 for runtime stats */
}

/*!
    \brief      get current timer count for runtime statistics
    \param[in]  none
    \param[out] none
    \retval     current timer count value (64-bit)
    \note       This function is called by FreeRTOS to get the current timer count for calculating
                task execution times. It returns the current value of TIMER50 counter as a 64-bit
                value to prevent overflow during long-running applications. The returned value
                represents the elapsed time since timer initialization and is used by FreeRTOS
                to calculate percentage CPU usage for each task.
                External declaration required: extern uint64_t GetTimeForRunTimeCount(void);
*/
uint64_t GetTimeForRunTimeCount(void)
{
    return (uint64_t)timer_counter_read(TIMER50);    /* return current TIMER50 count for stats */
}
#endif /* SYSTEM_SUPPORT_OS */

/*!
    \brief      TIMER5 interrupt handler for USART timeout
    \param[in]  none
    \param[out] none
    \retval     none
*/
void TIMER5_DAC_UDR_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER5, TIMER_INT_FLAG_UP) == SET)
    {
        /* clear interrupt flag */
        timer_interrupt_flag_clear(TIMER5, TIMER_INT_FLAG_UP);
        #if BSP_USART_SUPPORT_DMA
            if(g_bsp_usart_recv_length == BSP_USART_RECEIVE_LENGTH - dma_transfer_number_get(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL))
            {
                if(g_bsp_usart_recv_length == 0)
                {
                    g_bsp_usart_recv_length = BSP_USART_RECEIVE_LENGTH;
                    g_bsp_usart_recv_buff[g_bsp_usart_recv_length] = '\0';    /* add string terminator */ 
                }
                else
                {
                    g_bsp_usart_recv_buff[g_bsp_usart_recv_length] = '\0';    /* add string terminator */ 
                }
                g_bsp_usart_recv_complete_flag = 1;                          /* reception complete */   
            }
        #else
            g_recv_buff[g_bsp_usart_recv_length] = '\0';                      /* add string terminator */
            g_recv_complete_flag = 1;                                         /* reception complete */
        #endif
    }
}

/*!
    \brief      TIMER6 interrupt handler for terminal USART timeout
    \param[in]  none
    \param[out] none
    \retval     none
*/
void TIMER6_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER6, TIMER_INT_FLAG_UP) == SET)
    {
        /* clear interrupt flag */
        timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
        if(g_usart_terminal_recv_length == USART_TERMINAL_RECEIVE_LENGTH - dma_transfer_number_get(DMA0, DMA_CH2))
        {
            if(g_usart_terminal_recv_length == 0)
            {
                g_usart_terminal_recv_length = USART_TERMINAL_RECEIVE_LENGTH;
                g_usart_terminal_recv_buff[g_usart_terminal_recv_length] = '\0';    /* add string terminator */ 
            }
            else
            {
                g_usart_terminal_recv_buff[g_usart_terminal_recv_length] = '\0';    /* add string terminator */ 
            }
            g_usart_terminal_recv_complete_flag = 1;                               /* reception complete */   
        }
    }
}

/*!
    \brief      TIMER15 interrupt handler for UART4 timeout
    \param[in]  none
    \param[out] none
    \retval     none
*/
void TIMER15_IRQHandler(void)
{
    
    if(timer_interrupt_flag_get(TIMER15, TIMER_INT_FLAG_UP) == SET)
    {
        /* clear interrupt flag */
        timer_interrupt_flag_clear(TIMER15, TIMER_INT_FLAG_UP);
        if(g_uart4_recv_length == UART4_RECEIVE_LENGTH - dma_transfer_number_get(DMA0, DMA_CH4))
        {
            if(g_uart4_recv_length == 0)
            {
                g_uart4_recv_length = UART4_RECEIVE_LENGTH;
                g_uart4_recv_buff[g_uart4_recv_length] = '\0';              /* add string terminator */ 
            }
            else
            {
                g_uart4_recv_buff[g_uart4_recv_length] = '\0';              /* add string terminator */ 
            }
            g_uart4_recv_complete_flag = 1;                                 /* reception complete */   
        }
    }
}

/*!
    \brief      TIMER16 interrupt handler for watchdog feeding
    \param[in]  none
    \param[out] none
    \retval     none
*/
void TIMER16_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER16, TIMER_INT_FLAG_UP) == SET)
    {
        /* clear interrupt flag */
        timer_interrupt_flag_clear(TIMER16, TIMER_INT_FLAG_UP);
        /* ensure watchdog feeding operation completes */
        FWDGT_CTL = FWDGT_KEY_RELOAD;
    }
}
