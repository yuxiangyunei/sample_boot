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
 * Violates MISRA 2012 Required Rule 10.8, Impermissible cast of composite
 * expression (different essential type categories).
 * This is required by the conversion of a bit-field of a register into enum type.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 11.3, The cast performed between a pointer to
 * object type and a pointer to a different object type.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 18.4, Pointer arithmetic other than array
 * indexing used.
 * This is required to increment the source address.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.9, Could define variable at block scope
 * The variable is used in driver c file, so it must remain global.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.7, External could be made static.
 * Function is defined for usage by application code.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 10.5,
 * Impermissible cast; cannot cast from 'essentially unsigned' to 'essentially enum<i>'.
 * All possible values are covered by the enumeration, direct casting is used to optimize code.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 10.3,
 * Expression assigned to a narrower or different essential type.
 * All possible values are covered by the enumeration, direct casting is used to optimize code.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 15.4, More than one 'break' terminates loop
 * This operation is necessary because more than one source can stop the execution of the loop.
 * Also these cases can't be merged to use a single one break because for each case must be done some
 * specific operation.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 10.6, Composite expression assigned to a wider essential type.
 * This operation is used to read an unsigned long address.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 14.3, Boolean within 'if' always evaluates to True,
 * Boolean within 'if' always evaluates to False. This is required to optimize code size.
 *
 * @section [global]
 * Violates MISRA 2012 Mandatory Rule 9.1,  An Array conceivably not initialized.
 * This array has the changed length according to user configuration.
 */

#include "eee_common.h"


/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Variable to store internal erase cycle */
uint32_t g_erasingCycleInternal = 0x0U;

/* Variable to store index of the erasing block while swapping */
uint32_t g_sourceBlockIndexInternal = 0x0U;

/* Flag to keep track of ECC Error Status */
bool g_eccErrorStatusFlag = false;

/* Variable to store the number of EEE block is erased */
uint32_t g_numOfErase = 0x0U;

/* Variable to store the status of read function */
bool g_readStatusFlag = false;

#ifndef SWAP_CACHE_SIZE
#define SWAP_CACHE_SIZE    (4U)
#endif

static uint8_t swapCache[SWAP_CACHE_SIZE];

/* Variables to store internal for recovering EEE block */
static uint32_t activeNum = 0U;
static uint32_t copyDoneNum = 0U;
static uint32_t erasedNum = 0U;
static uint32_t updatedNum = 0U;
static uint32_t alterNum = 0U;
/* Used to determine that all Alter blocks have erase cycle value == 1 ? */
static uint32_t alterBlockEC = 1UL;
static uint32_t copyDoneIndex = 0xFFFFFFFFU;
static uint32_t lastestActiveIndex = 0xFFFFFFFFU;
static uint32_t tempBufferSize = 0U;
/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_FlashRead
 * Description   : This function is used to read the content of the flash memory.
 * Verify data and check the block is blank.
 *
 *END**************************************************************************/
status_t EEE_DRV_FlashRead(eee_read_code_t funcCode,
                           uint32_t dest,
                           uint32_t size,
                           uint32_t buffer)
{
    status_t returnCode = STATUS_SUCCESS;  /* return code */
    uint32_t counter;                      /* loop counter */
    uint8_t value;                         /* get the value */
    uint8_t temp = 0xFFU;                  /* use in blank check or verify */
    const eee_state_t * state = g_eeeState;

    /* Set-up ECC detection method */
#if EEE_ERR_IVOR_EXCEPTION
    /* Save old MSR */
    uint32_t tempMSR = MFMSR();
    /* Enable EE bit of MSR */
    ENABLE_INTERRUPTS();
    /* Set ECC Error Module flag */
    g_eccErrorModuleFlag = EEE_MODULE;
#endif

#if EEE_ERR_C55_MCR
    uint32_t addr;
    if ((C55FMC_MCR_EER_MASK & REG_READ32(C55FMC_BASE)) != 0U)
    {
        REG_BIT_SET32(C55FMC_BASE, C55FMC_MCR_EER_MASK);
    }
#endif

    /* Clear the EEC flag before reading */
    g_eccErrorStatusFlag = false;

    /* Read data
     ***********
     */
    for (counter = 1U; counter <= size; counter++)
    {
        /* Callback service */
        if ((counter % (state->numOfByteRead)) == 0U)
        {
            if (state->callback != NULL)
            {
                state->callback(state->callbackParam);
            }
        }

        /* Get data */
        value = *(volatile uint8_t*)(dest);

        /* Check for ECC */
#if EEE_ERR_C55_MCR
        addr = FLASH_DRV_GetFailedAddress();
        if (((C55FMC_MCR_EER_MASK & REG_READ32(C55FMC_BASE)) != 0U) && ((dest & (~0x3U)) == addr) && (value == 0xFFU))
        {
            g_eccErrorStatusFlag = true;
        }
#endif

        /* Reading operation */
        if (funcCode == EEE_READ)
        {
            *(volatile uint8_t*)(buffer) = value;
        }
        else
        {
            if (g_eccErrorStatusFlag != false)
            {
                returnCode = STATUS_ERROR;
            }
            else
            {
                /* Verify operation */
                if (funcCode == EEE_VERIFY)
                {
                    temp = *(volatile uint8_t*)(buffer);
                }

                if (temp != value)
                {
                    returnCode = STATUS_ERROR;
                }
            }
        }

        /* Stop the verification if there is an error */
        if (returnCode != STATUS_SUCCESS)
        {
            break;
        }

        /* Update destination and buffer */
        dest += 1U;
        buffer += 1U;
    }

#if EEE_ERR_IVOR_EXCEPTION
    /* Restore ECC Module flag */
    g_eccErrorModuleFlag = EEE_NONE;
    /* Check EE bit status of old MSR again  */
    if((temp & ENABLE_MSR) != ENABLE_MSR)
    {
        DISABLE_INTERRUPTS();
    }
#endif

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ExceptionHandler
 * Description   : This function is serviced routine for ECC in read interrupt.
 *
 *END**************************************************************************/
#if EEE_ERR_IVOR_EXCEPTION
uint32_t EEE_DRV_ExceptionHandler(uint32_t return_address,
                                  uint16_t instruction)
{
    if (g_eccErrorModuleFlag == EEE_MODULE)
    {
        g_eccErrorStatusFlag = true;

#if VLE_IS_ON
        if ((instruction & 0x9000U) == 0x1000U)
        {
            /* First 4 Bits have a value of 1,3,5,7 */
            /* Instruction was 32 bit */
            return_address += 4U;
        }
        else
        {
            /* First 4 Bits have a value of 0,2,4,6,8,9,A,B,C,D,E (and F, but F is reserved) */
            /* Instruction was 16 bit */
            return_address += 2U;
        }
#else
        /* BookE */
        return_address += 4U;
#endif /* VLE_IS_ON */
    }

    return return_address;
}
#endif /* EEE_ERR_IVOR_EXCEPTION */

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ReadRecordHead
 * Description   : This function will read record head from specified address.
 * It stores data into the data record structure, with the data size, data id
 * and some small data.
 *
 *END**************************************************************************/
static status_t EEE_DRV_ReadRecordHead(uint32_t dest,
                                       eee_data_record_head_t * pRecHead)
{
    DEV_ASSERT(pRecHead != NULL);
    status_t returnCode = STATUS_SUCCESS;
    const eee_state_t * state = g_eeeState;
    uint32_t buffer[(state->idOffset + (uint32_t)C55_WORD_SIZE)/(uint32_t)C55_WORD_SIZE];

    /* Read data record header from flash */
    returnCode = EEE_DRV_FlashRead(EEE_READ,
                                   dest,
                                   state->idOffset + C55_WORD_SIZE,
                                   (uint32_t)buffer);
    /* get all the value of the header record */
    pRecHead->dataStatus = buffer[0];
    pRecHead->dataSize = (uint16_t)buffer[(state->smallDataSize/C55_WORD_SIZE) + 2U];
    pRecHead->dataID = (uint16_t)(buffer[(state->smallDataSize/C55_WORD_SIZE) + 2U] >> 16U);

    if (state->sizeField == 0U)
    {
        /* For fix length record scheme */
        pRecHead->dataSize = (uint16_t)state->dataSize;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetRecordLength
 * Description   : This function will calculate the record length based on data
 * size input.
 *
 *END**************************************************************************/
uint32_t EEE_DRV_GetRecordLength(uint16_t dataSize)
{
    uint32_t recordLength;
    const eee_state_t * state = g_eeeState;

    if (dataSize <= (state->dataHeadSize))
    {
        recordLength = state->minRecordSize;
    }
    else
    {
        /* Increment by eccSize */
        if (((dataSize - (state->dataHeadSize)) % (state->eccSize)) != 0U)
        {
            recordLength = state->minRecordSize + ((state->eccSize) * (((dataSize - (state->dataHeadSize)) / (state->eccSize)) + 1U));
        }
        else
        {
            recordLength = (state->minRecordSize - (state->dataHeadSize)) + dataSize;
        }
    }

    return recordLength;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_CheckRecordStatus
 * Description   : This function search in the record status with various
 * type of record.
 *
 *END**************************************************************************/
static bool EEE_DRV_CheckRecordStatus(eee_block_config_t * blockConf,
                                      uint32_t searchAddr,
                                      uint32_t bufferAddress,
                                      uint16_t startID,
                                      uint16_t * nextStartID,
                                      uint16_t * dataSize)
{
    const eee_state_t * state = g_eeeState;
    eee_data_record_head_t record;        /* Local data record head structure */
    status_t returnCode = STATUS_SUCCESS; /* Return code */
    uint32_t recStatus;                   /* Record status */
    uint16_t endID;                       /* Define the data ID range */
    uint16_t nextRecId = *nextStartID;       /* Store the maximum ID found in the block */
    bool isFinish = false;                /* Finish searching flag */
    bool errorStatusFlagTemp;
    uint8_t stateOption;

    /* Calculate the boundary of the data ID range */
    endID = startID + (uint16_t)(tempBufferSize / C55_WORD_SIZE);

    /* Read the record */
    returnCode = EEE_DRV_ReadRecordHead(searchAddr,
                                        &record);
    recStatus = record.dataStatus;
    *dataSize = record.dataSize;

    errorStatusFlagTemp = g_eccErrorStatusFlag;

    /* If there is no ECC error, check the record status with various values */
    if ((recStatus == EEE_PROGRAMED_RECORD) && (errorStatusFlagTemp == false))
    {
        stateOption = 0U;
    }
    else if ((recStatus == EEE_DELETED_RECORD) && (errorStatusFlagTemp == false))
    {
        stateOption = 1U;
    }
    else if ((recStatus == EEE_ERASED_RECORD) && (errorStatusFlagTemp == false))
    {
        stateOption = 2U;
    }
    else /* Have ECC error or random value in read record head */
    {
        stateOption = 3U;
    }

    switch (stateOption)
    {
        case 0:
            if (bufferAddress == EEE_ERASED_WORD)
            {
                tempBufferSize++;
            }
            else
            {
                /* This is valid record and check the ID */
                if ((record.dataID >= startID) && (record.dataID < endID))
                {
                    REG_WRITE32(bufferAddress + ((uint32_t)(record.dataID - startID) * (uint32_t)C55_WORD_SIZE), searchAddr);
                }

                if ((record.dataID >= endID) && (record.dataID < nextRecId))
                {
                    /* The start ID for next call */
                    nextRecId = record.dataID;
                    *nextStartID = nextRecId;
                }
            }
            break;
        case 1:
            if (bufferAddress == EEE_ERASED_WORD)
            {
                tempBufferSize++;
            }
            else
            {
                /* Check the ID */
                if ((record.dataID >= startID) && (record.dataID < endID))
                {
                    /* Delete found result */
                    REG_WRITE32(bufferAddress + ((uint32_t)(record.dataID - startID) * (uint32_t)C55_WORD_SIZE), (uint32_t)EEE_DELETED_RECORD_IND);
                }
            }
        break;
        case 2:
            /* Do blank check for the record header */
            returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                           searchAddr,
                                           state->minRecordSize,
                                           0U);

            /* If record head is fully blanked, it means this blank space, so stop searching */
            if (returnCode == STATUS_SUCCESS)
            {
                /* Update blank space for the block */
                blockConf->blankSpace = searchAddr;
                /* Finish the searching */
                isFinish = true;
            }
            else /* Record head is not blanked, skip address determined by dataSize to continue searching */
            {
                /* Determine to dataSize for calculate record length */
                if (*dataSize == 0xFFFFU)
                {
                    *dataSize = (uint16_t)state->dataHeadSize;
                }
            }
        break;
        case 3:
            /* Check if having ECC in ID or SIZE field */
            if (state->sizeField != 0U)
            {
                returnCode = EEE_DRV_FlashRead(EEE_READ,
                                               searchAddr + (state->idOffset),
                                               C55_WORD_SIZE,
                                               (uint32_t)(&recStatus));
                if (g_eccErrorStatusFlag == true)
                {
                    *dataSize = (uint16_t)state->dataHeadSize;
                }
            }
        break;
        default:
            /* Nothing to do */
            break;
    }

    (void)returnCode;
    (void)nextRecId;

    return isFinish;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SearchRecordFromTop
 * Description   : This function will search the required IDs of the data record from
 * the start of the specific block region. It will check the number of records which
 * search records at most of EEprom block so it can return within the time limit.
 *
 *END**************************************************************************/
uint16_t EEE_DRV_SearchRecordFromTop(eee_block_config_t * blockConf,
                                     uint32_t bufferAddress,
                                     uint32_t bufferSize,
                                     uint16_t startID)
{
    DEV_ASSERT(blockConf != NULL);
    DEV_ASSERT(startID <= 0x7FFFU);
    const eee_state_t * state = g_eeeState;
    uint32_t index;                       /* Loop index */
    uint32_t searchAddr;                  /* Store address of record */
    uint32_t endAddrInBlock;              /* End address in block */
    uint32_t recordLength;                /* The length of the record */
    uint16_t dataSize;                    /* Size of record */
    uint16_t nextStartID = 0xFFFFU;       /* Store the maximum ID found in the block */
    bool isFinish = false;                /* Finish searching flag */
    tempBufferSize = bufferSize;

    /* Check the address of buffer */
    if (bufferAddress == EEE_ERASED_WORD)
    {
        /* Use bufferSize to store number of valid records (Completed or Deleted)
         * if use the function to count number of records
         * Reset counter */
        tempBufferSize = 0U;
    }

    /* Start at the first record in the block */
    searchAddr = blockConf->blockStartAddr + (4U * (state->eccSize));
    /* Calculate end address of the specific block */
    endAddrInBlock = blockConf->blockStartAddr + blockConf->blockSize;

    for (index = 1U; index <= (state->numOfRecordSearch); index++)
    {
        /* Callback service */
        if ((index % (state->numOfCycleSearch)) == 0U)
        {
            if (state->callback != NULL)
            {
                state->callback(state->callbackParam);
            }
        }

        /* Search in all data record */
        isFinish = EEE_DRV_CheckRecordStatus(blockConf,
                                             searchAddr,
                                             bufferAddress,
                                             startID,
                                             &nextStartID,
                                             &dataSize);

        if (isFinish == false)
        {
            /* Calculate record length */
            recordLength = EEE_DRV_GetRecordLength(dataSize);
            /* Update the start address */
            searchAddr += recordLength;

            /* Check for reaching end of the block */
            if (searchAddr > (endAddrInBlock - state->minRecordSize))
            {
                /* Update blank space for the block */
                if (searchAddr >= endAddrInBlock)
                {
                    blockConf->blankSpace = endAddrInBlock;
                }
                else
                {
                    blockConf->blankSpace = searchAddr;
                }

                isFinish = true;
            }
        }

        /* Finish searching */
        if (isFinish == true)
        {
            break;
        }
    }

    if (bufferAddress == EEE_ERASED_WORD)
    {
        /* If use the function to count number of valid (completed or deleted) records */
        nextStartID = (uint16_t)tempBufferSize;
    }

    return nextStartID;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SearchRecordFromBottom
 * Description   : This function will search the latest record ID with status
 * is completed or deleted in block.
 * Note: This function only use in case as fixing length for data record.
 *
 *END**************************************************************************/
static void EEE_DRV_SearchRecordFromBottom(const eee_block_config_t * blockConf,
                                           uint16_t recordID,
                                           uint32_t * recordAddr)
{
    DEV_ASSERT(blockConf != NULL);
    eee_data_record_head_t record;        /* Local data record head structure */
    uint32_t recordLength;                /* Length of record */
    uint32_t searchAddr;
    uint32_t count = 0U;
    status_t returnCode = STATUS_SUCCESS; /* Return code */
    bool errorStatusFlagTemp;
    const eee_state_t * state = g_eeeState;

    /* Begin searching at blank space */
    searchAddr = blockConf->blankSpace;

    /* Store number of searched records for CallBack() service */
    recordLength = EEE_DRV_GetRecordLength((uint16_t)state->dataSize);

    while (searchAddr > ((blockConf->blockStartAddr) + (4U * (state->eccSize))))
    {
        /* Point to start address of previous record */
        searchAddr -= recordLength;

        /* CallBack service routine */
        count++;
        if ((count % (state->numOfCycleSearch)) == 0U)
        {
            if (state->callback != NULL)
            {
                state->callback(state->callbackParam);
            }
        }

        /* Read record head */
        returnCode = EEE_DRV_ReadRecordHead(searchAddr,
                                            &record);

        errorStatusFlagTemp = g_eccErrorStatusFlag;
        /* If have no ECC while reading record head, check dataID and record status for validation */
        if ((recordID == record.dataID) && (errorStatusFlagTemp == false) && (returnCode == STATUS_SUCCESS))
        {
            if (record.dataStatus == (uint32_t)EEE_DELETED_RECORD)
            {
                /* Mark as deleted */
                *recordAddr = (uint32_t)EEE_DELETED_RECORD_IND;
            }
            else
            {
                /* Found the record */
                *recordAddr = searchAddr;
            }

            /* Stop searching */
            break;
        }
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ReadOtherStatus
 * Description   : This function will read status of EEE block with some
 * specific cases.
 *
 *END**************************************************************************/
static eee_block_status_t EEE_DRV_ReadOtherStatus(const eee_block_config_t * blockConf)
{
    DEV_ASSERT(blockConf != NULL);
    eee_block_status_t retStatus;
    const eee_state_t * state = g_eeeState;
    status_t returnCode;
    uint32_t temp = 0UL;
    uint32_t buffer = 0UL;
    uint32_t compareValue = 0U;
    bool bakActEccFlag;
    bool bakCopEccFlag;

    /* Read ACTIVE indicator, use the returnCode as buffer temporary */
    returnCode = EEE_DRV_FlashRead(EEE_READ,
                                   blockConf->blockStartAddr,
                                   C55_WORD_SIZE,
                                   (uint32_t)(&buffer));

    /* Backup ECC flag */
    bakActEccFlag = g_eccErrorStatusFlag;
    compareValue = buffer;
    /* Read COPY_DONE indicator, use the temp as buffer temporary */
    returnCode = EEE_DRV_FlashRead(EEE_READ,
                                   (blockConf->blockStartAddr) + (3U * (state->eccSize)),
                                   C55_WORD_SIZE,
                                   (uint32_t)(&temp));

    /* Backup ECC flag */
    bakCopEccFlag = g_eccErrorStatusFlag;

    /* Determine the block status */
    if ((bakActEccFlag == true) || (compareValue != 0xFFFFFFFFU))
    {
        retStatus = EEE_BLOCK_ACT;
    }
    else if (((bakActEccFlag == false) && (compareValue == 0xFFFFFFFFU)) && ((bakCopEccFlag == true) || (temp != 0xFFFFFFFFU)))
    {
        retStatus = EEE_BLOCK_COPY_DONE;
    }
    else
    {
        /* Blank check the record region */
        returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                       (blockConf->blockStartAddr) + (4U * (state->eccSize)),
                                       (blockConf->blockSize) - (4UL * (state->eccSize)),
                                       0U);

        if ((returnCode != STATUS_SUCCESS) && ((bakActEccFlag == false) && (compareValue == 0xFFFFFFFFU)) && ((bakCopEccFlag == false) && (temp == 0xFFFFFFFFU)))
        {
            retStatus = EEE_BLOCK_UPDATE;
        }
        else if ((returnCode == STATUS_SUCCESS) && ((bakActEccFlag == false) && (compareValue == 0xFFFFFFFFU)) && ((bakCopEccFlag == false) && (temp == 0xFFFFFFFFU)))
        {
            retStatus = EEE_BLOCK_ALT;
        }
        else
        {
            retStatus = EEE_BLOCK_INVALID;
        }
    }

    (void)returnCode;
    (void)buffer;
    (void)bakCopEccFlag;

    return retStatus;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ReadBlockStatus
 * Description   : This function will read status of EEE block.
 *
 *END**************************************************************************/
eee_block_status_t EEE_DRV_ReadBlockStatus(const eee_block_config_t * blockConf)
{
    DEV_ASSERT(blockConf != NULL);
    eee_block_status_t retStatus;
    const eee_state_t * state = g_eeeState;
    status_t returnCode;
    uint32_t temp = 0UL;
    uint32_t buffer = 0UL;
    uint32_t compareValue = 0U;
    bool errorStatusFlagTemp;

    /* Read the DEAD block indicator */
    returnCode = EEE_DRV_FlashRead(EEE_READ,
                                   blockConf->blockStartAddr + (2U * (state->eccSize)),
                                   C55_WORD_SIZE,
                                   (uint32_t)(&temp));

    errorStatusFlagTemp = g_eccErrorStatusFlag;
    compareValue = temp;
    /* Check if the dead block indicator != 0xFFFFFFFF or have ECC */
    if (((compareValue == EEE_ERASED_WORD) && (false == errorStatusFlagTemp)) == false)
    {
        retStatus = EEE_BLOCK_DEAD;
    }
    else
    {
        /* Read erase cycle, the EEE_DRV_FlashRead function will reset 'g_eccErrorStatusFlag' */
        returnCode = EEE_DRV_FlashRead(EEE_READ,
                                       (blockConf->blockStartAddr) + (state->eccSize),
                                       C55_WORD_SIZE,
                                       (uint32_t)(&temp));
        compareValue = temp;
        if (g_eccErrorStatusFlag == true)
        {
            retStatus = EEE_BLOCK_INVALID;
        }
        else if (compareValue == EEE_ERASED_WORD)
        {
            /* Check blank for the block */
            returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                           blockConf->blockStartAddr,
                                           blockConf->blockSize,
                                           0U);
            if (returnCode == STATUS_SUCCESS)
            {
                retStatus = EEE_BLOCK_ERASED;
            }
            else
            {
                retStatus = EEE_BLOCK_INVALID;
            }
        }
        else
        {
            /* Read the block status on other cases */
            retStatus = EEE_DRV_ReadOtherStatus(blockConf);
        }
    }

    (void)returnCode;
    (void)buffer;
    (void)errorStatusFlagTemp;

    return retStatus;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SyncProgram
 * Description   : This function will used to program the data into flash memory
 * with the size of data sources which are provided by user.
 *
 *END**************************************************************************/
status_t EEE_DRV_SyncProgram(uint32_t dest,
                             uint16_t size,
                             uint32_t source)
{
    status_t returnCode = STATUS_SUCCESS;
    flash_context_data_t pCtxData;
    flash_state_t opResult = C55_OK;
    const eee_state_t * state = g_eeeState;
    uint8_t buffer[C55_DWORD_SIZE];    /* Internal source data buffer */
    uint32_t counter = 0U;            /* Loop counter */
    uint32_t temp;                    /* Temporary variable */
    uint32_t destBk;
    uint32_t sourceBk;
    uint16_t sizeBk;

    /* Get all value of input parameters */
    destBk = dest;
    sizeBk = size;
    sourceBk = source;

    while (size > 0U)
    {
        /* Number of words need to be programmed */
        temp = (uint32_t)(size % (uint16_t)C55_DWORD_SIZE);
        size -= (uint16_t)temp;
        if (counter != 0U)
        {
            /* Initialize buffer */
            for (counter = 0U; counter < C55_DWORD_SIZE; counter++)
            {
                buffer[counter] = 0xFFU;
            }

            /* Prepare source buffer to program */
            for (counter = 0UL; counter < (uint32_t)(sizeBk % (uint16_t)C55_DWORD_SIZE); counter++)
            {
                /* Copy data */
                buffer[counter] = *(volatile uint8_t *)source;
                /* Update source data pointer */
                source += 1U;
            }

            /* Update source data */
            source = (uint32_t)buffer;
        }

        /* Program data */
        returnCode = FLASH_DRV_Program(&pCtxData,
                                       dest,
                                       size,
                                       source);

        /* Check status */
        if (returnCode == STATUS_SUCCESS)
        {
            returnCode = STATUS_FLASH_INPROGRESS;

            /* Check the return code status */
            while (returnCode == STATUS_FLASH_INPROGRESS)
            {
                /* CallBack service */
                if (state->callback != NULL)
                {
                    state->callback(state->callbackParam);
                }

                /* Check status */
                returnCode = FLASH_DRV_CheckProgramStatus(&pCtxData,
                                                          &opResult);
            }
        }

        /* Check if return an error */
        if (returnCode != STATUS_SUCCESS)
        {
            break;
        }

        if (temp != 0U)
        {
            dest += size;
            source += size;
            size = C55_DWORD_SIZE;
            counter++;
        }
        else
        {
            size = 0U;
        }
    }

    if ((returnCode == STATUS_SUCCESS) && (opResult == C55_OK))
    {
        returnCode = EEE_DRV_FlashRead(EEE_VERIFY,
                                       destBk,
                                       sizeBk,
                                       sourceBk);
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_UpdateBankSpaceInWriteRecord
 * Description   : This function will update the address of the blank space in
 * the EEE active block.
 *
 *END**************************************************************************/
static void EEE_DRV_UpdateBankSpaceInWriteRecord(eee_block_config_t * blockConf)
{
    const eee_state_t * state = g_eeeState;
    uint16_t size = 0U;
    uint32_t dest;

    /* Get destination */
    dest = blockConf->blankSpace;

    if (g_eccErrorStatusFlag == true)
    {
        blockConf->blankSpace += (state->eccSize * 2U);       /* Skip ID/Size and status pages */
    }
    else
    {
        /* Read the size value */
        size = *(volatile uint16_t*)(dest + (state->eccSize) + 2UL);

        if (size == 0xFFFFU)
        {
             blockConf->blankSpace += (state->eccSize * 2U);  /* Skip ID/Size and status pages */
        }
        else
        {
            /* Skip actual programmed size */
            blockConf->blankSpace += EEE_DRV_GetRecordLength((uint16_t)size);

            /* Set blank space to end of block if it is larger */
            dest = blockConf->blockStartAddr + blockConf->blockSize;
            if (blockConf->blankSpace > dest)
            {
                blockConf->blankSpace = dest;
            }
        }
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_WriteRecordID
 * Description   : This function is used to program the ID file of record.
 *
 *END**************************************************************************/
static status_t EEE_DRV_WriteRecordID(eee_block_config_t * blockConf,
                                      bool backupFlag,
                                      uint16_t dataID,
                                      uint16_t dataSize,
                                      uint32_t source)
{
    status_t returnCode = STATUS_SUCCESS;         /* Return code */
    const eee_state_t * state = g_eeeState;
    uint32_t dest;                                /* Destination of writing operation */
    uint32_t temp;                                /* Temporary variable */
    uint32_t i;                                   /* Loop variable */
    uint16_t size;
    uint8_t pgmBuff[state->eccSize];              /* Data buffer */
    uint8_t value;
    bool errorStatusFlagTemp;

    /* Initialize pgmBuff */
    for(i = 0UL; i < state->eccSize; i++)
    {
        pgmBuff[i] = 0xFFU;
    }

    /* Get destination */
    dest = blockConf->blankSpace;
    /* 1. Program ID_SIZE field
     **************************
     */
    if (backupFlag == false) /* Prepare source for normal record writing */
    {
        /* Prepare Record ID field */
        (*(volatile uint16_t*)(pgmBuff)) = dataID;

        if (state->sizeField != 0UL)
        {
            /* Prepare Record Size field for variable length configuration */
            *(volatile uint16_t*)(pgmBuff + (state->sizeField)) = dataSize;
        }
        else
        {
            /* Prepare Record Size field for fixed length configuration */
            dataSize = (uint16_t)state->dataSize;
        }

        if (dataSize > state->smallDataSize)
        {
            if (dataSize >= state->dataHeadSize)
            {
                temp = (uint32_t)(state->dataHeadSize - state->smallDataSize);
            }
            else
            {
                temp = state->smallDataSize;
                temp = (uint32_t)dataSize - temp;
            }

            for (i = 0UL; i < temp; i++)
            {
                value = *(volatile uint8_t*)(source + i + (state->smallDataSize));
                (*(volatile uint8_t*)(pgmBuff + i + (state->sizeField) + ID_FIELD_SIZE)) = value;
            }
        }
        temp = (uint32_t)pgmBuff;
    }
    else /* Prepare source for copying record */
    {
        temp = (source + state->idOffset);
    }

    /* Determine size for writing */
    size = (uint16_t)state->eccSize;

    if (((state->sizeField) == 0UL) && (((state->eccSize) - C55_DWORD_SIZE) > 0UL))
    {
        size = (uint16_t)(state->eccSize) - (uint16_t)C55_DWORD_SIZE;
    }

    /* Write data to the EEC_SIZE region that contain ID and SIZE fields */
    returnCode = EEE_DRV_SyncProgram(dest + (state->idOffset),
                                     size,
                                     temp);

    if (returnCode != STATUS_SUCCESS)
    {
        /* if this program operation fails, need to do blank check again. If this record head
         * is still fully blanked, do not update blank space. Otherwise, need to update blank space.
         * Use temporary dest, size, source to avoid impacting to the current dest, size, source values */
        temp = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                 dest + (state->idOffset),
                                 (uint32_t)size,
                                 0U);

        errorStatusFlagTemp = g_eccErrorStatusFlag;
        if ((temp == (uint32_t)STATUS_SUCCESS) && (errorStatusFlagTemp == false))
        {
            /* Do not update blank space because this location is still fully blanked */
        }
        else
        {
            if ((state->sizeField) != 0U)
            {
                EEE_DRV_UpdateBankSpaceInWriteRecord(blockConf);
            }
            else    /* Fix length */
            {
                blockConf->blankSpace += EEE_DRV_GetRecordLength(dataSize);
            }
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_WriteRestOfRecord
 * Description   : This function is used to write the data header which is
 * remaining record data.
 *
 *END**************************************************************************/
static status_t EEE_DRV_WriteRestOfRecord(eee_block_config_t * blockConf,
                                          bool backupFlag,
                                          uint32_t dest,
                                          uint16_t dataSize,
                                          uint32_t source)
{
    status_t returnCode = STATUS_SUCCESS;         /* Return code */
    const eee_state_t * state = g_eeeState;
    uint32_t sourceData;                                /* Temporary variable */

    if (dataSize > state->dataHeadSize)
    {
        /* Prepare source */
        if (backupFlag == false)
        {
            sourceData = source + state->dataHeadSize;
        }
        else
        {
            sourceData = source + state->minRecordSize;
        }

        /* Write data */
        returnCode = EEE_DRV_SyncProgram(dest + state->minRecordSize,
                                         dataSize - (uint16_t)(state->dataHeadSize),
                                         sourceData);

        if (returnCode != STATUS_SUCCESS)
        {
            if ((state->sizeField) == 0U)
            {
                sourceData = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                               dest + state->minRecordSize,
                                               dataSize - (state->dataHeadSize),
                                               0U);

                if (sourceData != (uint32_t)STATUS_SUCCESS)
                {
                    /* Update blank space */
                    blockConf->blankSpace += EEE_DRV_GetRecordLength(dataSize);
                }
            }
            else
            {
                /* Update blank space */
                blockConf->blankSpace += EEE_DRV_GetRecordLength(dataSize);
            }
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_WriteDataRecord
 * Description   : This function is used to program and verify one data record
 * into the Flash memory. If the dataID = 0xFFFF and then backing up data from
 * block swapping.
 *
 *END**************************************************************************/
status_t EEE_DRV_WriteDataRecord(eee_block_config_t * blockConf,
                                 bool backupFlag,
                                 uint16_t dataID,
                                 uint16_t dataSize,
                                 uint32_t source)
{
    DEV_ASSERT(blockConf != NULL);
    status_t returnCode = STATUS_SUCCESS;         /* Return code */
    const eee_state_t * state = g_eeeState;
    uint32_t dest;                                /* Destination of writing operation */
    uint32_t temp;                                /* Temporary variable */
    uint32_t i;                                   /* Loop variable */
    uint8_t pgmBuff[state->eccSize];              /* Data buffer */
    uint8_t value;

    /* Get destination */
    dest = blockConf->blankSpace;

    /* Initialize pgmBuff */
    for(i = 0UL; i < state->eccSize; i++)
    {
        pgmBuff[i] = 0xFFU;
    }

    /* 1. Program ID_SIZE field
     **************************
     */
    returnCode = EEE_DRV_WriteRecordID(blockConf,
                                       backupFlag,
                                       dataID,
                                       dataSize,
                                       source);

    /* 2. Program the rest of data
     *****************************
     */
    if (returnCode == STATUS_SUCCESS)
    {
        returnCode = EEE_DRV_WriteRestOfRecord(blockConf,
                                               backupFlag,
                                               dest,
                                               dataSize,
                                               source);
    }

    /* 3. Program small data after record status
     *******************************************
     */
    if ((returnCode == STATUS_SUCCESS) && ((state->sizeField) != 0U))
    {
        /* Prepare source */
        if (backupFlag == false)
        {
            for (i = 0U; i < state->eccSize; i++)
            {
                pgmBuff[i] = 0xFFU;
            }

            for (i = 0U; ((i < state->smallDataSize) && (i < dataSize)); i++)
            {
                value = *(volatile uint8_t*)(source + i);
                *(volatile uint8_t*)(pgmBuff + i) = value;
            }
            temp = (uint32_t)pgmBuff;
        }
        else
        {
            temp = source + C55_DWORD_SIZE;
        }

        /* Write data */
        returnCode = EEE_DRV_SyncProgram(dest + C55_DWORD_SIZE,
                                         (uint16_t)((state->eccSize) - C55_DWORD_SIZE),
                                         temp);

        if (returnCode != STATUS_SUCCESS)
        {
            blockConf->blankSpace += EEE_DRV_GetRecordLength(dataSize);
        }
    }

    /* 4. Update record status to COMPLETE
     *************************************
     */
    if (returnCode == STATUS_SUCCESS)
    {
        /* Prepare source */
        REG_WRITE32(pgmBuff, (uint32_t)EEE_PROGRAMED_RECORD);
        REG_WRITE32(((uint32_t)pgmBuff + C55_WORD_SIZE), (uint32_t)EEE_ERASED_WORD);

        /* Write data */
        returnCode = EEE_DRV_SyncProgram(dest,
                                         C55_DWORD_SIZE,
                                         (uint32_t)pgmBuff);

        /* Update blank space */
        blockConf->blankSpace += EEE_DRV_GetRecordLength(dataSize);
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_UpdateCacheTable
 * Description   : This function is used to update the contents of the cache
 * table while the data address is changed by writing a new value or block swapping.
 *
 *END**************************************************************************/
bool EEE_DRV_UpdateCacheTable(const eee_cache_table_t * pCacheTable,
                              uint16_t dataID,
                              uint32_t newValue)
{
    uint32_t dest;
    uint8_t * temp;
    uint32_t size;
    bool returnCode = true;

    /* Get the maximum data ID number in cache table */
    size = (pCacheTable->size) / CTABLE_ITEM_SIZE;

    /* Check if it is in the cache table */
    if (dataID < (uint16_t)size)
    {
        /* Insert the data ID into the cache table */
        temp = (uint8_t *)pCacheTable->startAddress;
        dest = (uint32_t)temp;

        /* Update the cache table item with new value */
        REG_WRITE32(dest + (uint32_t)(CTABLE_ITEM_SIZE * dataID), newValue);
    }
    else
    {
        /* Data record is out of range of the cache table */
        returnCode = false;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_CopyDataRecord
 * Description   : This function will write data into EEE block until is successful.
 *
 *END**************************************************************************/
status_t EEE_DRV_CopyDataRecord(eee_block_config_t * blockConf,
                                bool backupFlag,
                                uint16_t dataID,
                                uint16_t dataSize,
                                uint32_t source)
{
    DEV_ASSERT(blockConf != NULL);
    status_t returnCode = STATUS_SUCCESS;
    const eee_state_t * state = g_eeeState;
    uint32_t recordLength;
    uint32_t blankSpaceBk;
    bool temp = false;

    if (state->sizeField == 0U)
    {
        dataSize = (uint16_t)state->dataSize;
    }

    /* Calculate record length */
    recordLength = EEE_DRV_GetRecordLength(dataSize);

    do
    {
        /* Check the free space */
        if (recordLength > (blockConf->blockStartAddr + blockConf->blockSize - blockConf->blankSpace))
        {
            returnCode = STATUS_EEE_ERROR_NO_ENOUGH_SPACE;
        }
        else
        {
            /* Backup record address */
            blankSpaceBk = blockConf->blankSpace;

            /* Write internal data to destination block */
            returnCode = EEE_DRV_WriteDataRecord(blockConf,
                                                 backupFlag,
                                                 dataID,
                                                 dataSize,
                                                 source);

            if (returnCode == STATUS_SUCCESS)
            {
                /* Check if the cache table is enabled */
                if (state->cacheEnable == true)
                {
                    temp = EEE_DRV_UpdateCacheTable(state->cTable,
                                                    dataID,
                                                    blankSpaceBk);
                    (void)temp;
                }
            }
        }
    }
    /* if ret = STATUS_ERROR, re-write the record into a different position,
     * is calculated by the EEE_DRV_WriteDataRecord function */
    while ((returnCode != STATUS_SUCCESS) && (returnCode != STATUS_EEE_ERROR_NO_ENOUGH_SPACE));

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SerachInOldActiveBlock
 * Description   : This function will search the data ID in all the active block
 * to find the record address.
 *END**************************************************************************/
static status_t EEE_DRV_SerachInOldActiveBlock(eee_block_config_t ** activeBlock,
                                               eee_block_config_t * destBlock,
                                               uint16_t dataID,
                                               uint32_t recordAddr,
                                               bool * checkFlag)
{
    const eee_state_t * state = g_eeeState;
    uint32_t blockIndex;
    status_t returnCode = STATUS_SUCCESS;
    eee_block_config_t * blockConf;
    uint32_t foundAddr = 0UL;
    uint32_t temComFoundAddr = 0U;
    eee_data_record_head_t recordHead;
    uint16_t tempNextID = 0U;
    *checkFlag = false;

    /* Search the ID in the tempActiveBlock array, start at the newest ACTIVE block */
    for (blockIndex = (state->numberOfActBlock); blockIndex > 0U; blockIndex--)
    {
        foundAddr = (uint32_t)EEE_ERASED_WORD;
        blockConf = activeBlock[blockIndex - 1U];
        tempNextID = EEE_DRV_SearchRecordFromTop(blockConf,
                                                 (uint32_t)(&foundAddr),
                                                 C55_WORD_SIZE,
                                                 dataID);
        DEV_ASSERT(tempNextID != 0U);
        (void) tempNextID;
        (void) activeBlock;
        temComFoundAddr = foundAddr;
        if (temComFoundAddr != EEE_ERASED_WORD)
        {
            break;   /* If found the dataID in the block or it's deleted, continue with the next ID */
        }
    }

    /* If the dataID isn't in the block array, copy it */
    if (foundAddr == (uint32_t)EEE_ERASED_WORD)
    {
        /* Read the record */
        returnCode = EEE_DRV_ReadRecordHead(recordAddr,
                                            &recordHead);

        /* Copy the record */
        returnCode = EEE_DRV_CopyDataRecord(destBlock,
                                            true,
                                            recordHead.dataID,
                                            recordHead.dataSize,
                                            recordAddr);
        if (returnCode != STATUS_SUCCESS)
        {
            /* Exit the swapping progress if data copying failed */
            *checkFlag = true;
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_CopyRecordOfActiveBlock
 * Description   : This function will copy the lastest record which is valid into
 * the alternate block.
 *END**************************************************************************/
static status_t EEE_DRV_CopyRecordOfActiveBlock(eee_block_config_t * sourceBlock,
                                                uint16_t nextStartID)
{
    DEV_ASSERT(sourceBlock != NULL);
    const eee_state_t * state = g_eeeState;
    eee_block_config_t * destBlock;    /* The destination (UPDATE) block configuration pointer */
    eee_block_config_t * tempActiveBlock[state->numberOfActBlock]; /* Array of ACTIVE and UPDATE blocks */
    status_t returnCode = STATUS_SUCCESS;
    uint32_t blockIndex;
    uint32_t i;
    uint32_t recordAddr;
    uint16_t dataID;
    uint16_t nextID = nextStartID;
    bool checkFlag;

    /* Determine the destination block (the block right after the newest ACTIVE block) */
    blockIndex = (state->activeBlockIndex + 1U) % state->numberOfBlock;
    destBlock = state->flashBlocks[blockIndex];

    /* Determine source block (the oldest ACTIVE) */
    blockIndex = (blockIndex + state->numberOfBlock - state->numberOfActBlock) % (state->numberOfBlock);

    /* Fill the array of UPDATE and ACTIVE blocks, except the Oldest ACTIVE block */
    for (i = 0U; i < (state->numberOfActBlock); i++)
    {
        /* The blockIndex currently store index of the oldest ACTIVE block */
        blockIndex = (blockIndex + 1U) % state->numberOfBlock;
        tempActiveBlock[i] = state->flashBlocks[blockIndex];
    }

    while ((nextID < state->maxRecordId) && (returnCode == STATUS_SUCCESS))
    {
        /* Now, use swap cache to buffering all valid data in the oldest active block */
        /* Clear the swap cache */
        for (i = 0U; i < (uint32_t)(SWAP_CACHE_SIZE / CTABLE_ITEM_SIZE); i++)
        {
            REG_WRITE32(swapCache + (i * CTABLE_ITEM_SIZE), EEE_ERASED_WORD);
        }

        /* Start with the next ID */
        dataID = nextID;

        /* Search in the source block: all id which were not in normal cache table will be stored in swap cache */
        nextID = EEE_DRV_SearchRecordFromTop(sourceBlock,
                                             (uint32_t)swapCache,
                                             SWAP_CACHE_SIZE,
                                             dataID);

        /* Check all IDs in Swap Cache
         *****************************/
        for (i = 0U; i < (uint32_t)(SWAP_CACHE_SIZE / CTABLE_ITEM_SIZE); i++)
        {
            /* Get record address */
            recordAddr = REG_READ32(swapCache + (i * CTABLE_ITEM_SIZE));

            /* If record ID is in the oldest ACTIVE block, check if it is in tempActiveBlock array */
            if (recordAddr < (uint32_t)EEE_DELETED_RECORD_IND)
            {
                returnCode = EEE_DRV_SerachInOldActiveBlock(&tempActiveBlock[0U],
                                                            destBlock,
                                                            dataID,
                                                            recordAddr,
                                                            &checkFlag);
                if (checkFlag == true)
                {
                    break;
                }
            }

            /* Update ID for the next searching round */
            dataID += 1U;
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_BlockSwapping
 * Description   : This function will copy the latest data records from the
 * active block which is nearly full to the alternative block.
 *
 *END**************************************************************************/
status_t EEE_DRV_BlockSwapping(bool syncErase)
{
    eee_state_t * state = g_eeeState;
    eee_block_config_t * sourceBlock;  /* The source (oldest ACTIVE) block configuration pointer */
    eee_block_config_t * destBlock;    /* The destination (UPDATE) block configuration pointer */
    eee_data_record_head_t recordHead;
    status_t returnCode = STATUS_SUCCESS;
    uint32_t blockIndex;
    uint32_t i;
    uint32_t recordAddr;
    uint16_t nextStartID;
    uint32_t tempAddrInCache;

    /* Determine the destination block (the block right after the newest ACTIVE block) */
    blockIndex = (state->activeBlockIndex + 1U) % state->numberOfBlock;
    destBlock = state->flashBlocks[blockIndex];

    /* Determine source block (the oldest ACTIVE) */
    blockIndex = (blockIndex + state->numberOfBlock - state->numberOfActBlock) % (state->numberOfBlock);
    sourceBlock = state->flashBlocks[blockIndex];

    /* Backup index of the oldest ACTIVE block */
    g_sourceBlockIndexInternal = blockIndex;

    /* Search for item in the cache table for existing in the source block
     *********************************************************************
     */
    nextStartID = 0U; /* Set the start ID for searching */

    /* We only search in the cache table if it's enabled and the function is called at runtime,
     * because the EEE_DRV_InitEeprom call the function when the cache table hasn't fetched */
    if ((state->cacheEnable == true) && (syncErase == false))
    {
        /* Get number of items in the cache table */
        nextStartID = (uint16_t)((state->cTable->size) / CTABLE_ITEM_SIZE);

        /* Check for each item in the cache table */
        for (i = 0U; i < nextStartID; i++)
        {
            /* Get record address */
            tempAddrInCache = (uint32_t)(state->cTable->startAddress) + (i * CTABLE_ITEM_SIZE);
            recordAddr = REG_READ32(tempAddrInCache);

            /* If the record is in the source block */
            if ((recordAddr > sourceBlock->blockStartAddr) && (recordAddr <= (sourceBlock->blockStartAddr + sourceBlock->blockSize)))
            {
                /* Read the record */
                returnCode = EEE_DRV_ReadRecordHead(recordAddr,
                                                    &recordHead);

                returnCode = EEE_DRV_CopyDataRecord(destBlock,
                                                    true,
                                                    recordHead.dataID,
                                                    recordHead.dataSize,
                                                    recordAddr);
                if (returnCode != STATUS_SUCCESS)
                {
                    break;  /* Exit if failed to copy data */
                }
            }
        }
    }

    /* Continue copy records aren't in the cache table
     *************************************************
     */
    returnCode = EEE_DRV_CopyRecordOfActiveBlock(sourceBlock, nextStartID);

    if (returnCode == STATUS_SUCCESS)
    {
        /* Make the UPDATE block to COPY DONE */
        i = COPY_DONE;
        returnCode = EEE_DRV_ProgramBlockIndicator(destBlock->blockStartAddr + (3U * (state->eccSize)),
                                                   (uint32_t)(&i));
        if (returnCode == STATUS_SUCCESS)
        {
            /* Set activeBlockIndex to the COPY_DONE block */
            state->activeBlockIndex = (state->activeBlockIndex + 1U) % (state->numberOfBlock);

            if (syncErase == false)     /* Only erase if at runtime, not at initialization time */
            {
                /* Backup erase cycle and the block is erased to used in the main function */
                g_erasingCycleInternal = REG_READ32((sourceBlock->blockStartAddr) + (state->eccSize)) + 1UL;

                /* Erase the source block, blockIndex is containing index of the oldest ACTIVE block */
                returnCode = EEE_DRV_EraseEEBlock(g_sourceBlockIndexInternal,
                                                  syncErase);
            }
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ProcessImmediateRequest
 * Description   : This function will implement operation immediate and Suspend
 * for both reading and writing operation.
 *
 *END**************************************************************************/
status_t EEE_DRV_ProcessImmediateRequest(const eee_block_config_t * blockConf,
                                         eee_request_type_t reqType,
                                         flash_state_t * suspendState)
{
    const eee_block_config_t * internalBlockConf;
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    flash_state_t opResult;

    /* If there is an on-going high voltage operation */
    if (FLASH_DRV_CheckEraseStatus(&opResult) == STATUS_FLASH_INPROGRESS)
    {
        /* Get internal block configuration */
        internalBlockConf = state->flashBlocks[g_sourceBlockIndexInternal];

        /* Determine partition number of current block */
        if (blockConf->partSelect == internalBlockConf->partSelect)  /* On the same partition */
        {
            /* Suspend HVOP if the target to immediate read request is in same partition with the one is being erased */
            if (reqType != EEE_IMMEDIATE_NONE)
            {
                /* Suspend for both reading and writing operation */
                returnCode = FLASH_DRV_Suspend(suspendState);
            }
            else
            {
                /* Keep the operation continue */
                returnCode = STATUS_EEE_HVOP_INPROGRESS;
            }
        }
        else  /* On different partitions */
        {
            if ((reqType == EEE_IMMEDIATE_WRITE) || (reqType == EEE_IMMEDIATE_DELETE))
            {
                /* Only suspend for writing operation */
                returnCode = FLASH_DRV_Suspend(suspendState);
            }
            else if ((reqType == EEE_IMMEDIATE_NONE) && (g_readStatusFlag == false))
            {
                /* Return an error */
                returnCode = STATUS_EEE_HVOP_INPROGRESS;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SearchInAllBlocks
 * Description   : This function will is used to search the required data ID
 * in the specific block. It will go through the entire block for the latest
 * data record.
 *
 *END**************************************************************************/
status_t EEE_DRV_SearchInAllBlocks(uint16_t dataID,
                                   eee_request_type_t reqType,
                                   uint32_t * recordAddr,
                                   flash_state_t * suspendState)
{
    eee_block_config_t * pBlockConf;
    const eee_state_t * state = g_eeeState;
    uint32_t startIndex;
    uint32_t endIndex;
    status_t returnCode = STATUS_EEE_ERROR_DATA_NOT_FOUND;
    status_t returnReq = STATUS_SUCCESS;
    uint16_t nextStartID = 0U;

    /* Get index of the start block for searching */
    startIndex = state->activeBlockIndex;

    /* Calculate index of the end block */
    endIndex = (state->activeBlockIndex + state->numberOfBlock - state->numberOfActBlock) % state->numberOfBlock;

    /* Search from the newest to oldest ACTIVE block
     ***********************************************/
    while ((returnCode == STATUS_EEE_ERROR_DATA_NOT_FOUND) && (startIndex != endIndex))
    {
        /* Suspend the high voltage operation if the request is immediate,
         * but doesn't call the function if there is already a suspended operation */
        pBlockConf = state->flashBlocks[startIndex];
        if (*suspendState == C55_OK)
        {
            returnReq = EEE_DRV_ProcessImmediateRequest(pBlockConf,
                                                        reqType,
                                                        suspendState);
        }

        if (returnReq != STATUS_EEE_HVOP_INPROGRESS)
        {
            /* Search the ID in the block */
            *recordAddr = (uint32_t)EEE_ERASED_WORD;

            if (state->sizeField == 0U)
            {
                EEE_DRV_SearchRecordFromBottom(pBlockConf,
                                               dataID,
                                               recordAddr);
            }
            else
            {
                nextStartID = EEE_DRV_SearchRecordFromTop(pBlockConf,
                                                          (uint32_t)recordAddr,
                                                          C55_WORD_SIZE,
                                                          dataID);
                DEV_ASSERT(nextStartID != 0U);
                (void) nextStartID;
            }

            /* Found the record */
            if (*recordAddr != EEE_ERASED_WORD)
            {
                /* The record is valid */
                if (*recordAddr != (uint32_t)EEE_DELETED_RECORD_IND)
                {
                    returnCode = STATUS_SUCCESS;
                }

                break;  /* Stop searching */
            }
            else
            {
                /* Check for the next block */
                startIndex = (startIndex + state->numberOfBlock - 1U) % state->numberOfBlock;
            }
        }
        else
        {
            /* Stop searching */
            returnCode = STATUS_EEE_HVOP_INPROGRESS;
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SearchInTable
 * Description   : This function will search the data ID in the cache table.
 *
 *END**************************************************************************/
status_t EEE_DRV_SearchInTable(const eee_cache_table_t * pCacheTable,
                               uint16_t dataID,
                               uint32_t* expDataAddress)
{
    DEV_ASSERT(pCacheTable != NULL);
    status_t returnCode = STATUS_SUCCESS;
    uint32_t counter;         /* Loop counter */
    const uint8_t * temp;

    /* check if the data ID is in the cache table ID range */
    counter = pCacheTable->size / CTABLE_ITEM_SIZE;
    if (dataID >= counter)
    {
        /* Data ID is out of range */
        returnCode = STATUS_EEE_ERROR_NOT_IN_CACHE;
    }
    else
    {
        /* Fetch the required data record address */
        temp = (uint8_t *) pCacheTable->startAddress;
        *expDataAddress = REG_READ32((uint32_t)temp + (uint32_t)(dataID * (uint16_t)CTABLE_ITEM_SIZE));
        /* The record was deleted or not found */
        if (*expDataAddress >= (uint32_t)EEE_DELETED_RECORD_IND)
        {
            /* No data found */
            returnCode = STATUS_EEE_ERROR_DATA_NOT_FOUND;
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetEraseStatus
 * Description   : This function will get the status of the erase operation.
 *
 *END**************************************************************************/
static status_t EEE_DRV_GetEraseStatus(flash_state_t * opResult)
{
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_FLASH_INPROGRESS; /* The return code */

    /* Check status and end operation */
    while (returnCode == STATUS_FLASH_INPROGRESS)
    {
        if (state->callback != NULL)
        {
            state->callback(state->callbackParam);
        }

        returnCode = FLASH_DRV_CheckEraseStatus(opResult);
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_EraseEEBlock
 * Description   : This function will is used to launch command to erase the
 * specific blocks and it will re-erase for the number of maximum erase if failed
 * in case of not invoking on swapping.
 *
 *END**************************************************************************/
status_t EEE_DRV_EraseEEBlock(uint32_t blockIndex,
                              bool syncErase)
{
    const eee_block_config_t * blockConf;       /* Block configuration */
    const eee_state_t * state = g_eeeState;
    flash_block_select_t blockSelect = {0U, 0U, 0U, 0U, 0U};
    flash_state_t opResult;
    status_t returnCode = STATUS_SUCCESS; /* The return code */
    uint32_t dest;                        /* Blank checking destination address */
    uint32_t size;                        /* Blank checking size */
    uint32_t ersNum;                      /* Number of re-erase */

    /* Get block configuration */
    blockConf = state->flashBlocks[blockIndex];
    DEV_ASSERT(blockConf->blockSpace <= C55_BLOCK_HIGH);

    /* Check the address space */
    switch (blockConf->blockSpace)
    {
        case C55_BLOCK_LOW:
            blockSelect.lowBlockSelect = blockConf->enabledBlock;
            break;
        case C55_BLOCK_MID:
            blockSelect.midBlockSelect = blockConf->enabledBlock;
            break;
        case C55_BLOCK_HIGH:
            blockSelect.highBlockSelect = blockConf->enabledBlock;
            break;
        default:
            /* Nothing to do */
            break;
    }

    /* Number of erasing */
    ersNum = (uint32_t)(state->maxReEraseEeeBlock + 1U);

    do
    {
        returnCode = FLASH_DRV_Erase(ERS_OPT_MAIN_SPACE, &blockSelect);
        if (returnCode == STATUS_SUCCESS)
        {
            if (syncErase == true)
            {
                returnCode = EEE_DRV_GetEraseStatus(&opResult);

                if ((returnCode == STATUS_SUCCESS) && (opResult == C55_OK))
                {
                    /* Do blank check for erased block */
                    dest = blockConf->blockStartAddr;
                    size = blockConf->blockSize;
                    returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                                   dest,
                                                   size,
                                                   0U);
                }

                if (returnCode != STATUS_SUCCESS)
                {
                    ersNum--;
                }
            }
            else
            {
                /* Update erase status */
                g_eraseStatusFlag = EEE_ERASE_IN_PROGRESS;
            }
        }
        else
        {
            ersNum--;
        }
    }
    while ((returnCode != STATUS_SUCCESS) && (ersNum > 0U));    /* Re-erase if it failed and re-erase number still > 0 */

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ProgramBlockIndicator
 * Description   : This function will program into block indicator region.
 *
 *END**************************************************************************/
status_t EEE_DRV_ProgramBlockIndicator(uint32_t dest,
                                       uint32_t source)
{
    status_t returnCode = STATUS_SUCCESS;
    const eee_state_t * state = g_eeeState;
    uint32_t i = state->maxReProgram + 1U;  /* Programming times number, +1 for the default */
    uint32_t blkInd = 0UL;

    while (i > 0U)
    {
        returnCode = EEE_DRV_SyncProgram(dest,
                                         C55_WORD_SIZE,
                                         source);
        if (returnCode == STATUS_SUCCESS)
        {
            returnCode = EEE_DRV_FlashRead(EEE_READ,
                                           dest,
                                           C55_WORD_SIZE,
                                           (uint32_t)(&blkInd));

            if (((g_eccErrorStatusFlag == true) || (blkInd != (uint32_t)EEE_ERASED_WORD)) && (returnCode == STATUS_SUCCESS))   /* Program indicator successful */
            {
                break;
            }
        }

        if (returnCode != STATUS_SUCCESS)
        {
            returnCode = STATUS_EEE_ERROR_PROGRAM_INDICATOR;
        }

        /* Decrease re-erase number */
        i--;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_MakeBlockToDead
 * Description   : This function will make an EEE block to DEAD block and move
 * the DEAD block to the end of the block array.
 *
 *END**************************************************************************/
status_t EEE_DRV_MakeBlockToDead(const eee_block_config_t * pBlockConf)
{
    DEV_ASSERT(pBlockConf != NULL);
    const eee_block_config_t * pBlock;
    status_t returnCode = STATUS_SUCCESS;
    eee_state_t * state = g_eeeState;
    uint32_t value = DEAD_INDICATOR_DEAD;
    uint32_t dest = pBlockConf->blockStartAddr + (2U * (state->eccSize));
    uint32_t addr[state->numberOfBlock];
    uint32_t i;

    /* Program dead block indicator until it is not 0xFF */
    returnCode = EEE_DRV_ProgramBlockIndicator(dest,
                                               (uint32_t)&value);
    for (i = 0U; i < state->numberOfBlock; i++)
    {
        addr[i] = 0x0U;
    }

    if (returnCode == STATUS_SUCCESS)
    {
        /* Update DEAD block number */
        state->numberOfDeadBlock++;

        /* Get index of dead block */
        for (i = 0U; i < state->numberOfBlock; i++)
        {
            pBlock = state->flashBlocks[i];
            if (pBlock == pBlockConf)
            {
                value = i;
                break;
            }
        }

        /* Backup address of Eeprom blocks */
        for (i = 0U; i < state->numberOfBlock; i++)
        {
            addr[i] = (uint32_t)state->flashBlocks[i];
        }

        /* Shift blocks that at higher index for the DEAD block */
        for (i = value; i < (state->numberOfBlock - 1U); i++)
        {
            state->flashBlocks[i] = (eee_block_config_t*)addr[i + 1U];
        }

        /* Move the DEAD block to the end of the block array */
        state->flashBlocks[i] = (eee_block_config_t*)addr[value];

        /* Decrease non DEAD block number */
        state->numberOfBlock--;

        /* Update index of ACTIVE block */
        if (state->activeBlockIndex > value)
        {
            state->activeBlockIndex--;
        }
    }

    /* Check for number of blocks is enough of not for round-robin sequence */
    if (state->numberOfBlock < (state->numberOfActBlock + 1U))
    {
        returnCode = STATUS_EEE_ERROR_NO_ENOUGH_BLOCK;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ReadRecordAtAddr
 * Description   : This function will get the content of record.
 *
 *END**************************************************************************/
void EEE_DRV_ReadRecordAtAddr(uint32_t recordAddr,
                              uint16_t dataSize,
                              uint32_t bufferAddr)
{
    eee_data_record_head_t record;    /* Local data record head structure */
    const eee_state_t * state = g_eeeState;
    uint16_t size;                    /* Reading size */
    uint32_t i;
    uint32_t value;
    status_t returnCode = STATUS_SUCCESS;
    /* Read record head to get dataSize */
    returnCode = EEE_DRV_ReadRecordHead(recordAddr,
                                        &record);
    (void)returnCode;
    size = record.dataSize;

    if (size > dataSize)
    {
        size = dataSize;
    }

    /* Read the data portion allocated after record status field */
    for (i = 0UL; (i < state->smallDataSize) && (size > 0U); i++)
    {
        value = (uint8_t)(*(volatile uint8_t*)(recordAddr + i + (uint32_t)C55_DWORD_SIZE));
        *(volatile uint8_t*)bufferAddr = (uint8_t)value;
        bufferAddr++;
        size--;
    }

    /* Read the data portion allocated after size/id field */
    for (i = 0UL; (i < (state->dataHeadSize - state->smallDataSize)) && (size > 0U); i ++)
    {
        /* Size of ID = 2 */
        value = (uint8_t)(*(volatile uint8_t*)(recordAddr + i + (state->idOffset) + (uint32_t)ID_FIELD_SIZE + (state->sizeField)));
        *(volatile uint8_t*)bufferAddr = (uint8_t)value;
        bufferAddr++;
        size--;
    }

    /* Read the rest of data */
    for (i = 0UL; i < size; i++)
    {
        value = (uint8_t)(*(volatile uint8_t*)(recordAddr + (state->minRecordSize) + i));
        *(volatile uint8_t*)bufferAddr = (uint8_t)value;
        bufferAddr++;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetWriteRecordOption
 * Description   : This function will get the status of writing in the EEE block.
 *
 *END**************************************************************************/
eee_write_status_t EEE_DRV_GetWriteRecordOption(uint32_t recordLength)
{
    const eee_block_config_t * pBlockConf;
    const eee_state_t * state = g_eeeState;
    eee_block_status_t retStatus;

    eee_write_status_t returnCode;
    uint32_t temp;
    uint32_t i;

    /* Get the current ACTIVE block */
    pBlockConf = state->flashBlocks[state->activeBlockIndex];

    /* Check status of the current ACTIVE block */
    retStatus = EEE_DRV_ReadBlockStatus(pBlockConf);
    if (retStatus == EEE_BLOCK_COPY_DONE)
    {
        if (recordLength <= ((pBlockConf->blockStartAddr + pBlockConf->blockSize) - pBlockConf->blankSpace))
        {
            returnCode = EEE_WRITE_ON_COPY_DONE;
        }
        else
        {
            returnCode = EEE_WRITE_NO_ENOUGH_SPACE;
        }
    }
    else
    {
        if (recordLength <= ((pBlockConf->blockStartAddr + pBlockConf->blockSize) - pBlockConf->blankSpace))
        {
            returnCode = EEE_WRITE_NORMAL;
        }
        else
        {
            /* Use temp to store the number of ACTIVE blocks */
            temp = 0U;

            /* Get number of ACTIVE block */
            for (i = 0U; i < state->numberOfBlock; i++)
            {
                pBlockConf = state->flashBlocks[i];
                retStatus = EEE_DRV_ReadBlockStatus(pBlockConf);
                if (retStatus == EEE_BLOCK_ACT)
                {
                    temp++;
                }
            }

            /* Get write option */
            if (temp == state->numberOfActBlock)
            {
                returnCode = EEE_WRITE_SWAP;
            }
            else    /* temp < state->numberOfActBlock */
            {
                returnCode = EEE_WRITE_ON_NEW_ACTIVE;
            }
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetEraseEEBlockStatus
 * Description   : This function will get the erasing status of the EEE block
 *
 *END**************************************************************************/
status_t EEE_DRV_GetEraseEEBlockStatus(void)
{
    const eee_block_config_t * blockConf;
    status_t returnCode = STATUS_SUCCESS;
    eee_state_t * state = g_eeeState;
    uint32_t reEraseNum = g_eeeState->maxReEraseEeeBlock;
    bool re_erase_flag = false;
    flash_state_t opResult;

    /* Check flash status
     ********************/
    returnCode = FLASH_DRV_CheckEraseStatus(&opResult);

    if (returnCode == STATUS_SUCCESS)
    {
        /* Blank check */
        blockConf = state->flashBlocks[g_sourceBlockIndexInternal];
        returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                       blockConf->blockStartAddr,
                                       blockConf->blockSize,
                                       0U);
        if (returnCode == STATUS_SUCCESS)
        {
            /* Doesn't re-erase */
            re_erase_flag = false;
        }
        else
        {
             /* Re-erase */
            re_erase_flag = true;
        }
    }
    else if (returnCode == STATUS_ERROR)
    {
        /* Re-erase */
        re_erase_flag = true;
    }
    else    /* EE_INFO_HVOP_INPROGRESS */
    {
        returnCode = STATUS_EEE_HVOP_INPROGRESS;
        /* Doesn't re-erase */
        re_erase_flag = false;
    }

    /* Check for re-erasing
     **********************/
    if ((re_erase_flag == true) && (reEraseNum > 0UL))
    {
        /* Decrease the number of easre for EEE block */
        state->maxReEraseEeeBlock--;

        /* Design asynchronous need to re-erase the block */
        returnCode = EEE_DRV_EraseEEBlock(g_sourceBlockIndexInternal,
                                          false);
        if (returnCode == STATUS_SUCCESS)
        {
            returnCode = STATUS_EEE_HVOP_INPROGRESS;
        }
    }

    /* Reset re-erase number if the operation finish */
    if (returnCode != STATUS_EEE_HVOP_INPROGRESS)
    {
        state->maxReEraseEeeBlock = g_numOfErase;
    }

    (void)reEraseNum;
    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_EraseDeadBlock
 * Description   : This function will erase the DEAD block.
 *
 *END**************************************************************************/
static status_t EEE_DRV_EraseDeadBlock(eee_block_config_t * pBlockConf,
                                       uint32_t index,
                                       bool * checkFlag)
{
    status_t returnCode = STATUS_SUCCESS;

    *checkFlag = false;
    /* Synchronize erase the block */
    returnCode = EEE_DRV_EraseEEBlock(index, true);

    if (returnCode == STATUS_SUCCESS)
    {
        /* Mark the block as blank */
        pBlockConf->blankSpace = (uint32_t)EEE_BLOCK_ERASED;
    }
    /* Make the block to DEAD if there is an error */
    else if (returnCode == STATUS_ERROR)
    {
        /* Make the block to DEAD */
        returnCode = EEE_DRV_MakeBlockToDead(pBlockConf);

        if ((returnCode == STATUS_EEE_ERROR_PROGRAM_INDICATOR) ||
            (returnCode == STATUS_EEE_ERROR_NO_ENOUGH_BLOCK))
        {
            /* Exit if failed to make dead or
             * there isn't enough number of blocks for round robin sequence */
            *checkFlag = true;
        }
    }
    else    /* For returnCode = EE_INFO_HVOP_INPROGRESS  */
    {
        /* Break the loop */
        *checkFlag = true;
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ValidateDeadBlocks
 * Description   : This function will validate the DEAD block.
 *
 *END**************************************************************************/
status_t EEE_DRV_ValidateDeadBlocks(void)
{
    eee_block_config_t * pBlockConf;
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    uint32_t readBuffer = 0UL;
    uint32_t temp;
    uint32_t i = 0U;
    bool checkFlag = false;

    /* Get total number of blocks */
    temp = state->numberOfBlock;

    /* Validate all blocks */
    for (i = 0UL; i < temp; i++)
    {
        pBlockConf = state->flashBlocks[i];

        /* Only check the next block if the previous loop successful */
        if (returnCode == STATUS_SUCCESS)
        {
            returnCode = EEE_DRV_FlashRead(EEE_READ,
                                           pBlockConf->blockStartAddr + (2U * (state->eccSize)),
                                           C55_WORD_SIZE,
                                           (uint32_t)(&readBuffer));
        }

        /* Erase Dead blocks */
        if ((readBuffer != EEE_ERASED_WORD) || (g_eccErrorStatusFlag != false))
        {
            returnCode = EEE_DRV_EraseDeadBlock(pBlockConf,
                                                i,
                                                &checkFlag);
            if (checkFlag == true)
            {
                break;
            }
        }
        else
        {
            /* Reset returnCode to continue the next loop */
            returnCode = STATUS_SUCCESS;
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_ValidateCopyDoneBlock
 * Description   : This function will count number of valid records in the block.
 * If it return 0, this mean the block is an invalid COPY_DONE block.
 *
 *END**************************************************************************/
static uint32_t EEE_DRV_ValidateCopyDoneBlock(eee_block_config_t * blockConf)
{
    DEV_ASSERT(blockConf != NULL);
    const eee_state_t * state = g_eeeState;
    status_t returnCode;
    uint32_t value = 0UL;   /* Store number of valid records found in the block */
    bool errorStatusFlagTemp;

    /* Get erase cycle value */
    returnCode = EEE_DRV_FlashRead(EEE_READ,
                                   blockConf->blockStartAddr + (state->eccSize),
                                   C55_WORD_SIZE,
                                   (uint32_t)&value);

    errorStatusFlagTemp = g_eccErrorStatusFlag;
    /* Check erase cycle value for there isn't ECC in the field or its value must be smaller than endurance */
    if ((errorStatusFlagTemp == true) || (value > (uint32_t)BLOCK_MAX_ENDURANCE))
    {
        /* The block is Invalid */
        value = 0U;
    }
    else
    {
        /* Count number of valid records by passing an invalid buffer address */
        value = EEE_DRV_SearchRecordFromTop(blockConf,
                                            EEE_ERASED_WORD,
                                            4U,
                                            0U);
        if (value != 0U)
        {
            /* Continue checking for data non-continuous block by blank check the rest of block from blank space */
            returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                           blockConf->blankSpace,
                                           blockConf->blockStartAddr + blockConf->blockSize - blockConf->blankSpace,
                                           0UL);
            if (returnCode != STATUS_SUCCESS)
            {
                value = 0U;
            }
        }
    }

    return value;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetCopyDoneBlockFirstTime
 * Description   : This function will copy the EEE block with status as DONE state
 * at the first time.
 *
 *END**************************************************************************/
static status_t EEE_DRV_GetCopyDoneBlockFirstTime(eee_last_job_status_t * lastJob)
{
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    eee_block_config_t * pBlockConf;
    uint32_t tempValue = 0U;

    /* Blank check data region of the first block */
    returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                   (state->flashBlocks[0U]->blockStartAddr) + (4UL * (state->eccSize)),
                                   (state->flashBlocks[0U]->blockSize) - (4UL * (state->eccSize)),
                                   0U);

    if (returnCode == STATUS_SUCCESS)
    {
        pBlockConf = state->flashBlocks[1U];
        tempValue = EEE_DRV_ValidateCopyDoneBlock(pBlockConf);
        /* The block is invalid */
        if (tempValue == 0U)
        {
            *lastJob = EEE_LAST_JOB_FIRST_TIME;
            g_sourceBlockIndexInternal = 0xFFFFFFFFU;
        }
        else
        {
            /* The block is valid, so restore it status */
            state->flashBlocks[1U]->blankSpace = (uint32_t)EEE_BLOCK_COPY_DONE;
            *lastJob = EEE_LAST_JOB_COPY_DONE;
            /* Set the last ACTIVE block index */
            lastestActiveIndex = 1U;
        }
    }

    return returnCode;
}
/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetLastJobSpecialEeprom
 * Description   : This function will get the last of job in case the EEE block
 * only has 2 blocks.
 *
 *END**************************************************************************/
static eee_last_job_status_t EEE_DRV_GetLastJobSpecialEeprom(uint32_t numOfCopyDone,
                                                             eee_block_status_t firstBlockStatus,
                                                             eee_block_status_t secondBlockStatus)
{
    eee_block_config_t * pBlockConf;
    const eee_state_t * state = g_eeeState;
    eee_last_job_status_t lastJob = EEE_LAST_JOB_NONE;
    status_t returnCode = STATUS_SUCCESS;
    uint32_t temp = 0U;
    uint32_t tempCompare = 0U;
    uint32_t i = 0UL;

    if (numOfCopyDone == 2U)
    {
        pBlockConf = state->flashBlocks[0U];
        temp = EEE_DRV_ValidateCopyDoneBlock(pBlockConf);
        pBlockConf = state->flashBlocks[1U];

        /* The 2nd block is invalid */
        if (temp < EEE_DRV_ValidateCopyDoneBlock(pBlockConf))
        {
            state->flashBlocks[0U]->blankSpace = (uint32_t)EEE_BLOCK_INVALID;
            lastestActiveIndex = 1U;
        }
        else
        {
            /* The block is valid, so restore it status */
            state->flashBlocks[0U]->blankSpace = (uint32_t)EEE_BLOCK_COPY_DONE;
            /* The second block is invalid */
            state->flashBlocks[1U]->blankSpace = (uint32_t)EEE_BLOCK_INVALID;
            lastestActiveIndex = 0U;
        }

        lastJob = EEE_LAST_JOB_COPY_DONE;
    }
    else if ((firstBlockStatus == EEE_BLOCK_COPY_DONE) && (secondBlockStatus == EEE_BLOCK_ERASED))
    {
        pBlockConf = state->flashBlocks[0U];
        temp = EEE_DRV_ValidateCopyDoneBlock(pBlockConf);
        /* The block is invalid */
        if (temp == 0U)
        {
            lastJob = EEE_LAST_JOB_FIRST_TIME;
            g_sourceBlockIndexInternal = 1U;
        }
        else
        {
            /* The block is valid, so restore it status */
            state->flashBlocks[0U]->blankSpace = (uint32_t)EEE_BLOCK_COPY_DONE;
            lastJob = EEE_LAST_JOB_COPY_DONE;
            /* Set the last ACTIVE block index */
            lastestActiveIndex = 0U;
        }
    }
    else if ((firstBlockStatus == EEE_BLOCK_ACT) && (secondBlockStatus == EEE_BLOCK_COPY_DONE))
    {
        /* Read erase cycle of the first block */
        returnCode = EEE_DRV_FlashRead(EEE_READ,
                                       (state->flashBlocks[0U]->blockStartAddr) + (state->eccSize),
                                       C55_WORD_SIZE,
                                       (uint32_t)(&i));
        tempCompare = i;

        if (tempCompare == 1U)
        {
            returnCode = EEE_DRV_GetCopyDoneBlockFirstTime(&lastJob);
        }
    }
    else
    {
        /* Nothing to do */
    }

    (void)returnCode;

    return lastJob;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_CheckStatusLastBlock
 * Description   : This function will get the status of last job of the EEE block.
 *
 *END**************************************************************************/
static status_t EEE_DRV_CheckStatusLastBlock(eee_block_config_t * pBlockConf,
                                             eee_block_status_t retStatus,
                                             uint32_t index,
                                             uint32_t numOfBlock,
                                             eee_block_status_t previousBlockStatus)
{
    DEV_ASSERT(pBlockConf != NULL);
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    uint32_t compAlterBlockEC = 0U;

    /* ACTIVE block */
    if (retStatus == EEE_BLOCK_ACT)
    {
        /* Increase ACTIVE block number */
        activeNum++;

        /* If the last ACTIVE block at the end of the block array */
        if ((index == (numOfBlock - 1U)) && (lastestActiveIndex == 0xFFFFFFFFU))
        {
            lastestActiveIndex = index;
        }

        pBlockConf->blankSpace = (uint32_t)EEE_BLOCK_ACT;
    }
    else
    {
        /* The last ACTIVE block is follow by a non-ACTIVE block */
        if ((activeNum > 0U) && (lastestActiveIndex == 0xFFFFFFFFU))
        {
            lastestActiveIndex = index - 1U;
        }

        switch(retStatus)
        {
            case EEE_BLOCK_COPY_DONE:
                copyDoneNum++;

                if (copyDoneIndex == 0xFFFFFFFFU)
                {
                    copyDoneIndex = index;
                }

                if (previousBlockStatus == EEE_BLOCK_ACT)   /* state->numberOfActBlock > 1 */
                {
                    copyDoneIndex = index;
                }

                if ((previousBlockStatus == EEE_BLOCK_COPY_DONE) && (state->numberOfActBlock == 1U))
                {
                    copyDoneIndex = index;
                }

                pBlockConf->blankSpace = (uint32_t)EEE_BLOCK_COPY_DONE;
            break;
            case EEE_BLOCK_UPDATE:
                updatedNum++;
                pBlockConf->blankSpace = (uint32_t)EEE_BLOCK_UPDATE;
            break;
            case EEE_BLOCK_ERASED:
                erasedNum++;
                pBlockConf->blankSpace = (uint32_t)EEE_BLOCK_ERASED;

                if (g_sourceBlockIndexInternal == 0xFFFFFFFFU)
                {
                    /* Found the brown-out block in the first time initialization */
                    g_sourceBlockIndexInternal = index;
                }
            break;
            case EEE_BLOCK_INVALID:
                pBlockConf->blankSpace = (uint32_t)EEE_BLOCK_INVALID;
            break;
            case EEE_BLOCK_ALT:
            case EEE_BLOCK_DEAD:
                alterNum++;

                /* Only read Erase cycle value if all Alter blocks have erase cycle value = 1 */
                if (alterBlockEC == 1UL)
                {
                    /* Read erase cycle, use 'lastJob' as temporary return value */
                    returnCode = EEE_DRV_FlashRead(EEE_READ,
                                                pBlockConf->blockStartAddr + (state->eccSize),
                                                C55_WORD_SIZE,
                                                (uint32_t)(&alterBlockEC));
                    compAlterBlockEC = alterBlockEC;

                    if ((compAlterBlockEC != 1UL) && (returnCode == STATUS_SUCCESS))
                    {
                        /* Stop read Erase cycle of Alter blocks */
                        alterBlockEC = 0xFFFFFFFFU;
                    }
                }

                pBlockConf->blankSpace = (uint32_t)EEE_BLOCK_ALT;
            break;
            default:
                /* Nothing to do */
                break;
        }
    }

    (void)copyDoneNum;
    (void)erasedNum;
    (void)updatedNum;
    (void)alterNum;

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetLastActiveBlockFirstTime
 * Description   : This function will get
 *
 *END**************************************************************************/
static status_t EEE_DRV_GetLastActiveBlockFirstTime(eee_last_job_status_t * lastJob,
                                                    eee_block_status_t secondBlockStatus)
{
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;

    /* Blank check data region of the first block */
    returnCode = EEE_DRV_FlashRead(EEE_BLANK_CHECK,
                                   (state->flashBlocks[0U]->blockStartAddr) + (4U * (state->eccSize)),
                                   (state->flashBlocks[0U]->blockSize) - (4UL * (state->eccSize)),
                                   0U);

    /* The first block at the first time initialization state */
    if (returnCode == STATUS_SUCCESS)
    {
        if ((alterNum == (state->numberOfBlock - 1U)) && (alterBlockEC == 1U))
        {
            *lastJob = EEE_LAST_JOB_NORMAL;
        }
        else if ((state->numberOfBlock == 2U) || (erasedNum > 0U) || (secondBlockStatus == EEE_BLOCK_ALT))
        {
            *lastJob = EEE_LAST_JOB_FIRST_TIME;
        }
        else
        {
            /* Continue determine last job */
        }
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetLastActiveBlockStatus
 * Description   : This function will get the last job of active blocks.
 *
 *END**************************************************************************/
static status_t EEE_DRV_GetLastActiveBlockStatus(eee_last_job_status_t * lastJob,
                                                 eee_block_status_t firstBlockStatus,
                                                 eee_block_status_t secondBlockStatus,
                                                 uint32_t index)
{
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    eee_last_job_status_t lastJobStat = *lastJob;

    if (lastJobStat == EEE_LAST_JOB_NONE)
    {
        if (((activeNum == 0U) && (copyDoneNum == 0U)) || \
            (((firstBlockStatus == EEE_BLOCK_COPY_DONE) || (firstBlockStatus == EEE_BLOCK_ACT)) && (erasedNum == (state->numberOfBlock - 1U))))
        {
            lastJobStat = EEE_LAST_JOB_FIRST_TIME;
        }
        else
        {
            /* Check the first block */
            if (firstBlockStatus == EEE_BLOCK_ACT)
            {
                /* Read erase cycle of the first block */
                returnCode = EEE_DRV_FlashRead(EEE_READ,
                                               (state->flashBlocks[0U]->blockStartAddr) + (state->eccSize),
                                               C55_WORD_SIZE,
                                               (uint32_t)(&index));

                if (index == 1UL)
                {
                    returnCode = EEE_DRV_GetLastActiveBlockFirstTime(&lastJobStat,
                                                                     secondBlockStatus);
                }
            }
        }
    }

    /* Check for other cases - isn't the first time initialization
     *************************************************************/
    if (lastJobStat == EEE_LAST_JOB_NONE)
    {
        if ((activeNum == state->numberOfActBlock) && (updatedNum != 0U))
        {
            lastJobStat = EEE_LAST_JOB_UPDATE;
        }
        else if (copyDoneNum >= 1U)
        {
            lastJobStat = EEE_LAST_JOB_COPY_DONE;

            /* Set the last ACTIVE block index */
            lastestActiveIndex = copyDoneIndex;
        }
        else
        {
            lastJobStat = EEE_LAST_JOB_NORMAL;
        }
    }

    *lastJob = lastJobStat;

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_GetLastJobStatus
 * Description   : This function will get the last job of all EEE blocks.
 *
 *END**************************************************************************/
eee_last_job_status_t EEE_DRV_GetLastJobStatus(void)
{
    eee_block_config_t * pBlockConf;
    eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    eee_block_status_t retStatus;
    uint32_t temp;
    uint32_t temNumOfBlock;
    uint32_t i;
    copyDoneIndex = 0xFFFFFFFFU;
    lastestActiveIndex = 0xFFFFFFFFU;
    eee_block_status_t firstBlockStatus = EEE_BLOCK_ERASED;
    eee_block_status_t secondBlockStatus = EEE_BLOCK_ERASED;
    eee_block_status_t previousBlockStatus;     /* Used in determining index of the COPY_DONE block */
    alterBlockEC = 1UL;       /* Used to determine that all Alter blocks have erase cycle value == 1 ? */
    eee_last_job_status_t lastJob = EEE_LAST_JOB_NONE;
    activeNum = 0U;
    copyDoneNum = 0U;
    erasedNum = 0U;
    updatedNum = 0U;
    alterNum = 0U;

    /* Temporarily store index of the brownout block in the progress of the first time initialization */
    g_sourceBlockIndexInternal = 0xFFFFFFFFU;

    /*
     * Loop through all blocks to:
     * - Read block status and store it into the blank space element of the block temporarily,
     *   it will be updated correctly at the end of the initialization process
     * - Get number of ACTIVE, COPY_DONE, UPDATED, ERASE, ALTERNATIVE
     * - Get index of the COPY_DONE block, index of the current ACTIVE block
     ****************************************************************************************
     */

    /* Use temp as temporary block number */
    temp = state->numberOfBlock;
    /* First, read status of the last block */
    pBlockConf = state->flashBlocks[temp - 1U];

    /* This happen when the block is erased in the DEAD validation */
    if (pBlockConf->blankSpace == 0U)
    {
        previousBlockStatus = EEE_BLOCK_ERASED;
    }
    else
    {
        previousBlockStatus = EEE_DRV_ReadBlockStatus(pBlockConf);
        /* Backup status of the last block into the global variable 'g_erasingCycleInternal' */
        g_erasingCycleInternal = (uint32_t)previousBlockStatus;
    }

    /* Scan all blocks */
    for (i = 0U; i < temp; i++)
    {
        /* Get block status */
        pBlockConf = state->flashBlocks[i];

        /* The last block is already read for status */
        if (i == (temp - 1U))
        {
            retStatus = (eee_block_status_t)g_erasingCycleInternal;
        }
        else if (pBlockConf->blankSpace == 0U)    /* This happen when the block is erased in the DEAD validation */
        {
            retStatus = EEE_BLOCK_ERASED;
        }
        else
        {
            retStatus = EEE_DRV_ReadBlockStatus(pBlockConf);
        }

        /* Backup status of the first and second block, used to checking for first time initialization */
        if (i == 0U)
        {
            firstBlockStatus = retStatus;
        }

        if (i == 1U)
        {
            secondBlockStatus = retStatus;
        }

        temNumOfBlock = temp;

        returnCode = EEE_DRV_CheckStatusLastBlock(pBlockConf, retStatus, i, temNumOfBlock, previousBlockStatus);
        /* Backup status for using in the next round */
        previousBlockStatus = retStatus;
    }

    /* Check for the special cases:
     * if the EE system has 2 blocks in statuses:
     * 1. COPY_DONE - COPY_DONE
     * 2. COPY_DONE - ERASED
     * 3. ACTIVE(1) - COPY_DONE: The ACTIVE block has erase cycle value of 1
     ***********************************************************************/
    if (state->numberOfBlock == 2UL)
    {
        lastJob = EEE_DRV_GetLastJobSpecialEeprom(copyDoneNum,
                                                  firstBlockStatus,
                                                  secondBlockStatus);
    }

    /* Check for the first time initialization
     *****************************************/
    returnCode = EEE_DRV_GetLastActiveBlockStatus(&lastJob,
                                                  firstBlockStatus,
                                                  secondBlockStatus,
                                                  i);
    (void)returnCode;

    /* Set the last ACTIVE block index */
    state->activeBlockIndex = lastestActiveIndex;

    /* Determine the brown-out block in the first time initialization progress */
    if (lastJob == EEE_LAST_JOB_FIRST_TIME)
    {
        if (g_sourceBlockIndexInternal == 0U)
        {
            /* All blocks are blank */
            g_sourceBlockIndexInternal = 0xFFFFFFFFU;
        }
        else if (g_sourceBlockIndexInternal == 0xFFFFFFFFU)
        {
            g_sourceBlockIndexInternal = state->numberOfBlock - 1U;
        }
        else
        {
            g_sourceBlockIndexInternal -= 1U;
        }
    }

    return lastJob;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_RecoverCopyDoneBlock
 * Description   : This function will make the alternate block to active block
 * after the EEE block is copied successfull.
 *
 *END**************************************************************************/
static status_t EEE_DRV_RecoverCopyDoneBlock(void)
{
    const eee_state_t * state = g_eeeState;
    const eee_block_config_t * pBlockConf = state->flashBlocks[0];
    status_t returnCode = STATUS_SUCCESS;
    uint32_t temp = 0U;
    uint32_t source;
    uint32_t eraseCycle = 0U;
    uint32_t oldestActIdx = 0U;
    uint32_t i = 0U;
    bool tempbool = false;

    /* Get index of the oldest ACTIVE block */
    oldestActIdx = (state->activeBlockIndex + state->numberOfBlock - state->numberOfActBlock) % state->numberOfBlock;
    pBlockConf = state->flashBlocks[oldestActIdx];

    /* Note: We use blankSpace field of block to store it's status temporarily, it is determined in the EEE_DRV_GetLastJobStatus function,
       blankSpace of blocks will be updated correctly at the end of the initialization process */
    /* The block isn't BLANK or ALTER */
    if (pBlockConf->blankSpace > (uint32_t)EEE_BLOCK_ALT)
    {
        tempbool = true;        /* Erase the block */
    }
    else if (pBlockConf->blankSpace == (uint32_t)EEE_BLOCK_ALT)    /* The block is ALTER */
    {
        /* Get erase cycle of the oldest ACTIVE block */
        eraseCycle = REG_READ32((state->flashBlocks[oldestActIdx]->blockStartAddr) + (state->eccSize));

        /* Get erased cycle of the block right after the erased blocks */
        i = (oldestActIdx + 1U) % state->numberOfBlock;
        temp = REG_READ32((state->flashBlocks[i]->blockStartAddr) + (state->eccSize));
        /* Calculate erase cycle for the erased block base on the round-robin sequence */
        if (i != 0U)
        {
            temp++;
        }

        if (eraseCycle != temp)
        {
            /* Erase the block */
            tempbool = true;
        }
        else
        {
            /* The block is already an ALTER block with valid erase cycle value */
            eraseCycle = 0xFFFFFFFFU;
        }
    }
    else /* The block is BLANK */
    {
        /* Don't erase the block */
        tempbool = false;
    }

    if (tempbool == true)
    {
        /* Erase the oldest ACTIVE block */
        returnCode = EEE_DRV_EraseEEBlock(oldestActIdx,
                                          true);
        if (returnCode != STATUS_SUCCESS)
        {
            returnCode = EEE_DRV_MakeBlockToDead(pBlockConf);
            if (returnCode == STATUS_SUCCESS)
            {
                /* Re-get index of the oldest ACTIVE block */
                oldestActIdx = (state->activeBlockIndex + state->numberOfBlock - state->numberOfActBlock) % state->numberOfBlock;
                pBlockConf = state->flashBlocks[oldestActIdx];
            }
        }
    }

    /* Make the erased block to ALTER */
    if ((returnCode == STATUS_SUCCESS) && (eraseCycle != 0xFFFFFFFFU))
    {
        /* Get index of the block right after the erased blocks */
        i = (oldestActIdx + 1U) % state->numberOfBlock;

        /* Get erased cycle of that block */
        temp = REG_READ32((state->flashBlocks[i]->blockStartAddr) + (state->eccSize));

        /* Calculate erase cycle for the erased block base on the round-robin sequence */
        if (i != 0U)
        {
            temp++;
        }

        /* Write erase cycle to the erased block */
        returnCode = EEE_DRV_SyncProgram(pBlockConf->blockStartAddr + (state->eccSize),
                                         C55_WORD_SIZE,
                                         (uint32_t)(&temp));
    }

    /* Make the COPY_DONE block to ACTIVE */
    if (returnCode == STATUS_SUCCESS)
    {
        /* Get the COPY_DONE block */
        pBlockConf = state->flashBlocks[state->activeBlockIndex];
        source = ACT_INDICATOR_ACT;

        /* Mark as ACTIVE */
        returnCode = EEE_DRV_ProgramBlockIndicator(pBlockConf->blockStartAddr,
                                                  (uint32_t)(&source));
    }

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_SetBlockToDead
 * Description   : This function will erase the oldest active block after that
 * make the EEE block to dead to remove this block from round robin if the erase
 * operation is failed.
 *
 *END**************************************************************************/
static status_t EEE_DRV_SetBlockToDead(const eee_block_config_t * pBlockConf,
                                       uint32_t * oldestActIdx)
{
    const eee_state_t * state = g_eeeState;
    status_t returnCode = STATUS_SUCCESS;
    uint32_t indexTemp = *oldestActIdx;

    /* Erase the oldest ACTIVE block */
    returnCode = EEE_DRV_EraseEEBlock(indexTemp,
                                      true);
    if (returnCode != STATUS_SUCCESS)
    {
        returnCode = EEE_DRV_MakeBlockToDead(pBlockConf);
        if (returnCode == STATUS_SUCCESS)
        {
            /* Re-get index of the oldest ACTIVE block */
            *oldestActIdx = (state->activeBlockIndex + state->numberOfBlock - state->numberOfActBlock) % state->numberOfBlock;
            indexTemp = *oldestActIdx;
            pBlockConf = state->flashBlocks[indexTemp];
        }
    }
    (void) pBlockConf;

    return returnCode;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : EEE_DRV_RecoverEeprom
 * Description   : This function will recover the EEE block is based on the last
 * job of EEE block.
 *
 *END**************************************************************************/
status_t EEE_DRV_RecoverEeprom(eee_last_job_status_t lastJob)
{
    eee_state_t * state = g_eeeState;
    const eee_block_config_t * pBlockConf = state->flashBlocks[0];
    status_t returnCode = STATUS_SUCCESS;
    uint32_t temp = 0U;
    uint32_t source;
    uint32_t eraseCycle;
    uint32_t oldestActIdx = 0U;
    uint32_t i = 0U;

    if (lastJob == EEE_LAST_JOB_FIRST_TIME)
    {
        /* There is block need to be re-erased */
        if (g_sourceBlockIndexInternal != 0xFFFFFFFFU)
        {
            /* Re-erase the brown-outed block - right before the blank */
            returnCode = EEE_DRV_EraseEEBlock(g_sourceBlockIndexInternal,
                                              true);

            /* Make to DEAD if erasing failed */
            if (returnCode != STATUS_SUCCESS)
            {
                pBlockConf = state->flashBlocks[g_sourceBlockIndexInternal];
                returnCode = EEE_DRV_MakeBlockToDead(pBlockConf);
            }
        }
        else
        {
            g_sourceBlockIndexInternal = 0U;  /* All blocks are blank */
        }

        /* Continue initialize the rest of blocks
           **************************************/
        /* Re-get number of blocks because may there's DEAD block */
        temp = state->numberOfBlock;
        /* The initial erase-cycle value */
        eraseCycle = 1U;

        for (i = g_sourceBlockIndexInternal; i < temp; i++)   /* Start from the brownouted block */
        {
            /* Only make the block to ALTER by writing erase cycle if the previous step passed*/
            if (returnCode == STATUS_SUCCESS)
            {
                pBlockConf = state->flashBlocks[i];
                returnCode = EEE_DRV_SyncProgram(pBlockConf->blockStartAddr + (state->eccSize),
                                                 C55_WORD_SIZE,
                                                 (uint32_t)(&eraseCycle));
            }

            /* Make the first block to ACTIVE */
            if ((returnCode == STATUS_SUCCESS) && (i == 0U))
            {
                source = ACT_INDICATOR_ACT;
                returnCode = EEE_DRV_SyncProgram(pBlockConf->blockStartAddr,
                                                 C55_WORD_SIZE,
                                                 (uint32_t)(&source));
            }
        }

        /* Set the last of ACTIVE block index */
        state->activeBlockIndex = 0U;
    }
    else if (lastJob == EEE_LAST_JOB_UPDATE)
    {
        /* Call swapping function synchronously to finish the brown-out progress */
        returnCode = EEE_DRV_BlockSwapping(true);

        if (returnCode == STATUS_SUCCESS)
        {
            /* Get index of the oldest ACTIVE block */
            oldestActIdx = (state->activeBlockIndex + state->numberOfBlock - state->numberOfActBlock) % state->numberOfBlock;
            pBlockConf = state->flashBlocks[oldestActIdx];

            /* Erase the oldest ACTIVE block */
            returnCode = EEE_DRV_SetBlockToDead(pBlockConf,
                                                &oldestActIdx);
        }

        /* Make the erased block to ALTER */
        if (returnCode == STATUS_SUCCESS)
        {
            /* Get index of the block right after the erased blocks */
            i = (oldestActIdx + 1U) % state->numberOfBlock;

            /* Get erased cycle of that block */
            temp = REG_READ32((state->flashBlocks[i]->blockStartAddr) + (state->eccSize));

            /* Calculate erase cycle for the erased block base on the round-robin sequence */
            if (i != 0U)
            {
                temp++;
            }

            /* Write erase cycle to the erased block */
            returnCode = EEE_DRV_SyncProgram(pBlockConf->blockStartAddr + (state->eccSize),
                                             C55_WORD_SIZE,
                                             (uint32_t)(&temp));
        }

        /* Make the COPY_DONE block to ACTIVE */
        if (returnCode == STATUS_SUCCESS)
        {
            /* Get the COPY_DONE block */
            pBlockConf = state->flashBlocks[state->activeBlockIndex];
            source = ACT_INDICATOR_ACT;

            /* Mark as ACTIVE */
            returnCode = EEE_DRV_ProgramBlockIndicator(pBlockConf->blockStartAddr,
                                                      (uint32_t)(&source));
        }
    }
    else if (lastJob == EEE_LAST_JOB_COPY_DONE)
    {
        returnCode = EEE_DRV_RecoverCopyDoneBlock();
    }
    else /* EEE_LAST_JOB_NORMAL */
    {
         /* Do nothing */
    }

    return returnCode;
}
/*******************************************************************************
 * EOF
 ******************************************************************************/
