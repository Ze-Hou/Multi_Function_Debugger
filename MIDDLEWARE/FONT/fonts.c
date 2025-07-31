/*!
    \file       fonts.c
    \brief      font management module driver
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "./FONT/fonts.h"
#include "./SCREEN/RGBLCD/rgblcd.h"
#include "./MALLOC/malloc.h"
#include "./DELAY/delay.h"
#include "./USART/usart.h"
#include "./W25Q256/w25q256.h"
#include "ff.h"
#include <string.h>

#define FONTINFOADDR        0
#define FONTSECSIZE         8147    /* sectors need to be erased */
 
/* stores various font library information including address and size */
font_info_struct font_info;

/* font library file paths in memory */
char *const FONT_GBK_PATH[21] =
{
    "/UNIGBK.BIN",                  /* UNIGBK.BIN storage location */ 
    "/ASCll12_ST.BIN",              /* ASCll12_ST.BIN storage location */
    "/ASCll16_ST.BIN",              /* ASCll16_ST.BIN storage location */
    "/ASCll24_ST.BIN",              /* ASCll24_ST.BIN storage location */
    "/ASCll32_ST.BIN",              /* ASCll32_ST.BIN storage location */
    "/ASCll12_SP.BIN",              /* ASCll12_SP.BIN storage location */
    "/ASCll16_SP.BIN",              /* ASCll16_SP.BIN storage location */
    "/ASCll24_SP.BIN",              /* ASCll24_SP.BIN storage location */
    "/ASCll32_SP.BIN",              /* ASCll32_SP.BIN storage location */
    "/GBK16.FON",                   /* GBK16.FON storage location */
    "/lv_font_simsun_12.bin",       /* lv_font_simsun_12.bin storage location */
    "/lv_font_simsun_16.bin",       /* lv_font_simsun_16.bin storage location */
    "/lv_font_simsun_24.bin",       /* lv_font_simsun_24.bin storage location */
    "/lv_font_simsun_32.bin",       /* lv_font_simsun_32.bin storage location */
    "/lv_font_simsun_48.bin",       /* lv_font_simsun_48.bin storage location */
    "/lv_font_fzst_12.bin",         /* lv_font_fzst_12.bin storage location */
    "/lv_font_fzst_16.bin",         /* lv_font_fzst_16.bin storage location */
    "/lv_font_fzst_24.bin",         /* lv_font_fzst_24.bin storage location */
    "/lv_font_fzst_32.bin",         /* lv_font_fzst_32.bin storage location */
    "/lv_font_fzst_48.bin",         /* lv_font_fzst_48.bin storage location */
    "/lv_font_fzst_56.bin",         /* lv_font_fzst_56.bin storage location */
};

/* update progress display messages */
char *const FONT_UPDATE_REMIND_TBL[21] =
{
    "Updating UNIGBK.BIN",                  /* indicates updating UNIGBK.bin */
    "Updating ASCll12_ST.BIN",              /* indicates updating ASCll12_ST */
    "Updating ASCll16_ST.BIN",              /* indicates updating ASCll16_ST */
    "Updating ASCll24_ST.BIN",              /* indicates updating ASCll24_ST */
    "Updating ASCll32_ST.BIN",              /* indicates updating ASCll32_ST */
    "Updating ASCll12_SP.BIN",              /* indicates updating ASCll12_SP */
    "Updating ASCll16_SP.BIN",              /* indicates updating ASCll16_SP */
    "Updating ASCll24_SP.BIN",              /* indicates updating ASCll24_SP */
    "Updating ASCll32_SP.BIN",              /* indicates updating ASCll32_SP */
    "Updating GBK16.FON",                   /* indicates updating GBK16 */
    "Updating lv_font_simsun_12.bin",       /* indicates updating lv_font_simsun_12 */
    "Updating lv_font_simsun_16.bin",       /* indicates updating lv_font_simsun_16 */
    "Updating lv_font_simsun_24.bin",       /* indicates updating lv_font_simsun_24 */
    "Updating lv_font_simsun_32.bin",       /* indicates updating lv_font_simsun_32 */
    "Updating lv_font_simsun_48.bin",       /* indicates updating lv_font_simsun_48 */
    "Updating lv_font_fzst_12.bin",         /* indicates updating lv_font_fzst_12 */
    "Updating lv_font_fzst_16.bin",         /* indicates updating lv_font_fzst_16 */
    "Updating lv_font_fzst_24.bin",         /* indicates updating lv_font_fzst_24 */
    "Updating lv_font_fzst_32.bin",         /* indicates updating lv_font_fzst_32 */
    "Updating lv_font_fzst_48.bin",         /* indicates updating lv_font_fzst_48 */
    "Updating lv_font_fzst_56.bin",         /* indicates updating lv_font_fzst_56 */
};

/*!
    \brief      show current font update progress
    \param[in]  x, y: coordinates
    \param[in]  totsize: total file size
    \param[in]  pos: current file pointer position
    \param[in]  color: font color
    \param[out] none
    \retval     none
*/
static void fonts_progress_show(uint16_t x, uint16_t y, uint32_t totsize, uint32_t pos, uint16_t color)
{
    float prog;
    uint8_t t = 0xFF;
    prog = (float)pos / totsize;
    prog *= 100;

    if (t != prog)
    {
        rgblcd_show_string(x + 3 * 16 / 2, y, 240, 320, "%", RGBLCD_FONT_16, color);
        t = prog;

        if (t > 100)t = 100;

        rgblcd_show_num(x, y, t, 3, RGBLCD_FONT_16, color);  /* display numeric value */
    }
}

/*!
    \brief      update a specific font library
    \param[in]  x, y: display information coordinates
    \param[in]  fpath: file path
    \param[in]  fx: font index to update
      \arg        0, ungbk;
      \arg        1, gbk12;
      \arg        2, gbk16;
      \arg        3, gbk24;
    \param[in]  color: font color
    \param[out] none
    \retval     0, success; others, error code;
*/
static uint8_t fonts_update_fontx(uint16_t x, uint16_t y, uint8_t *fpath, uint8_t fx, uint16_t color)
{
    uint32_t flashaddr = 0;
    FIL *fftemp;
    uint8_t *tempbuf;
    uint8_t res;
    uint16_t bread;
    uint32_t offx = 0;
    uint8_t rval = 0;
    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));  /* allocate memory */

    if (fftemp == NULL)rval = 1;

    tempbuf = mymalloc(SRAMIN, 4096);               /* allocate 4096 bytes space */

    if (tempbuf == NULL)rval = 1;

    res = f_open(fftemp, (const TCHAR *)fpath, FA_READ);

    if (res)rval = 2;   /* open file failed */

    if (rval == 0)
    {
        switch (fx)
        {
            case 0: /* update UNIGBK.BIN */
                font_info.ugbkaddr = FONTINFOADDR + sizeof(font_info);      /* after info header, next is UNIGBK conversion table */
                font_info.ugbksize = fftemp->obj.objsize;                   /* UNIGBK size */
                flashaddr = font_info.ugbkaddr;
                break;

            case 1: /* update ascii12st */
                font_info.ascii12staddr = font_info.ugbkaddr + font_info.ugbksize;  /* after UNIGBK, next is ascii12st font */
                font_info.ascii12stsize = fftemp->obj.objsize;                      /* ascii12st font size */
                flashaddr = font_info.ascii12staddr;                                /* ascii12st start address */
                break;

            case 2: /* update ascii16st */
                font_info.ascii16staddr = font_info.ascii12staddr + font_info.ascii12stsize;    /* after ascii12st, next is ascii16st font */
                font_info.ascii16stsize = fftemp->obj.objsize;                                  /* ascii16st font size */
                flashaddr = font_info.ascii16staddr;                                            /* ascii16st start address */
                break;

            case 3: /* update ascii24st */
                font_info.ascii24staddr = font_info.ascii16staddr + font_info.ascii16stsize;    /* after ascii16st, next is ascii24st font */
                font_info.ascii24stsize = fftemp->obj.objsize;                                  /* ascii24st font size */
                flashaddr = font_info.ascii24staddr;                                            /* ascii24st start address */
                break;
            case 4: /* update ascii32st */
                font_info.ascii32staddr = font_info.ascii24staddr + font_info.ascii24stsize;    /* after ascii24st, next is ascii32st font */
                font_info.ascii32stsize = fftemp->obj.objsize;                                  /* ascii32st font size */
                flashaddr = font_info.ascii32staddr;                                            /* ascii32st start address */
                break;
            
            case 5: /* update ascii12sp */
                font_info.ascii12spaddr = font_info.ascii32staddr + font_info.ascii32stsize;    /* after ascii32st, next is ascii12sp font */
                font_info.ascii12spsize = fftemp->obj.objsize;                                  /* ascii12sp font size */
                flashaddr = font_info.ascii12spaddr;                                            /* ascii12sp start address */
                break;
            
            case 6: /* update ascii16sp */
                font_info.ascii16spaddr = font_info.ascii12spaddr + font_info.ascii12spsize;    /* after ascii12sp, next is ascii16sp font */
                font_info.ascii16spsize = fftemp->obj.objsize;                                  /* ascii16sp font size */
                flashaddr = font_info.ascii16spaddr;                                            /* ascii16sp start address */
                break;
            
            case 7: /* update ascii24sp */
                font_info.ascii24spaddr = font_info.ascii16spaddr + font_info.ascii16spsize;    /* after ascii16sp, next is ascii24sp font */
                font_info.ascii24spsize = fftemp->obj.objsize;                                  /* ascii24sp font size */
                flashaddr = font_info.ascii24spaddr;                                            /* ascii24sp start address */
                break;
            
            case 8: /* update ascii32sp */
                font_info.ascii32spaddr = font_info.ascii24spaddr + font_info.ascii24spsize;    /* after ascii24sp, next is ascii32sp font */
                font_info.ascii32spsize = fftemp->obj.objsize;                                  /* ascii32sp font size */
                flashaddr = font_info.ascii32spaddr;                                            /* ascii32sp start address */
                break;
            
            case 9: /* update gbk16 */
                font_info.gbk16addr = font_info.ascii32spaddr + font_info.ascii32spsize;    /* after ascii32sp, next is gbk16 font */
                font_info.gbk16size = fftemp->obj.objsize;                                  /* gbk16 font size */
                flashaddr = font_info.gbk16addr;                                            /* gbk16 start address */
                break;
            
            case 10: /* update lvfontsimsun12 */
                font_info.lvfontsimsun12addr = font_info.gbk16addr + font_info.gbk16size;   /* after gbk16, next is lvfontsimsun12 font */
                font_info.lvfontsimsun12size = fftemp->obj.objsize;                         /* lvfontsimsun12 font size */
                flashaddr = font_info.lvfontsimsun12addr;                                   /* lvfontsimsun12 start address */
                break;
            
            case 11: /* update lvfontsimsun16 */
                font_info.lvfontsimsun16addr = font_info.lvfontsimsun12addr + font_info.lvfontsimsun12size; /* after lvfontsimsun12, next is lvfontsimsun16 font */
                font_info.lvfontsimsun16size = fftemp->obj.objsize;                                         /* lvfontsimsun16 font size */
                flashaddr = font_info.lvfontsimsun16addr;                                                   /* lvfontsimsun16 start address */
                break;
  
            case 12: /* update lvfontsimsun24 */
                font_info.lvfontsimsun24addr = font_info.lvfontsimsun16addr + font_info.lvfontsimsun16size; /* after lvfontsimsun16, next is lvfontsimsun24 font */
                font_info.lvfontsimsun24size = fftemp->obj.objsize;                                         /* lvfontsimsun24 font size */
                flashaddr = font_info.lvfontsimsun24addr;                                                   /* lvfontsimsun24 start address */
                break;
            
            case 13: /* update lvfontsimsun32 */
                font_info.lvfontsimsun32addr = font_info.lvfontsimsun24addr + font_info.lvfontsimsun24size; /* after lvfontsimsun24, next is lvfontsimsun32 font */
                font_info.lvfontsimsun32size = fftemp->obj.objsize;                                         /* lvfontsimsun32 font size */
                flashaddr = font_info.lvfontsimsun32addr;                                                   /* lvfontsimsun32 start address */
                break;
            
            case 14: /* update lvfontsimsun48 */
                font_info.lvfontsimsun48addr = font_info.lvfontsimsun32addr + font_info.lvfontsimsun32size; /* after lvfontsimsun32, next is lvfontsimsun48 font */
                font_info.lvfontsimsun48size = fftemp->obj.objsize;                                         /* lvfontsimsun48 font size */
                flashaddr = font_info.lvfontsimsun48addr;                                                   /* lvfontsimsun48 start address */
                break;
            
            case 15: /* update lvfontfzst12 */
                font_info.lvfontfzst12addr = font_info.lvfontsimsun48addr + font_info.lvfontsimsun48size;   /* after lvfontst48, next is lvfontfzst12 font */
                font_info.lvfontfzst12size = fftemp->obj.objsize;                                           /* lvfontfzst12 font size */
                flashaddr = font_info.lvfontfzst12addr;                                                     /* lvfontfzst12 start address */
                break;
            
            case 16: /* update lvfontfzst16 */
                font_info.lvfontfzst16addr = font_info.lvfontfzst12addr + font_info.lvfontfzst12size;   /* after lvfontfzst12, next is lvfontfzst16 font */
                font_info.lvfontfzst16size = fftemp->obj.objsize;                                       /* lvfontfzst16 font size */
                flashaddr = font_info.lvfontfzst16addr;                                                 /* lvfontfzst16 start address */
                break;
            
            case 17: /* update lvfontfzst24 */
                font_info.lvfontfzst24addr = font_info.lvfontfzst16addr + font_info.lvfontfzst16size;   /* after lvfontfzst16, next is lvfontfzst24 font */
                font_info.lvfontfzst24size = fftemp->obj.objsize;                                       /* lvfontfzst24 font size */
                flashaddr = font_info.lvfontfzst24addr;                                                 /* lvfontfzst24 start address */
                break;
            
            case 18: /* update lvfontfzst32 */
                font_info.lvfontfzst32addr = font_info.lvfontfzst24addr + font_info.lvfontfzst24size;   /* after lvfontfzst24, next is lvfontfzst32 font */
                font_info.lvfontfzst32size = fftemp->obj.objsize;                                       /* lvfontfzst32 font size */
                flashaddr = font_info.lvfontfzst32addr;                                                 /* lvfontfzst32 start address */
                break;
            
            case 19: /* update lvfontfzst48 */
                font_info.lvfontfzst48addr = font_info.lvfontfzst32addr + font_info.lvfontfzst32size;   /* after lvfontfzst32, next is lvfontfzst48 font */
                font_info.lvfontfzst48size = fftemp->obj.objsize;                                       /* lvfontfzst48 font size */
                flashaddr = font_info.lvfontfzst48addr;                                                 /* lvfontfzst48 start address */
                break;
            
            case 20: /* update lvfontfzst56 */
                font_info.lvfontfzst56addr = font_info.lvfontfzst48addr + font_info.lvfontfzst48size;   /* after lvfontst48, next is lvfontfzst56 font */
                font_info.lvfontfzst56size = fftemp->obj.objsize;                                       /* lvfontfzst56 font size */
                flashaddr = font_info.lvfontfzst56addr;                                                 /* lvfontfzst56 start address */
                break;
            
            default: break;
        }

        while (res == FR_OK)   /* loop execution */
        {
            res = f_read(fftemp, tempbuf, 4096, (UINT *)&bread);    /* read data */

            if (res != FR_OK)break;     /* execution error */

            w25q256_write_data(offx + flashaddr, tempbuf, bread);       /* write bread bytes from 0 */
            offx += bread;
            fonts_progress_show(x, y, fftemp->obj.objsize, offx, color);    /* progress display */

            if (bread != 4096)break;    /* read complete */
        }

        f_close(fftemp);
    }

    myfree(SRAMIN, fftemp);     /* free memory */
    myfree(SRAMIN, tempbuf);    /* free memory */
    
    return res;
}

/*!
    \brief      update all font files
    \note       updates all font libraries (UNIGBK,GBK12,GBK16,GBK24)
    \param[in]  x, y: display information coordinates
    \param[in]  src: font file source path
    \param[in]  color: font color
    \param[out] none
    \retval     0, success; others, error code;
*/
uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t *src, uint16_t color)
{
    uint8_t *pname;
    uint32_t *buf;
    uint8_t res = 0;
    uint16_t i, j;
    FIL *fftemp;
    uint8_t rval = 0;
    res = 0xFF;
    font_info.fontok = 0xFF;
    pname = mymalloc(SRAMIN, 100);                          /* allocate 100 bytes memory */
    buf = mymalloc(SRAMIN, 4096);                           /* allocate 4K bytes memory */
    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));          /* allocate memory */

    if (buf == NULL || pname == NULL || fftemp == NULL)
    {
        myfree(SRAMIN, fftemp);
        myfree(SRAMIN, pname);
        myfree(SRAMIN, buf);
        return 5;           /* memory allocation failed */
    }

    for (i = 0; i < 21; i++) /* first test if files exist */
    {
        strcpy((char *)pname, (char *)src);                 /* copy src data to pname */
        strcat((char *)pname, (char *)FONT_GBK_PATH[i]);    /* append specific file path */
        res = f_open(fftemp, (const TCHAR *)pname, FA_READ);/* try to open */

        if (res)
        {
            rval |= 1 << 7; /* mark open file failed */
            break;          /* error, exit directly */
        }
    }

    myfree(SRAMIN, fftemp); /* free memory */

    if (rval == 0)          /* font files exist */
    {
        rgblcd_ipa_fill(x, y, x + 256, y + 16, rgblcd_status.back_color);
        rgblcd_show_string(x, y, 256, 16, "Erasing sectors...", RGBLCD_FONT_16, color);            /* display erasing sectors */

        for (i = 0; i < FONTSECSIZE; i++)           /* first erase font sectors to increase write speed */
        {
            fonts_progress_show(x + 256 + 16, y, FONTSECSIZE, i, color);     /* progress display */
            w25q256_read_data(((FONTINFOADDR / 4096) + i) * 4096, (uint8_t *)buf, 4096);    /* read one sector data */

            for (j = 0; j < 1024; j++)              /* check data */
            {
                if (buf[j] != 0xFFFFFFFF) break;    /* needs erase */
            }

            if (j != 1024)
            {
                w25q256_erase_sector((FONTINFOADDR / 4096) + i); /* needs to erase this sector */
            }
        }

        for (i = 0; i < 21; i++)
        {
            rgblcd_ipa_fill(x, y, x + 256, y + 16, rgblcd_status.back_color);
            rgblcd_show_string(x, y, 256, 16, FONT_UPDATE_REMIND_TBL[i], RGBLCD_FONT_16, color);
            strcpy((char *)pname, (char *)src);                     /* copy src data to pname */
            strcat((char *)pname, (char *)FONT_GBK_PATH[i]);        /* append specific file path */
            res = fonts_update_fontx(x + 256 + 16, y, pname, i, color);  /* update font */

            if (res)
            {
                myfree(SRAMIN, buf);
                myfree(SRAMIN, pname);
                return 1 + i;
            }
        }

        /* all updates completed */
        font_info.fontok = 0xAA;
        w25q256_write_data(FONTINFOADDR, (uint8_t *)&font_info, sizeof(font_info));           /* write font info */
    }

    myfree(SRAMIN, pname);  /* free memory */
    myfree(SRAMIN, buf);
    
    return rval;            /* no error */
}

/*!
    \brief      initialize font library
    \param[in]  none
    \param[out] none
    \retval     0, font exists; others, font lost;
*/
uint8_t fonts_init(void)
{
    uint8_t t = 0;

    while (t < 10)  /* try to read 10 times, if still error, means really no font, need to update font */
    {
        t++;
        w25q256_read_data(FONTINFOADDR, (uint8_t *)&font_info, sizeof(font_info));    /* read font_info structure data */

        if (font_info.fontok == 0xAA)
        {
            break;
        }
        
        delay_xms(20);
    }

    if (font_info.fontok != 0xAA)
    {
        return 1;
    }
    
    return 0;
}
