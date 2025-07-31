#include "LED/led.h"
#include "MPU/mpu.h"
#include "system_gd32h7xx.h"
#include "gd32h7xx_libopt.h"
#include "./MPU/mpu.h"
#include "./SYSTEM/system.h"
#include "./DELAY/delay.h"
#include "./TIMER/timer.h"
#include "./USART/usart.h"
#include "./LED/led.h"
#include "./KEY/key.h"
#include "./CH224K/ch224k.h"
#include "./RTC/rtc.h"
#include "./IIC/iic.h"
#include "./AT24CXX/at24cxx.h"
#include "./INA226/ina226.h"
#include "./TRIGSEL/trigsel.h"
#include "./ADC/adc.h"
#include "./SC8721/sc8721.h"
#include "./CAN/can.h"
#include "./EXMC/exmc_sdram.h"
#include "./SDIO/sdio_emmc.h"
#include "./W25Q256/w25q256.h"
#include "./SCREEN/RGBLCD/rgblcd.h"
#include "./WIRELESS/wireless.h"
#include "system_config.h"

/* middleware header files */
#include "./MALLOC/malloc.h"
#include "./FATFS/fatfs_config.h"
#include "./FONT/fonts.h"
#include "usb_dwc2_reg.h"
#include "./DAP/dap_main.h"
#include "lvgl.h"
#include "lvgl_config.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lvgl_main.h"
#include "lvgl_setting.h"
#include "lvgl_usart.h"
#include "lvgl_can.h"
#include "freertos_main.h"

/* Standard library header files */
#include <stdint.h>
#include <stdbool.h>

int main() 
{
    uint8_t res = 0;

    SystemCoreClockUpdate();                                            /* update system clock */
    system_nvic_vector_table_config(ITCMRAM_BASE, 0);
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);    /* set NVIC priority group */
    system_cache_enable();                                              /* enable system cache */
    mpu_memory_protection();                                            /* initialize MPU for memory protection */
    system_fwdgt_init();                                                /* initialize watchdog */
    system_dwt_init();                                                  /* initialize DWT */
    system_rcu_peripheral_clock_config();                               /* configure peripheral clocks (ADC, SDIO, TLI) */

    delay_init();                                                       /* initialize delay function */
    timer_general14_config(300, 1000);                      /* every 1ms, repetition counter decrements by 1 */ 
    timer_general16_config(30000, 20000);                   /* configure TIMER16 for automatic watchdog feeding */
    timer_general41_config(300, 1000, 1000 * 60);   /* 60s timer, generates interrupt on overflow */
    timer_base51_config(30000, 10000 * 60 * 5);             /* maximum timing 5 minutes */
    usart_init(921600);                                      /* initialize USART */
    usart_terminal_init(115200);                             /* initialize USART1 terminal for debugging */
    uart4_init(115200);                                      /* initialize UART4 for wireless module communication */
    system_info_get();                                                  /* get system information */
    system_info_print();                                                /* print system information */
    led_init();                                                         /* initialize LED GPIO configuration */
    key_init();                                                         /* initialize key GPIO configuration */
    ch224k_init();                                                      /* initialize CH224K GPIO configuration */
    rtc_config(2025, 7, 24, 0, 0, 0, NULL);  /* configure RTC date and time */
    at24cxx_init();                                                     /* initialize AT24CXX EEPROM */
    ina226_init();                                                      /* initialize INA226 power monitor ICs */
    trigsel_config();                                                   /* configure TRIGSEL for peripheral trigger source selection */
    adc2_internal_channel_config();                                     /* configure ADC2 for internal channel sampling */
    sc8721_init();                                                      /* initialize SC8721 step-up converter */
    can_config(1, 2, 5, 2, 15, CAN_LOOPBACK_SILENT_MODE); /* configure CAN bus parameters and timing, BaudRate=1MHz */
    sdram_init(EXMC_SDRAM_DEVICE0);                       /* initialize SDRAM peripheral */
    emmc_init();                                                        /* initialize eMMC peripheral */

    my_mem_init(SRAMIN);                                          /* initialize internal SRAM memory pool */
    my_mem_init(SRAM0_1);                                         /* initialize internal SRAM0+SRAM1 memory pool */
    my_mem_init(SRAMDTCM);                                        /* initialize DTCM memory pool */
    my_mem_init(SRAMEX);                                          /* initialize external SDRAM memory pool */

    w25q256_init();                                                     /* initialize W25Q256 SPI flash memory */
    rgblcd_init();                                                      /* initialize RGB LCD display */
    wireless_init(115200);                                   /* initialize wireless module */

    fatfs_config();                                                     /* allocate memory for FATFS related variables */

    _remount:
        res = f_mount(fatfs[0], "C:", 1);                 /* immediately mount eMMC card */

        if(!res)
        {
            PRINT_INFO("fatfs mount emmc successfully.\r\n");
            rgblcd_show_string(10, 0, 512, 16, "log: fatfs mount emmc successfully", RGBLCD_FONT_16, BLACK); 
        }
        else
        {
            switch((FRESULT)res)
            {
                case FR_NO_FILESYSTEM:
                    PRINT_ERROR("There is no valid FAT volume in emmc.\r\n");
                    PRINT_ERROR("Please format a FAT volume.\r\n");
//                    f_mkfs("C:", NULL, NULL, FF_MIN_SS);
                    goto _remount;      /* remount EMMC */
                    break;
                
                default:
                    break;
            }
            PRINT_ERROR("fatfs mount emmc unsuccessfully.\r\n");
            rgblcd_show_string(10, 0, 512, 16, "error: fatfs mount emmc unsuccessfully", RGBLCD_FONT_16, RED);
        }
    
    _reinit_fonts:
        res = fonts_init();                             /* initialize font information */

        if(!res)
        {
            PRINT_INFO("initlize font successfully.\r\n");
            rgblcd_show_string(10, 20, 512, 16, "log: initlize font successfully", RGBLCD_FONT_16, BLACK);
        }
        else
        {
            PRINT_ERROR("initlize font unsuccessfully.\r\n");
            rgblcd_show_string(10, 20, 512, 16, "error: initlize font unsuccessfully", RGBLCD_FONT_16, RED);
            fonts_update_font(10, 40, (uint8_t *)"C:/SYSTEM/FONT", RED);
            goto _reinit_fonts;                         /* reinitialize fonts */
        }

    
    cmsisdap_init(0, USBHS0);
    system_parameter_init_from_cfg_file();
    wireless_config();

    delay_xms(1000);
    rgblcd_clear(BLACK);

    lv_init();
    lvgl_config();
    lv_port_disp_init();
    lv_port_indev_init();
    lvgl_config_indev_read_timer();

    PRINT_INFO("enter system.\r\n");
    freertos_main();
}
