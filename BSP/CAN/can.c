/*!
    \file       can.c
    \brief      CAN bus communication driver with DMA support
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./CAN/can.h"
#include <string.h>

/* CAN message descriptors */
static can_mailbox_descriptor_struct can_transmit_message_descriptor;       /*!< CAN transmit message descriptor */
static can_mailbox_descriptor_struct can_receive_message_descriptor;        /*!< CAN receive message descriptor */

/* CAN data buffers */
uint32_t gCanReceiveBuffer[CAN_RECEIVE_LENGTH / 4][4];                      /*!< raw CAN message buffer with ID and control information */
uint8_t gCanReceiveData[CAN_RECEIVE_LENGTH / 4 * 8 + 1];                    /*!< extracted CAN payload data buffer */
uint16_t gCanReceiveDataLength = 0;                                         /*!< length of received CAN data */

/* static function declarations */
static void can_gpio_config(void);                                         /*!< configure GPIO pins for CAN communication */
static void can_receive_dma_config(void);                                  /*!< configure DMA for CAN receive operation */
static void can_dma_receive_reset(void);                                   /*!< reset DMA receive configuration */
static inline uint32_t big_to_little_endian(uint32_t big_endian);          /*!< convert big endian to little endian format */

/*!
    \brief      configure GPIO pins for CAN communication
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void can_gpio_config(void)
{
    /* enable GPIOB peripheral clock */
    rcu_periph_clock_enable(RCU_GPIOB);
    
    /* configure alternate function for CAN1 pins: PB12 (RX), PB13 (TX) */
    gpio_af_set(GPIOB, GPIO_AF_9, GPIO_PIN_12);
    gpio_af_set(GPIOB, GPIO_AF_9, GPIO_PIN_13);
    
    /* configure PB12 as CAN1 RX pin */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_12);
    
    /* configure PB13 as CAN1 TX pin */
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_13);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_13);
}

/*!
    \brief      configure DMA for CAN receive operation
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void can_receive_dma_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    /* enable DMA0 and DMAMUX peripheral clocks */
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    
    /* reset DMA0 channel 6 configuration */
    dma_deinit(DMA0, DMA_CH6);
    
    /* configure DMA0 channel 6 parameters */
	dma_init_struct.request 			= DMA_REQUEST_CAN1;                                 /* DMA request source */
    dma_init_struct.periph_addr 		= CAN_RECEIVE_FIFO_ADDR;                            /* peripheral data address: CAN1 receive FIFO */  
    dma_init_struct.memory0_addr 		= (uint32_t)gCanReceiveBuffer;                      /* memory address: receive buffer */  
    dma_init_struct.number 				= CAN_RECEIVE_LENGTH;                               /* transfer data size */   		    
    dma_init_struct.periph_inc 			= DMA_PERIPH_INCREASE_DISABLE;                      /* peripheral address increment */   
    dma_init_struct.memory_inc 			= DMA_MEMORY_INCREASE_ENABLE;                       /* memory address increment */   
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_32BIT;                           /* data width: 32-bit */   
    dma_init_struct.direction 			= DMA_PERIPH_TO_MEMORY;                             /* transfer direction: peripheral to memory */    	
    dma_init_struct.priority 			= DMA_PRIORITY_HIGH;                                /* DMA priority: high */  
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;                         /* DMA mode: circular */
    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_init_struct);
    		   
    /* enable DMA channel */
    dma_channel_enable(DMA0, DMA_CH6); 
}

/*!
    \brief      configure CAN bus parameters and timing
    \param[in]  tsjw: resync jump width (1-4)
    \param[in]  tpts: prop time segment (1-8)
    \param[in]  tpbs1: time segment 1 (1-16)
    \param[in]  tpbs2: time segment 2 (1-8)
    \param[in]  baudpsc: baud rate prescaler (1-1024)
    \param[in]  mode: CAN operation mode
    \param[out] none
    \retval     none
*/
void can_config(uint8_t tsjw, uint8_t tpts, uint8_t tpbs1, uint8_t tpbs2, uint16_t baudpsc, can_operation_modes_enum mode)
{
    can_parameter_struct can_parameter;
    can_fifo_parameter_struct can_fifo_parameter;
    
    /* configure CAN1 clock source APB2/2 */
    rcu_can_clock_config(IDX_CAN1, RCU_CANSRC_APB2_DIV2);
    
    rcu_periph_clock_enable(RCU_CAN1);
    
    can_gpio_config();

    can_deinit(CAN1);

    /* initialize CAN parameters */
    can_parameter.internal_counter_source = CAN_TIMER_SOURCE_BIT_CLOCK;     /* internal counter clock source */
    can_parameter.self_reception = DISABLE;                                 /* self reception disable */
    can_parameter.mb_tx_order = CAN_TX_HIGH_PRIORITY_MB_FIRST;              /* mailbox transmit order */
    can_parameter.mb_tx_abort_enable = ENABLE;                              /* mailbox transmit abort enable */
    can_parameter.local_priority_enable = DISABLE;                          /* local priority enable */
    can_parameter.mb_rx_ide_rtr_type = CAN_IDE_RTR_FILTERED;                /* IDE and RTR filtering type */
    can_parameter.mb_remote_frame = CAN_STORE_REMOTE_REQUEST_FRAME;         /* remote frame storage */
    can_parameter.rx_private_filter_queue_enable = DISABLE;                 /* private filter queue enable */
    can_parameter.edge_filter_enable = DISABLE;                             /* edge filter enable */
    can_parameter.protocol_exception_enable = DISABLE;                      /* protocol exception enable */
    can_parameter.rx_filter_order = CAN_RX_FILTER_ORDER_MAILBOX_FIRST;      /* receive filter order */
    can_parameter.memory_size = CAN_MEMSIZE_32_UNIT;                        /* memory size */
    can_parameter.mb_public_filter = 0U;                                    /* mailbox public filter */

    /* configure bit timing parameters */
    can_parameter.resync_jump_width = tsjw;                                 /* resync jump width */
    can_parameter.prop_time_segment = tpts;                                 /* prop time segment */
    can_parameter.time_segment_1 = tpbs1;                                   /* time segment 1 */
    can_parameter.time_segment_2 = tpbs2;                                   /* time segment 2 */
    can_parameter.prescaler = baudpsc;                                      /* baud rate prescaler */
    /* BaudRate = CAN CLK / ((resync_jump_width + prop_time_segment + time_segment_1 + time_segment_2) * prescaler) */
    
    /* initialize CAN1 with configured parameters */
    can_init(CAN1, &can_parameter);
    
    /* configure receive FIFO parameters */
    can_fifo_parameter.dma_enable = ENABLE;
    can_fifo_parameter.filter_format_and_number = CAN_RXFIFO_FILTER_A_NUM_8;
    can_fifo_parameter.fifo_public_filter = 0U;
    
    /* configure receive FIFO and DMA */
    can_rx_fifo_config(CAN1, &can_fifo_parameter);
    can_receive_dma_config();
    
    /* enter specified operation mode */
    can_operation_mode_enter(CAN1, mode);
}

/*!
    \brief      transmit CAN message via specified mailbox
    \param[in]  id: CAN message identifier
    \param[in]  data_bytes: number of data bytes (0-8)
    \param[in]  p_buffer: pointer to data buffer
    \param[in]  mailbox_index: mailbox index for transmission
    \param[out] none
    \retval     none
*/
void can_transmit_message(uint32_t id, uint8_t data_bytes, uint8_t *p_buffer, uint8_t mailbox_index)
{
    /* configure transmit message descriptor */
    can_transmit_message_descriptor.rtr             = 0U;                   /* data frame (not remote request) */
    can_transmit_message_descriptor.ide             = 0U;                   /* standard identifier format */
    can_transmit_message_descriptor.code            = CAN_MB_TX_STATUS_DATA;/* mailbox transmit status */
    can_transmit_message_descriptor.brs             = 0U;                   /* bit rate switch disabled */
    can_transmit_message_descriptor.esi             = 0U;                   /* error state indicator */
    can_transmit_message_descriptor.fdf             = 0U;                   /* CAN FD format disabled */
    can_transmit_message_descriptor.prio            = 0U;                   /* message priority */
    
    /* configure message parameters */
    can_transmit_message_descriptor.id              = id;                   /* message identifier */
    can_transmit_message_descriptor.data_bytes      = data_bytes;           /* data length */
    can_transmit_message_descriptor.data            = (uint32_t *)p_buffer; /* data buffer pointer */
    
    /* configure mailbox and trigger transmission */
    can_mailbox_config(CAN1, mailbox_index, &can_transmit_message_descriptor);
}

/*!
    \brief      reset DMA receive configuration
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void can_dma_receive_reset(void)
{
    /* disable DMA channel */
    dma_channel_disable(DMA0, DMA_CH6);
    
    /* clear DMA interrupt flags */
    DMA_INTC1(DMA0) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, DMA_CH6 - 4);
    
    /* reconfigure transfer count */
    dma_transfer_number_config(DMA0, DMA_CH6, CAN_RECEIVE_LENGTH);
    
    /* re-enable DMA channel */
    dma_channel_enable(DMA0, DMA_CH6);
}

/*!
    \brief      convert big endian to little endian format
    \param[in]  big_endian: 32-bit value in big endian format
    \param[out] none
    \retval     32-bit value in little endian format
*/
static inline uint32_t big_to_little_endian(uint32_t big_endian)
{
    return ((big_endian >> 24) & 0x000000FF) |          /* move MSB to LSB position */
           ((big_endian >> 8)  & 0x0000FF00) |          /* move byte 2 to byte 1 position */
           ((big_endian << 8)  & 0x00FF0000) |          /* move byte 1 to byte 2 position */
           ((big_endian << 24) & 0xFF000000);           /* move LSB to MSB position */
}

/*!
    \brief      read and process DMA received CAN data
    \param[in]  none
    \param[out] none
    \retval     none
*/
void can_read_dam_receive_data(void)
{
    uint8_t data_length = 0, temp;
    uint16_t i = 0, j = 0;
    uint16_t dma_length = CAN_RECEIVE_LENGTH - dma_transfer_number_get(DMA0, DMA_CH6);
    uint8_t *p_data = (uint8_t *)gCanReceiveData;
    uint32_t data_little_endian;
    
    /* reset receive data length */
    gCanReceiveDataLength = 0;
    
    /* check if there is new data available */
    if((dma_length == 0) && (dma_flag_get(DMA0, DMA_CH6, DMA_FLAG_FTF) == RESET))
    {
        return;
    }
    
    /* handle buffer wrap-around case */
    if(dma_length == 0)
    {
        dma_length = CAN_RECEIVE_LENGTH;
    }
    
    /* process received data only if FIFO is not empty */
    if(!(CAN_STAT(CAN1) & CAN_STAT_MS5_RFNE))
    {
        /* extract data from each CAN message */
        for(i = 0; i < dma_length / 4; i++)
        {
            /* extract data length from message header */
            data_length = ((gCanReceiveBuffer[i][0] >> 16) & 0xF);
            gCanReceiveDataLength += data_length;
            
            /* copy 4-byte aligned data */
            for(j = 0; j < data_length / 4; j++)
            {
                data_little_endian = big_to_little_endian(gCanReceiveBuffer[i][j + 2]);
                memcpy(p_data, &data_little_endian, 4);
                p_data += 4;
            }
            
            /* copy remaining bytes (less than 4) */
            temp = data_length % 4;
            if(temp)
            {
                data_little_endian = big_to_little_endian(gCanReceiveBuffer[i][j + 2]);
                memcpy(p_data, &data_little_endian, temp);
                p_data += temp;
            }
        }
        /* null-terminate the data string */
        *p_data = '\0';
    }
    
    /* reset DMA for next reception */
    can_dma_receive_reset();
}