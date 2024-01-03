#include "flash_c55_driver.h"
#include "flash_drv.h"

#define FLASH_FMC PFLASH_BASE

typedef struct
{
    uint32_t start_address;
    uint32_t end_address;
    flash_block_select_t sel;
} flash_sel_item;

#define FLASH_PFCR1 0x000000000U
#define FLASH_PFCR2 0x000000004U
#define FLASH_FMC_BFEN_MASK 0x000000001U

/* Lock State */
#define UNLOCK_LOW_BLOCKS 0x00000000U
#define UNLOCK_MID_BLOCKS 0x00000000U
#define UNLOCK_HIGH_BLOCKS 0x00000000U
#define UNLOCK_FIRST256_BLOCKS 0x00000000U
#define UNLOCK_SECOND256_BLOCKS 0x00000000U

static const flash_sel_item flash_sel_table[] =
    {
        {0x00400000, 0x00403FFF, {0x8000, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00404000, 0x00407FFF, {0x0002, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00610000, 0x0061FFFF, {0x0080, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00620000, 0x0062FFFF, {0x0200, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00F80000, 0x00F83FFF, {0x0000, 0x0000, 0x0001, 0x00000000, 0x0000}},
        {0x00F84000, 0x00F87FFF, {0x0000, 0x0000, 0x0002, 0x00000000, 0x0000}},
        {0x00F8C000, 0x00F8FFFF, {0x0001, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00F90000, 0x00F93FFF, {0x0000, 0x0001, 0x0000, 0x00000000, 0x0000}},
        {0x00F94000, 0x00F97FFF, {0x0000, 0x0002, 0x0000, 0x00000000, 0x0000}},
        {0x00F98000, 0x00F9BFFF, {0x0000, 0x0004, 0x0000, 0x00000000, 0x0000}},
        {0x00F9C000, 0x00F9FFFF, {0x0000, 0x0008, 0x0000, 0x00000000, 0x0000}},
        {0x00FA0000, 0x00FA3FFF, {0x0000, 0x0010, 0x0000, 0x00000000, 0x0000}},
        {0x00FA4000, 0x00FA7FFF, {0x0000, 0x0020, 0x0000, 0x00000000, 0x0000}},
        {0x00FA8000, 0x00FABFFF, {0x0000, 0x0040, 0x0000, 0x00000000, 0x0000}},
        {0x00FAC000, 0x00FAFFFF, {0x0000, 0x0080, 0x0000, 0x00000000, 0x0000}},
        {0x00FB0000, 0x00FB7FFF, {0x0000, 0x0100, 0x0000, 0x00000000, 0x0000}},
        {0x00FB8000, 0x00FBFFFF, {0x0000, 0x0200, 0x0000, 0x00000000, 0x0000}},
        {0x00FC0000, 0x00FC7FFF, {0x0004, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00FC8000, 0x00FCFFFF, {0x0008, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00FD0000, 0x00FD7FFF, {0x0010, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00FD8000, 0x00FDFFFF, {0x0020, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00FE0000, 0x00FEFFFF, {0x0040, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x00FF0000, 0x00FFFFFF, {0x0100, 0x0000, 0x0000, 0x00000000, 0x0000}},
        {0x01000000, 0x0103FFFF, {0x0000, 0x0000, 0x0000, 0x00000001, 0x0000}},
        {0x01040000, 0x0107FFFF, {0x0000, 0x0000, 0x0000, 0x00000002, 0x0000}},
        {0x01080000, 0x010BFFFF, {0x0000, 0x0000, 0x0000, 0x00000004, 0x0000}},
        {0x010C0000, 0x010FFFFF, {0x0000, 0x0000, 0x0000, 0x00000008, 0x0000}},
        {0x01100000, 0x0113FFFF, {0x0000, 0x0000, 0x0000, 0x00000010, 0x0000}},
        {0x01140000, 0x0117FFFF, {0x0000, 0x0000, 0x0000, 0x00000020, 0x0000}},
        {0x01180000, 0x011BFFFF, {0x0000, 0x0000, 0x0000, 0x00000040, 0x0000}},
        {0x011C0000, 0x011FFFFF, {0x0000, 0x0000, 0x0000, 0x00000080, 0x0000}},
        {0x01200000, 0x0123FFFF, {0x0000, 0x0000, 0x0000, 0x00000100, 0x0000}},
        {0x01240000, 0x0127FFFF, {0x0000, 0x0000, 0x0000, 0x00000200, 0x0000}},
        {0x01280000, 0x012BFFFF, {0x0000, 0x0000, 0x0000, 0x00000400, 0x0000}},
        {0x012C0000, 0x012FFFFF, {0x0000, 0x0000, 0x0000, 0x00000800, 0x0000}},
        {0x01300000, 0x0133FFFF, {0x0000, 0x0000, 0x0000, 0x00001000, 0x0000}},
        {0x01340000, 0x0137FFFF, {0x0000, 0x0000, 0x0000, 0x00002000, 0x0000}},
        {0x01380000, 0x013BFFFF, {0x0000, 0x0000, 0x0000, 0x00004000, 0x0000}},
        {0x013C0000, 0x013FFFFF, {0x0000, 0x0000, 0x0000, 0x00008000, 0x0000}},
        {0x01400000, 0x0143FFFF, {0x0000, 0x0000, 0x0000, 0x00010000, 0x0000}},
        {0x01440000, 0x0147FFFF, {0x0000, 0x0000, 0x0000, 0x00020000, 0x0000}},
        {0x01480000, 0x014BFFFF, {0x0000, 0x0000, 0x0000, 0x00040000, 0x0000}},
        {0x014C0000, 0x014FFFFF, {0x0000, 0x0000, 0x0000, 0x00080000, 0x0000}},
        {0x01500000, 0x0153FFFF, {0x0000, 0x0000, 0x0000, 0x00100000, 0x0000}},
        {0x01540000, 0x0157FFFF, {0x0000, 0x0000, 0x0000, 0x00200000, 0x0000}}};

/**************************************************************
*                          Disable Flash Cache                *
***************************************************************/
static inline void DisableFlashControllerCache(uint32_t flashConfigReg, uint32_t disableVal, uint32_t *origin_pflash_pfcr)
{
    /* Read the values of the register of flash configuration */
    *origin_pflash_pfcr = REG_READ32(FLASH_FMC + flashConfigReg);

    /* Disable Caches */
    REG_BIT_CLEAR32(FLASH_FMC + flashConfigReg, disableVal);
}

/*****************************************************************
*               Restore configuration register of FCM            *
******************************************************************/
static inline void RestoreFlashControllerCache(uint32_t flashConfigReg, uint32_t pflash_pfcr)
{
    REG_WRITE32(FLASH_FMC + flashConfigReg, pflash_pfcr);
}

static void flash_get_block_select(flash_block_select_t *sel, uint32_t start_address, uint32_t size)
{
    unsigned int i;
    uint32_t end_address;
    if (sel != NULL)
    {
        sel->lowBlockSelect = 0;
        sel->midBlockSelect = 0;
        sel->highBlockSelect = 0;
        sel->first256KBlockSelect = 0;
        sel->second256KBlockSelect = 0;
        end_address = start_address + size - 1;
        for (i = 0; i < sizeof(flash_sel_table) / sizeof(flash_sel_table[0]); i++)
        {
            if (((flash_sel_table[i].start_address <= start_address) && (flash_sel_table[i].end_address >= start_address)) ||
                ((flash_sel_table[i].start_address <= end_address) && (flash_sel_table[i].end_address >= end_address)) ||
                ((flash_sel_table[i].start_address >= start_address) && (flash_sel_table[i].end_address <= end_address)))
            {
                sel->lowBlockSelect |= flash_sel_table[i].sel.lowBlockSelect;
                sel->midBlockSelect |= flash_sel_table[i].sel.midBlockSelect;
                sel->highBlockSelect |= flash_sel_table[i].sel.highBlockSelect;
                sel->first256KBlockSelect |= flash_sel_table[i].sel.first256KBlockSelect;
                sel->second256KBlockSelect |= flash_sel_table[i].sel.second256KBlockSelect;
            }
        }
    }
}

status_t flash_drv_init(void)
{
    status_t ret;
    uint32_t blkLockState = 0; /* block lock status to be retrieved */
    ret = FLASH_DRV_Init();
    if (ret == STATUS_SUCCESS)
    {
        ret = FLASH_DRV_GetLock(C55_BLOCK_UTEST, &blkLockState);
        if (!(blkLockState & 0x00000001U))
        {
            ret = FLASH_DRV_SetLock(C55_BLOCK_UTEST, 0x1U);
        }
        if (ret == STATUS_SUCCESS)
        {
            ret = FLASH_DRV_SetLock(C55_BLOCK_LOW, UNLOCK_LOW_BLOCKS);
            if (ret == STATUS_SUCCESS)
            {
                ret = FLASH_DRV_SetLock(C55_BLOCK_MID, UNLOCK_MID_BLOCKS);
                if (ret == STATUS_SUCCESS)
                {
                    ret = FLASH_DRV_SetLock(C55_BLOCK_HIGH, UNLOCK_HIGH_BLOCKS);
                    if (ret == STATUS_SUCCESS)
                    {
                        ret = FLASH_DRV_SetLock(C55_BLOCK_256K_FIRST, UNLOCK_FIRST256_BLOCKS);
                        if (ret == STATUS_SUCCESS)
                        {
                            ret = FLASH_DRV_SetLock(C55_BLOCK_256K_SECOND, UNLOCK_SECOND256_BLOCKS);
                        }
                    }
                }
            }
        }
    }
    return ret;
}

status_t flash_erase(uint32_t address, uint32_t size)
{
    status_t ret;
    flash_block_select_t blockSelect; /* select the address space */
    flash_state_t opResult;           /* store the state of flash */
    uint32_t pflash_pfcr1, pflash_pfcr2;
    flash_get_block_select(&blockSelect, address, size);
    /* Invalidate flash controller cache */
    DisableFlashControllerCache(FLASH_PFCR1, FLASH_FMC_BFEN_MASK, &pflash_pfcr1);
    DisableFlashControllerCache(FLASH_PFCR2, FLASH_FMC_BFEN_MASK, &pflash_pfcr2);

    /* erase block */
    ret = FLASH_DRV_Erase(ERS_OPT_MAIN_SPACE, &blockSelect);
    if (STATUS_SUCCESS == ret)
    {
        do
        {
            ret = FLASH_DRV_CheckEraseStatus(&opResult);
        } while (ret == STATUS_FLASH_INPROGRESS);
        if (ret == STATUS_SUCCESS)
        {
            if (opResult != C55_OK)
            {
                ret = (0x900 | opResult);
            }
        }
    }
    RestoreFlashControllerCache(FLASH_PFCR1, pflash_pfcr1);
    RestoreFlashControllerCache(FLASH_PFCR2, pflash_pfcr2);
    return ret;
}

status_t flash_write(uint32_t address, void *data, uint32_t size)
{
    status_t ret;
    flash_context_data_t pCtxData;
    flash_state_t opResult; /* store the state of flash */
    uint32_t failedAddress; /* save the failed address in flash */
    uint32_t pflash_pfcr1, pflash_pfcr2;
    /* Invalidate flash controller cache */
    DisableFlashControllerCache(FLASH_PFCR1, FLASH_FMC_BFEN_MASK, &pflash_pfcr1);
    DisableFlashControllerCache(FLASH_PFCR2, FLASH_FMC_BFEN_MASK, &pflash_pfcr2);

    if ((size % 4) != 0)
        size += (4 - (size % 4));

    ret = FLASH_DRV_BlankCheck(address, size, (size / C55_WORD_SIZE + 1), &failedAddress, NULL_CALLBACK);
    if (ret == STATUS_SUCCESS)
    {
        ret = FLASH_DRV_Program(&pCtxData, address, size, data);
        do
        {
            ret = FLASH_DRV_CheckProgramStatus(&pCtxData, &opResult);
        } while (ret == STATUS_FLASH_INPROGRESS);
        if (ret == STATUS_SUCCESS)
        {
            if (opResult != C55_OK)
            {
                ret = (0x900 | opResult);
            }
        }
        /*
        if (ret == STATUS_SUCCESS)
        {
            ret = FLASH_DRV_ProgramVerify(address, size, data, (size/C55_WORD_SIZE + 1), &failedAddress, NULL_CALLBACK);
        }
        */
    }
    RestoreFlashControllerCache(FLASH_PFCR1, pflash_pfcr1);
    RestoreFlashControllerCache(FLASH_PFCR2, pflash_pfcr2);
    return ret;
}
