/*!
    \file       usart.h
    \brief      USART communication module header file with multi-level debug support
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef __USART_H
#define __USART_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* debug print level configuration */
#define DEBUG_LEVEL_NONE    0                                               /*!< no debug output */
#define DEBUG_LEVEL_ERROR   1                                               /*!< only error messages */
#define DEBUG_LEVEL_WARN    2                                               /*!< error and warning messages */
#define DEBUG_LEVEL_INFO    3                                               /*!< error, warning and info messages */
#define DEBUG_LEVEL_DEBUG   4                                               /*!< all messages including debug */

/* current debug level setting */
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL         DEBUG_LEVEL_DEBUG                                /*!< default debug level */
#endif

/* print macros with different levels */
#if (DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)
    #define PRINT_ERROR(fmt, ...)   printf("[ERROR] " fmt, ##__VA_ARGS__)   /*!< error level print macro */
#else
    #define PRINT_ERROR(fmt, ...)   do {} while(0)                          /*!< disabled error print */
#endif

#if (DEBUG_LEVEL >= DEBUG_LEVEL_WARN)
    #define PRINT_WARN(fmt, ...)    printf("[WARN]  " fmt, ##__VA_ARGS__)   /*!< warning level print macro */
#else
    #define PRINT_WARN(fmt, ...)    do {} while(0)                          /*!< disabled warning print */
#endif

#if (DEBUG_LEVEL >= DEBUG_LEVEL_INFO)
    #define PRINT_INFO(fmt, ...)    printf("[INFO]  " fmt, ##__VA_ARGS__)   /*!< info level print macro */
#else
    #define PRINT_INFO(fmt, ...)    do {} while(0)                          /*!< disabled info print */
#endif

#if (DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG)
    #define PRINT_DEBUG(fmt, ...)   printf("[DEBUG] " fmt, ##__VA_ARGS__)   /*!< debug level print macro */
#else
    #define PRINT_DEBUG(fmt, ...)   do {} while(0)                          /*!< disabled debug print */
#endif

/* general print macro (always enabled) */
#define PRINT(fmt, ...)             printf(fmt, ##__VA_ARGS__)              /*!< general print macro */


/*!
    \brief USART0 configuration macros for system communication
*/
#define BSP_USART_SUPPORT_DMA       1                                       /*!< enable DMA support for USART */

#define BSP_USART_TX_RCU            RCU_GPIOA                               /*!< USART TX port clock */
#define BSP_USART_RX_RCU            RCU_GPIOA                               /*!< USART RX port clock */
#define BSP_USART_RCU               RCU_USART0                              /*!< USART0 peripheral clock */

#define BSP_USART_TX_PORT           GPIOA                                   /*!< USART TX port */
#define BSP_USART_RX_PORT           GPIOA                                   /*!< USART RX port */
#define BSP_USART_AF                GPIO_AF_7                               /*!< USART0 alternate function */
#define BSP_USART_TX_PIN            GPIO_PIN_9                              /*!< USART TX pin */
#define BSP_USART_RX_PIN            GPIO_PIN_10                             /*!< USART RX pin */

#define BSP_USART                   USART0                                  /*!< USART0 peripheral */
#define BSP_USART_IRQ               USART0_IRQn                             /*!< USART0 interrupt */
#define BSP_USART_IRQHandler        USART0_IRQHandler                       /*!< USART0 interrupt handler */

/*!
    \brief USART0 DMA configuration macros
*/
/* USART corresponding DMA request channel */
/* channel 1 reserved for TX */
#define  BSP_USART_RX_DMA_CHANNEL           DMA_CH0                         /*!< USART0 RX DMA channel */
#define  BSP_USART_DMA_CLOCK                RCU_DMA0                        /*!< DMA clock for USART0 */
#define  BSP_USART_DMA                      DMA0                            /*!< DMA controller for USART0 */
/* peripheral register address */
#define  BSP_USART_RD_ADDRESS               (USART0 + 0x24)                 /*!< USART0 receive data register address */

/*!
    \brief USART0 buffer configuration
*/
#define BSP_USART_RECEIVE_LENGTH        1024                                /*!< USART0 receive buffer length */

extern uint8_t  g_bsp_usart_recv_buff[BSP_USART_RECEIVE_LENGTH+1];          /*!< USART0 receive buffer */
extern uint16_t g_bsp_usart_recv_length;                                    /*!< USART0 received data length */
extern uint8_t  g_bsp_usart_recv_complete_flag;                             /*!< USART0 receive complete flag */

/*!
    \brief USART1 terminal configuration macros
*/
#define  USART_TERMINAL_RD_ADDRESS               (USART1 + 0x24)             /*!< USART1 receive data register address */
#define  USART_TERMINAL_TD_ADDRESS               (USART1 + 0x28)             /*!< USART1 transmit data register address */

#define USART_TERMINAL_RECEIVE_LENGTH    1024                                /*!< USART1 terminal receive buffer length */
#define USART_TERMINAL_SEND_LENGTH       1024                                /*!< USART1 terminal send buffer length */

extern uint8_t  g_usart_terminal_recv_buff[USART_TERMINAL_RECEIVE_LENGTH + 1];  /*!< USART1 terminal receive buffer */
extern uint16_t g_usart_terminal_recv_length;                               /*!< USART1 terminal received data length */
extern uint8_t  g_usart_terminal_recv_complete_flag;                        /*!< USART1 terminal receive complete flag */

extern uint8_t  gUsartTerminalSendBuff[USART_TERMINAL_SEND_LENGTH + 1];     /*!< USART1 terminal send buffer */

/*!
    \brief UART4 wireless module configuration macros
*/
#define  UART4_RD_ADDRESS               (UART4 + 0x24)                       /*!< UART4 receive data register address */
#define  UART4_TD_ADDRESS               (UART4 + 0x28)                       /*!< UART4 transmit data register address */

#define UART4_RECEIVE_LENGTH    1024                                         /*!< UART4 receive buffer length */
#define UART4_SEND_LENGTH       1024                                         /*!< UART4 send buffer length */

extern uint8_t  g_uart4_recv_buff[UART4_RECEIVE_LENGTH + 1];                /*!< UART4 receive buffer */
extern uint16_t g_uart4_recv_length;                                        /*!< UART4 received data length */
extern uint8_t  g_uart4_recv_complete_flag;                                 /*!< UART4 receive complete flag */

extern uint8_t  gUart4SendBuff[UART4_SEND_LENGTH + 1];                      /*!< UART4 send buffer */

/*!
    \brief Function declarations for USART communication
*/
/* function declarations */
void usart_init(uint32_t baud_rate);                                        /*!< initialize USART0 with specified baud rate */
void usart_rx_dma_receive_reset(void);                                      /*!< reset USART0 DMA receive */
void usart_rx_buffer_print(void);                                           /*!< print USART0 receive buffer content */
void usart_terminal_init(uint32_t baud_rate);                               /*!< initialize USART1 terminal with specified baud rate */
void usart_terminal_rx_dma_receive_reset(void);                             /*!< reset USART1 terminal DMA receive */
void usart_terminal_print_fmt(const char *fmt, ...);                        /*!< formatted print to USART1 terminal */
void uart4_init(uint32_t baud_rate);                                        /*!< initialize UART4 with specified baud rate */
void uart4_rx_dma_receive_reset(void);                                      /*!< reset UART4 DMA receive */
void uart4_print_fmt(const char *fmt, ...);                                 /*!< formatted print to UART4 */
#endif
