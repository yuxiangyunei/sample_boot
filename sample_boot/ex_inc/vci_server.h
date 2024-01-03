/*
 * vci_server.h
 *
 *  Created on: 2019��2��23��
 *      Author: Fred
 */

#ifndef VCI_SERVER_VCI_SERVER_H_
#define VCI_SERVER_VCI_SERVER_H_

#include "version.h"
#include "drivers.h"
#include "rtos.h"
#include "tcpip.h"

#ifdef USE_SDIO_RAM
#include "sdio_fifo.h"
#endif

#define USE_DEFAULT_MAX_CTO (8U)
#define USE_DEFAULT_AG (1U)

#define VCI_SERVER_TASK_STACK_SIZE (1024U)
#define VCI_SERVER_TASK_PRIO (4U)

#define VCI_SERVER_COMM_BUFF_SIZE (1536U)

#define VCI_ENET_PKT_SIZE (1440U)

#define VCI_BATCH_TIMEOUT_VALUE (500u)

#ifdef USE_TCP
#define VCI_ENET_TX_QUEUE_SIZE (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS / 2)
#else
#define VCI_ENET_TX_QUEUE_SIZE (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS - 20U)
#endif

typedef void (*function_entry_t)(void);

#define APP_ENTRY ((function_entry_t)0x01001000)
#define APP_VALID_FLAG (*(uint32_t *)0x01000000)
#define APP_VALID_PATTERN (0x55555555)
#define ENTER_BOOT_REQ_PATTERN (0x12345678)
#define ENTER_BOOT_REQ_FLAG (__APP_BOOT_SHARE_DATA[0])

extern uint32_t __APP_BOOT_SHARE_DATA[];

typedef void (*vci_rx_timeout_callback_t)(void);

typedef enum
{
	DEVICE_CTRL_REQ = 0,
	DEV_LIC_SK_WRITE_REQ,
	TIMESTAMP_CTRL_REQ,
	CAN_CH_SET_MODE_REQ,
	CAN_CH_SET_BAUD_REQ,
	CAN_CH1_TX_MSG_REQ,
	CAN_CH2_TX_MSG_REQ,
	CAN_CH3_TX_MSG_REQ,
	CAN_CH4_TX_MSG_REQ,
	CAN_CH5_TX_MSG_REQ,
	LIN_K_SET_MODE_REQ,
	LIN_K_RESET_BUFFER,
	LIN_K_CH1_TX_MSG_REQ,
	LIN_K_CH2_TX_MSG_REQ,
	LIN_K_CH3_TX_MSG_REQ,
	LIN_K_CH4_TX_MSG_REQ,
	LIN_K_CH5_TX_MSG_REQ,
	LIN_K_CH6_TX_MSG_REQ,
	CAN_CH6_TX_MSG_REQ,
	CAN_CH7_TX_MSG_REQ,
	CAN_CH8_TX_MSG_REQ,
	CAN_CH9_TX_MSG_REQ,
	CAN_CH10_TX_MSG_REQ,
	CAN_CH11_TX_MSG_REQ,
	CAN_CH12_TX_MSG_REQ,
	CAN_CH13_TX_MSG_REQ,
	CAN_CH14_TX_MSG_REQ,
	CAN_CH15_TX_MSG_REQ,
	CAN_CH16_TX_MSG_REQ,
	CCP_XCP_BATCH_MSG_REQ,
	CANTP_CHANNEL_OPEN_REQ,
	CANTP_CHANNEL_CLOSE_REQ,
	CANTP_TX_BUFF_WRITE_REQ,
	CANTP_START_TX_REQ,
	CANTP_GET_INFO_REQ,
	CANTP_CHANNELS_CTRL_REQ,
	LIN_K_CH7_TX_MSG_REQ,
	LIN_K_CH8_TX_MSG_REQ,
	LIN_K_CH9_TX_MSG_REQ,
	LIN_K_CH10_TX_MSG_REQ,
	LIN_K_CH11_TX_MSG_REQ,
	LIN_K_CH12_TX_MSG_REQ,
	LIN_K_CH13_TX_MSG_REQ,
	LIN_K_CH14_TX_MSG_REQ,
	LIN_K_CH15_TX_MSG_REQ,
	LIN_K_CH16_TX_MSG_REQ,
	FLEXRAY_SET_CONFIG_REQ,
	FLEXRAY_CH1_CONFIG_MSG_REQ,
	FLEXRAY_CH1_SET_MSG_REQ,
	FLEXRAY_CH2_CONFIG_MSG_REQ,
	FLEXRAY_CH2_SET_MSG_REQ,
	SET_XCP_BLOCK_PARAMETER_REQ,
	CANTP_LIST_CHANNELS_REQ = 0x0100,
	CANTP_CLOSE_CHANNELS_REQ,
	DEVICE_GET_INFO_REQ = 0x0110,
	DEVICE_READ_FLASH_REQ,
	DEVICE_WRITE_FLASH_REQ,
	DEVICE_SET_PERIOD_REQ,
	DEVICE_GOTO_BOOTLOADER_REQ,
	DEVICE_SET_GPO_OUTPUT_REQ,
	DEVICE_SET_WAKE_REQ,
	DEVICE_RESET_REQ,
	DEVICE_POWEROFF_REQ,
	DEVICE_CLEARFLASH_REQ,
	DEVICE_POWERON_REQ,
	VCI_TX_PACKET_PORT_NUM
} VCI_TX_PACKET_PORT;

typedef enum
{
	VCI_RESP_STS_OK = 0,
	VCI_RESP_INVALID_REQ_LEN,
	VCI_RESP_INVALID_REQ_FRAME_ID,
	VCI_RESP_INVALID_REQ_DATA,
	VCI_RESP_EXEC_FAIL,
	VCI_RESP_EXEC_TIMEOUT,
	VCI_RESP_INTERNAL_ERROR
} VCI_RESP_ERR_CODE;

typedef enum
{
	VCI_MEM_ALLOC_FAIL = -3,
	VCI_SERVER_SOCKET_ERROR = -2,
	VCI_SERVER_INVALID_PARAM = -1,
	VCI_SERVER_OK = 0,
	VCI_SERVER_COMM_ACTIVE = 1,
} vci_server_status_t;

typedef enum
{
	VCI_CLIENT_CONNECT_FAIL = 0,
	VCI_CLIENT_CONNECT_SUCCESS = 1,
} vci_client_connected_status_t;

typedef enum
{
	VCI_BMR_PUSH_NULL = 0,
	VCI_BMR_PUSH_FRAME = 1,
	VCI_BMR_PUSH_PACKET = 2,
} vci_bmr_record_push_flags_t;

typedef enum
{
	VCI_BATCH_MSG_REQ_CAN = 0x00,
	VCI_BATCH_MSG_REQ_CCP = 0x01,
	VCI_BATCH_MSG_REQ_XCP = 0x02,
	VCI_BATCH_MSG_REQ_XCP_BLOCK = 0x03,
} vci_batch_msg_mode_t;

typedef struct
{
	Socket_t listen_sock;
	Socket_t config_sock;
	Socket_t ext_display_sock;
#ifdef USE_TCP
	Socket_t time_sock;
	Socket_t client_connect_sock;
#endif
	unsigned short vci_port_nbo;		 // vci service port, network byte order
	unsigned short cfg_port_nbo;		 // cfg service port, network byte order
	unsigned short ext_display_port_nbo; // vci service port, network byte order
	TaskHandle_t server_task;
	TaskHandle_t ext_display_task;
	TaskHandle_t bmr_task;
	TaskHandle_t cfg_server_task;
	struct freertos_sockaddr remote_addr;
	struct freertos_sockaddr ext_display_remote_addr;
	SemaphoreHandle_t working_enet_tx_buff_mutex;
	QueueHandle_t enet_tx_queue;
	unsigned char client_connected_status;
	unsigned char ext_client_connected_status;
	unsigned int tx_buff_size;
	unsigned char *working_enet_tx_buff;
	unsigned int working_enet_tx_buff_idx;
	unsigned short enet_tx_fail_cnt;
	unsigned short enet_ppkt_fail_cnt;
	unsigned char resp_buf[VCI_SERVER_COMM_BUFF_SIZE];
	vci_server_status_t status;
	// unsigned int process_bmr_list_timeout;
	int rx_timeout;
	vci_rx_timeout_callback_t rx_timeout_callback;
#ifdef USE_SDIO_RAM
	sdio_fifo_t sdio_fifo;
	uint8_t *sdio_ram_buffer;
	uint32_t sdio_ram_buffer_block_num;
#endif
} vci_server_runtime_t;

extern vci_server_runtime_t vci_server_runtime;

vci_server_status_t vci_server_init(vci_server_runtime_t *runtime, unsigned short port, int initial_rx_timeout, int rx_timeout, vci_rx_timeout_callback_t rx_timeout_callback);
int bmr_record_push(vci_server_runtime_t *runtime, unsigned char channel, uint64_t ts_us, unsigned int id, unsigned char *data, unsigned short data_len, uint8_t flags, uint8_t ptp_flags);
int vci_server_send(vci_server_runtime_t *runtime, unsigned char *data, unsigned short data_len, int send_flags);

#endif /* VCI_SERVER_VCI_SERVER_H_ */
