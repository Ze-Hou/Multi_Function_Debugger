/*!
    \file       fatfs_config.h
    \brief      header file for FATFS configuration module
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __FATFS_CONFIG_H
#define __FATFS_CONFIG_H
#include <stdint.h>
#include "ff.h"

/* logical drive working area array */
extern FATFS *fatfs[FF_VOLUMES];

/* function declarations */
uint8_t fatfs_config(void);             /*!< configure FATFS memory allocation */
#endif
