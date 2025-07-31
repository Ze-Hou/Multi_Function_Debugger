/*!
    \file       malloc.c
    \brief      Dynamic memory allocation management implementation
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
    \note       Ported and modified from ATK (Alientek)
*/

#include "./MALLOC/malloc.h"

#if (__ARMCC_VERSION >= 6010050)   /* for AC6 compiler */
#include "cmsis_compiler.h"
#endif

#if !(__ARMCC_VERSION >= 6010050)   /* not using AC6 compiler or using AC5 compiler */

/* memory pools (32 bytes aligned) */
static __align(32) uint8_t mem1base[MEM1_MAX_SIZE]                                          /*!< internal SRAM memory pool */
static __align(32) uint8_t mem2base[MEM2_MAX_SIZE] __attribute__((at(0X30000000)));         /*!< internal SRAM1+SRAM2 memory pool */
static __align(32) uint8_t mem3base[MEM3_MAX_SIZE] __attribute__((at(0X20000000)));         /*!< internal DTCM memory pool */
static __align(32) uint8_t mem4base[MEM4_MAX_SIZE] __attribute__((at(0XC0200000)));         /*!< external SDRAM memory pool, first 2M reserved for TLI */

/* memory allocation tables */
static uint32_t mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                  /*!< internal SRAM memory allocation MAP */
static uint32_t mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((at(0X30000000 + MEM2_MAX_SIZE)));  /*!< internal SRAM1+SRAM2 memory allocation MAP */
static uint32_t mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((at(0X20000000 + MEM3_MAX_SIZE)));  /*!< internal DTCM memory allocation MAP */
static uint32_t mem4mapbase[MEM4_ALLOC_TABLE_SIZE] __attribute__((at(0XC0200000 + MEM4_MAX_SIZE)));  /*!< external SDRAM memory allocation MAP */

#else      /* using AC6 compiler */

/* memory pools (32 bytes aligned) */
static __ALIGNED(32) uint8_t mem1base[MEM1_MAX_SIZE];                                                       /*!< internal SRAM memory pool */
static __ALIGNED(32) uint8_t mem2base[MEM2_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0X30000000")));  /*!< internal SRAM1+SRAM2 memory pool */
static __ALIGNED(32) uint8_t mem3base[MEM3_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0X20010000")));  /*!< internal DTCM memory pool */
static __ALIGNED(32) uint8_t mem4base[MEM4_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0XC0200000")));  /*!< external SDRAM memory pool, start from 2M */

/* memory allocation tables */
static uint32_t mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                       /*!< internal SRAM memory allocation MAP */
static uint32_t mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0X30007800")));  /*!< internal SRAM1+SRAM2 memory allocation MAP */
static uint32_t mem3mapbase[MEM3_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0X2001F000")));  /*!< internal DTCM memory allocation MAP */
static uint32_t mem4mapbase[MEM4_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0XC1E3C000")));  /*!< external SDRAM memory allocation MAP */

#endif

/* memory parameter constants */
const uint32_t memtblsize[SRAMBANK] = {MEM1_ALLOC_TABLE_SIZE, MEM2_ALLOC_TABLE_SIZE, MEM3_ALLOC_TABLE_SIZE, MEM4_ALLOC_TABLE_SIZE};  /*!< memory table sizes */
const uint32_t memblksize[SRAMBANK] = {MEM1_BLOCK_SIZE, MEM2_BLOCK_SIZE, MEM3_BLOCK_SIZE, MEM4_BLOCK_SIZE};                    /*!< memory block sizes */
const uint32_t memsize[SRAMBANK] ={MEM1_MAX_SIZE, MEM2_MAX_SIZE, MEM3_MAX_SIZE, MEM4_MAX_SIZE};                              /*!< memory pool total sizes */

/* memory management device structure */
struct _m_mallco_dev mallco_dev=
{
  my_mem_init,                                                  /*!< memory initialization function */
  my_mem_perused,                                               /*!< memory usage function */
  mem1base,mem2base,mem3base,mem4base,                          /*!< memory pools */
  mem1mapbase,mem2mapbase,mem3mapbase,mem4mapbase,              /*!< memory allocation status tables */
  0, 0, 0, 0,                                                   /*!< memory management not initialized */
};

/*!
    \brief      copy memory content from source to destination
    \param[in]  des: destination address pointer
    \param[in]  src: source address pointer  
    \param[in]  n: number of bytes to copy
    \retval     none
*/
void my_mem_copy(void *des, void *src, uint32_t n)  
{  
    uint8_t *xdes = des;
    uint8_t *xsrc = src; 
    while (n--)*xdes++ = *xsrc++;  
}  

/*!
    \brief      set memory content to specified value
    \param[in]  s: memory start address pointer
    \param[in]  c: value to be set
    \param[in]  count: number of bytes to set
    \retval     none
*/
void my_mem_set(void *s, uint8_t c, uint32_t count)  
{  
    uint8_t *xs = s;  
    while (count--)*xs++ = c;  
}  

/*!
    \brief      initialize memory pool
    \param[in]  memx: memory pool to initialize
    \retval     none
*/
void my_mem_init(uint8_t memx)  
{  
    my_mem_set(mallco_dev.memmap[memx], 0, memtblsize[memx] * 4);  /* clear memory status table */
    mallco_dev.memrdy[memx] = 1;                                   /* memory pool initialization OK */
}

/*!
    \brief      get memory usage percentage
    \param[in]  memx: memory pool to check
    \retval     usage percentage (expanded by 10 times, 0~1000, represents 0.0%~100.0%)
*/
uint16_t my_mem_perused(uint8_t memx)  
{  
    uint32_t used = 0;  
    uint32_t i;

    for (i = 0;i < memtblsize[memx]; i++)  
    {  
        if (mallco_dev.memmap[memx][i])used++; 
    }

    return (used * 1000) / (memtblsize[memx]);  
}

/*!
    \brief      allocate memory (internal function)
    \param[in]  memx: memory pool to allocate from
    \param[in]  size: size of memory to allocate (bytes)
    \retval     memory offset address
      \arg        0 ~ 0xFFFFFFFE: valid memory offset address
      \arg        0xFFFFFFFF: invalid memory offset address
*/
uint32_t my_mem_malloc(uint8_t memx, uint32_t size)  
{  
    signed long offset = 0;  
    uint32_t nmemb;     /* required memory blocks */
    uint32_t cmemb = 0; /* consecutive memory blocks */
    uint32_t i;

    if (!mallco_dev.memrdy[memx])
    {
        mallco_dev.init(memx);          /* not initialized, execute initialization */
    }
    if (size == 0) return 0XFFFFFFFF;   /* no allocation needed */
    nmemb = size / memblksize[memx];    /* get required number of memory blocks */

    if (size % memblksize[memx]) nmemb++;

    for (offset = memtblsize[memx] - 1;offset >= 0; offset--)   /* search for available memory blocks */
    {
        if (!mallco_dev.memmap[memx][offset])
        {
            cmemb++;                /* count consecutive memory blocks */
        }
        else 
        {
            cmemb = 0;              /* reset consecutive count */
        }

        if (cmemb == nmemb)         /* found enough consecutive nmemb memory blocks */
        {
            for (i = 0;i < nmemb; i++)            /* mark memory as allocated  */
            {  
                mallco_dev.memmap[memx][offset + i] = nmemb;  
            }

            return (offset * memblksize[memx]);   /* return offset address  */
        }
    }

    return 0XFFFFFFFF;             /* no suitable consecutive memory blocks found */
}

/*!
    \brief      free memory (internal function)
    \param[in]  memx: memory pool to free from
    \param[in]  offset: memory address offset
    \retval     free result
      \arg        0: free successful
      \arg        1: free failed
      \arg        2: parameter error (failed)
*/
uint8_t my_mem_free(uint8_t memx, uint32_t offset)
{
    int i;

    if (!mallco_dev.memrdy[memx])   /* not initialized, execute initialization */
    {
        mallco_dev.init(memx);
        return 1;                   /* not initialized */
    }

    if (offset < memsize[memx])     /* offset within memory pool */
    {
        int index = offset / memblksize[memx];      /* offset to memory block index */
        int nmemb = mallco_dev.memmap[memx][index]; /* number of memory blocks */

        for (i = 0; i < nmemb; i++)                 /* clear memory blocks */
        {
            mallco_dev.memmap[memx][index + i] = 0;
        }

        return 0;
    }
    else
    {
        return 2;   /* offset exceeds limit */
    }
}

/*!
    \brief      free memory (external function)
    \param[in]  memx: memory pool to free from
    \param[in]  ptr: memory start address pointer
    \retval     none
*/
void myfree(uint8_t memx, void *ptr)
{
    uint32_t offset;

    if (ptr == NULL)return;     /* address is 0 */

    offset = (uint32_t)ptr - (uint32_t)mallco_dev.membase[memx];
    my_mem_free(memx, offset);  /* free memory */
}

/*!
    \brief      allocate memory (external function)
    \param[in]  memx: memory pool to allocate from
    \param[in]  size: size of memory to allocate (bytes)
    \retval     allocated memory start address pointer
*/
void *mymalloc(uint8_t memx, uint32_t size)
{
    uint32_t offset;
    offset = my_mem_malloc(memx, size);

    if (offset == 0xFFFFFFFF)   /* allocation failed */
    {
        return NULL;            /* return null (0) */
    }
    else    /* allocation successful, return start address */
    {
        return (void *)((uint32_t)mallco_dev.membase[memx] + offset);
    }
}

/*!
    \brief      reallocate memory (external function)
    \param[in]  memx: memory pool to reallocate from
    \param[in]  ptr: old memory start address pointer
    \param[in]  size: size of memory to reallocate (bytes)
    \retval     reallocated memory start address pointer
*/
void *myrealloc(uint8_t memx, void *ptr, uint32_t size)
{
    uint32_t offset;
    offset = my_mem_malloc(memx, size);

    if (offset == 0xFFFFFFFF)   /* allocation failed */
    {
        return NULL;            /* return null (0) */
    }
    else    /* allocation successful, return start address */
    {
        my_mem_copy((void *)((uint32_t)mallco_dev.membase[memx] + offset), ptr, size); /* copy old memory data to new memory */
        myfree(memx, ptr);                                                             /* free old memory */
        return (void *)((uint32_t)mallco_dev.membase[memx] + offset);                  /* return new memory start address */
    }
}

