/*!
    \file       led.h
    \brief      header file for LED driver
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef __LED_H
#define __LED_H
#include <stdint.h>

/* LED1 gpio definitions */
#define RCU_LED1  	    RCU_GPIOH                               /* LED1 port clock */
#define PORT_LED1 	    GPIOH                                   /* LED1 port */
#define PIN_LED1 		GPIO_PIN_6                              /* LED1 pin */

/* LED2 gpio definitions */
#define RCU_LED2  	    RCU_GPIOH                               /* LED2 port clock */
#define PORT_LED2 	    GPIOH                                   /* LED2 port */
#define PIN_LED2 		GPIO_PIN_7                              /* LED2 pin */

/* LED control macros */
#define LED1(n)		(n?gpio_bit_write(PORT_LED1,PIN_LED1,RESET): \
										 gpio_bit_write(PORT_LED1,PIN_LED1,SET))
#define LED1_toggle (gpio_bit_toggle(PORT_LED1, PIN_LED1))      /* LED1 output level toggle */

#define LED2(n)		(n?gpio_bit_write(PORT_LED2,PIN_LED2,RESET): \
										 gpio_bit_write(PORT_LED2,PIN_LED2,SET))
#define LED2_toggle (gpio_bit_toggle(PORT_LED2, PIN_LED2))      /* LED2 output level toggle */

/* function declarations */
void led_init(void);                                            /*!< initialize LED GPIO configuration */
#endif /* __LED_H */
