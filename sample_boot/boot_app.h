/*
 * app.h
 *
 *  Created on: 2019��2��14��
 *      Author: Fred
 */

#ifndef APP_H_
#define APP_H_
#include <stdint.h>

//#define APP_BOOT_SHARE_DATA_SECTION  __attribute__((section(".app_boot_share_data")))

typedef struct 
{
	void *data;
	uint16_t data_len;
	uint8_t access_ctrl; // b0 - read enable; b1 - write enable
} boot_data_identifier_desc_t;

typedef struct
{
	uint8_t session;
	uint8_t unlocked;
	uint8_t reset_req;
	uint8_t flash_prog_state; // 0 - init, 1 - download req rcvd
	uint8_t expected_xfer_block_sn;
	uint32_t total_xfer_data_cnt;
	uint32_t xfer_data_rcvd_cnt;
	uint32_t seed;
	uint32_t checksum;
	uint32_t download_req_addr;
	uint32_t download_req_size;
	uint8_t encrypt_flag;
	uint8_t compress_flag;

} boot_service_data_t;

typedef int (*boot_service_fn_t)(boot_service_data_t*state, unsigned char *data, int len);
typedef void (*function_entry_t)(void);

#define APP_FLASH_ADDR_START (0x01001000)
#define APP_FLASH_SIZE (5564 * 1024)

#define APP_VALID_FLAG_ADDR (0x01000000)
#define APP_ENTRY ((function_entry_t)APP_FLASH_ADDR_START)
#define APP_VALID_FLAG (((uint32_t *)APP_VALID_FLAG_ADDR)[0])
#define APP_VALID_FLAG_INV (((uint32_t *)APP_VALID_FLAG_ADDR)[1])
#define APP_VALID_PATTERN (0x55555555)
#define ENTER_BOOT_REQ_PATTERN (0x12345678)
#define ENTER_BOOT_REQ_FLAG (__APP_BOOT_SHARE_DATA[0])

extern uint32_t __APP_BOOT_SHARE_DATA[];

typedef struct
{
	uint8_t sid;
	uint8_t unlock_required;
	uint8_t supported_session_mask;
	int min_len;
	int max_len;
	boot_service_fn_t fn;
} boot_service_handle_t;

typedef struct 
{
	uint8_t state; // 0 - idle, 1 - on processing, 2 - complete
	uint8_t result; // 0 - no req, 1 - start req, 2 - stop req
} boot_checksum_routine_t;

typedef struct
{
	uint8_t state;	// 0 - idle, 1 - on processing, 2 - complete
	uint8_t req;	// 0 - no req, 1 - start req, 2 - stop req
	uint8_t result; // 0 - no result, 1 - OK, 2 - fail
	uint32_t address;
	uint32_t size;
	int error_code;
} boot_erase_flash_routine_t;

//extern APP_BOOT_SHARE_DATA_SECTION uint32_t AppBootShareData[];

void app_init(void);


#endif /* APP_H_ */
