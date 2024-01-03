/*
 * Copyright 2017 - 2019 NXP
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * @page misra_violations MISRA-C:2012 violations
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 1.3,  Taking address of near auto variable.
 * The code is not dynamically linked. An absolute stack address is obtained
 * when taking the address of the near auto variable.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 11.4, Conversion between a pointer and
 * integer type.
 * The cast is required to initialize a pointer with an unsigned long define,
 * representing an address.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 11.6, Cast from unsigned int to pointer.
 * The cast is required to initialize a pointer with an unsigned long define,
 * representing an address.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 17.8, function parameter modified.
 * The input parameter can be modified in the function.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 18.8,  Variable length array declared.
 * The length of array can be changed.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.7, External could be made static.
 * Function is defined for usage by application code.
 */

#include "eee_driver.h"
#include "eee_common.h"

/*******************************************************************
| global variable definitions (scope: module-local)
|------------------------------------------------------------------*/
/* Flag to keep track of Erase State */
eee_erase_status_t g_eraseStatusFlag = EEE_ERASE_NOT_STARTED;

/* Flag to check error in EEE module */
eee_module_type_t g_eccErrorModuleFlag = EEE_NONE;

/* Variable to store the EEE state */
eee_state_t * g_eeeState = {NULL};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_InitEeprom
 * Description   : This function Initialize the EEPROM Emulation driver.
 *
 * Implements    : EEE_DRV_InitEeprom_Activity
 *END**************************************************************************/
status_t EEE_DRV_InitEeprom(const eee_user_config_t * userConf,
                            eee_state_t * state)
{
    DEV_ASSERT(userConf != NULL);
    DEV_ASSERT(state != NULL);
    eee_block_config_t * pBlockConf;
    status_t returnCode = STATUS_SUCCESS;
    eee_last_job_status_t retLastJob = EEE_LAST_JOB_NONE;
    eee_block_status_t retBlockStatus;
    uint32_t temp = 0UL;
    uint32_t i = 0UL;
    uint32_t dest;
    uint32_t size;
    uint32_t eraseCycle;
    uint32_t oldestActIdx = 0UL;
    uint16_t nextStartID = 0U;

    /* Check the first of initialization */
    if (g_eeeState == NULL)
    {
        g_eeeState = state;

        state->numberOfBlock = userConf->numberOfBlock;
        state->numberOfActBlock = userConf->numberOfActBlock;

        state->numberOfDeadBlock = 0U;
        state->activeBlockIndex = 0U;
        state->blockWriteFlag = false;
        state->cTable = userConf->cTable;
        state->flashBlocks = userConf->flashBlocks;

        state->numOfByteRead = userConf->numOfByteRead;
        state->numOfCycleSearch = userConf->numOfCycleSearch;
        state->numOfRecordSearch = userConf->numOfRecordSearch;
        state->dataSize = userConf->dataSize;
        state->maxReEraseEeeBlock = userConf->maxReEraseEeeBlock;
        g_numOfErase = userConf->maxReEraseEeeBlock;
        state->maxReProgram = userConf->maxReProgram;
        state->cacheEnable = userConf->cacheEnable;
        state->maxRecordId = userConf->maxRecordId;

        state->callback = userConf->callback;
        state->callbackParam = userConf->callbackParam;

        /* Check the scheme selection */
#if EEE_ECC4
        if (userConf->schemeSelection == EEE_FIXLENGTH)
        {
            state->eccSize = 4U;
            state->sizeField = 0U;
            state->minRecordSize = 8U;
        }
        else
        {
            state->eccSize = 4U;
            state->sizeField = 2U;
            state->minRecordSize = 8U;
        }
#elif EEE_ECC8
        if (userConf->schemeSelection == EEE_FIXLENGTH)
        {
            state->eccSize = 8U;
            state->sizeField = 0U;
            state->minRecordSize = 16U;
        }
        else
        {
            state->eccSize = 8U;
            state->sizeField = 2U;
            state->minRecordSize = 16U;
        }
#elif EEE_ECC16
        if (userConf->schemeSelection == EEE_FIXLENGTH)
        {
            state->eccSize = 16U;
            state->sizeField = 0U;
            state->minRecordSize = 16U;
        }
        else
        {
            state->eccSize = 16U;
            state->sizeField = 2U;
            state->minRecordSize = 32U;
        }
#elif EEE_ECC32
        if (userConf->schemeSelection == EEE_FIXLENGTH)
        {
            state->eccSize = 32U;
            state->sizeField = 0U;
            state->minRecordSize = 32U;
        }
        else
        {
            state->eccSize = 32U;
            state->sizeField = 2U;
            state->minRecordSize = 64U;
        }
#endif

        /* Check the size of data in the record head */
        if (state->sizeField != 0U)
        {
            state->idOffset = state->eccSize;
#if EEE_ECC4
            state->smallDataSize = 0U;
#else
            state->smallDataSize = state->eccSize - C55_DWORD_SIZE;
#endif
            state->dataHeadSize = (state->smallDataSize) + (state->eccSize) - C55_WORD_SIZE;
        }
        else
        {
            state->idOffset = C55_DWORD_SIZE;
            state->smallDataSize = 0;
#if EEE_ECC4
            state->dataHeadSize = state->eccSize - ID_FIELD_SIZE;
#elif EEE_ECC8
            state->dataHeadSize = state->eccSize - ID_FIELD_SIZE;
#else
            state->dataHeadSize = state->eccSize - ID_FIELD_SIZE - state->idOffset;
#endif
        }
    }
    else
    {
        state = g_eeeState;
    }

    if (state->numberOfBlock <= state->numberOfActBlock)
    {
        /* There isn't enough number of block for emulation */
        returnCode = STATUS_EEE_ERROR_NO_ENOUGH_BLOCK;
        g_eeeState = NULL;
    }
    else if (state->blockWriteFlag == true)       /* Check for Write operation */
    {
        returnCode = STATUS_BUSY;
    }
    else
    {
        /* Set the write flag */
        state->blockWriteFlag = true;

        /* Reset erase status flag */
        g_eraseStatusFlag = EEE_ERASE_NOT_STARTED;

        /* Clear Cache table */
        if (state->cacheEnable == true)
        {
            /* Get number of Cache items */
            temp = (state->cTable->size) / C55_WORD_SIZE;
            for (i = 0U; i < temp; i++)
            {
                 (void)EEE_DRV_UpdateCacheTable(state->cTable,
                                                (uint16_t)i,
                                                0xFFFFFFFFU);
            }
        }

        /* Set initial blank space value for all blocks */
        temp = state->numberOfBlock;
        for (i = 0U; i < temp; i++)
        {
            pBlockConf = state->flashBlocks[i];
            pBlockConf->blankSpace = 0xFFFFFFFFU;
        }

        /* Validate Dead blocks */
        returnCode = EEE_DRV_ValidateDeadBlocks();

        /* Get the last job before the brown-out. The last of ACTIVE block index is also updated */
        if (returnCode == STATUS_SUCCESS)
        {
            retLastJob = EEE_DRV_GetLastJobStatus();

            /* Recover the brown-out operations */
            returnCode = EEE_DRV_RecoverEeprom(retLastJob);
        }

        /* Unlock the write lock */
        state->blockWriteFlag = false;
    }

    /* Update blankSpace and fill cache table
     ****************************************/
    if (returnCode == STATUS_SUCCESS)
    {
        /* Get index of the oldest ACTIVE block */
        oldestActIdx = (state->activeBlockIndex + state->numberOfBlock - state->numberOfActBlock) % state->numberOfBlock;

        /* Get address and size of the cache table */
        if (state->cacheEnable == true)
        {
            /* Use dest to store address of cache table */
            dest = (uint32_t)(state->cTable->startAddress);
            /* Use size to store size of cache table */
            size = state->cTable->size;
        }
        else
        {
            /* Use eraseCycle as temporary buffer */
            dest = (uint32_t)(&eraseCycle);
            size = C55_WORD_SIZE;
        }

        /* Scan from the oldest ACTIVE block to ensure that
           newest records will be update into cache table */
        for (i = oldestActIdx; i < (state->numberOfBlock + oldestActIdx); i++)
        {
            /* Get block configuration */
            temp = i % state->numberOfBlock;
            pBlockConf = state->flashBlocks[temp];

            /* Read block status */
            retBlockStatus = EEE_DRV_ReadBlockStatus(pBlockConf);
            if (retBlockStatus == EEE_BLOCK_ACT)
            {
                /* The function fill cache table and update the blank space at the same time */
                nextStartID = EEE_DRV_SearchRecordFromTop(pBlockConf,
                                                          dest,
                                                          size,
                                                          0U);
                DEV_ASSERT(nextStartID != 0U);
                (void) nextStartID;
            }
            if (retBlockStatus == EEE_BLOCK_ALT)
            {
                /* With ALT block the blank space is start of Record region */
                pBlockConf->blankSpace = (pBlockConf->blockStartAddr) + (4UL * (state->eccSize));
            }
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_WriteRecordSelection
 * Description   : This function will write data records to the EEPROM block
 * with the different situations of record status.
 *END**************************************************************************/
static status_t EEE_DRV_WriteRecordSelection(uint16_t dataID,
                                             uint16_t dataSize,
                                             uint32_t source,
                                             eee_write_status_t recordOption)
{
    eee_block_config_t * blockConf;       /* BlockConf configuration */
    eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    uint32_t sourceInd;
    uint32_t recordAddr;
    bool updateCache = false;

    /* Write record base on situations
     *********************************/
    if ((recordOption == EEE_WRITE_NORMAL) ||
        (recordOption == EEE_WRITE_ON_NEW_ACTIVE) ||
        (recordOption == EEE_WRITE_ON_COPY_DONE))
    {
        if (recordOption == EEE_WRITE_ON_NEW_ACTIVE)
        {
            /* Update index of the next ACTIVE block */
            state->activeBlockIndex = (state->activeBlockIndex + 1U) % state->numberOfBlock;

            /* Make the next ALT block to ACTIVE */
            blockConf = state->flashBlocks[state->activeBlockIndex];
            sourceInd = ACT_INDICATOR_ACT;
            returnCode = EEE_DRV_ProgramBlockIndicator(blockConf->blockStartAddr,
                                                       (uint32_t)(&sourceInd));

            /* Update blank space */
            if (returnCode == STATUS_SUCCESS)
            {
                blockConf->blankSpace = blockConf->blockStartAddr + (4U * (state->eccSize));
            }
        }
        else
        {
            /* Reset returnCode, to goto the next step */
            returnCode = STATUS_SUCCESS;
        }

        if (returnCode == STATUS_SUCCESS)
        {
            /* Set blockConf to current ACTIVE block */
            blockConf = state->flashBlocks[state->activeBlockIndex];

            /* Backup record address */
            recordAddr = blockConf->blankSpace;

            /* Copy one data record to free space in the active block */
            returnCode = EEE_DRV_WriteDataRecord(blockConf,
                                                 false,
                                                 dataID,
                                                 dataSize,
                                                 source);
            if (returnCode == STATUS_SUCCESS)
            {
                /* Check if the cache table is enabled */
                if (state->cacheEnable == true)
                {
                    updateCache = EEE_DRV_UpdateCacheTable(state->cTable,
                                                           dataID,
                                                           recordAddr);
                    (void) updateCache;
                }
            }
        }
    }
    else if (recordOption == EEE_WRITE_SWAP)
    {
        /* Get the next ALT block */
        /* Re-use the sourceInd variable */
        sourceInd = (state->activeBlockIndex + 1U) % state->numberOfBlock;
        blockConf = state->flashBlocks[sourceInd];

        /* Write data to the next ALT block. We use EEE_DRV_CopyDataRecord instead of EEE_DRV_WriteDataRecord because
         * nest swapping isn't allowed. If it isn't enough space, EEE_DRV_CopyDataRecord will return
         * EE_ERROR_NO_ENOUGH_SPACE*/
        returnCode = EEE_DRV_CopyDataRecord(blockConf,
                                            false,
                                            dataID,
                                            dataSize,
                                            source);

        if (returnCode == STATUS_SUCCESS)
        {
            /* Asynchronous Swapping */
            returnCode = EEE_DRV_BlockSwapping(false);
        }
    }
    else /* EE_ERROR_NO_ENOUGH_SPACE */
    {
        /* Return an error */
        returnCode = STATUS_EEE_ERROR_NO_ENOUGH_SPACE;
    }

    return returnCode;
}
/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_WriteEeprom
 * Description   : This function is to write data records to the EEPROM emulated Flash
 * and re-write data record if this program operation fails.
 * comment:
 * If an immediate data request while an erase operation is going on, the operation
 * will be suspended to serve this writing in advance.Note that if this API is called
 * to write a normal data while an erase is going on such as a swapping is being going
 * on, it will return STATUS_EEE_HVOP_INPROGRESS.
 *
 * Implements    : EEE_DRV_WriteEeprom_Activity
 *END**************************************************************************/
status_t EEE_DRV_WriteEeprom(uint16_t dataID,
                             uint16_t dataSize,
                             uint32_t source,
                             eee_request_type_t iReq)
{
    const eee_block_config_t * blockConf;       /* BlockConf configuration */
    eee_state_t * state = g_eeeState;
    DEV_ASSERT(NULL != state);
    DEV_ASSERT((EEE_IMMEDIATE_NONE == iReq) || (EEE_IMMEDIATE_WRITE == iReq));
    status_t returnCode = STATUS_SUCCESS;
    status_t retTemp = STATUS_SUCCESS;
    uint32_t recordLength;                /* Length of record */
    flash_state_t suspendState;
    eee_write_status_t recordOption;

    if (state->sizeField == 0U)
    {
        dataSize = (uint16_t)state->dataSize;
    }

    /* Check the write lock */
    if (state->blockWriteFlag == true)
    {
        /* A write operation is already in progress, no write operation is permitted */
        returnCode = STATUS_BUSY;
    }
    else
    {
        /* Set the write lock flag */
        state->blockWriteFlag = true;

        /* Suspend the high voltage operation if the request is immediate */
        blockConf = state->flashBlocks[state->activeBlockIndex];
        returnCode = EEE_DRV_ProcessImmediateRequest(blockConf,
                                                     iReq,
                                                     &suspendState);

        /* Start writing the record
        **************************/
        if (returnCode != STATUS_EEE_HVOP_INPROGRESS)
        {
            /* Get record length base on size of raw data */
            recordLength = EEE_DRV_GetRecordLength(dataSize);

            do
            {
                /* Get writing option base on recordLength */
                recordOption = EEE_DRV_GetWriteRecordOption(recordLength);

                returnCode = EEE_DRV_WriteRecordSelection(dataID,
                                                          dataSize,
                                                          source,
                                                          recordOption);
            }
            while ((returnCode != STATUS_SUCCESS) && (returnCode != STATUS_EEE_ERROR_NO_ENOUGH_SPACE));   /* Rewrite if there is an STATUS_ERROR error */
        }
        else
        {
            /* Return an HVOP error */
            returnCode = STATUS_EEE_HVOP_INPROGRESS;
        }

        /* Check erase suspend
         *********************/
        if (suspendState == C55_ERS_SUS)
        {
            retTemp = FLASH_DRV_Resume(&suspendState);

            if ((returnCode == STATUS_SUCCESS) && (retTemp != STATUS_SUCCESS))
            {
                returnCode = retTemp;
            }
        }

        /* Unlock the write lock */
        state->blockWriteFlag = false;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SearchIdInCacheTable
 * Description   : This function will search the data ID in the cache table.
 *
 *END**************************************************************************/
static status_t EEE_DRV_SearchIdInCacheTable(eee_request_type_t iReq,
                                             const uint32_t * recordAddr,
                                             flash_state_t * suspendState)
{
    uint32_t i;
    const eee_block_config_t * blockConf;         /* Local block configuration pointer */
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;         /* Return code */

    for (i = 0U; i < state->numberOfBlock; i++)
    {
        blockConf = state->flashBlocks[i];

        /* If the record address belong to the block range */
        if ((blockConf->blockStartAddr <= *recordAddr) && (*recordAddr < (blockConf->blockStartAddr + blockConf->blockSize)))
        {
            /* Suspend the high voltage operation if the request is immediate */
            returnCode = EEE_DRV_ProcessImmediateRequest(blockConf,
                                                         iReq,
                                                         suspendState);
            break;
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ReadEeprom
 * Description   : This function is to read the specific data record. The data
 * record size is determined by the data size written into the EEPROM emulation.
 * Note: User must ensure that the data size to be read does not exceed the RAM available.
 * This API can be called when an erase is going on such as a swapping is being done.
 * If the erased block is in different partition with the read target block, it will
 * read the expected data record without suspending that high voltage operation. Otherwise,
 * if the erased block is in the same partition, it will read the expected data record
 * after suspending this high voltage. However, EEE_DRV_MainFunction still need to be called
 * after that to update block status for all blocks.
 *
 * Implements    : EEE_DRV_ReadEeprom_Activity
 *END**************************************************************************/
status_t EEE_DRV_ReadEeprom(uint16_t dataID,
                            uint16_t dataSize,
                            uint32_t buffAddr,
                            uint32_t * recordAddr,
                            eee_request_type_t iReq)
{
    const eee_state_t * state = g_eeeState;
    DEV_ASSERT(NULL != state);
    DEV_ASSERT((EEE_IMMEDIATE_NONE == iReq) || (EEE_IMMEDIATE_READ == iReq));
    status_t returnCode = STATUS_SUCCESS;         /* Return code */
    flash_state_t suspendState = C55_OK;
    uint32_t i;

    /* Check the write lock */
    if (state->blockWriteFlag == true)
    {
        /* A write operation is already in progress, no write operation is permitted */
        returnCode = STATUS_BUSY;
    }
    else
    {
        /* Set the status flag for reading */
        g_readStatusFlag = true;

        /* Search in the cache table */
        if (state->cacheEnable == true)
        {
            returnCode = EEE_DRV_SearchInTable(state->cTable,
                                               dataID,
                                               recordAddr);
            if (returnCode == STATUS_SUCCESS)
            {
                returnCode = EEE_DRV_SearchIdInCacheTable(iReq,
                                                          recordAddr,
                                                          &suspendState);
            }
        }

        /* Search in flash block if cache is disable or the record is not in cache
         *************************************************************************/
        if ((state->cacheEnable == false) || (returnCode == STATUS_EEE_ERROR_NOT_IN_CACHE))
        {
            /* The EEE_DRV_SearchInAllBlocks function will suspend High voltage operation if need */
            returnCode = EEE_DRV_SearchInAllBlocks(dataID,
                                                   iReq,
                                                   recordAddr,
                                                   &suspendState);
        }

        /* Read record if found */
        if (returnCode == STATUS_SUCCESS)
        {
            /* Read the record if found */
            i = *recordAddr;
            EEE_DRV_ReadRecordAtAddr(i,
                                     dataSize,
                                     buffAddr);
        }

        /* Resume high voltage if it has been suspended */
        if (suspendState == C55_ERS_SUS)
        {
            returnCode = FLASH_DRV_Resume(&suspendState);
        }

        /* Reset the status flag */
        g_readStatusFlag = false;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_DeleteRecord
 * Description   : This function is to delete a data record in the EEPROM emulated Flash
 *
 * Implements    : EEE_DRV_DeleteRecord_Activity
 *END**************************************************************************/
status_t EEE_DRV_DeleteRecord(uint16_t dataID,
                              eee_request_type_t iReq)
{
    eee_state_t * state = g_eeeState;
    DEV_ASSERT(NULL != state);
    DEV_ASSERT((EEE_IMMEDIATE_NONE == iReq) || (EEE_IMMEDIATE_DELETE == iReq));
    status_t returnCode = STATUS_SUCCESS;   /* Return code */
    uint32_t recordAddr = 0UL;              /* Data record address */
    flash_state_t suspendState = C55_OK;
    uint32_t i;
    bool updateCache = false;

    /* Check the write lock */
    if (state->blockWriteFlag == true)
    {
        /* A write operation is already in progress, no write operation is permitted */
        returnCode = STATUS_BUSY;
    }
    else
    {
        /* Set the write lock flag */
        state->blockWriteFlag = true;

        /* Search in the cache table
         ***************************/
        if (state->cacheEnable == true)
        {
            returnCode = EEE_DRV_SearchInTable(state->cTable,
                                               dataID,
                                               (uint32_t*)(&recordAddr));
            if (returnCode == STATUS_SUCCESS)
            {
                returnCode = EEE_DRV_SearchIdInCacheTable(iReq,
                                                          (uint32_t*)(&recordAddr),
                                                          &suspendState);
            }
        }

        /* Search in flash block if cache is disable or the record is not in cache
         *************************************************************************/
        if ((state->cacheEnable == false) || (returnCode == STATUS_EEE_ERROR_NOT_IN_CACHE))
        {
            /* The EEE_DRV_SearchInAllBlocks function will suspend High voltage operation if need */
            returnCode = EEE_DRV_SearchInAllBlocks(dataID,
                                                   iReq,
                                                   (uint32_t*)(&recordAddr),
                                                   (&suspendState));
        }

        /* Mark the record as deleted */
        if (returnCode == STATUS_SUCCESS)
        {
            i = (uint32_t)EEE_DELETED_RECORD;   /* Use i as a data source for writing */
            returnCode = EEE_DRV_SyncProgram(recordAddr,
                                             C55_DWORD_SIZE,
                                             (uint32_t)(&i));

            if (returnCode == STATUS_SUCCESS)
            {
                /* Check if the cache table is enabled */
                if (state->cacheEnable == true)
                {
                    updateCache = EEE_DRV_UpdateCacheTable(state->cTable,
                                                           dataID,
                                                           (uint32_t)EEE_DELETED_RECORD_IND);
                    (void) updateCache;
                }
            }
        }

        /* Resume high voltage if it has been suspended */
        if (suspendState == C55_ERS_SUS)
        {
            returnCode = FLASH_DRV_Resume(&suspendState);
        }

        /* Unlock the write lock */
        state->blockWriteFlag = false;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ReportEepromStatus
 * Description   : This function is to report block erasing cycles and check
 * the block status.
 *
 * Implements    : EEE_DRV_ReportEepromStatus_Activity
 *END**************************************************************************/
status_t EEE_DRV_ReportEepromStatus(uint32_t * erasingCycles)
{
    const eee_block_config_t * blockConf;   /* local block configuration pointer */
    const eee_state_t * state = g_eeeState;
    DEV_ASSERT(NULL != state);
    status_t returnCode = STATUS_SUCCESS;   /* Return code */
    flash_state_t opResult;
    eee_block_status_t blockStatus;         /* Block status */
    uint32_t i;                             /* Loop counter */

    /* Check the write lock */
    if (state->blockWriteFlag != false)
    {
        /* A write operation is already in progress, */
        /* no write operation is permitted */
        returnCode = STATUS_BUSY;
    }
    else
    {
        /* Get flash status */
        returnCode = FLASH_DRV_CheckEraseStatus(&opResult);

        if (returnCode != STATUS_FLASH_INPROGRESS)
        {
            /* Read erase cycle of the last ACTIVE block */
            blockConf = state->flashBlocks[state->activeBlockIndex];

            /* Use blockStatus as temporary return value */
            returnCode = EEE_DRV_FlashRead(EEE_READ,
                                           blockConf->blockStartAddr + (state->eccSize),
                                           C55_WORD_SIZE,
                                           (uint32_t)erasingCycles);
            DEV_ASSERT(returnCode == STATUS_SUCCESS);

            /* Check the block status */
            for (i = 0U; i < state->numberOfBlock; i++)
            {
                /* Get block configuration */
                blockConf = state->flashBlocks[i];
                blockStatus = EEE_DRV_ReadBlockStatus(blockConf);

                /* Check the block status */
                if ((blockStatus != EEE_BLOCK_ACT) && (blockStatus != EEE_BLOCK_COPY_DONE) &&
                    (blockStatus != EEE_BLOCK_ALT) && (blockStatus != EEE_BLOCK_ERASED))
                {
                    /* Error status -> need call EEE_DRV_InitEeprom to refresh the system */
                    returnCode = STATUS_ERROR;
                    break;
                }
            }
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_RemoveEeprom
 * Description   : This function is to clear all blocks used for EEPROM emulation.
 *
 * Implements    : EEE_DRV_RemoveEeprom_Activity
 *END**************************************************************************/
status_t EEE_DRV_RemoveEeprom(void)
{
    eee_state_t * state = g_eeeState;
    DEV_ASSERT(NULL != state);
    status_t returnCode = STATUS_SUCCESS;      /* Return code */
    uint32_t i;

    /* Check the write lock of the FlashEE */
    if (state->blockWriteFlag != false)
    {
        /* A write operation is already in progress, */
        /* no write operation is permitted */
        returnCode = STATUS_BUSY;
    }
    else
    {
        /* Set the write lock flag */
        state->blockWriteFlag = true;

        /* Erase all blocks */
        for (i = 0U; i < state->numberOfBlock; i++)
        {
            returnCode = EEE_DRV_EraseEEBlock(i,
                                              true);
            if (returnCode != STATUS_SUCCESS)
            {
                break;
            }
        }

        /* Unlock the write lock */
        state->blockWriteFlag = false;

        /* Check the return code is successful */
        if (returnCode == STATUS_SUCCESS)
        {
            g_eeeState = NULL;
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_MainFunction
 * Description   : This function will help in erasing the oldest ACTIVE block
 * asynchronously, update erase cycles and block status.
 *
 * Implements    : EEE_DRV_MainFunction_Activity
 *END**************************************************************************/
status_t EEE_DRV_MainFunction(void)
{
    eee_block_config_t * blockConf;
    const eee_state_t * state = g_eeeState;
    DEV_ASSERT(NULL != state);
    status_t returnCode = STATUS_SUCCESS;
    uint32_t blockStatus;

    /* CallBack service routine */
    if (state->callback != NULL)
    {
        state->callback(state->callbackParam);
    }

    /* Check status and end operation
     ********************************/
    if (g_eraseStatusFlag == EEE_ERASE_IN_PROGRESS)
    {
        blockConf = state->flashBlocks[g_sourceBlockIndexInternal];

        /* Get the erase operation status */
        returnCode = EEE_DRV_GetEraseEEBlockStatus();

        if (returnCode == STATUS_EEE_HVOP_INPROGRESS)
        {
            /* Update erase status */
            g_eraseStatusFlag = EEE_ERASE_IN_PROGRESS;
        }
        else if (returnCode == STATUS_ERROR)   /* Failed to erase */
        {
            /* Update erase status */
            g_eraseStatusFlag = EEE_ERASE_FAIL;

            /* Make the block to DEAD */
            returnCode = EEE_DRV_MakeBlockToDead(blockConf);
            if (returnCode != STATUS_SUCCESS)
            {
                g_eraseStatusFlag = EEE_ERASE_SWAP_ERROR;
            }
        }
        else    /* Erase successful */
        {
            /* Update erase status */
            g_eraseStatusFlag = EEE_ERASE_DONE;

            /* Write erase cycle to the erased block */
            returnCode = EEE_DRV_SyncProgram(blockConf->blockStartAddr + (state->eccSize),
                                             C55_WORD_SIZE,
                                             (uint32_t)(&g_erasingCycleInternal));
            if (returnCode != STATUS_SUCCESS)
            {
                g_eraseStatusFlag = EEE_ERASE_SWAP_ERROR;
                returnCode = STATUS_EEE_ERROR_PROGRAM_INDICATOR;
            }
            else
            {
                /* Update blank space pointer */
                blockConf->blankSpace = blockConf->blockStartAddr + (4U * (state->eccSize));
            }
        }

        /* Make the COPY_DONE block to ACTIVE
         ************************************/
        if (returnCode == STATUS_SUCCESS)
        {
            /* Make the COPY_DONE block to ACTIVE */
            blockStatus = ACT_INDICATOR_ACT;
            blockConf = state->flashBlocks[state->activeBlockIndex];
            returnCode = EEE_DRV_ProgramBlockIndicator(blockConf->blockStartAddr,
                                                       (uint32_t)(&blockStatus));
            if (returnCode == STATUS_SUCCESS)
            {
                /* Reset erase flag */
                g_eraseStatusFlag = EEE_ERASE_NOT_STARTED;
            }
            else
            {
                g_eraseStatusFlag = EEE_ERASE_SWAP_ERROR;
                returnCode = STATUS_EEE_ERROR_PROGRAM_INDICATOR;
            }
        }
    }

    if (returnCode == STATUS_EEE_HVOP_INPROGRESS)
    {
        /* If there is a on-going HVOP, we continue waiting.
         * So reset the returnCode */
        returnCode = STATUS_SUCCESS;
    }

    return returnCode;
}
/*******************************************************************************
 * EOF
 ******************************************************************************/
