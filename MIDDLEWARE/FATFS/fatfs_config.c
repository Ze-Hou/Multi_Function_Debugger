/*!
    \file       fatfs_config.c
    \brief      FATFS configuration module driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "./FATFS/fatfs_config.h"
#include "./MALLOC/malloc.h"
   
/******************************************************************************************/
/* configuration file system, using malloc mechanism */

/* logical drive working area (before calling any FATFS related functions, allocate memory for fs first) */
FATFS *fatfs[FF_VOLUMES];  

/******************************************************************************************/

/*!
    \brief      configure FATFS memory allocation
    \param[in]  none
    \param[out] none
    \retval     0, success; 1, memory allocation failed
*/
uint8_t fatfs_config(void)
{
    uint8_t i;
    uint8_t res = 0;

    for (i = 0; i < FF_VOLUMES; i++)
    {
        fatfs[i] = (FATFS *)mymalloc(SRAMIN, sizeof(FATFS));   /* allocate memory for logical drive i */

        if (!fatfs[i])break;
    }

    if (i == FF_VOLUMES)
    {
        return 0;   /* all allocations successful */
    }
    else 
    {
        return 1;   /* at least one allocation failed */
    }
}