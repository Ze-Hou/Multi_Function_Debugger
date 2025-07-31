/*!
    \file       fonts.h
    \brief      header file for font management module
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#ifndef __FONTS_H
#define __FONTS_H
#include "gd32h7xx.h"
#include <stdint.h>

/* font information storage base address
 * occupies 41 bytes, first 1 byte is used to mark whether font library exists, then every 8 bytes as a group, respectively storing start address and file size
 */
extern uint32_t FONTINFOADDR;

/* font information structure definition
 * stores various font library information including address and size
 */
typedef __PACKED_STRUCT
{
    uint8_t fontok;                         /* font exist flag, 0XAA means font library exists, other values mean font library does not exist */
    uint32_t ugbkaddr;                      /* unigbk address */
    uint32_t ugbksize;                      /* unigbk size */
    uint32_t ascii12staddr;                 /* ascii12st address */
    uint32_t ascii12stsize;                 /* ascii12st size */
    uint32_t ascii16staddr;                 /* ascii16st address */
    uint32_t ascii16stsize;                 /* ascii16st size */
    uint32_t ascii24staddr;                 /* ascii24st address */
    uint32_t ascii24stsize;                 /* ascii24st size */
    uint32_t ascii32staddr;                 /* ascii32st address */
    uint32_t ascii32stsize;                 /* ascii32st size */
    uint32_t ascii12spaddr;                 /* ascii12sp address */
    uint32_t ascii12spsize;                 /* ascii12sp size */
    uint32_t ascii16spaddr;                 /* ascii16sp address */
    uint32_t ascii16spsize;                 /* ascii16sp size */
    uint32_t ascii24spaddr;                 /* ascii24sp address */
    uint32_t ascii24spsize;                 /* ascii24sp size */
    uint32_t ascii32spaddr;                 /* ascii32sp address */
    uint32_t ascii32spsize;                 /* ascii32sp size */
    uint32_t gbk16addr;                     /* gbk16 address */
    uint32_t gbk16size;                     /* gbk16 size */
    uint32_t lvfontsimsun12addr;            /* lvfontsimsun12 address */
    uint32_t lvfontsimsun12size;            /* lvfontsimsun12 size */
    uint32_t lvfontsimsun16addr;            /* lvfontsimsun16 address */
    uint32_t lvfontsimsun16size;            /* lvfontsimsun16 size */
    uint32_t lvfontsimsun24addr;            /* lvfontsimsun24 address */
    uint32_t lvfontsimsun24size;            /* lvfontsimsun24 size */
    uint32_t lvfontsimsun32addr;            /* lvfontsimsun32 address */
    uint32_t lvfontsimsun32size;            /* lvfontsimsun32 size */
    uint32_t lvfontsimsun48addr;            /* lvfontsimsun48 address */
    uint32_t lvfontsimsun48size;            /* lvfontsimsun48 size */
    uint32_t lvfontfzst12addr;              /* lvfontfzst12 address */
    uint32_t lvfontfzst12size;              /* lvfontfzst12 size */
    uint32_t lvfontfzst16addr;              /* lvfontfzst16 address */
    uint32_t lvfontfzst16size;              /* lvfontfzst16 size */
    uint32_t lvfontfzst24addr;              /* lvfontfzst24 address */
    uint32_t lvfontfzst24size;              /* lvfontfzst24 size */
    uint32_t lvfontfzst32addr;              /* lvfontfzst32 address */
    uint32_t lvfontfzst32size;              /* lvfontfzst32 size */
    uint32_t lvfontfzst48addr;              /* lvfontfzst48 address */
    uint32_t lvfontfzst48size;              /* lvfontfzst48 size */
    uint32_t lvfontfzst56addr;              /* lvfontfzst56 address */
    uint32_t lvfontfzst56size;              /* lvfontfzst56 size */
}font_info_struct;

/* font information structure instance */
extern font_info_struct font_info;

uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t *src, uint16_t color);  /*!< update all font libraries */
uint8_t fonts_init(void);       /*!< initialize font library */
#endif