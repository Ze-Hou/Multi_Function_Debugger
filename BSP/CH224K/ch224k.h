/*!
    \file       ch224k.h
    \brief      header file for CH224K power delivery controller driver
    \version    1.0
    \date       2025-07-23
    \author     Ze-Hou
*/

#ifndef _CH224K_H
#define _CH224K_H
#include <stdint.h>

/* CH224K voltage configuration table
   CFG1    CFG2    CFG3    Output Voltage
    1       -       -         5V
    0       0       0         9V
    0       0       1         12V
    0       1       1         15V
    0       1       0         20V
*/

/* CH224K configuration pin definitions */
/* CFG1-PA6  CFG2-PA4  CFG3-PA5 */
#define CH224K_CFG1_RCU            RCU_GPIOA                    /* CFG1 port clock */
#define CH224K_CFG2_RCU            RCU_GPIOA                    /* CFG2 port clock */
#define CH224K_CFG3_RCU            RCU_GPIOA                    /* CFG3 port clock */

#define CH224K_CFG1_PORT           GPIOA                        /* CFG1 port */
#define CH224K_CFG2_PORT           GPIOA                        /* CFG2 port */
#define CH224K_CFG3_PORT 		   GPIOA                        /* CFG3 port */
#define CH224K_CFG1_PIN            GPIO_PIN_6                   /* CFG1 pin PA6 */
#define CH224K_CFG2_PIN            GPIO_PIN_4                   /* CFG2 pin PA4 */
#define CH224K_CFG3_PIN            GPIO_PIN_5                   /* CFG3 pin PA5 */

/* CH224K supported voltage enumeration */
typedef enum
{
    POWER_5V = 0,   /* 5V output */
    POWER_9V,       /* 9V output */
    POWER_12V,      /* 12V output */
    POWER_15V,      /* 15V output */
    POWER_20V,      /* 20V output */
}ch224k_power_type_enum;

/* function declarations */
void ch224k_init(void);                                         /*!< initialize CH224K GPIO configuration */
void ch224k_config(ch224k_power_type_enum power_enum);          /*!< configure CH224K output voltage */

#endif /* _CH224K_H */
