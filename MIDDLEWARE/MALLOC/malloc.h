/*!
    \file       malloc.h
    \brief      Dynamic memory allocation management header file
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
    \note       Ported and modified from ATK (Alientek)
*/

#ifndef __MALLOC_H
#define __MALLOC_H
#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif

/* define memory pools */
#define SRAMIN                  0                               /*!< AXI SRAM memory pool */
#define SRAM0_1                 1                               /*!< SRAM0/1 memory pool, SRAM0+SRAM1, total 32KB */
#define SRAMDTCM                2                               /*!< DTCM memory pool, DTCM is 128KB, this memory can be accessed by CPU and MDMA (via AHBS) */
#define SRAMEX                  3                               /*!< external memory pool (SDRAM), SDRAM is 32MB, available 30MB */

#define SRAMBANK                4                               /*!< number of supported SRAM banks */

/* memory calculation formula, memory pool occupies total space size calculation formula:
 * size = MEM1_MAX_SIZE + (MEM1_MAX_SIZE / MEM1_BLOCK_SIZE) * sizeof(uint32_t)
 * for SDRAM: size = 30840 * 1024 + (30840 * 1024 / 64) * 4 = 33553920 = 32767KB

 * known memory pool size, calculate memory pool calculation formula:
 * MEM1_MAX_SIZE = (MEM1_BLOCK_SIZE * size) / (MEM1_BLOCK_SIZE + sizeof(uint32_t))
 * for SDRAM: MEM2_MAX_SIZE = (64 * 32*1024) / (64 + 4) = 30840.47KB ≈ 30840KB
 */

/* mem1 memory parameter settings. mem1 is all internal SRAM memory, occupies about 128KB internal SRAM */
#define MEM1_BLOCK_SIZE         64                                      /*!< memory block size is 64 bytes */
#define MEM1_MAX_SIZE           120 * 1024                              /*!< maximum allocatable memory 120K */
#define MEM1_ALLOC_TABLE_SIZE  MEM1_MAX_SIZE/MEM1_BLOCK_SIZE            /*!< memory table size */
     
/* mem2 memory parameter settings. mem2 is internal SRAM0+SRAM1, occupies about 32KB internal SRAM */
#define MEM2_BLOCK_SIZE         64                                      /*!< memory block size is 64 bytes */
#define MEM2_MAX_SIZE           30 * 1024                               /*!< maximum allocatable memory 30K */
#define MEM2_ALLOC_TABLE_SIZE   MEM2_MAX_SIZE/MEM2_BLOCK_SIZE           /*!< memory table size */

/* mem3 memory parameter settings. mem3 is internal DTCM memory, occupies about 128KB internal DTCM, this memory can be accessed by CPU and MDMA */
#define MEM3_BLOCK_SIZE         64                                      /*!< memory block size is 64 bytes */
#define MEM3_MAX_SIZE           60 * 1024                               /*!< maximum allocatable memory 60K */
#define MEM3_ALLOC_TABLE_SIZE   MEM3_MAX_SIZE / MEM3_BLOCK_SIZE         /*!< memory table size */
     
/* mem4 memory parameter settings. mem4 memory pool uses external SDRAM, occupies about 30MB external SDRAM */
#define MEM4_BLOCK_SIZE         64                                      /*!< memory block size is 64 bytes */
#define MEM4_MAX_SIZE           28912 * 1024                            /*!< maximum allocatable memory 28912K */
#define MEM4_ALLOC_TABLE_SIZE   MEM4_MAX_SIZE/MEM4_BLOCK_SIZE           /*!< memory table size */

/* memory management device structure */
struct _m_mallco_dev
{
    void (*init)(uint8_t);              /*!< initialization function */
    uint16_t (*perused)(uint8_t);       /*!< memory usage function */
    uint8_t *membase[SRAMBANK];         /*!< memory pool, stores SRAMBANK different memory pools */
    uint32_t *memmap[SRAMBANK];         /*!< memory allocation status table */
    uint8_t  memrdy[SRAMBANK];          /*!< memory management ready status */
};

extern struct _m_mallco_dev mallco_dev; /* defined in mallco.c */

/* internal functions */
void my_mem_set(void *s, uint8_t c, uint32_t count);            /* set memory values */
void my_mem_copy(void *des, void *src, uint32_t n);             /* copy memory */
void my_mem_init(uint8_t memx);                                 /* memory pool initialization function (internal/external call) */
uint32_t my_mem_malloc(uint8_t memx, uint32_t size);            /* memory allocation (internal call) */
uint8_t my_mem_free(uint8_t memx, uint32_t offset);             /* memory release (internal call) */
uint16_t my_mem_perused(uint8_t memx) ;                         /* get memory usage (internal/external call) */

/* user interface functions */
void myfree(uint8_t memx, void *ptr);                           /* memory release (external call) */
void *mymalloc(uint8_t memx, uint32_t size);                    /* memory allocation (external call) */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size);        /* reallocate memory (external call) */
#endif
