#ifndef __CPU_UTILS_H
#define __CPU_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* ��ⳤ�ȣ�tick �� */ 
#define CALCULATION_PERIOD    1000

/* Exported functions ------------------------------------------------------- */
uint8_t osGetCpuUUsage(void);

#ifdef __cplusplus
}
#endif

#endif /* __CPU_UTILS_H */
