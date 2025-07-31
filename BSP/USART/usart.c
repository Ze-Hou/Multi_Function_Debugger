/*!
    \file       usart.c
    \brief      USART driver implementation with DMA support
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./USART/usart.h"
#include "./DELAY/delay.h"
#include "TIMER/timer.h"

/* support printf function, usemicrolib is unnecessary */
#if (__ARMCC_VERSION > 6000000)
  __asm (".global __use_no_semihosting\n\t");
  /*!
      \brief      system exit function (semihosting disabled)
      \param[in]  x: exit code
      \param[out] none
      \retval     none
  */
  void _sys_exit(int x)
  {
    x = x;
  }
  /*!
      \brief      character write function (semihosting disabled)
      \param[in]  ch: character to write
      \param[out] none
      \retval     none
  */
  void _ttywrch(int ch)
  {
    ch = ch;
  }
  FILE __stdout;
#else
 #ifdef __CC_ARM
  #pragma import(__use_no_semihosting)
  struct __FILE
  {
    int handle;
  };
  FILE __stdout;
  /*!
      \brief      system exit function (semihosting disabled)
      \param[in]  x: exit code
      \param[out] none
      \retval     none
  */
  void _sys_exit(int x)
  {
    x = x;
  }
  /*!
      \brief      character write function (semihosting disabled)
      \param[in]  ch: character to write
      \param[out] none
      \retval     none
  */
  void _ttywrch(int ch)
  {
    ch = ch;
  }
 #endif
#endif

#if defined (__GNUC__) && !defined (__clang__)
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

/*!
    \brief      retargets the C library printf function to the USART
    \param[in]  ch: character to transmit
    \param[out] none
    \retval     transmitted character
*/
PUTCHAR_PROTOTYPE
{
    usart_data_transmit(BSP_USART, ch);
    
    while((USART_REG_VAL(BSP_USART, USART_FLAG_TFT) & BIT(USART_BIT_POS(USART_FLAG_TFT))))
    {
        while((USART_REG_VAL(BSP_USART, USART_FLAG_TFE) & BIT(USART_BIT_POS(USART_FLAG_TFE))) == RESET);
    }

    return ch;
}

uint8_t 	g_bsp_usart_recv_buff[BSP_USART_RECEIVE_LENGTH+1];                /* receive buffer */
uint16_t 	g_bsp_usart_recv_length = 0;									    /* received data length */
uint8_t	    g_bsp_usart_recv_complete_flag = 0; 					            /* receive complete flag */

/*!
    \brief      configure USART receive DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void usart_rx_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    /* enable DMA clock */
    rcu_periph_clock_enable(BSP_USART_DMA_CLOCK);
    
    rcu_periph_clock_enable(RCU_DMAMUX);
    
    dma_deinit(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL);                                        
    dma_init_struct.request 			= DMA_REQUEST_USART0_RX;                                
    dma_init_struct.periph_addr 		= BSP_USART_RD_ADDRESS;                                   
    dma_init_struct.memory0_addr 		= (uint32_t)g_bsp_usart_recv_buff;                                
    dma_init_struct.number 				= BSP_USART_RECEIVE_LENGTH;                               		    
    dma_init_struct.periph_inc 			= DMA_PERIPH_INCREASE_DISABLE;                            
    dma_init_struct.memory_inc 			= DMA_MEMORY_INCREASE_ENABLE;                             
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;                                
    dma_init_struct.direction 			= DMA_PERIPH_TO_MEMORY;                                   	
    dma_init_struct.priority 			= DMA_PRIORITY_HIGH;                                    
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;                            
    dma_single_data_mode_init(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL, &dma_init_struct);       
               
    dma_channel_enable(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL);                                 
}

/*!
    \brief      initialize USART with specified baud rate
    \param[in]  baud_rate: USART baud rate
    \param[out] none
    \retval     none
*/
void usart_init(uint32_t baud_rate)
{
    /* enable clocks */
    rcu_periph_clock_enable(BSP_USART_TX_RCU);
    rcu_periph_clock_enable(BSP_USART_RX_RCU);
    rcu_periph_clock_enable(BSP_USART_RCU);

    /* configure GPIO alternate function */
    gpio_af_set(BSP_USART_TX_PORT, BSP_USART_AF, BSP_USART_TX_PIN);	
    gpio_af_set(BSP_USART_RX_PORT, BSP_USART_AF, BSP_USART_RX_PIN);	

    /* configure GPIO mode */
    gpio_mode_set(BSP_USART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_USART_TX_PIN);
    gpio_mode_set(BSP_USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_USART_RX_PIN);

    /* configure GPIO output options */
    gpio_output_options_set(BSP_USART_TX_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, BSP_USART_TX_PIN);
    gpio_output_options_set(BSP_USART_RX_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, BSP_USART_RX_PIN);

    /* configure USART parameters */
    usart_deinit(BSP_USART);                                            
    usart_baudrate_set(BSP_USART, baud_rate);                           
    usart_parity_config(BSP_USART, USART_PM_NONE);                      
    usart_word_length_set(BSP_USART, USART_WL_8BIT);                    
    usart_stop_bit_set(BSP_USART, USART_STB_1BIT);     			        
    
    usart_transmit_fifo_threshold_config(BSP_USART, USART_TFTCFG_THRESHOLD_1_2);
    usart_receive_fifo_threshold_config(BSP_USART, USART_RFTCFG_THRESHOLD_1_2);
    usart_fifo_enable(BSP_USART);

    /* interrupt configuration */
    nvic_irq_enable(BSP_USART_IRQ, 5, 0); 								
    
    #if BSP_USART_SUPPORT_DMA
        usart_rx_dma_config();				                            
        usart_dma_receive_config(BSP_USART, USART_RECEIVE_DMA_ENABLE);  
    #else
        usart_interrupt_enable(BSP_USART, USART_INT_RFNE);				    
    #endif
    usart_interrupt_flag_clear(BSP_USART, USART_INT_FLAG_IDLE);
    usart_interrupt_enable(BSP_USART, USART_INT_IDLE);				    
    
    /* enable USART */
    usart_transmit_config(BSP_USART, USART_TRANSMIT_ENABLE);            
    usart_receive_config(BSP_USART, USART_RECEIVE_ENABLE);              
    usart_enable(BSP_USART);                          			        

    timer_base5_config(300, 1000);                                     
    timer_disable(TIMER5);
}

/*!
    \brief      reset USART receive DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usart_rx_dma_receive_reset(void)
{
    dma_channel_disable(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL);
    g_bsp_usart_recv_length = 0;
    g_bsp_usart_recv_complete_flag = 0;
    DMA_INTC0(BSP_USART_DMA) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, BSP_USART_RX_DMA_CHANNEL);
    dma_transfer_number_config(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL, BSP_USART_RECEIVE_LENGTH);
    dma_channel_enable(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL);
}

/*!
    \brief      print received data buffer and its length
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usart_rx_buffer_print(void)
{
    if(g_bsp_usart_recv_complete_flag == 1)
    {
        #if BSP_USART_SUPPORT_DMA
            SCB_InvalidateDCache_by_Addr(g_bsp_usart_recv_buff, BSP_USART_RECEIVE_LENGTH);
            printf("%s,%d\r\n", g_bsp_usart_recv_buff, g_bsp_usart_recv_length);
            memset((uint8_t *)g_bsp_usart_recv_buff, 0x00, BSP_USART_RECEIVE_LENGTH);
            usart_rx_dma_receive_reset();
        #else
            printf("%s,%d\r\n", g_recv_buff, g_recv_length);
            memset((uint8_t *)g_bsp_usart_recv_buff, 0x00, BSP_USART_RECEIVE_LENGTH);
            g_bsp_usart_recv_length = 0;
            g_bsp_usart_recv_complete_flag = 0;
        #endif
    }
}

/*!
    \brief      USART interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BSP_USART_IRQHandler(void)
{
    #if BSP_USART_SUPPORT_DMA
        if(usart_interrupt_flag_get(BSP_USART, USART_INT_FLAG_IDLE) == SET)
        {
            usart_interrupt_flag_clear(BSP_USART, USART_INT_FLAG_IDLE);
            g_bsp_usart_recv_length = BSP_USART_RECEIVE_LENGTH - dma_transfer_number_get(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL);
            if((g_bsp_usart_recv_length == 0) && dma_flag_get(BSP_USART_DMA, BSP_USART_RX_DMA_CHANNEL, DMA_FLAG_FTF) == RESET)
            {
                return;
            }
            timer_counter_value_config(TIMER5, 0);
            timer_enable(TIMER5);
        }
    #else
        if(usart_interrupt_flag_get(BSP_USART, USART_INT_FLAG_RFNE) == SET)
        {
            if(g_recv_complete_flag == 2)
            {
                timer_disable(TIMER5);
                g_recv_complete_flag = 0;
            }
            if(g_recv_length == BSP_USART_RECEIVE_LENGTH)g_recv_length=0;
            g_recv_buff[g_recv_length++] = usart_data_receive(BSP_USART);
        }
        
        if(usart_interrupt_flag_get(BSP_USART, USART_INT_FLAG_IDLE) == SET)
        {
            usart_interrupt_flag_clear(BSP_USART, USART_INT_FLAG_IDLE);
            g_recv_complete_flag = 2;
            timer_counter_value_config(TIMER5, 0);
            timer_enable(TIMER5);
        }
    #endif
}

/* Terminal USART variables */
uint8_t 	g_usart_terminal_recv_buff[USART_TERMINAL_RECEIVE_LENGTH + 1];          
uint16_t 	g_usart_terminal_recv_length = 0;									    
uint8_t	    g_usart_terminal_recv_complete_flag = 0; 					            

uint8_t 	gUsartTerminalSendBuff[USART_TERMINAL_SEND_LENGTH + 1];                     

/*!
    \brief      configure terminal USART receive DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void usart_terminal_rx_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    
    dma_deinit(DMA0, DMA_CH2);                                                                   
    dma_init_struct.request 			= DMA_REQUEST_USART1_RX;                                
    dma_init_struct.periph_addr 		= USART_TERMINAL_RD_ADDRESS;                            
    dma_init_struct.memory0_addr 		= (uint32_t)g_usart_terminal_recv_buff;                 
    dma_init_struct.number 				= USART_TERMINAL_RECEIVE_LENGTH;                           		    
    dma_init_struct.periph_inc 			= DMA_PERIPH_INCREASE_DISABLE;                          
    dma_init_struct.memory_inc 			= DMA_MEMORY_INCREASE_ENABLE;                           
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;                                
    dma_init_struct.direction 			= DMA_PERIPH_TO_MEMORY;                                 	
    dma_init_struct.priority 			= DMA_PRIORITY_HIGH;                                        
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;                             
    dma_single_data_mode_init(DMA0, DMA_CH2, &dma_init_struct);                                 
               
    dma_channel_enable(DMA0, DMA_CH2);                                                          
}

/*!
    \brief      configure terminal USART transmit DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void usart_terminal_tx_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    
    dma_deinit(DMA0, DMA_CH3);                                                                   
    dma_init_struct.request 			= DMA_REQUEST_USART1_TX;                                
    dma_init_struct.periph_addr 		= USART_TERMINAL_TD_ADDRESS;                            
    dma_init_struct.memory0_addr 		= (uint32_t)gUsartTerminalSendBuff;                     
    dma_init_struct.number 				= USART_TERMINAL_SEND_LENGTH;                              		    
    dma_init_struct.periph_inc 			= DMA_PERIPH_INCREASE_DISABLE;                          
    dma_init_struct.memory_inc 			= DMA_MEMORY_INCREASE_ENABLE;                           
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;                                
    dma_init_struct.direction 			= DMA_MEMORY_TO_PERIPH;                                 	
    dma_init_struct.priority 			= DMA_PRIORITY_HIGH;                                      
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;                            
    dma_single_data_mode_init(DMA0, DMA_CH3, &dma_init_struct);                                 
}

/*!
    \brief      initialize terminal USART with specified baud rate
    \param[in]  baud_rate: USART baud rate
    \param[out] none
    \retval     none
*/
void usart_terminal_init(uint32_t baud_rate)
{
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_USART1);
    
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_5);	
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_6);	

    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);

    gpio_output_options_set(GPIOD, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GPIO_PIN_5);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GPIO_PIN_6);

    usart_deinit(USART1);                                            
    usart_baudrate_set(USART1, baud_rate);                           
    usart_parity_config(USART1, USART_PM_NONE);                      
    usart_word_length_set(USART1, USART_WL_8BIT);                    
    usart_stop_bit_set(USART1, USART_STB_1BIT);     			     
    
    usart_transmit_fifo_threshold_config(USART1, USART_TFTCFG_THRESHOLD_1_2);
    usart_receive_fifo_threshold_config(USART1, USART_RFTCFG_THRESHOLD_1_2);
    usart_fifo_enable(USART1);

    nvic_irq_enable(USART1_IRQn, 5, 0); 								

    usart_terminal_tx_dma_config();
    usart_terminal_rx_dma_config();				                        
    
    usart_dma_transmit_config(USART1, USART_TRANSMIT_DMA_ENABLE);       
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);         
    
    usart_interrupt_flag_clear(USART1, USART_INT_FLAG_IDLE);
    usart_interrupt_enable(USART1, USART_INT_IDLE);				        

    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);               
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);                 
    usart_enable(USART1);                          			            
    
    timer_base6_config(300, 1000);                                     
    timer_disable(TIMER6);
}

/*!
    \brief      reset terminal USART receive DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usart_terminal_rx_dma_receive_reset(void)
{
    dma_channel_disable(DMA0, DMA_CH2);
    g_usart_terminal_recv_length = 0;
    g_usart_terminal_recv_complete_flag = 0;
    DMA_INTC0(DMA0) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, DMA_CH2);
    dma_transfer_number_config(DMA0, DMA_CH2, USART_TERMINAL_RECEIVE_LENGTH);
    dma_channel_enable(DMA0, DMA_CH2);
}

/*!
    \brief      print formatted string via terminal USART using DMA
    \param[in]  fmt: format string
    \param[in]  ...: variable arguments
    \param[out] none
    \retval     none
*/
void usart_terminal_print_fmt(const char *fmt, ...)
{
    uint16_t fmt_length = 0;
    va_list args;

    while(usart_flag_get(USART1, USART_FLAG_TFE) == RESET);
    dma_channel_disable(DMA0, DMA_CH3);
    va_start(args, fmt);
    fmt_length = vsnprintf((char *)gUsartTerminalSendBuff, USART_TERMINAL_SEND_LENGTH + 1, fmt, args);
    fmt_length = (fmt_length > USART_TERMINAL_SEND_LENGTH) ?  USART_TERMINAL_SEND_LENGTH : fmt_length;
    va_end(args);
    DMA_INTC0(DMA0) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, DMA_CH3);
    dma_transfer_number_config(DMA0, DMA_CH3, fmt_length);
    dma_channel_enable(DMA0, DMA_CH3);
}

/*!
    \brief      terminal USART interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USART1_IRQHandler(void)
{
    if(usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE) == SET)
    {
        usart_interrupt_flag_clear(USART1, USART_INT_FLAG_IDLE);
        g_usart_terminal_recv_length = USART_TERMINAL_RECEIVE_LENGTH - dma_transfer_number_get(DMA0, DMA_CH2);
        if((g_usart_terminal_recv_length == 0) && dma_flag_get(DMA0, DMA_CH2, DMA_FLAG_FTF) == RESET)
        {
            return;
        }
        timer_counter_value_config(TIMER6, 0);
        timer_enable(TIMER6);
    }
}

/* Module communication UART variables */
uint8_t 	g_uart4_recv_buff[UART4_RECEIVE_LENGTH + 1];                    
uint16_t 	g_uart4_recv_length = 0;									    
uint8_t	    g_uart4_recv_complete_flag = 0; 					            

uint8_t 	gUart4SendBuff[UART4_SEND_LENGTH + 1];                              

/*!
    \brief      configure UART4 receive DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void uart4_rx_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    
    dma_deinit(DMA0, DMA_CH4);                                                                   
    dma_init_struct.request 			= DMA_REQUEST_UART4_RX;                                 
    dma_init_struct.periph_addr 		= UART4_RD_ADDRESS;                                     
    dma_init_struct.memory0_addr 		= (uint32_t)g_uart4_recv_buff;                          
    dma_init_struct.number 				= UART4_RECEIVE_LENGTH;                                    		    
    dma_init_struct.periph_inc 			= DMA_PERIPH_INCREASE_DISABLE;                          
    dma_init_struct.memory_inc 			= DMA_MEMORY_INCREASE_ENABLE;                           
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;                                
    dma_init_struct.direction 			= DMA_PERIPH_TO_MEMORY;                                 	
    dma_init_struct.priority 			= DMA_PRIORITY_HIGH;                                        
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;                             
    dma_single_data_mode_init(DMA0, DMA_CH4, &dma_init_struct);                                 
               
    dma_channel_enable(DMA0, DMA_CH4);                                                          
}

/*!
    \brief      configure UART4 transmit DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void uart4_tx_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    
    dma_deinit(DMA0, DMA_CH5);                                                                   
    dma_init_struct.request 			= DMA_REQUEST_UART4_TX;                                 
    dma_init_struct.periph_addr 		= UART4_TD_ADDRESS;                                     
    dma_init_struct.memory0_addr 		= (uint32_t)gUart4SendBuff;                             
    dma_init_struct.number 				= UART4_SEND_LENGTH;                                       		    
    dma_init_struct.periph_inc 			= DMA_PERIPH_INCREASE_DISABLE;                          
    dma_init_struct.memory_inc 			= DMA_MEMORY_INCREASE_ENABLE;                           
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;                                
    dma_init_struct.direction 			= DMA_MEMORY_TO_PERIPH;                                 	
    dma_init_struct.priority 			= DMA_PRIORITY_HIGH;                                      
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;                            
    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_init_struct);                                 
}

/*!
    \brief      initialize UART4 with specified baud rate
    \param[in]  baud_rate: UART baud rate
    \param[out] none
    \retval     none
*/
void uart4_init(uint32_t baud_rate)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_UART4);
    
    gpio_af_set(GPIOB, GPIO_AF_14, GPIO_PIN_5);	
    gpio_af_set(GPIOC, GPIO_AF_8, GPIO_PIN_12);	

    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_12);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);

    gpio_output_options_set(GPIOC, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GPIO_PIN_12);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GPIO_PIN_5);

    usart_deinit(UART4);                                            
    usart_baudrate_set(UART4, baud_rate);                           
    usart_parity_config(UART4, USART_PM_NONE);                      
    usart_word_length_set(UART4, USART_WL_8BIT);                    
    usart_stop_bit_set(UART4, USART_STB_1BIT);     			        
    
    usart_transmit_fifo_threshold_config(UART4, USART_TFTCFG_THRESHOLD_1_2);
    usart_receive_fifo_threshold_config(UART4, USART_RFTCFG_THRESHOLD_1_2);
    usart_fifo_enable(UART4);

    nvic_irq_enable(UART4_IRQn, 5, 0); 								

    uart4_tx_dma_config();
    uart4_rx_dma_config();				                        
    
    usart_dma_transmit_config(UART4, USART_TRANSMIT_DMA_ENABLE);       
    usart_dma_receive_config(UART4, USART_RECEIVE_DMA_ENABLE);         
    
    usart_interrupt_flag_clear(UART4, USART_INT_FLAG_IDLE);
    usart_interrupt_enable(UART4, USART_INT_IDLE);				        

    usart_transmit_config(UART4, USART_TRANSMIT_ENABLE);               
    usart_receive_config(UART4, USART_RECEIVE_ENABLE);                 
    usart_enable(UART4);                          			            
    
    timer_general15_config(300, 1000);                                     
    timer_disable(TIMER15);
}

/*!
    \brief      reset UART4 receive DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
void uart4_rx_dma_receive_reset(void)
{
    dma_channel_disable(DMA0, DMA_CH4);
    g_uart4_recv_length = 0;
    g_uart4_recv_complete_flag = 0;
    DMA_INTC1(DMA0) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, DMA_CH4 - 4);
    dma_transfer_number_config(DMA0, DMA_CH4, UART4_RECEIVE_LENGTH);
    dma_channel_enable(DMA0, DMA_CH4);
}

/*!
    \brief      print formatted string via UART4 using DMA
    \param[in]  fmt: format string
    \param[in]  ...: variable arguments
    \param[out] none
    \retval     none
*/
void uart4_print_fmt(const char *fmt, ...)
{
    uint16_t fmt_length = 0;
    va_list args;

    while(usart_flag_get(UART4, USART_FLAG_TFE) == RESET);
    dma_channel_disable(DMA0, DMA_CH5);
    va_start(args, fmt);
    fmt_length = vsnprintf((char *)gUart4SendBuff, UART4_SEND_LENGTH + 1, fmt, args);
    fmt_length = (fmt_length > UART4_SEND_LENGTH) ?  UART4_SEND_LENGTH : fmt_length;
    va_end(args);
    DMA_INTC1(DMA0) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, DMA_CH5 - 4);
    dma_transfer_number_config(DMA0, DMA_CH5, fmt_length);
    dma_channel_enable(DMA0, DMA_CH5);
}

/*!
    \brief      UART4 interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UART4_IRQHandler(void)
{
    if(usart_interrupt_flag_get(UART4, USART_INT_FLAG_IDLE) == SET)
    {
        usart_interrupt_flag_clear(UART4, USART_INT_FLAG_IDLE);
        g_uart4_recv_length = UART4_RECEIVE_LENGTH - dma_transfer_number_get(DMA0, DMA_CH4);
        if((g_uart4_recv_length == 0) && dma_flag_get(DMA0, DMA_CH4, DMA_FLAG_FTF) == RESET)
        {
            return;
        }
        timer_counter_value_config(TIMER15, 0);
        timer_enable(TIMER15);
    }
}
