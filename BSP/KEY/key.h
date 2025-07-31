/*!
    \file       key.h
    \brief      header file for key driver
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef __KEY_H
#define __KEY_H
#include <stdint.h>

/* key gpio definitions */
#define WK_UP_RCU   RCU_GPIOC                                   /* key port clock */
#define WK_UP_PORT  GPIOC                                       /* key port */
#define WK_UP_PIN   GPIO_PIN_13                                 /* key pin */

/* key input reading macro */
#define WK_UP       gpio_input_bit_get(WK_UP_PORT, WK_UP_PIN)   /* WKUP key PC13 */

/* key value definitions */
#define WKUP_PRES   1											/* WKUP key pressed return value */

/* function declarations */
void key_init(void);                                            /*!< initialize key GPIO configuration */
uint8_t key_scan(uint8_t mode);                                 /*!< key scanning function with debounce processing */
#endif /* __KEY_H */
