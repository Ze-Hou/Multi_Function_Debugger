#include "MPU/mpu.h"
#include "system_gd32h7xx.h"
#include "gd32h7xx_libopt.h"
#include "./MPU/mpu.h"
#include "./SYSTEM/system.h"
#include "./DELAY/delay.h"
#include "./TIMER/timer.h"
#include "./USART/usart.h"

// Standard library header files
#include <stdint.h>

int main() {
    SystemCoreClockUpdate();                                            /* update system clock */
    nvic_priority_group_set( NVIC_PRIGROUP_PRE4_SUB0);
    mpu_memory_protection();
    system_fwdgt_init();
    system_cache_enable();
    system_dwt_init();

    delay_init();                                                       /* initialize delay function */
    timer_general16_config(30000, 20000);                   /* configure TIMER16 for automatic watchdog feeding */
    usart_init(921600);                                      /* initialize USART */

    while(1)
    {
        printf("Hello World!\r\n");
        delay_ms(5000);
    }
}
