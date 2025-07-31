/*!
    \file       mpu.c
    \brief      memory protection unit (MPU) driver implementation for GD32H7xx microcontroller
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./MPU/mpu.h"

/*!
    \brief      configure memory protection region with specified attributes
    \param[in]  baseaddr: base address of protection region
    \param[in]  size: protection region size (MPU_REGION_SIZE_32B to MPU_REGION_SIZE_4GB)
    \param[in]  rnum: protection region number (MPU_REGION_NUMBER0 to MPU_REGION_NUMBER15)
    \param[in]  de: instruction access permission (MPU_INSTRUCTION_EXEC_PERMIT/NOT_PERMIT)
    \param[in]  tex: type extension field (MPU_TEX_TYPE0/TYPE1)
    \param[in]  ap: access permission (refer to gd32h7xx_misc.h)
    \param[in]  sen: shareable attribute (MPU_ACCESS_SHAREABLE/NON_SHAREABLE)
    \param[in]  cen: cacheable attribute (MPU_ACCESS_CACHEABLE/NON_CACHEABLE)
    \param[in]  ben: bufferable attribute (MPU_ACCESS_BUFFERABLE/NON_BUFFERABLE)
    \param[out] none
    \retval     success status (0 for success)
    \note       Memory attribute combinations:
                TEX=0, CEN=1, BEN=0: write-through, no write allocate
                TEX=0, CEN=1, BEN=1: write-back, no write allocate
                TEX=1, CEN=0, BEN=0: non-cacheable
                TEX=1, CEN=1, BEN=1: write-back, write and read allocate
                SEN field can be 0 or 1 for all combinations above.
                tex     sen     cen     ben     result
                ------------------------------------------------------------------
                |     |        |       |  0       write-through,no write allocate|
                |0    |  0/1   |  1    |-----------------------------------------|
                |     |        |       |  1       write-back,no write allocate   |
                | ---------------------------------------------------------------|
                |     |        |  0    |  0       noncacheable                   |
                |1    |  0/1   |-------|-----------------------------------------|
                |     |        |  1    |  1       write-back,w and r allocate    |
                ------------------------------------------------------------------
*/
uint8_t mpu_set_protection(uint32_t baseaddr, uint32_t size, uint32_t rnum, uint8_t de, \
                           uint8_t tex, uint8_t ap, uint8_t sen, uint8_t cen, uint8_t ben)
{
    mpu_region_init_struct mpu_init_struct;

    ARM_MPU_Disable();                                                      /* disable MPU before configuration, enable after setup */

    mpu_init_struct.region_number = rnum;                                   /* set protection region number */
    mpu_init_struct.region_base_address = baseaddr;                         /* set base address */
    mpu_init_struct.instruction_exec = de;                                  /* instruction access permission */
    mpu_init_struct.region_size = size;                                     /* set protection region size */
    mpu_init_struct.subregion_disable = 0X00;                               /* do not use subregion disable */
    mpu_init_struct.tex_type = tex;                                         /* set type extension field */
    mpu_init_struct.access_permission = (uint8_t)ap;                        /* set access permission */
    mpu_init_struct.access_shareable = sen;                                 /* shareable attribute */
    mpu_init_struct.access_cacheable = cen;                                 /* cacheable attribute */
    mpu_init_struct.access_bufferable = ben;                                /* bufferable attribute */
    mpu_region_config(&mpu_init_struct);                   /* configure MPU region */
    mpu_region_enable();
    ARM_MPU_Enable(MPU_MODE_PRIV_DEFAULT);                     /* enable MPU */
    
    return 0;
}

/*!
    \brief      configure memory protection for all system memory regions
    \param[in]  none
    \param[out] none
    \retval     none
*/
void mpu_memory_protection(void)
{
    /* protect entire ITCM, 64KB, write-back, no write allocate */
    mpu_set_protection( 0x00000000,                                 /* base address */
                        MPU_REGION_SIZE_64KB,                           /* size */
                        MPU_REGION_NUMBER0,                             /* region 0 */
                        MPU_INSTRUCTION_EXEC_PERMIT,                      /* allow instruction access */
                        MPU_TEX_TYPE0,                                   /* MPU TEX type 0 */
                        MPU_AP_FULL_ACCESS,                               /* full access */
                        MPU_ACCESS_NON_SHAREABLE,                        /* non-shareable */
                        MPU_ACCESS_CACHEABLE,                            /* cacheable */
                        MPU_ACCESS_BUFFERABLE);                          /* bufferable */
    
    /* protect entire DTCM, 128KB, write-back, no write allocate */
    mpu_set_protection( 0x20000000,                                 /* base address */
                        MPU_REGION_SIZE_128KB,                          /* size */
                        MPU_REGION_NUMBER1,                             /* region 1 */
                        MPU_INSTRUCTION_EXEC_PERMIT,                      /* allow instruction access */
                        MPU_TEX_TYPE0,                                   /* MPU TEX type 0 */
                        MPU_AP_FULL_ACCESS,                               /* full access */
                        MPU_ACCESS_NON_SHAREABLE,                        /* non-shareable */
                        MPU_ACCESS_CACHEABLE,                            /* cacheable */
                        MPU_ACCESS_BUFFERABLE);                          /* bufferable */

    /* protect entire AXI SRAM, 832KB, write-through, no write allocate */
    mpu_set_protection( 0x24000000,                                 /* base address */
                        MPU_REGION_SIZE_512KB,                          /* size */
                        MPU_REGION_NUMBER2,                             /* region 2 */
                        MPU_INSTRUCTION_EXEC_PERMIT,                      /* allow instruction access */
                        MPU_TEX_TYPE0,                                   /* MPU TEX type 0 */
                        MPU_AP_FULL_ACCESS,                               /* full access */
                        MPU_ACCESS_SHAREABLE,                            /* shareable */
                        MPU_ACCESS_CACHEABLE,                            /* cacheable */
                        MPU_ACCESS_NON_BUFFERABLE);                      /* non-bufferable */
    
    mpu_set_protection( 0x24080000,                                 /* base address */
                        MPU_REGION_SIZE_256KB,                          /* size */
                        MPU_REGION_NUMBER3,                             /* region 3 */
                        MPU_INSTRUCTION_EXEC_PERMIT,                      /* allow instruction access */
                        MPU_TEX_TYPE0,                                   /* MPU TEX type 0 */
                        MPU_AP_FULL_ACCESS,                               /* full access */
                        MPU_ACCESS_SHAREABLE,                            /* shareable */
                        MPU_ACCESS_CACHEABLE,                            /* cacheable */
                        MPU_ACCESS_NON_BUFFERABLE);                      /* non-bufferable */
                        
    mpu_set_protection( 0x240C0000,                                 /* base address */
                        MPU_REGION_SIZE_64KB,                           /* size */
                        MPU_REGION_NUMBER4,                             /* region 4 */
                        MPU_INSTRUCTION_EXEC_PERMIT,                      /* allow instruction access */
                        MPU_TEX_TYPE0,                                   /* MPU TEX type 0 */
                        MPU_AP_FULL_ACCESS,                               /* full access */
                        MPU_ACCESS_SHAREABLE,                            /* shareable */
                        MPU_ACCESS_CACHEABLE,                            /* cacheable */
                        MPU_ACCESS_NON_BUFFERABLE);                      /* non-bufferable */
    
    /* protect entire SRAM0~SRAM1, 32KB, non-cacheable */
    mpu_set_protection( 0x30000000,                                 /* base address */
                        MPU_REGION_SIZE_32KB,                           /* size */
                        MPU_REGION_NUMBER5,                             /* region 5 */
                        MPU_INSTRUCTION_EXEC_PERMIT,                      /* allow instruction access */
                        MPU_TEX_TYPE1,                                   /* MPU TEX type 1 */
                        MPU_AP_FULL_ACCESS,                               /* full access */
                        MPU_ACCESS_SHAREABLE,                            /* shareable */
                        MPU_ACCESS_NON_CACHEABLE,                        /* non-cacheable */
                        MPU_ACCESS_NON_BUFFERABLE);                      /* non-bufferable */
                        
    /* protect SDRAM region, 32MB, write-back, no write allocate */
    mpu_set_protection( 0xC0000000,                                 /* base address */
                        MPU_REGION_SIZE_32MB,                           /* size */
                        MPU_REGION_NUMBER6,                             /* region 6 */
                        MPU_INSTRUCTION_EXEC_NOT_PERMIT,                  /* disable instruction access */
                        MPU_TEX_TYPE0,                                   /* MPU TEX type 0 */
                        MPU_AP_FULL_ACCESS,                               /* full access */
                        MPU_ACCESS_NON_SHAREABLE,                        /* non-shareable */
                        MPU_ACCESS_CACHEABLE,                            /* cacheable */
                        MPU_ACCESS_NON_BUFFERABLE);                      /* non-bufferable */
}
