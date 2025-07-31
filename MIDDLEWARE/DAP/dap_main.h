#ifndef DAP_MAIN_H
#define DAP_MAIN_H
#include <stdint.h>

void cmsisdap_init(uint8_t busid, uintptr_t reg_base);
void chry_dap_handle(void);
#endif