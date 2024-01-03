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
 */

#ifndef EEE_DRIVER_H
#define EEE_DRIVER_H

/*! @file eee_driver.h */

#include "flash_c55_driver.h"

/*!
 * @defgroup eee_driver EEE Driver
 * @ingroup eee
 * @details This section describes the programming interface of the EEE driver.
 * @{
 */

/*******************************************************************************
 * Enumerations.
 ******************************************************************************/
/*!
 * @brief Record scheme option.
 *
 * This is used to select the type of recording.
 * Implements : eee_record_option_t_Class
 */
typedef enum
{
    EEE_FIXLENGTH  = 0x00U, /*!< The type of record is fix length */
    EEE_VARLENGTH  = 0x01U, /*!< The type of record is variable length */
} eee_record_option_t;

/*!
 * @brief Immediate request type.
 *
 * This is used to select the immediate request for reading and writing.
 * Implements : eee_request_type_t_Class
 */
typedef enum
{
    EEE_IMMEDIATE_NONE   = 0x00U, /*!< Not request the immediate operation */
    EEE_IMMEDIATE_READ   = 0x01U, /*!< The request is to read immediate */
    EEE_IMMEDIATE_WRITE  = 0x02U, /*!< The request is to write immediate */
    EEE_IMMEDIATE_DELETE = 0x03U  /*!< The request is to delete immediate */
} eee_request_type_t;

/*!
 * @brief Modules for ECC exception handler.
 *
 * This is used to check EEC on EEE module.
 * Implements : eee_module_type_t_Class
 */
typedef enum
{
    EEE_NONE     = 0x00U, /*!< The none EEE module */
    EEE_MODULE   = 0x01U  /*!< The EEE module */
} eee_module_type_t;

/*!
 * @brief Status of Erasing.
 *
 * This is used to indicate the erasing status when occurs a swapping.
 * Implements : eee_erase_status_t_Class
 */
typedef enum
{
    EEE_ERASE_NOT_STARTED = 0x00U, /*!< The erase status is not started */
    EEE_ERASE_DONE        = 0x01U, /*!< The erase done */
    EEE_ERASE_FAIL        = 0x02U, /*!< The failed erase */
    EEE_ERASE_IN_PROGRESS = 0x03U, /*!< The erase status is in-progress */
    EEE_ERASE_SWAP_ERROR  = 0x04U  /*!< The erase status is error in swapping */
} eee_erase_status_t;

/*!
 * @brief The block configuration structure.
 * Implements : eee_block_config_t_Class
 */
typedef struct
{
    uint32_t enabledBlock;            /*!< The block bit map in specific space (block 0 is corresponding to bit 0; block 1 is corresponding to bit 1) */
    uint32_t blockStartAddr;          /*!< The block start address */
    uint32_t blockSize;               /*!< The block size */
    uint32_t blankSpace;              /*!< The address pointer to the blank space */
    flash_address_space_t blockSpace; /*!< The space (low, middle or high) for the block */
    uint32_t partSelect;              /*!< Partition number of selected blocks */
} eee_block_config_t;

/*!
 * @brief Define cache table type.
 * Implements : eee_cache_table_t_Class
 */
typedef struct
{
    uint32_t * startAddress; /*!< The start address of cache table */
    uint32_t size;           /*!< The size of cache table in byte */
} eee_cache_table_t;

/*! @brief EEE callback function type. */
typedef void (* eee_callback_t)(void * param);

/*!
 * @brief Define EEPROM user configuration structure.
 * Implements : eee_user_config_t_Class
 */
typedef struct
{
    uint32_t numberOfBlock;                     /*!< The number of blocks used for EEPROM emulation */
    uint32_t numberOfActBlock;                  /*!< The number of active blocks being used emulation */
    uint32_t numOfByteRead;                     /*!< The number of byte to service the callback function in reading */
    uint32_t numOfCycleSearch;                  /*!< The number of cycle to service the callback function in searching record */
    uint32_t numOfRecordSearch;                 /*!< The number of record need to search after that service the callback function */
    eee_callback_t callback;                    /*!< User callback function for synchronous */
    void * callbackParam;                       /*!< Parameter for user callback */
    eee_cache_table_t * cTable;                 /*!< The cache table structure */
    eee_block_config_t ** flashBlocks;          /*!< The block configuration array pointer */
    eee_record_option_t schemeSelection;        /*!< The record scheme for emulation */
    uint32_t dataSize;                          /*!< The data size of record which is mandatory for fixed length record schemes */
    uint32_t maxReEraseEeeBlock;                /*!< The maximum number of erasing a block if it is failed to erase */
    uint32_t maxReProgram;                      /*!< The maximum number of programming block indicator words if it is failed to make to non-blank */
    bool cacheEnable;                           /*!< The flag to enable/disable the cache table */
    uint16_t maxRecordId;                       /*!< The maximum number of record is available in an EEE block*/
} eee_user_config_t;

/*!
 * @brief The state of EEPROM emulation.
 */
typedef struct
{
/*! @cond DRIVER_INTERNAL_USE_ONLY */
    uint32_t numberOfActBlock;             /*!< The number of blocks used for EEPROM emulation */
    uint32_t numOfByteRead;                /*!< The number of byte to service the callback function in reading */
    uint32_t numOfCycleSearch;             /*!< The number of cycle to service the callback function in searching record */
    uint32_t numOfRecordSearch;            /*!< The number of record need to search after that service the callback function */
    eee_callback_t callback;               /*!< User callback function for synchronous */
    void * callbackParam;                  /*!< Parameter for user callback */
    uint32_t eccSize;                      /*!< The size of ECC scheme */
    uint32_t sizeField;                    /*!< The size filed in byte */
    uint32_t minRecordSize;                /*!< The size of the minimum record */
    uint32_t dataHeadSize;                 /*!< The size of the head data */
    uint32_t smallDataSize;                /*!< The size of small data */
    uint32_t idOffset;                     /*!< The offset of ID filed */
    uint32_t dataSize;                     /*!< The size of data */
    uint32_t maxReEraseEeeBlock;           /*!< The maximum number to re-erase an EEE block */
    uint32_t maxReProgram;                 /*!< The maximum number to re-program a block indicator */
    bool cacheEnable;                      /*!< The flag to enable/disable the cache table */
    uint16_t maxRecordId;                  /*!< The maximum number of record is available in an EEE block */
    uint32_t numberOfBlock;                /*!< The number of blocks being used emulation */
    uint32_t numberOfDeadBlock;            /*!< The number of dead block */
    uint32_t activeBlockIndex;             /*!< The active block index */
    volatile bool blockWriteFlag;          /*!< The writing flag of the blocks */
    eee_cache_table_t * cTable;            /*!< The cache table structure */
    eee_block_config_t ** flashBlocks;     /*!< The block configuration array pointer */
/*! @endcond */
} eee_state_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Flag to check error in EEE module */
extern eee_module_type_t g_eccErrorModuleFlag;
/* Flag to keep track of Erase State */
extern eee_erase_status_t g_eraseStatusFlag;
/* Variable to store the EEE state */
extern eee_state_t * g_eeeState;

/*******************************************************************************
 * API
 ******************************************************************************/
/*!
 * @name EEE DRIVER API
 * @{
 */

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Initializes the EEE driver
 *
 * This function Initialize the EEPROM Emulation driver.
 *
 * @param[in] userConf The user configuration
 * @param[out] state The state of EEPROM emulation
 * @return Execution status
 */
status_t EEE_DRV_InitEeprom(const eee_user_config_t * userConf,
                            eee_state_t * state);

/*!
 * @brief Write data records into EEPROM emulation
 *
 * This function is to write data records to the EEPROM emulated Flash
 * and re-write data record if this program operation fails.
 *
 * @param[in] dataID The ID of data record
 * @param[in] dataSize The size of data record
 * @param[in] source The destination store data record
 * @param[in] iReq The type of request immediate
 * @return Execution status
 */
status_t EEE_DRV_WriteEeprom(uint16_t dataID,
                             uint16_t dataSize,
                             uint32_t source,
                             eee_request_type_t iReq);

/*!
 * @brief Read the requested ID from EEPROM emulation
 *
 * This function is to read the specific data record. The data
 * record size is determined by the data size written into the EEPROM emulation.
 *
 * @param[in] dataID The ID of data record
 * @param[in] dataSize The size of data record
 * @param[in] buffAddr The address store data record
 * @param[out] recordAddr The address of record
 * @param[in] iReq The type of request immediate
 * @return Execution status
 */
status_t EEE_DRV_ReadEeprom(uint16_t dataID,
                            uint16_t dataSize,
                            uint32_t buffAddr,
                            uint32_t * recordAddr,
                            eee_request_type_t iReq);

/*!
 * @brief Delete a data record from EEPROM emulation
 *
 * This function is to delete a data record in the EEPROM emulated Flash
 *
 * @param[in] dataID The ID of data record
 * @param[in] iReq The type of request immediate
 * @return Execution status
 */
status_t EEE_DRV_DeleteRecord(uint16_t dataID,
                              eee_request_type_t iReq);

/*!
 * @brief Report the EEPROM status
 *
 * This function is to report block erasing cycles and check
 * the block status.
 *
 * @param[in] erasingCycles The number of cycle is erased block
 * @return Execution status
 */
status_t EEE_DRV_ReportEepromStatus(uint32_t * erasingCycles);

/*!
 * @brief Remove the EEPROM emulation
 *
 * This function is to clear all blocks used for EEPROM emulation.
 *
 * @return Execution status
 */
status_t EEE_DRV_RemoveEeprom(void);

/*!
 * @brief The main function to help completion in swap progress
 *
 * This function will help in erasing the oldest ACTIVE block
 * asynchronously, update erase cycles and block status.
 *
 * @return Execution status
 */
status_t EEE_DRV_MainFunction(void);

/*! @} */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* EEE_DRIVER_H */
/*******************************************************************************
 * EOF
 ******************************************************************************/
