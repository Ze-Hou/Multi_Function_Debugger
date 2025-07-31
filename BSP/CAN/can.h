/*!
    \file       can.h
    \brief      header file for CAN bus communication driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __CAN_H
#define __CAN_H
#include <stdint.h>
#include "gd32h7xx_can.h"

/* CAN configuration definitions */
#define CAN_RECEIVE_FIFO_ADDR   (CAN1 + 0x80)          /*!< CAN1 receive FIFO address for DMA */
#define CAN_RECEIVE_LENGTH      (32 * 4)               /*!< CAN receive buffer length (32 messages * 4 words) */

/* external variables */
extern uint32_t gCanReceiveBuffer[CAN_RECEIVE_LENGTH / 4][4];               /*!< CAN receive buffer for raw message data including ID and control info */
extern uint8_t gCanReceiveData[CAN_RECEIVE_LENGTH / 4 * 8 + 1];             /*!< CAN receive data buffer for extracted payload data */
extern uint16_t gCanReceiveDataLength;                                      /*!< length of received CAN data */

/* function declarations */
void can_config(uint8_t tsjw, uint8_t tpts, uint8_t tpbs1, uint8_t tpbs2, uint16_t baudpsc, can_operation_modes_enum mode); /*!< configure CAN bus parameters and timing */
void can_transmit_message(uint32_t id, uint8_t data_bytes, uint8_t *p_buffer, uint8_t mailbox_index);   /*!< transmit CAN message via specified mailbox */
void can_read_dam_receive_data(void);   /*!< read and process DMA received CAN data */
#endif
