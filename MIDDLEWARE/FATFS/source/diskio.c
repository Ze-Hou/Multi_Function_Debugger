/*!
    \file       diskio.c
    \brief      FATFS disk I/O interface driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
    \note       Ported and modified from FatFs
*/

#include "diskio.h"
#include "./MALLOC/malloc.h"
#include "./USART/usart.h"
#include "./SDIO/sdio_emmc.h"

#define EMMC_CARD     0       /* eMMC card, device number is 0 */

/*!
    \brief      get disk status
    \param[in]  pdrv: drive number 0~9
    \param[out] none
    \retval     disk status
*/
DSTATUS disk_status (
    BYTE pdrv       /* Physical drive number to identify the drive */
)
{
    return RES_OK;
}

/*!
    \brief      initialize disk
    \param[in]  pdrv: drive number 0~9
    \param[out] none
    \retval     disk status
*/
DSTATUS disk_initialize (
    BYTE pdrv       /* Physical drive number to identify the drive */
)
{
    uint8_t res = 0;

    switch (pdrv)
    {
        case EMMC_CARD:                 /* eMMC card */
            if(emmc_info.emmc_init_state !=0xAA)
            {
                res = emmc_init();          /* eMMC card initialization */
            }
            break;

        default:
            res = 1;
            break;
    }

    if (res)
    {
        return  STA_NOINIT;
    }
    else
    {
        return 0; /* initialization successful */
    }
}

/*!
    \brief      read disk sectors
    \param[in]  pdrv: drive number 0~9
    \param[in]  buff: data receive buffer start address
    \param[in]  sector: sector address
    \param[in]  count: number of sectors to read
    \param[out] none
    \retval     read result
*/
DRESULT disk_read (
    BYTE pdrv,      /* Physical drive number to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    DWORD sector,   /* Sector address in LBA */
    UINT count      /* Number of sectors to read */
)
{
    uint8_t res = 0;

    if (!count)return RES_PARERR;   /* count cannot be 0, otherwise return parameter error */

    switch (pdrv)
    {
        case EMMC_CARD:       /* eMMC card */
            res = emmc_read_disk((uint32_t *)buff, sector, count);
            break;

        default:
            res = 1;
    }

    /* process return value, convert to return value for ff.c */
    if (res == 0x00)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR; 
    }
}

/*!
    \brief      write disk sectors
    \param[in]  pdrv: drive number 0~9
    \param[in]  buff: data to write buffer start address
    \param[in]  sector: sector address
    \param[in]  count: number of sectors to write
    \param[out] none
    \retval     write result
*/
DRESULT disk_write (
    BYTE pdrv,          /* Physical drive number to identify the drive */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Sector address in LBA */
    UINT count          /* Number of sectors to write */
)
{
    uint8_t res = 0;

    if (!count)return RES_PARERR;   /* count cannot be 0, otherwise return parameter error */

    switch (pdrv)
    {
        case EMMC_CARD:       /* eMMC card */
            res = emmc_write_disk((uint32_t *)buff, sector, count);
            break;
        default:
            res = 1;
    }

    /* process return value, convert to return value for ff.c */
    if (res == 0x00)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR; 
    }
}

/*!
    \brief      get disk drive parameters
    \param[in]  pdrv: drive number 0~9
    \param[in]  cmd: control command
    \param[in]  buff: send/receive buffer pointer
    \param[out] none
    \retval     control result
*/
DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive number (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    DRESULT res;

    if (pdrv == EMMC_CARD)    /* eMMC card */
    {
        switch (cmd)
        {
            case CTRL_SYNC:
                res = RES_OK;
                break;

            case GET_SECTOR_SIZE:
                *(DWORD *)buff = 512;
                res = RES_OK;
                break;

            case GET_BLOCK_SIZE:
                *(WORD *)buff = (((uint8_t)(emmc_info.csd[1] >> 10) & 0x1F) + 1) * (((uint8_t)(emmc_info.csd[1] >> 5) & 0x1F) + 1);
                res = RES_OK;
                break;

            case GET_SECTOR_COUNT:
                *(DWORD *)buff = (*(uint32_t *)((uint8_t *)&emmc_info.ext_csd + 212));
                res = RES_OK;
                break;

            default:
                res = RES_PARERR;
                break;
        }
    }
    else
    {
        res = RES_ERROR;    /* other devices not supported */
    }
    
    return res;
}
