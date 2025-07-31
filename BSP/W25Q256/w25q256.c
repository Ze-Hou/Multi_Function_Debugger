/*!
    \file       w25q256.c
    \brief      W25Q256 SPI flash memory driver implementation
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./W25Q256/w25q256.h"
#include "./DELAY/delay.h"
#include "./USART/usart.h"
#include "./MALLOC/malloc.h"

/* define ospi init struct */
static ospi_parameter_struct ospi_struct = {0}; /*!< OSPI initialization structure */

/*!
    \brief      configure OSPI interface for W25Q256 in indirect mode
    \param[in]  none
    \retval     none
*/
static void w25q256_ospi_config(void)
{
    mdma_deinit();
    ospim_deinit();
    ospi_deinit(OSPI0);
    
    rcu_periph_clock_enable(RCU_MDMA);
    rcu_periph_clock_enable(RCU_OSPIM);
    rcu_periph_clock_enable(RCU_OSPI0);
    
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOF);
    
    /* configure OSPIM GPIO pin:
       OSPIM_P0_SCK(PB2)
       OSPIM_P0_CSN(PB6)
       OSPIM_P0_IO0(PF8)
       OSPIM_P0_IO1(PF9)
       OSPIM_P0_IO2(PF7)
       OSPIM_P0_IO3(PF6) */
    gpio_af_set(GPIOB, GPIO_AF_9, GPIO_PIN_2);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_2);
    
    gpio_af_set(GPIOB, GPIO_AF_10, GPIO_PIN_6);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_6);
    
    gpio_af_set(GPIOF, GPIO_AF_10, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    
    ospim_port_sck_source_select(OSPIM_PORT0, OSPIM_SCK_SOURCE_OSPI0_SCK);
    ospim_port_csn_source_select(OSPIM_PORT0, OSPIM_CSN_SOURCE_OSPI0_CSN);
    ospim_port_io3_0_source_select(OSPIM_PORT0, OSPIM_SRCPLIO_OSPI0_IO_LOW);
    
    ospi_struct_init(&ospi_struct);
    
    ospi_struct.prescaler        = 2U;                                  /*!< kernel clock frequency divider for OSPI clock, ospi_sck=ck_ahb/(prescaler+1) */
    ospi_struct.sample_shift     = OSPI_SAMPLE_SHIFTING_HALF_CYCLE;     /*!< specify data sampling shift */
    ospi_struct.fifo_threshold   = OSPI_FIFO_THRESHOLD_16;              /*!< FIFO threshold bytes */
    ospi_struct.device_size      = OSPI_MESZ_32_MBS;                    /*!< specify device size */
    ospi_struct.wrap_size        = OSPI_DIRECT;                         /*!< specify external device wrap size */
    ospi_struct.cs_hightime      = OSPI_CS_HIGH_TIME_2_CYCLE;           /*!< chip select high time */
    ospi_struct.memory_type      = OSPI_MICRON_MODE;                    /*!< external device type */
    ospi_struct.delay_hold_cycle = OSPI_DELAY_HOLD_NONE;                /*!< delay hold 0 cycle */
    ospi_init(OSPI0, &ospi_struct);
    
    ospi_enable(OSPI0);
}

/*!
    \brief      send OSPI command to W25Q256
    \param[in]  operation_type: OSPI operation type
      \arg        OSPI_OPTYPE_COMMON_CFG: common configuration
      \arg        OSPI_OPTYPE_READ_CFG: read configuration  
      \arg        OSPI_OPTYPE_WRITE_CFG: write configuration
      \arg        OSPI_OPTYPE_WRAP_CFG: wrap configuration
    \param[in]  ins_mode: instruction mode
      \arg        OSPI_INSTRUCTION_NONE: no instruction
      \arg        OSPI_INSTRUCTION_1_LINE: 1-line instruction
      \arg        OSPI_INSTRUCTION_2_LINES: 2-line instruction
      \arg        OSPI_INSTRUCTION_4_LINES: 4-line instruction
      \arg        OSPI_INSTRUCTION_8_LINES: 8-line instruction
    \param[in]  instruction: instruction to send to external memory
    \param[in]  addr_mode: address mode
      \arg        OSPI_ADDRESS_NONE: no address
      \arg        OSPI_ADDRESS_1_LINE: 1-line address
      \arg        OSPI_ADDRESS_2_LINES: 2-line address
      \arg        OSPI_ADDRESS_4_LINES: 4-line address
      \arg        OSPI_ADDRESS_8_LINES: 8-line address
    \param[in]  address: memory access address
    \param[in]  addr_size: address size
      \arg        OSPI_ADDRESS_8_BITS: 8-bit address
      \arg        OSPI_ADDRESS_16_BITS: 16-bit address
      \arg        OSPI_ADDRESS_24_BITS: 24-bit address
      \arg        OSPI_ADDRESS_32_BITS: 32-bit address
    \param[in]  data_mode: data mode
      \arg        OSPI_DATA_NONE: no data
      \arg        OSPI_DATA_1_LINE: 1-line data
      \arg        OSPI_DATA_2_LINE: 2-line data
      \arg        OSPI_DATA_4_LINE: 4-line data
      \arg        OSPI_DATA_8_LINE: 8-line data
    \param[in]  nbdata: data length
    \param[in]  dummy_cycles: dummy cycles between instruction and data
    \retval     none
*/
static void ospi_send_command(uint32_t operation_type, uint32_t ins_mode, uint32_t instruction, \
                              uint32_t addr_mode, uint32_t address, uint32_t addr_size, \
                              uint32_t data_mode, uint32_t nbdata, uint32_t dummy_cycles)
{
    ospi_regular_cmd_struct cmd_struct = {0};
    
    cmd_struct.operation_type       = operation_type;
    cmd_struct.ins_mode             = OSPI_INSTRUCTION_1_LINE;  /* 1 line instruction */
    cmd_struct.instruction          = instruction;
    cmd_struct.ins_size             = OSPI_INSTRUCTION_8_BITS;  /* 8 bits instruction */
    cmd_struct.addr_mode            = addr_mode;
    cmd_struct.address              = address;
    cmd_struct.addr_size            = addr_size;
    cmd_struct.addr_dtr_mode        = OSPI_ADDRDTR_MODE_DISABLE;
    cmd_struct.alter_bytes_mode     = OSPI_ALTERNATE_BYTES_NONE;
    cmd_struct.alter_bytes_dtr_mode = OSPI_ABDTR_MODE_DISABLE;
    cmd_struct.data_mode            = data_mode;
    cmd_struct.nbdata               = nbdata;
    cmd_struct.data_dtr_mode        = OSPI_DADTR_MODE_DISABLE;
    cmd_struct.dummy_cycles         = dummy_cycles;
    
    ospi_command_config(OSPI0, &ospi_struct, &cmd_struct);
}

/*!
    \brief      auto-polling to wait for memory ready (busy bit clear)
    \param[in]  none
    \retval     none
*/
static void ospi_autopolling_memready(void)
{
    ospi_autopolling_struct autopl_cfg_struct = {0};
    
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_ReadStatusReg1, \
                          OSPI_ADDRESS_NONE, NULL, NULL, \
                          OSPI_DATA_1_LINE, 1, OSPI_DUMYC_CYCLES_0); /* send command */
    
    autopl_cfg_struct.match = 0x00;
    autopl_cfg_struct.mask  = 0x01;
    autopl_cfg_struct.match_mode = OSPI_MATCH_MODE_AND;
    autopl_cfg_struct.interval = 0x10;
    autopl_cfg_struct.automatic_stop = OSPI_AUTOMATIC_STOP_MATCH;
    ospi_autopolling_mode(OSPI0, &ospi_struct, &autopl_cfg_struct);
}

/*!
    \brief      configure MDMA for OSPI data transfer (OSPI->memory)
    \param[in]  ospi_periph: OSPI peripheral
      \arg        OSPI0: OSPI0 peripheral
      \arg        OSPI1: OSPI1 peripheral
    \param[in]  pbuffer: destination buffer pointer
    \param[in]  datalen: data transfer length
    \retval     none
*/
static void ospi_mdma_config(uint32_t ospi_periph, uint8_t *pbuffer, uint16_t datalen)
{
    uint32_t mdma_request = MDMA_REQUEST_OSPI0_FT;
    uint32_t mdma_destination_bus = MDMA_DESTINATION_AXI;
    mdma_parameter_struct mdma_init_struct;
    
    if(ospi_periph == OSPI1)
    {
       mdma_request = MDMA_REQUEST_OSPI1_FT; 
    }
    
    if((((uint32_t)pbuffer & 0xFF000000U) == 0x20000000U) || \
       (((uint32_t)pbuffer & 0xFF000000U) == 0x00000000U)    )
    {
        mdma_destination_bus = MDMA_DESTINATION_AHB_TCM;
    }
    
    mdma_init_struct.request                = mdma_request;
    mdma_init_struct.trans_trig_mode        = MDMA_BUFFER_TRANSFER;
    mdma_init_struct.priority               = MDMA_PRIORITY_HIGH;
    mdma_init_struct.endianness             = MDMA_LITTLE_ENDIANNESS;
    mdma_init_struct.source_addr            = (uint32_t)&OSPI_DATA(ospi_periph);
    mdma_init_struct.destination_addr       = (uint32_t)pbuffer;
    mdma_init_struct.source_inc             = MDMA_SOURCE_INCREASE_DISABLE;
    mdma_init_struct.dest_inc               = MDMA_DESTINATION_INCREASE_8BIT;
    mdma_init_struct.source_data_size       = MDMA_SOURCE_DATASIZE_8BIT;
    mdma_init_struct.dest_data_dize         = MDMA_DESTINATION_DATASIZE_8BIT;
    mdma_init_struct.source_burst           = MDMA_SOURCE_BURST_SINGLE;
    mdma_init_struct.dest_burst             = MDMA_DESTINATION_BURST_16BEATS;
    mdma_init_struct.source_bus             = MDMA_SOURCE_AXI;
    mdma_init_struct.destination_bus        = mdma_destination_bus;
    mdma_init_struct.data_alignment         = MDMA_DATAALIGN_PKEN;
    mdma_init_struct.buff_trans_len         = 15;
    mdma_init_struct.tbytes_num_in_block    = datalen;
    mdma_init_struct.mask_addr              = 0;
    mdma_init_struct.mask_data              = 0;
    mdma_init_struct.bufferable_write_mode  = MDMA_BUFFERABLE_WRITE_DISABLE;
    
    mdma_init(MDMA_CH0, &mdma_init_struct);
    mdma_channel_enable(MDMA_CH0);
}

/*!
    \brief      enable write operation for W25Q256
    \param[in]  none
    \retval     none
*/
static void w25q256_write_enable(void)
{
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_WriteEnable, \
                      OSPI_ADDRESS_NONE, NULL, NULL, \
                      OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */ 
}

/*!
    \brief      disable write operation for W25Q256
    \param[in]  none
    \retval     none
*/
static void w25q256_write_disable(void)
{
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_WriteDisable, \
                      OSPI_ADDRESS_NONE, NULL, NULL, \
                      OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */ 
}

/*!
    \brief      enable write operation for volatile status register
    \param[in]  none
    \retval     none
*/
static void w25q256_write_enable_for_volatile_sr(void)
{
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_WriteEnableForVolatileSR, \
                      OSPI_ADDRESS_NONE, NULL, NULL, \
                      OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */
}

/*!
    \brief      read W25Q256 status register
    \param[in]  sr_num: status register number (1-3)
    \note       Status register description:
                reg1: bit 7   6   5   4   3   2   1   0
                          SRP TB  BP3 BP2 BP1 BP0 WEL BUSY
                reg2: bit 7   6   5   4   3   2   1   0
                          SUS CMP LB3 LB2 LB1 R   QE  SRL
                reg3: bit 7   6   5   4   3   2   1   0
                          R   D1  D0  R   R   WPS ADP ADS
                Where R: reserved, D1: DRV1, D0: DRV0
    \retval     status register value
*/
static uint8_t w25q256_read_sr(uint8_t sr_num)
{
    uint8_t command = 0, status = 0;
    
    switch(sr_num)
    {
        case 1:
            command = W25Q_ReadStatusReg1;
            break;
        case 2:
            command = W25Q_ReadStatusReg2;
            break;
        case 3:
            command = W25Q_ReadStatusReg3;
            break;
        default: break;
    }
    
    if(command)
    {
        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, command, \
                          OSPI_ADDRESS_NONE, NULL, NULL, \
                          OSPI_DATA_1_LINE, 1, OSPI_DUMYC_CYCLES_0); /* send command */ 
        ospi_receive(OSPI0, (uint8_t *)&status); /* receive data */
    }
    
    return status;
}

/*!
    \brief      write W25Q256 status register
    \param[in]  sr_num: status register number (1-3)
    \param[in]  bit: register bit mask to write
    \param[in]  data: data to write to register bits
    \note       Important: W25Q256 and GD25Q256 have different register behaviors
                For reg1: bit 7   6   5   4   3   2   1   0
                              SRP TB  BP3 BP2 BP1 BP0 WEL BUSY
                For reg2: bit 7   6   5   4   3   2   1   0
                              SUS CMP LB3 LB2 LB1 R   QE  SRL
                For reg3: bit 7   6   5   4   3   2   1   0
                              R   D1  D0  R   R   WPS ADP ADS
                LB1-3 are one-time programmable, use w25q256_write_sr_lb() to write them.
                ADP: Non-Volatile Writable, others: Volatile/Non-Volatile Writable
                R: reserved, D1: DRV1, D0: DRV0
    \retval     none
*/
static void w25q256_write_sr(uint8_t sr_num, uint8_t bit, uint8_t data)
{
    uint8_t command = 0, status = 0;
    
    status = w25q256_read_sr(sr_num);
    
    if(!((sr_num == 2) && (data & 0x38)))
    {
        switch(sr_num)
        {
            case 1:
                command = W25Q_WriteStatusReg1;
                break;
            case 2:
                command = W25Q_WriteStatusReg2;
                break;
            case 3:
                command = W25Q_WriteStatusReg3;
                break;
            default: break;
        }
        
        if(command)
        {
            status &= (~bit); /*!< clear bits to be written */
            status |= data; /*!< set new data bits */
            ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, command, \
                              OSPI_ADDRESS_NONE, NULL, NULL, \
                              OSPI_DATA_1_LINE, 1, OSPI_DUMYC_CYCLES_0); /* send command */ 
            ospi_transmit(OSPI0, (uint8_t *)&status); /* receive data */
            ospi_autopolling_memready();
        }
    }
}

/*!
    \brief      reset W25Q256 device
    \param[in]  none
    \retval     none
*/
static void w25q256_reset(void)
{
    while((w25q256_read_sr(1)&0x01) || (w25q256_read_sr(2)&0x80)); /*!< wait for busy bit and suspend bit to clear */
    
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_ResetEnable, \
                      OSPI_ADDRESS_NONE, NULL, NULL, \
                      OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */ 
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_ResetDevice, \
                      OSPI_ADDRESS_NONE, NULL, NULL, \
                      OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */ 
    delay_us(500);
}

/*!
    \brief      enter 4-byte address mode
    \param[in]  none
    \retval     none
*/
static void w25q256_enter_4byte_address_mode(void)
{
    /* W25Q256 */
//    if(!(w25q256_read_sr(3) & 0x01))
//    {
//        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_Enter4ByteAddressMode, \
//                          OSPI_ADDRESS_NONE, NULL, NULL, \
//                          OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */ 
//    }
//    
//    if(w25q256_read_sr(3) & 0x01)
//    {
//        PRINT_INFO("w25q256 enter 4-byte address mode successfully\r\n");
//    }
//    else
//    {
//        PRINT_ERROR("w25q256 enter 4-byte address mode unsuccessfully\r\n");
//    }
    
    /* GD25Q256 */
    if(!(w25q256_read_sr(2) & 0x01))
    {
        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_Enter4ByteAddressMode, \
                          OSPI_ADDRESS_NONE, NULL, NULL, \
                          OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /*!< send command */
    }
    
    if(w25q256_read_sr(2) & 0x01)
    {
        PRINT_INFO("w25q256 enter 4-byte address mode successfully\r\n");
    }
    else
    {
        PRINT_ERROR("w25q256 enter 4-byte address mode unsuccessfully\r\n");
    }
}

/*!
    \brief      exit 4-byte address mode
    \param[in]  none
    \retval     none
*/
static void w25q256_exit_4byte_address_mode(void)
{
    /* W25Q256 */
//    if((w25q256_read_sr(3) & 0x01))
//    {
//        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_Exit4ByteAddressMode, \
//                          OSPI_ADDRESS_NONE, NULL, NULL, \
//                          OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */ 
//    }
//    
//    if(!(w25q256_read_sr(3) & 0x01))
//    {
//        PRINT_INFO("w25q256 exit 4-byte address mode successfully\r\n");
//    }
//    else
//    {
//        PRINT_ERROR("w25q256 exit 4-byte address mode unsuccessfully\r\n");
//    }
    
    /* GD25Q256 */
    if((w25q256_read_sr(2) & 0x01))
    {
        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_Exit4ByteAddressMode, \
                          OSPI_ADDRESS_NONE, NULL, NULL, \
                          OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /* send command */ 
    }
    
    if(!(w25q256_read_sr(2) & 0x01))
    {
        PRINT_INFO("w25q256 exit 4-byte address mode successfully\r\n");
    }
    else
    {
        PRINT_ERROR("w25q256 exit 4-byte address mode unsuccessfully\r\n");
    }
}

/*!
    \brief      enable quad SPI mode for W25Q256
    \param[in]  none
    \retval     none
*/
static void w25q256_quad_enable(void)
{
    if((w25q256_read_sr(2) & 0x02) != 0x02)
    {
        w25q256_write_enable();
        w25q256_write_sr(2, 1<<1, 0x02);
    }
    
    if((w25q256_read_sr(2) & 0x02) == 0x02)
    {
        PRINT_INFO("w25q256 quad enable successfully\r\n");
    }
    else
    {
        PRINT_ERROR("w25q256 quad enable unsuccessfully\r\n");
    }
}

/*!
    \brief      disable quad SPI mode for W25Q256
    \param[in]  none
    \retval     none
*/
static void w25q256_quad_disable(void)
{
    PRINT_INFO("no operation(w25q256 quad disable)\r\n");
}

/*!
    \brief      set driver strength for W25Q256 output pins
    \param[in]  drv: driver strength level (0-3)
      \arg        0: 100% driver strength
      \arg        1: 75% driver strength
      \arg        2: 50% driver strength
      \arg        3: 25% driver strength
    \retval     none
*/
static void w25q256_set_drv(uint8_t drv)
{
    uint8_t status = 0;
    
    switch(drv)
    {
        case 0:
            status = 0x60;
            break;
        case 1:
            status = 0x40;
            break;
        case 2:
            status = 0x20;
            break;
        case 3:
            status = 0x00;
            break;
        default: 
            status = 0x60;
            break;
    }
    
    if((status != (w25q256_read_sr(3)&0x60)))
    {
        /* W25Q256 */
//        w25q256_write_enable_for_volatile_sr();
        
        /* GD25Q256 */
        w25q256_write_enable();
        
        w25q256_write_sr(3, 1<<5 | 1<<6, status);
    }
    
    if((status == (w25q256_read_sr(3)&0x60)))
    {
        PRINT_INFO("w25q256 set driver strength(%u%%) successfully\r\n",(drv+1)*25);
    }
    else
    {
        PRINT_ERROR("w25q256 set driver strength(%u%%) unsuccessfully\r\n",(drv+1)*25);
    }
}

/*!
    \brief      initialize W25Q256 device
    \param[in]  none
    \retval     W25Q256 initialization result
      \arg        0: initialization successful
      \arg        1: initialization failed
*/
uint8_t w25q256_init(void)
{
    uint32_t temp = 0;
  
    w25q256_ospi_config();
    temp = w25q256_read_device_id();
    
    /* support GD25Q256 */
    if(((temp & W25Q256) != W25Q256) && ((temp & GD25Q256) != GD25Q256))
    {
        PRINT_ERROR("w25q256 device id error\r\n");
        return 1;
    }
    
    w25q256_reset();                    /*!< reset W25Q256 */
    w25q256_enter_4byte_address_mode(); /*!< enter 4-byte address mode */
    w25q256_quad_enable();              /*!< enable W25Q256 quad mode */               
    w25q256_set_drv(3);                 /*!< set driver strength */
    w25q256_info_print();
    
    return 0;
}

/*!
    \brief      print W25Q256 device information
    \param[in]  none
    \retval     none
*/
void w25q256_info_print(void)
{
    uint8_t temp[8];
    
    PRINT_INFO("print w25q256 information>>\r\n");
    PRINT_INFO("/*********************************************************************/\r\n");
    *(uint32_t *)temp = w25q256_read_device_id();
    PRINT_INFO("w25q256 device id: 0x%03X\r\n",*(uint32_t *)temp);
    *(uint64_t *)temp = w25q256_read_unique_id();  
    PRINT_INFO("w25q256 unique id: 0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", \
                *(uint8_t *)temp, *(uint8_t *)(temp+1),*(uint8_t *)(temp+2),*(uint8_t *)(temp+3), \
                *(uint8_t *)(temp+4), *(uint8_t *)(temp+5),*(uint8_t *)(temp+6),*(uint8_t *)(temp+7));
    PRINT_INFO("w25q256 status register-1: 0x%02X\r\n",w25q256_read_sr(1));
    PRINT_INFO("w25q256 status register-2: 0x%02X\r\n",w25q256_read_sr(2));
    PRINT_INFO("w25q256 status register-3: 0x%02X\r\n",w25q256_read_sr(3));
    PRINT_INFO("/*********************************************************************/\r\n");
}

/*!
    \brief      read W25Q256 device ID
    \param[in]  none
    \retval     device ID
*/
uint32_t w25q256_read_device_id(void)
{
    uint8_t device_id[3];
    
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_DeviceID, 
                      OSPI_ADDRESS_NONE, NULL, NULL, \
                      OSPI_DATA_1_LINE, 3, OSPI_DUMYC_CYCLES_0);            /*!< send command */
    ospi_receive(OSPI0, device_id);                                         /*!< receive data */
    
    return (device_id[0]<<16) | (device_id[1]<<8) | device_id[2];
}

/*!
    \brief      read W25Q256 unique ID
    \param[in]  none
    \retval     unique ID
*/
uint64_t w25q256_read_unique_id(void)
{
    uint64_t unique_id = 0;

    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_UniqueID, \
                      OSPI_ADDRESS_1_LINE, 0, OSPI_ADDRESS_24_BITS, \
                      OSPI_DATA_1_LINE, 8, OSPI_DUMYC_CYCLES_16); /*!< send command */
    ospi_receive(OSPI0, (uint8_t *)&unique_id);  /*!< receive data */

    return unique_id;
}

/*!
    \brief      read data from W25Q256
    \param[in]  address: start address
    \param[in]  pbuffer: destination buffer pointer
    \param[in]  read_size: size of data to read
    \retval     W25Q256 read status
      \arg        0: read successful
      \arg        1: read failed
*/
uint8_t w25q256_read_data(uint32_t address, uint8_t *pbuffer, uint16_t read_size)
{
    if(read_size == 0)return 1;
    
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_FastReadQuad, \
                      OSPI_ADDRESS_4_LINES, address, OSPI_ADDRESS_32_BITS, \
                      OSPI_DATA_4_LINES, read_size, OSPI_DUMYC_CYCLES_6); /*!< send command */
  
    OSPI_CTL(OSPI0) = (OSPI_CTL(OSPI0) & ~OSPI_CTL_FMOD) | OSPI_INDIRECT_READ; /*!< indirect read mode */
    ospi_mdma_config(OSPI0, pbuffer, read_size);
    ospi_dma_enable(OSPI0); /*!< enable OSPI DMA transfer */
    OSPI_ADDR(OSPI0) = address; /*!< start OSPI transfer */
    while((MDMA_CHXSTAT0(MDMA_CH0) & MDMA_FLAG_CHTCF) == RESET); /*!< wait for MDMA transfer complete */
    MDMA_CHXSTATC(MDMA_CH0)   = 0x1F;
    while((OSPI_STAT(OSPI0) & OSPI_FLAG_TC) == RESET);
    OSPI_STATC(OSPI0) = OSPI_STATC_TCC; /*!< clear transfer complete flag */
    ospi_dma_disable(OSPI0); /*!< disable OSPI DMA transfer */
    
    return 0;
}

/*!
    \brief      erase W25Q256 sector
    \param[in]  sector: sector number (0-8191)
    \retval     none
*/
void w25q256_erase_sector(uint16_t sector)
{
    if(sector < 8192)
    {
        w25q256_write_enable();
        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_SectorErase, \
                          OSPI_ADDRESS_1_LINE, sector*4096 , OSPI_ADDRESS_32_BITS, \
                          OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /*!< send command */   
        ospi_autopolling_memready();
    }
}

/*!
    \brief      erase entire W25Q256 chip
    \param[in]  sector: parameter not used (for API compatibility)
    \retval     none
*/
void w25q256_erase_chip(uint16_t sector)
{
    w25q256_write_enable();
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_ChipErase, \
                      OSPI_ADDRESS_NONE, NULL , NULL, \
                      OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /*!< send command */   
    ospi_autopolling_memready();
}

/*!
    \brief      write page data to W25Q256
    \param[in]  address: start address
    \param[in]  pbuffer: source data buffer pointer
    \param[in]  write_size: size of data to write
    \retval     none
*/
static void w25q256_write_page(uint32_t address, uint8_t *pbuffer, uint16_t write_size)
{
    w25q256_write_enable();
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_WritePageQuad, \
                      OSPI_ADDRESS_1_LINE, address, OSPI_ADDRESS_32_BITS, \
                      OSPI_DATA_4_LINES, write_size, OSPI_DUMYC_CYCLES_0); /*!< send command */
    ospi_transmit(OSPI0, (uint8_t *)pbuffer);
    ospi_autopolling_memready();
}

/*!
    \brief      write page data without boundary check
    \param[in]  address: start address
    \param[in]  pbuffer: source data buffer pointer
    \param[in]  write_size: size of data to write
    \retval     none
*/
static void w25q256_write_page_nocheck(uint32_t address, uint8_t *pbuffer, uint16_t write_size)
{
    uint16_t pageremain = 0; /*!< remaining bytes in current page */
    
    pageremain = 256 - address % 256;
    
    if(write_size <= pageremain) /*!< write data size <= remaining bytes in current page */
    {
        pageremain = write_size;
    }
    
    while(1)
    {
        w25q256_write_page(address, pbuffer, pageremain);
        
        if(write_size == pageremain) /*!< write complete */
        {
            break;
        }
        else
        {
            pbuffer += pageremain;
            address += pageremain;
            write_size -= pageremain;
            
            if(write_size > 256)
            {
                pageremain = 256;
            }
            else
            {
                pageremain = write_size;
            }
        }
    }
}

/*!
    \brief      write data to W25Q256
    \param[in]  address: start address
    \param[in]  pbuffer: source data buffer pointer
    \param[in]  write_size: size of data to write
    \retval     W25Q256 write status
      \arg        0: write successful
      \arg        1: write failed
*/
uint8_t w25q256_write_data(uint32_t address, uint8_t *pbuffer, uint16_t write_size)
{
    if(write_size == 0)return 1;
    
    uint8_t *flash_buf;         /*!< flash buffer */
    uint16_t secpos;            /*!< sector position */
    uint16_t secoff;            /*!< offset within sector */
    uint16_t secremain;         /*!< remaining space in sector */
    uint16_t i;

    secpos = address / 4096;
    secoff = address % 4096;
    secremain = 4096 - secoff;
    
    flash_buf = mymalloc(SRAMIN, 4096);
    
    if(flash_buf == NULL)
    {
        PRINT_ERROR("apply 4 kbyte memory unsuccessfully in w25q256_write_data\r\n");
        return 1;
    }
    
    if (write_size <= secremain)
    {
        secremain = write_size;    /*!< not exceeding 4096 bytes */
    } 
    
    while(1)
    {
        w25q256_read_data(secpos * 4096, flash_buf, 4096);   /*!< read entire sector data */
        
        for (i = 0; i < secremain; i++)   /*!< check data */
        {
            if (flash_buf[secoff + i] != 0XFF)
            {
                break;      /*!< need erase, exit for loop */
            }
        }
        
        if (i < secremain)   /*!< need erase */
        {
            w25q256_erase_sector(secpos);  /*!< erase this sector */

            for (i = 0; i < secremain; i++)   /*!< copy data */
            {
                flash_buf[i + secoff] = pbuffer[i];
            }

            w25q256_write_page_nocheck(secpos * 4096, flash_buf, 4096);  /*!< write entire sector */
        }
        else        /*!< write to already erased area, write data directly */
        {
            w25q256_write_page_nocheck(address, pbuffer, secremain);  /*!< write data directly */
        }
        
        if (write_size == secremain)
        {
            break;  /*!< write complete */
        }
        else        /*!< write not complete */
        {
            secpos++;               /*!< sector address +1 */
            secoff = 0;             /*!< offset position to 0 */
            pbuffer += secremain;      /*!< pointer offset */
            address += secremain;      /*!< write address offset */
            write_size -= secremain;   /*!< byte count decrement */
            
            if (write_size > 4096)
            {
                secremain = 4096;      /*!< next sector cannot exceed write size */
            }
            else
            {
                secremain = write_size; /*!< next sector cannot exceed write size */
            }
        }
    }
    
    myfree(SRAMIN, flash_buf);
    
    return 0;
}

/*!
    \brief      read W25Q256 security register
    \param[in]  reg: security register number (1-3)
    \param[in]  address: start address within register
    \param[in]  pbuffer: destination buffer pointer
    \param[in]  read_size: size of data to read
    \note       Each security register is 256 bytes, address wraps automatically, read-only after programming
    \retval     W25Q256 security register read status
      \arg        0: read successful
      \arg        1: read failed
*/
uint8_t w25q256_read_security_register(uint8_t reg, uint8_t address, uint8_t *pbuffer, uint16_t read_size)
{
    uint32_t read_address = address;
    
    switch(reg)
    {
        case 1:
            read_address |= 0x00001000;
            break;
        case 2:
            read_address |= 0x00002000;
            break;
        case 3:
            read_address |= 0x00003000;
            break;
        default: break;
    }
    
    if(!(read_address & 0x00003000)) /*!< check address bits A13,A12 */
    {
        return 1;
    }
    
    if(read_size > (256 - address))
    {
        read_size = 256 - address;
    }

    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_ReadSecurityReg, \
                      OSPI_ADDRESS_1_LINE, read_address, OSPI_ADDRESS_32_BITS, \
                      OSPI_DATA_1_LINE, read_size, OSPI_DUMYC_CYCLES_8);
    ospi_receive(OSPI0, pbuffer);
    
    return 0;
}

/*!
    \brief      erase W25Q256 security register
    \param[in]  reg: security register number (1-3)
    \retval     W25Q256 security register erase status
      \arg        0: erase successful
      \arg        1: erase failed
*/
uint8_t w25q256_erase_security_register(uint8_t reg)
{
    uint32_t erase_address = 0;
    
    switch(reg)
    {
        case 1:
            erase_address |= 0x00001000;
            break;
        case 2:
            erase_address |= 0x00002000;
            break;
        case 3:
            erase_address |= 0x00003000;
            break;
        default: break;
    }
    
    if(!(erase_address & 0x00003000)) /*!< check address bits A13,A12 */
    {
        return 1;
    }
    
    w25q256_write_enable();
    ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_SecurityRegErase, \
                      OSPI_ADDRESS_1_LINE, erase_address, OSPI_ADDRESS_32_BITS, \
                      OSPI_DATA_NONE, NULL, OSPI_DUMYC_CYCLES_0); /*!< send command */   
    ospi_autopolling_memready();
    
    return 0;
}

/*!
    \brief      write W25Q256 security register
    \param[in]  reg: security register number (1-3)
    \param[in]  address: start address within register
    \param[in]  pbuffer: source data buffer pointer
    \param[in]  write_size: size of data to write
    \note       Each security register is 256 bytes, address wraps automatically, read-only after programming
    \retval     W25Q256 security register write status
      \arg        0: write successful
      \arg        1: write failed (memory allocation failed)
      \arg        2: write failed (invalid parameter)
*/
uint8_t w25q256_write_security_register(uint8_t reg, uint8_t address, uint8_t *pbuffer, uint16_t write_size)
{
    uint8_t *temp_buf;
    uint16_t i;
    uint16_t regremain = 0; /*!< remaining bytes in security register */
    uint32_t write_address = address;
    
    regremain = 256 - address;
    
    if(write_size <= regremain)
    {
        regremain = write_size;
    }
    
    temp_buf = mymalloc(SRAMIN, 256);
    
    if(temp_buf == NULL)
    {
        PRINT_ERROR("apply 256 byte memory unsuccessfully in w25q256_write_security_register\r\n");
        return 1;
    }
    
    switch(reg)
    {
        case 1:
            write_address |= 0x00001000;
            break;
        case 2:
            write_address |= 0x00002000;
            break;
        case 3:
            write_address |= 0x00003000;
            break;
        default: break;
    }
    
    if(!(write_address & 0x00003000)) /*!< check address bits A13,A12 */
    {
        myfree(SRAMIN, temp_buf);
        
        return 2;
    }
    
    w25q256_read_security_register(reg, write_address-address, temp_buf, 256);
    
    for(i = 0; i < regremain; i++)
    {
        if (temp_buf[address + i] != 0XFF)
        {
            break;      /*!< need erase, exit for loop */
        }
    }
    
    if(i < regremain)
    {
        w25q256_erase_security_register(reg);
        
        for(i = 0; i < regremain; i++)
        {
            temp_buf[address + i] = pbuffer[i];
        }
        
        w25q256_write_enable();
        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_WriteSecurityReg, \
                          OSPI_ADDRESS_1_LINE, write_address-address, OSPI_ADDRESS_32_BITS, \
                          OSPI_DATA_1_LINE, 256, OSPI_DUMYC_CYCLES_0);
        ospi_transmit(OSPI0, temp_buf);
        ospi_autopolling_memready();
    }
    else
    {
        w25q256_write_enable();
        ospi_send_command(OSPI_OPTYPE_COMMON_CFG, OSPI_INSTRUCTION_1_LINE, W25Q_WriteSecurityReg, \
                          OSPI_ADDRESS_1_LINE, write_address, OSPI_ADDRESS_32_BITS, \
                          OSPI_DATA_1_LINE, regremain, OSPI_DUMYC_CYCLES_0);
        ospi_transmit(OSPI0, pbuffer);
        ospi_autopolling_memready();
    }
    
    myfree(SRAMIN, temp_buf);
    
    return 0;
}
