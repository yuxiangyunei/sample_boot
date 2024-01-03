#ifndef FLASH_DRV_H
#define FLASH_DRV_H
#include <stdint.h>
#include "status.h"



status_t flash_drv_init(void);
status_t flash_erase(uint32_t address, uint32_t size);
status_t flash_write(uint32_t address, void *data, uint32_t size);

//void DisableFlashControllerCache(uint32_t flashConfigReg, uint32_t disableVal, uint32_t *origin_pflash_pfcr);
//void RestoreFlashControllerCache(uint32_t flashConfigReg, uint32_t pflash_pfcr);

#endif
