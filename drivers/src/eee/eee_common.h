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
 * Violates MISRA 2012 Advisory Rule 2.5, Global macro not referenced.
 * This is required to enable the use of a macro needed by
 * the user code (even if the macro is not used inside the flash driver code)
 * or driver code in the future.
 *
 */

#ifndef EEE_COMMON_H
#define EEE_COMMON_H

/*! @file eee_common.h */


#include "eee_driver.h"

/*!
 * @details This section describes the internal interface of the EEE driver.
 * @{
 */

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Variable to store internal erase cycle */
extern uint32_t g_erasingCycleInternal;

/* Variable to store index of the erasing block while swapping */
extern uint32_t g_sourceBlockIndexInternal;

/* Flag to keep track of ECC Error Status */
extern bool g_eccErrorStatusFlag;

/* Variable to store the number of EEE block is erased */
extern uint32_t g_numOfErase;

/* Variable to store the status of read function */
extern bool g_readStatusFlag;

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* The erased state */
#define EEE_ERASED_WORD            0xFFFFFFFFU

/* The size in byte of ID field */
#define ID_FIELD_SIZE              2U
/* The size in bytes of a cache entry */
#define CTABLE_ITEM_SIZE           4U

#define VLE_IS_ON                  (1)

/* Enable MSR register */
#define ENABLE_MSR                0x00008000U

/* The state of the block indicator */
#define ACT_INDICATOR_ACT         0x0000FFFFU
#define DEAD_INDICATOR_DEAD       0x0000FFFFU
#define COPY_DONE                 0x0000FFFFU

/* The status of record is programmed */
#define EEE_PROGRAMED_RECORD      0xFFFF0000U
/* The status of record is deleted */
#define EEE_DELETED_RECORD        0x00000000U
/*! Used this value to mark a record is deleted in buffer while searching */
#define EEE_DELETED_RECORD_IND    0xFFFFFFFEU
/*!< The status of record is erased */
#define EEE_ERASED_RECORD         0xFFFFFFFFU

/*******************************************************************************
 * Enumerations.
 ******************************************************************************/

/*!
 * @brief Reading options.
 *
 * This is used to select an code for reading function.
 */
typedef enum
{
    EEE_BLANK_CHECK = 0x00U, /*!< The blank checking option */
    EEE_VERIFY      = 0x01U, /*!< The verifying option */
    EEE_READ        = 0x02U  /*!< The reading option */
} eee_read_code_t;

/*!
 * @brief Block status state.
 *
 * This is used to indicate the status of EEE block.
 */
typedef enum
{
    EEE_BLOCK_ERASED    = 0x00U, /*!< The EEE block is erased */
    EEE_BLOCK_ALT       = 0x01U, /*!< The EEE block is alternate */
    EEE_BLOCK_ACT       = 0x02U, /*!< The EEE block is active */
    EEE_BLOCK_UPDATE    = 0x03U, /*!< The EEE block is updated status*/
    EEE_BLOCK_INVALID   = 0x04U, /*!< The EEE block is invalid */
    EEE_BLOCK_DEAD      = 0x05U, /*!< The EEE block is dead status */
    EEE_BLOCK_COPY_DONE = 0x06U  /*!< The EEE block is copied success */
} eee_block_status_t;

/*!
 * @brief Last job status from brown-out.
 *
 * This is used to get the last of EEE block state.
 */
typedef enum
{
    EEE_LAST_JOB_NONE       = 0x00U, /*!< The last job does not have nothing */
    EEE_LAST_JOB_FIRST_TIME = 0x01U, /*!< The first time of job */
    EEE_LAST_JOB_NORMAL     = 0x02U, /*!< The last job is normal state */
    EEE_LAST_JOB_UPDATE     = 0x03U, /*!< The last job is updated block status */
    EEE_LAST_JOB_COPY_DONE  = 0x04U, /*!< The last job is copied block */
    EEE_LAST_JOB_ERASE      = 0x05U  /*!< The last job is erased block */
} eee_last_job_status_t;

/*!
 * @brief Write EEPROM options: normal, swap or not enough space.
 *
 * This is used to indicate the status of writing into data record.
 */
typedef enum
{
    EEE_WRITE_NORMAL          = 0x00U, /*!< The writing data is normal */
    EEE_WRITE_ON_NEW_ACTIVE   = 0x01U, /*!< The writing data is on the new active block */
    EEE_WRITE_ON_COPY_DONE    = 0x02U, /*!< The writing data is on the copied block */
    EEE_WRITE_SWAP            = 0x03U, /*!< The writing data is swapping progress */
    EEE_WRITE_NO_ENOUGH_SPACE = 0x04U  /*!< No enough space to write data */
} eee_write_status_t;

/*!
 * @brief This structure is to read the data from record status to ID and size field.
 * It does not read data which is right after ID and size field.
 */
typedef struct
{
    uint32_t dataStatus; /*!< The record status */
    uint16_t dataID;     /*!< The unique data ID */
    uint16_t dataSize;   /*!< The data size */
} eee_data_record_head_t;

/*******************************************************************************
 * Private API declaration
 ******************************************************************************/

/*!
 * @brief Erase EEE block
 *
 * This function will is used to launch command to erase the
 * specific blocks and it will re-erase for the number of maximum erase if failed
 * in case of not invoking on swapping.
 *
 * @param[in] blockIndex The index of EEE block is erased
 * @param[in] syncErase The type of erase operation
 * @return Execution status
 */
status_t EEE_DRV_EraseEEBlock(uint32_t blockIndex,
                              bool syncErase);

/*!
 * @brief Program data into block indicator
 *
 * This function will program into block indicator region.
 *
 * @param[in] dest The destination of block indicator
 * @param[in] source The destination of data source
 * @return Execution status
 */
status_t EEE_DRV_ProgramBlockIndicator(uint32_t dest,
                                       uint32_t source);

/*!
 * @brief Get the erasing block status
 *
 * This function will get the erasing status of the EEE block
 *
 * @return Execution status
 */
status_t EEE_DRV_GetEraseEEBlockStatus(void);

/*!
 * @brief Read, Verify or Check the content of the flash memory
 *
 * This function is used to read the content of the flash memory.
 * Verify data and check the block is blank.
 *
 * @param[in] funcCode The code option for reading
 * @param[in] dest The destination in flash
 * @param[in] size The size need to read, verify or check
 * @param[in] buffer The buffer store data
 * @return Execution status
 */
status_t EEE_DRV_FlashRead(eee_read_code_t funcCode,
                           uint32_t dest,
                           uint32_t size,
                           uint32_t buffer);

/*!
 * @brief Get the length of data record
 *
 * This function will calculate the record length based on data
 * size input.
 *
 * @param[in] dataSize The size of data field
 * @return Execution status
 */
uint32_t EEE_DRV_GetRecordLength(uint16_t dataSize);

/*!
 * @brief Get the writing status into record
 *
 * This function will get the status of writing in the EEE block.
 *
 * @param[in] recordLength The length of record
 * @return Execution status
 */
eee_write_status_t EEE_DRV_GetWriteRecordOption(uint32_t recordLength);

/*!
 * @brief Write data into record
 *
 * This function is used to program and verify one data record
 * into the Flash memory. If the dataID = 0xFFFF and then backing up data from
 * block swapping.
 *
 * @param[in] blockConf The EEE block configuration
 * @param[in] backupFlag The flag select the backup option
 * @param[in] dataID The ID of data record
 * @param[in] dataSize The size of data
 * @param[in] source The destination of data source
 * @return Execution status
 */
status_t EEE_DRV_WriteDataRecord(eee_block_config_t * blockConf,
                                 bool backupFlag,
                                 uint16_t dataID,
                                 uint16_t dataSize,
                                 uint32_t source);

/*!
 * @brief Read the content of record
 *
 * This function will get the content of record.
 *
 * @param[in] recordAddr The address of record
 * @param[in] dataSize The size of data record
 * @param[in] bufferAddr The destination store the content of data record
 * @return Execution status
 */
void EEE_DRV_ReadRecordAtAddr(uint32_t recordAddr,
                              uint16_t dataSize,
                              uint32_t bufferAddr);

/*!
 * @brief Implement some immediate request
 *
 * This function will implement operation immediate and Suspend
 * for both reading and writing operation.
 *
 * @param[in] blockConf The EEE block configuration
 * @param[in] reqType The type of request
 * @param[out] suspendState The sate of suspend operation
 * @return Execution status
 */
status_t EEE_DRV_ProcessImmediateRequest(const eee_block_config_t * blockConf,
                                         eee_request_type_t reqType,
                                         flash_state_t * suspendState);

/*!
 * @brief Search the required data ID in all EEE blocks
 *
 * This function will is used to search the required data ID
 * in the specific block. It will go through the entire block for the latest
 * data record.
 *
 * @param[in] dataID The ID of data record
 * @param[in] reqType The type of request
 * @param[out] recordAddr The destination of data record
 * @param[out] suspendState The sate of suspend operation
 * @return Execution status
 */
status_t EEE_DRV_SearchInAllBlocks(uint16_t dataID,
                                   eee_request_type_t reqType,
                                   uint32_t * recordAddr,
                                   flash_state_t * suspendState);

/*!
 * @brief Read EEE block status
 *
 * This function will read status of EEE block.
 *
 * @param[in] blockConf The EEE block configuration
 * @return Execution status
 */
eee_block_status_t EEE_DRV_ReadBlockStatus(const eee_block_config_t * blockConf);

/*!
 * @brief Program data with synchronous design
 *
 * This function will used to program the data into flash memory
 * with the size of data sources which are provided by user.
 *
 * @param[in] dest The destination need to be programmed
 * @param[in] size The size of byte need to be programmed
 * @param[in] source The destination of data source
 * @return Execution status
 */
status_t EEE_DRV_SyncProgram(uint32_t dest,
                             uint16_t size,
                             uint32_t source);

/*!
 * @brief Search the required ID from the start of EEE block
 *
 * This function will search the required IDs of the data record from
 * the start of the specific block region. It will check the number of records which
 * search records at most of EEprom block so it can return within the time limit.
 *
 * @param[in] blockConf The EEE block configuration structure
 * @param[in] bufferAddress The address of buffer
 * @param[in] bufferSize The size of buffer
 * @param[in] startID The start of data ID
 * @return Execution status
 */
uint16_t EEE_DRV_SearchRecordFromTop(eee_block_config_t * blockConf,
                                     uint32_t bufferAddress,
                                     uint32_t bufferSize,
                                     uint16_t startID);

/*!
 * @brief Swap the EEE block when all active blocks are full
 *
 * This function will copy the latest data records from the
 * active block which is nearly full to the alternative block.
 *
 * @param[in] syncErase The type of erase operation
 * @return Execution status
 */
status_t EEE_DRV_BlockSwapping(bool syncErase);

/*!
 * @brief Copy data record in EEE block
 *
 * This function will write data into EEE block until is successful.
 *
 * @param[in] blockConf The EEE block configuration structure
 * @param[in] backupFlag The flag to enable/disable backup data record
 * @param[in] dataID The ID of data record
 * @param[in] dataSize The size of data record
 * @param[in] source The destination of data source
 * @return Execution status
 */
status_t EEE_DRV_CopyDataRecord(eee_block_config_t * blockConf,
                                bool backupFlag,
                                uint16_t dataID,
                                uint16_t dataSize,
                                uint32_t source);

/*!
 * @brief Update the cache table is used in EEPROM emulation
 *
 * This function is used to update the contents of the cache
 * table while the data address is changed by writing a new value or block swapping.
 *
 * @param[in] pCacheTable The cache table structure pointer
 * @param[in] dataID The ID of data record
 * @param[in] newValue The new value of data ID
 * @return Execution status
 */
bool EEE_DRV_UpdateCacheTable(const eee_cache_table_t * pCacheTable,
                              uint16_t dataID,
                              uint32_t newValue);

/*!
 * @brief Search data ID in the cache table
 *
 * This function will is used to search the required data record
 * ID in the cache table.
 *
 * @param[in] pCacheTable The cache table structure pointer
 * @param[in] dataID The ID of data record
 * @param[in] expDataAddress
 * @return Execution status
 */
status_t EEE_DRV_SearchInTable(const eee_cache_table_t * pCacheTable,
                               uint16_t dataID,
                               uint32_t * expDataAddress);

/*!
 * @brief Validate the dead block
 *
 * This function will validate the DEAD block.
 *
 * @return Execution status
 */
status_t EEE_DRV_ValidateDeadBlocks(void);

/*!
 * @brief Make a block to dead
 *
 * This function will make an EEE block to DEAD block and move
 * the DEAD block to the end of the block array.
 *
 * @param[in] pBlockConf The EEE block configuration
 * @return Execution status
 */
status_t EEE_DRV_MakeBlockToDead(const eee_block_config_t * pBlockConf);

/*!
 * @brief Get the last job of EEE status
 *
 * This function will get the last job of EEE block.
 *
 * @return Execution status
 */
eee_last_job_status_t EEE_DRV_GetLastJobStatus(void);

/*!
 * @brief Recover EEPROM from brown-out
 *
 * This function will recover the EEE block is based on the last
 * job of EEE block.
 *
 * @param[in] lastJob The last job of EEE block in emulation
 * @return Execution status (success)
 */
status_t EEE_DRV_RecoverEeprom(eee_last_job_status_t lastJob);

/*! @} */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* EEE_COMMON_H */
/*******************************************************************************
 * EOF
 ******************************************************************************/
