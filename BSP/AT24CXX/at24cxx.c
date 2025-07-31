/*!
    \file       at24cxx.c
    \brief      AT24CXX EEPROM driver implementation with file system support
    \version    1.0
    \date       2025-07-24
    \author     Ze-Hou
*/

#include "./AT24CXX/at24cxx.h"
#include "./IIC/iic.h"
#include "./DELAY/delay.h"
#include "./USART/usart.h"
#include <stdbool.h>
#include <string.h>

at24cxx_file_system_struct at24cxx_file_system_info;

/*!
    \brief      initialize AT24CXX EEPROM
    \param[in]  none
    \param[out] none
    \retval     none
*/
void at24cxx_init(void)
{
    if(iicDevice_1.iic_state != true)
    {
        iic_init(&iicDevice_1);
    }
    at24cxx_file_system_info_print();
}

/*!
    \brief      read one byte from AT24CXX at specified address
    \param[in]  device: device index (0-7) for multiple EEPROM support
    \param[in]  addr: memory address to read from (16-bit address)
    \param[out] none
    \retval     data byte read from EEPROM
*/
uint8_t at24cxx_read_one_byte(uint8_t device, uint16_t addr)
{
    uint8_t temp = 0;
    uint8_t device_write = (device << 1) & 0xFE;
    uint8_t device_read = (device << 1) | 1;
    iic_start(&iicDevice_1);    /* Send I2C start signal */

    /* Different addressing schemes for AT24CXX variants:
     * 1. >AT24C16: Uses 2-byte addressing
     * 2. <=AT24C16: Uses device address embedding + 1-byte addressing
     *    AT24C01/02: 1 0 1 0 A2 A1 A0 R/W
     *    AT24C04:    1 0 1 0 A2 A1 a8 R/W  
     *    AT24C08:    1 0 1 0 A2 a9 a8 R/W
     *    AT24C16:    1 0 1 0 a10 a9 a8 R/W
     *    R/W: Read/Write bit (0=write, 1=read)
     *    A0/A1/A2: Hardware address pins
     *    a8/a9/a10: Memory address bits embedded in device address
     */    
    if (EE_TYPE > AT24C16)                          /* For >AT24C16, use 2-byte addressing */
    {
        iic_send_byte(device_write,&iicDevice_1);    /* Send device write address */
        iic_wait_ack(&iicDevice_1);                  /* Wait for ACK after each byte */
        iic_send_byte(addr >> 8,&iicDevice_1);       /* Send high byte address */
    }
    else 
    {
        iic_send_byte(device_write + ((addr >> 8) << 1),&iicDevice_1);   /* Send device address with embedded page bits */
    }
    
    iic_wait_ack(&iicDevice_1);                      /* Wait for ACK */
    iic_send_byte(addr % 256,&iicDevice_1);          /* Send low byte address */
    iic_wait_ack(&iicDevice_1);                      /* Address setup complete */
    
    iic_start(&iicDevice_1);                         /* Send repeated start signal */ 
    iic_send_byte(device_read,&iicDevice_1);         /* Send device read address */
    iic_wait_ack(&iicDevice_1);                      /* Wait for ACK */
    temp = iic_read_byte(0,&iicDevice_1);            /* Read one byte with NACK */
    iic_stop(&iicDevice_1);                          /* Send stop signal */
    return temp;
}

/*!
    \brief      write one byte to AT24CXX at specified address
    \param[in]  device: device index (0-7) for multiple EEPROM support
    \param[in]  addr: memory address to write to (16-bit address)
    \param[in]  data: data byte to write to EEPROM
    \param[out] none
    \retval     none
*/
void at24cxx_write_one_byte(uint8_t device, uint16_t addr, uint8_t data)
{
    uint8_t device_write = (device << 1) & 0xFE;
    /* Address format same as at24cxx_read_one_byte */
    iic_start(&iicDevice_1);                         /* Send I2C start signal */

    if (EE_TYPE > AT24C16)                          /* For >AT24C16, use 2-byte addressing */
    {
        iic_send_byte(device_write,&iicDevice_1);    /* Send device write address */
        iic_wait_ack(&iicDevice_1);                  /* Wait for ACK after each byte */
        iic_send_byte(addr >> 8,&iicDevice_1);       /* Send high byte address */
    }
    else
    {
        iic_send_byte(device_write + ((addr >> 8) << 1),&iicDevice_1);   /* Send device address with embedded page bits */
    }
    
    iic_wait_ack(&iicDevice_1);                      /* Wait for ACK */
    iic_send_byte(addr % 256,&iicDevice_1);          /* Send low byte address */
    iic_wait_ack(&iicDevice_1);                      /* Address setup complete */
    
    /* No repeated start needed for write - continue with data */
    iic_send_byte(data,&iicDevice_1);                /* Send data byte */
    iic_wait_ack(&iicDevice_1);                      /* Wait for ACK */
    iic_stop(&iicDevice_1);                          /* Send stop signal */
    delay_xms(10);                                   /* EEPROM write cycle time - wait 10ms before next operation */
}

#if EE_TYPE == AT24C512
/*!
    \brief      read page data from AT24C512 EEPROM
    \param[in]  device: device index (0-7) for multiple EEPROM support
    \param[in]  addr: starting memory address to read from
    \param[in]  datalen: number of bytes to read (max 128 for AT24C512)
    \param[out] pbuf: pointer to buffer for storing read data
    \retval     none
*/
static void at24c512_read_page(uint8_t device, uint16_t addr, uint8_t *pbuf, uint8_t datalen)
{
    uint8_t device_write = (device << 1) & 0xFE;
    uint8_t device_read = (device << 1) | 1;
    
    iic_start(&iicDevice_1);                         /* Send I2C start signal */
    iic_send_byte(device_write,&iicDevice_1);        /* Send device write address */
    iic_wait_ack(&iicDevice_1);                      /* Wait for ACK */
    iic_send_byte(addr >> 8,&iicDevice_1);           /* Send high byte address */
    iic_wait_ack(&iicDevice_1);                      /* Wait for ACK */
    iic_send_byte(addr % 256,&iicDevice_1);          /* Send low byte address */
    iic_wait_ack(&iicDevice_1);                      /* Address setup complete */
    
    iic_start(&iicDevice_1);                         /* Send repeated start signal */ 
    iic_send_byte(device_read,&iicDevice_1);         /* Send device read address */
    iic_wait_ack(&iicDevice_1);                      /* Wait for ACK */
    while(datalen--)
    {
        if(datalen == 0)                    
        {
            *pbuf++ = iic_read_byte(0,&iicDevice_1); /* Last byte: read with NACK */
        }
        else
        {
            *pbuf++ = iic_read_byte(1,&iicDevice_1); /* Read with ACK for more data */
        }
        
    }
    iic_stop(&iicDevice_1);                          /* Send stop signal */
}

/*!
    \brief      write page data to AT24C512 EEPROM
    \param[in]  device: device index (0-7) for multiple EEPROM support
    \param[in]  addr: starting memory address to write to
    \param[in]  pbuf: pointer to data buffer to write
    \param[in]  datalen: number of bytes to write (max 128 for AT24C512)
    \param[out] none
    \retval     none
*/
static void at24c512_write_page(uint8_t device, uint16_t addr, uint8_t *pbuf, uint8_t datalen)
{
    uint8_t device_write = (device << 1) & 0xFE;
    
    iic_start(&iicDevice_1);                     /* Send I2C start signal */
    iic_send_byte(device_write,&iicDevice_1);    /* Send device write address */
    iic_wait_ack(&iicDevice_1);                  /* Wait for ACK */
    iic_send_byte(addr >> 8,&iicDevice_1);       /* Send high byte address */
    iic_wait_ack(&iicDevice_1);                  /* Wait for ACK */
    iic_send_byte(addr % 256,&iicDevice_1);      /* Send low byte address */
    iic_wait_ack(&iicDevice_1);                  /* Address setup complete */
    
    /* For write operations, no repeated start needed - continue with data */
    while(datalen--)
    {
        iic_send_byte(*pbuf++, &iicDevice_1);    /* Send data byte */
        iic_wait_ack(&iicDevice_1);              /* Wait for ACK */
    }
    iic_stop(&iicDevice_1);                      /* Send stop signal */
    delay_xms(10);                               /* Page write cycle time */
}
#endif

/*!
    \brief      read multiple bytes from AT24CXX EEPROM
    \param[in]  addr: starting memory address to read from (32-bit address)
    \param[in]  datalen: number of bytes to read
    \param[out] pbuf: pointer to buffer for storing read data
    \retval     none
*/
void at24cxx_read(uint32_t addr, uint8_t *pbuf, uint16_t datalen)
{
    uint8_t state = 0;
    addr = addr % 131072;
    #if EE_TYPE == AT24C512
        while (datalen)
        {
            if(addr >= 65536)
            {
                addr = addr - 65536;
                state =1;
            }
            if(addr%128 && (datalen >= ((addr / 128 + 1)*128 - addr)))            /* Check if crossing page boundary */
            {
                if(state)
                {
                    at24c512_read_page(AT24CXX_DEVICE2_ADDR, addr, pbuf, (addr / 128 + 1)*128 - addr);
                }
                else
                {
                    at24c512_read_page(AT24CXX_DEVICE1_ADDR, addr, pbuf, (addr / 128 + 1)*128 - addr);
                }
                datalen = datalen - ((addr / 128 + 1)*128 - addr);
                pbuf += (addr / 128 + 1)*128 - addr;
                addr = (addr / 128 + 1)*128;
            }
            else if(datalen >= 128)
            {
                if(state)
                {
                   at24c512_read_page(AT24CXX_DEVICE2_ADDR, addr, pbuf, 128); 
                }
                else
                {
                    at24c512_read_page(AT24CXX_DEVICE1_ADDR, addr, pbuf, 128); 
                }
                addr += 128;
                pbuf += 128;
                datalen -= 128;
            }
            else
            {
                if(state)
                {
                   at24c512_read_page(AT24CXX_DEVICE2_ADDR, addr, pbuf, datalen); 
                }
                else
                {
                    at24c512_read_page(AT24CXX_DEVICE1_ADDR, addr, pbuf, datalen); 
                }
                datalen = 0;
            }
            if(state && datalen && (addr == 65536))
            {
                PRINT_ERROR("no memory available(at24cxx), the remaining %d byte(s) is(are) unread\r\n", datalen);
                break;
            }
        }
    #else
        while (datalen--)
        {
            if(addr >= 65536)
            {
                addr = addr - 65536;
                state =1;
            }
            if(state)
            {
               *pbuf++ = at24cxx_read_one_byte(AT24CXX_DEVICE2_ADDR, addr++); 
            }
            else
            {
                *pbuf++ = at24cxx_read_one_byte(AT24CXX_DEVICE1_ADDR, addr++);
            }
        }
    #endif
}

/*!
    \brief      write multiple bytes to AT24CXX EEPROM
    \param[in]  addr: starting memory address to write to (32-bit address)
    \param[in]  pbuf: pointer to data buffer to write
    \param[in]  datalen: number of bytes to write
    \param[out] none
    \retval     none
*/
void at24cxx_write(uint32_t addr, uint8_t *pbuf, uint16_t datalen)
{
    uint8_t state = 0;
    addr = addr % 131072;  
    #if EE_TYPE == AT24C512
        while (datalen)
        {
            if(addr >= 65536)
            {
                addr = addr - 65536;
                state =1;
            }
            if(addr%128 && (datalen >= ((addr / 128 + 1)*128 - addr)))            /* Check if crossing page boundary for write */
            { 
                if(state)
                {
                    at24c512_write_page(AT24CXX_DEVICE2_ADDR, addr, pbuf, (addr / 128 + 1)*128 - addr);
                }
                else
                {
                    at24c512_write_page(AT24CXX_DEVICE1_ADDR, addr, pbuf, (addr / 128 + 1)*128 - addr);
                }
                datalen = datalen - ((addr / 128 + 1)*128 - addr);
                pbuf += (addr / 128 + 1)*128 - addr;
                addr = (addr / 128 + 1)*128;
            }
            else if(datalen >= 128)
            {
                if(state)
                {
                    at24c512_write_page(AT24CXX_DEVICE2_ADDR, addr, pbuf, 128); 
                }
                else
                {
                    at24c512_write_page(AT24CXX_DEVICE1_ADDR, addr, pbuf, 128); 
                }
                addr += 128;
                pbuf += 128;
                datalen -= 128;
            }
            else
            {
                if(state)
                {
                    at24c512_write_page(AT24CXX_DEVICE2_ADDR, addr, pbuf, datalen); 
                }
                else
                {
                    at24c512_write_page(AT24CXX_DEVICE1_ADDR, addr, pbuf, datalen); 
                }
                datalen = 0;
            }
            if(state && datalen && (addr == 65536))
            {
                PRINT_ERROR("no memory available(at24cxx), the remaining %d byte(s) is(are) not written\r\n",datalen);
                break;
            }
        }
    #else    
        while (datalen--)
        {
            if(addr >= 65536)
            {
                addr = addr - 65536;
                state =1;
            }
            if(state)
            {
               at24cxx_write_one_byte(AT24CXX_DEVICE2_ADDR, addr++, *pbuf++);
            }
            else
            {
                at24cxx_write_one_byte(AT24CXX_DEVICE1_ADDR, addr++, *pbuf++);
            }
        }
    #endif
}

/*!
    \brief      format AT24CXX file system
    \param[in]  none
    \param[out] none
    \retval     none
*/
void at24cxx_file_system_format(void)
{
    uint8_t data_temp[AT24CXX_FILE_SYSTEM_PAGE_SIZE];
    uint8_t i=0;
    uint16_t write_start_page=0;

    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_PAGE_SIZE);    
    
    for(i=0; i < AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM; i++)                                      /* Format file system info pages */
    {
        at24cxx_write(write_start_page*128, data_temp, AT24CXX_FILE_SYSTEM_PAGE_SIZE);        /* Format info page */
        write_start_page ++;
    }
                                  
    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
    for(i=0; i < AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM; i++)                                 /* Format file system catalogue pages */
    {
        at24cxx_write(write_start_page*128, data_temp, AT24CXX_FILE_SYSTEM_PAGE_SIZE);        /* Format catalogue page */
        write_start_page ++;
    }
    
    memset(data_temp, 0xEE, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
    for(i=0; i < AT24CXX_FILE_SYSTEM_INDEX_PAGE_NUM; i++)                                     /* Format file system index pages */
    {
        at24cxx_write(write_start_page*128, data_temp, AT24CXX_FILE_SYSTEM_PAGE_SIZE);        /* Format index page */
        write_start_page ++;
    }
    
    at24cxx_file_system_info.file_system_state = 0xAA;                                        /* Set file system state flag */
    at24cxx_file_system_info.file_system_total_size = AT24CXX_FILE_SYSTEM_TOTAL_SIZE;
    at24cxx_file_system_info.file_system_page_size = AT24CXX_FILE_SYSTEM_PAGE_SIZE;
    at24cxx_file_system_info.file_system_info_page_num = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
    at24cxx_file_system_info.file_system_catalogue_page_num = AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM;
    at24cxx_file_system_info.file_system_index_page_num = AT24CXX_FILE_SYSTEM_INDEX_PAGE_NUM;
    at24cxx_file_system_info.file_system_storge_page_num = AT24CXX_FILE_SYSTEM_STORAGE_PAGE_NUM;
    at24cxx_file_system_info.file_system_catalogue_size = AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE;
    at24cxx_file_system_info.file_system_index_size = AT24CXX_FILE_SYSTEM_INDEX_SIZE;
    at24cxx_file_system_info.file_system_size_used = 0;
    at24cxx_file_system_info.file_system_size_idle = AT24CXX_FILE_SYSTEM_STORAGE_PAGE_NUM * AT24CXX_FILE_SYSTEM_PAGE_SIZE;
    at24cxx_file_system_info_set(at24cxx_file_system_info);
    at24cxx_file_system_info_print();
    PRINT_INFO("at24cxx_file_system is formatted successfully.\r\n");
}

/*!
    \brief      set file system information
    \param[in]  at24cxx_file_system_info: file system information structure to write
    \param[out] none
    \retval     none
*/
void at24cxx_file_system_info_set(at24cxx_file_system_struct at24cxx_file_system_info)
{
     at24cxx_write(0, (uint8_t *)&at24cxx_file_system_info, sizeof(at24cxx_file_system_info));
}

/*!
    \brief      print file system information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void at24cxx_file_system_info_print(void)
{ 
    at24cxx_read(0, (uint8_t *)&at24cxx_file_system_info, sizeof(at24cxx_file_system_info));
    if(at24cxx_file_system_info.file_system_state != 0xAA)
    {
        PRINT_ERROR("error: at24cxx_file_system does not exist, please format it.\r\n");
    }
    else
    {
        PRINT_INFO("print at24cxx_file_system information>>\r\n");
        PRINT_INFO("/*********************************************************************/\r\n");
        PRINT_INFO("at24cxx_file_system_state: 0x%X\r\n",at24cxx_file_system_info.file_system_state);
        PRINT_INFO("at24cxx_file_system_total_size: %d byte(s)\r\n",at24cxx_file_system_info.file_system_total_size);
        PRINT_INFO("at24cxx_file_system_page_size: %d byte(s)\r\n",at24cxx_file_system_info.file_system_page_size);
        PRINT_INFO("at24cxx_file_system_info_page_num: %d\r\n",at24cxx_file_system_info.file_system_info_page_num);
        PRINT_INFO("at24cxx_file_system_catalogue_page_num: %d\r\n",at24cxx_file_system_info.file_system_catalogue_page_num);
        PRINT_INFO("at24cxx_file_system_index_page_num: %d\r\n",at24cxx_file_system_info.file_system_index_page_num);
        PRINT_INFO("at24cxx_file_system_storge_page_num: %d\r\n",at24cxx_file_system_info.file_system_storge_page_num);
        PRINT_INFO("at24cxx_file_system_catalogue_size: %d byte(s)\r\n",at24cxx_file_system_info.file_system_catalogue_size);
        PRINT_INFO("at24cxx_file_system_index_size: %d byte(s)\r\n",at24cxx_file_system_info.file_system_index_size);
        PRINT_INFO("at24cxx_file_system_size_used: %d byte(s)\r\n",at24cxx_file_system_info.file_system_size_used);
        PRINT_INFO("at24cxx_file_system_size_idle: %d byte(s)\r\n",at24cxx_file_system_info.file_system_size_idle);
        PRINT_INFO("/*********************************************************************/\r\n");
    }
}

/*!
    \brief      get and display all files in the file system
    \param[in]  none
    \param[out] none
    \retval     none
*/
void at24cxx_file_system_file_get(void)
{
    uint8_t data_temp[AT24CXX_FILE_SYSTEM_PAGE_SIZE];
    uint8_t i = 0, j = 0, state = 0;
    uint8_t offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
    
    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
    for(i = 0; i < AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM; i++)
    {
        at24cxx_read((offset + i)*AT24CXX_FILE_SYSTEM_PAGE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
        for(j = 0; j < AT24CXX_FILE_SYSTEM_PAGE_SIZE; j += AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE)
        {
            if(data_temp[j] != 0xFF)
            {
                PRINT_INFO("[%d]%s\r\n",state, &data_temp[j]);
                state++;
            }
        }
    }
    if(state == 0)
    {
        PRINT_INFO("no files in the at24cxx_file_system.\r\n");
    }
}

/*!
    \brief      create a new file in the file system
    \param[in]  file_name: pointer to the file name string
    \param[out] none
    \retval     0 if successful, 0xFF if creation failed
*/
uint8_t at24cxx_file_system_file_creat(const uint8_t *file_name)
{
    uint8_t data_temp[AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE];
    uint8_t i = 0, addr = 0, state = 0xFF;
    uint8_t offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
    
    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
    for(i = 0; i < (AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM * AT24CXX_FILE_SYSTEM_PAGE_SIZE / AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE); i++)
    {
        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + i*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
        if(data_temp[0] == 0xFF)
        {
            addr = i;
            state = 0;
            break;
        }
    }
    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
    for(i = 0; i < (AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM * AT24CXX_FILE_SYSTEM_PAGE_SIZE / AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE); i++)  /* Check for duplicate filenames */
    {
        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + i*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
        if(data_temp[0] != 0xFF)
        {
            if(strcmp((char *)data_temp, (char *)file_name) == 0)
            {
                state = 1;
            }
        }
    }
    if(state == 0)
    {
        if(strlen((char *)file_name) < 28)                      /* Filename must be < 28 chars (32-byte entry - 2 bytes size - 2 bytes index) */
        {
            at24cxx_write(offset * AT24CXX_FILE_SYSTEM_PAGE_SIZE + addr*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE, (uint8_t *)file_name, strlen((char *)file_name)+1);/* Write filename */
            state = 0;
        }
        else
        {
            state = 2;
        } 
    }
    /* Print result */
    if(state == 0)
    {
        PRINT_INFO("file(%s) is created in the at24cxx_file_system.\r\n",file_name);
    }
    else if(state == 1)
    {
        PRINT_ERROR("file(%s) already exists in the at24cxx_file_system.\r\n",file_name);
    }
    else if(state == 2)
    {
        PRINT_ERROR("file name(%s) contains a maximum of 27 characters in the at24cxx_file_system.\r\n",file_name);
    }
    else
    {
        PRINT_ERROR("files full in the at24cxx_file_system.\r\n");
    }
    return state;
}

/*!
    \brief      open a file in the file system
    \param[in]  file_name: pointer to the file name string to search for
    \param[out] none
    \retval     file index if found, 0xFF if file not found
*/
uint8_t at24cxx_file_system_file_open(const uint8_t *file_name)
{
    uint8_t data_temp[AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE];
    uint8_t i = 0, addr = 0, state = 0xFF;
    uint8_t offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
    
    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
    for(i = 0; i < (AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM * AT24CXX_FILE_SYSTEM_PAGE_SIZE / AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE); i++)
    {
        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + i*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
        if(data_temp[0] != 0xFF)
        {
            if(strcmp((char *)data_temp, (char *)file_name) == 0)
            {
                state = 0;
                addr = i;
                break;
            }
        }
    }
    if(state == 0)
    {
        PRINT_INFO("file(%s) opened  successfully in the at24cxx_file_system.\r\n",file_name);
        return addr; 
    }
    else
    {
        PRINT_ERROR("file(%s) does not exist in the at24cxx_file_system.\r\n",file_name);
        return 0xFF;
    }
}

/*!
    \brief      write data to a file in the file system
    \param[in]  file_name: pointer to the file name string
    \param[in]  write_data: pointer to data buffer to write
    \param[in]  file_size: size of data to write (max 1KB)
    \param[out] none
    \retval     0 if successful, error code if write failed
*/
uint8_t at24cxx_file_system_file_write(const uint8_t *file_name, const uint8_t *write_data, const uint16_t file_size)
{
    uint8_t addr = 0, index_num = 0, temp = 0, sum_check_write=0, sum_check_read=0, n=0;
    uint8_t data_temp[AT24CXX_FILE_SYSTEM_PAGE_SIZE];
    uint16_t offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
    uint16_t i=0;
    uint16_t index[9];
    
    addr = at24cxx_file_system_file_open(file_name);
    if(addr != 0xFF)
    {
        memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + addr*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
        if(file_size <= 1024 && file_size)                                  /* Write data size must be <= 1KB */  
        {
            index_num = file_size/AT24CXX_FILE_SYSTEM_PAGE_SIZE;
            if(file_size%AT24CXX_FILE_SYSTEM_PAGE_SIZE)index_num += 1;      /* Calculate required pages */
            temp = index_num;
            if(*(uint16_t *)(data_temp+30) == 0xFFFF)                       /* First time writing to file */                       
            {

                offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM+AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM;
                for(i = 0; i < AT24CXX_FILE_SYSTEM_STORAGE_PAGE_NUM; i++)    /* Search for free pages */
                {
                    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                    at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + i*AT24CXX_FILE_SYSTEM_INDEX_SIZE, data_temp, AT24CXX_FILE_SYSTEM_INDEX_SIZE);

                    if(*(uint16_t *)(data_temp) == 0xEEEE)                  /* Check if page is available */
                    {
                        index[index_num - temp] = i;
                        temp--;
                        if(temp == 0)
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + addr*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE+30, (uint8_t *)&index[0], AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM+AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM;
                for(i = 0; i < 8; i++)                                                      /* Search for existing file pages */
                {
                    at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, (uint8_t *)&index[i+1], AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                    if(index[i+1] == 0xFFFF)                                                /* End of file page chain */
                    {
                        temp = i+1;
                        break;
                    }
                }
                if(temp < index_num)                                                        /* File needs more pages than before */
                {
                    for(i = 0; i < AT24CXX_FILE_SYSTEM_STORAGE_PAGE_NUM; i++)               /* Search for additional free pages */
                    {
                        memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + i*AT24CXX_FILE_SYSTEM_INDEX_SIZE, data_temp, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                        if(*(uint16_t *)(data_temp) == 0xEEEE)
                        {
                            index[temp] = i;
                            temp++;
                            if(temp == index_num)
                            {
                                temp = 0;
                                break;
                            }
                        }
                    }
                }
                else if(temp >= index_num)                                                   /* File needs fewer pages than before */
                {
                    for(i = index_num; i < temp; i++)
                    {
                        memset(data_temp, 0xEE, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                        at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, data_temp, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                    }
                    temp = 0;
                }
            }
            if(temp == 0)
            {
                /* Write index data */
                offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
                at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + addr*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE+30, (uint8_t *)&index[0], 2);      /* Write directory index */
                offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM+AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM;
                for(i = 0; i < index_num-1; i++)
                {
                    at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, (uint8_t *)&index[i+1], 2);   /* Write page chain */
                }
                memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[index_num-1]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, data_temp, AT24CXX_FILE_SYSTEM_INDEX_SIZE);/* Mark end of chain */
                /* Write file data */
                offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM+AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM+AT24CXX_FILE_SYSTEM_INDEX_PAGE_NUM;
                for(i = 0; i < index_num-1; i++)
                {
                    for(n = 0; n < AT24CXX_FILE_SYSTEM_PAGE_SIZE; n++)
                    {
                        sum_check_write += *(write_data + n);
                    }
                    at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i]*AT24CXX_FILE_SYSTEM_PAGE_SIZE, (uint8_t *)write_data, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
                    write_data += AT24CXX_FILE_SYSTEM_PAGE_SIZE;
                    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
                    at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i]*AT24CXX_FILE_SYSTEM_PAGE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
                    for(n = 0; n < AT24CXX_FILE_SYSTEM_PAGE_SIZE; n++)
                    {
                        sum_check_read += *(data_temp + n);
                    }
                    if(sum_check_write != sum_check_read)
                    {
                        PRINT_ERROR("file(%s) write failure in the at24cxx_file_system.\r\n",file_name);
                        return 3;
                    }
                }
                /* Write last partial page */
                for(n = 0; n < file_size-AT24CXX_FILE_SYSTEM_PAGE_SIZE*(index_num-1); n++)
                {
                    sum_check_write += *(write_data + n);
                }
                at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[index_num-1]*AT24CXX_FILE_SYSTEM_PAGE_SIZE, (uint8_t *)write_data, \
                              file_size-AT24CXX_FILE_SYSTEM_PAGE_SIZE*(index_num-1));
                memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
                at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[index_num-1]*AT24CXX_FILE_SYSTEM_PAGE_SIZE, data_temp, \
                             file_size-AT24CXX_FILE_SYSTEM_PAGE_SIZE*(index_num-1));
                for(n = 0; n < file_size-AT24CXX_FILE_SYSTEM_PAGE_SIZE*(index_num-1); n++)
                {
                    sum_check_read += *(data_temp + n);
                }
                if(sum_check_write != sum_check_read)
                {
                    PRINT_ERROR("file(%s) write failure in the at24cxx_file_system.\r\n",file_name);
                    return 3;
                }
                offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
                at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + addr*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE+28, (uint8_t *)&file_size, 2);/* Write file size to directory */
                PRINT_INFO("file(%s) write successfully in the at24cxx_file_system.\r\n",file_name);
            }
            else
            {
                PRINT_ERROR("the available space is insufficient in the at24cxx_file_system.\r\n");
                return 2;
            }
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

/*!
    \brief      read data from a file in the file system
    \param[in]  file_name: pointer to the file name string to read
    \param[out] read_data: pointer to buffer for storing read data
    \retval     0 if successful, error code if read failed
*/
uint8_t at24cxx_file_system_file_read(const uint8_t *file_name, uint8_t *read_data)
{
    uint8_t addr = 0, index_num = 0;
    uint16_t i=0, file_size = 0, temp = 0;
    uint16_t index[8];
    uint16_t offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
    
    addr = at24cxx_file_system_file_open(file_name);
    if(addr != 0xFF)
    {
        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + addr*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE+28, (uint8_t *)&file_size, 2); /* Get file size */
        if(file_size <= 1024 && file_size)
        {
            index_num = file_size/AT24CXX_FILE_SYSTEM_PAGE_SIZE;
            if(file_size%AT24CXX_FILE_SYSTEM_PAGE_SIZE)index_num += 1;
            at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + addr*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE+30, (uint8_t *)&index[0], AT24CXX_FILE_SYSTEM_INDEX_SIZE);/* Get first page index */
            offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM+AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM;
            if(index[0] != 0xFFFF)
            {
                for(i = 0; i < index_num-1; i++)                /* Search page chain */
                {
                    at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, (uint8_t *)&index[i+1], AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                    at24cxx_read((offset+AT24CXX_FILE_SYSTEM_INDEX_PAGE_NUM)*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i]*AT24CXX_FILE_SYSTEM_PAGE_SIZE, read_data, AT24CXX_FILE_SYSTEM_PAGE_SIZE);
                    read_data += AT24CXX_FILE_SYSTEM_PAGE_SIZE;
                    if(i+1 == index_num-1)                      /* Read last page validation */
                    {
                        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[i+1]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, (uint8_t *)&temp, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                        if(temp != 0xFFFF)                      /* Check if chain ends correctly */
                        {
                            PRINT_ERROR("file(%s) read failure in the at24cxx_file_system.\r\n",file_name);
                            return 3;
                        }
                    }
                }           
                at24cxx_read((offset+AT24CXX_FILE_SYSTEM_INDEX_PAGE_NUM)*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[index_num-1]*AT24CXX_FILE_SYSTEM_PAGE_SIZE,\
                              read_data, file_size-AT24CXX_FILE_SYSTEM_PAGE_SIZE*(index_num-1));
            }
        }
        else
        {
            PRINT_ERROR("file(%s) error in the at24cxx_file_system.\r\n",file_name);
            return 2;
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

/*!
    \brief      delete a file from the file system
    \param[in]  file_name: pointer to the file name string to delete
    \param[out] none
    \retval     0 if successful, error code if deletion failed
*/
uint8_t at24cxx_file_system_file_delete(const uint8_t *file_name)
{
    uint8_t data_temp[AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE];
    uint8_t i = 0, j = 0, state = 0xFF;
    uint8_t offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM;
    uint16_t index[9];
    
    memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
    for(i = 0; i < (AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM * AT24CXX_FILE_SYSTEM_PAGE_SIZE / AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE); i++)
    {
        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + i*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
        if(data_temp[0] != 0xFF)
        {
            if(strcmp((char *)data_temp, (char *)file_name) == 0)
            {
                index[0] = *(uint16_t *)(data_temp+30);
                memset(data_temp, 0xFF, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
                at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + i*AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE, data_temp, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);/* Clear directory entry */
                offset = AT24CXX_FILE_SYSTEM_INFO_PAGE_NUM+AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM;
                memset(data_temp, 0xEE, AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE);
                if(index[0] != 0xFFFF)
                {
                    for(j = 0; j < 8; j++)
                    {
                        at24cxx_read(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[j]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, (uint8_t *)&index[j+1], AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                        at24cxx_write(offset*AT24CXX_FILE_SYSTEM_PAGE_SIZE + index[j]*AT24CXX_FILE_SYSTEM_INDEX_SIZE, data_temp, AT24CXX_FILE_SYSTEM_INDEX_SIZE);
                        if(index[j+1] == 0xFFFF)
                        {
                            state = 0;
                            break;
                        }                   
                    }
                }
                break;
            }
            else
            {
                state = 1;
            }
        }
        if(i+1 == AT24CXX_FILE_SYSTEM_CATALOGUE_PAGE_NUM * AT24CXX_FILE_SYSTEM_PAGE_SIZE / AT24CXX_FILE_SYSTEM_CATALOGUE_SIZE)
        {
            state = 1;
        }
    }
    if(state == 0)
    {
        PRINT_INFO("file(%s) is deleteed successfully in the at24cxx_file_system.\r\n",file_name);
    }
    else if(state == 1)
    {
        PRINT_ERROR("file(%s) does not exist in the at24cxx_file_system.\r\n",file_name);
    }
    return state;
}
