/*
 * app.c
 *
 *  Created on: 2019��2��14��
 *      Author: Fred
 */
#include "version.h"
#include "drivers.h"
#include "rtos.h"
#include "tcpip.h"
#include "boot_board.h"
#include "boot_app.h"
#include "flash_drv.h"
#include "crc32.h"
#include "rnd.h"
#include "rc4.h"

#define ROUTINE_ID_ERASE_MEMORY (0xFF00)
#define ROUTINE_ID_CHECKSUM (0xFF01)

#define CPYPT_MASK (0x55)

static int session_ctrl_svc(boot_service_data_t*state, unsigned char *req, int len);
static int reset_svc(boot_service_data_t*state, unsigned char *req, int len);
static int tester_present_svc(boot_service_data_t*state, unsigned char *req, int len);
static int routine_ctrl_svc(boot_service_data_t*state, unsigned char *req, int len);
static int download_req_svc(boot_service_data_t*state, unsigned char *req, int len);
static int xfer_data_svc(boot_service_data_t*state, unsigned char *req, int len);
static int exit_xfer_svc(boot_service_data_t*state, unsigned char *req, int len);
static int sec_access_svc(boot_service_data_t*state, unsigned char *req, int len);
static int write_data_by_id_svc(boot_service_data_t*state, unsigned char *req, int len);
static int read_data_by_id_svc(boot_service_data_t*state, unsigned char *req, int len);

//APP_BOOT_SHARE_DATA_SECTION uint32_t AppBootShareData[8];
static const uint8_t enc_key[16] = {'k','U','n','Y','i','@','V','a','R','v','C','i',0x20,0x19,0x10,0x28};

static uint8_t enc_header[8] = {0,0,0,0,0,0,0,0};
static const uint32_t svn_rev = SVN_REV;
static const boot_data_identifier_desc_t boot_data_table[] = 
{
	{enc_header, sizeof(enc_header), 0x03},
	{&svn_rev, sizeof(svn_rev), 0x01}
};

rc4_key rc4_ctx;

boot_erase_flash_routine_t erase_routine_data;
boot_checksum_routine_t checksum_routine_data;

const boot_service_handle_t boot_service_table[] =
{
	//SID, SecAccess, Session, Len_Min, Len_Max, Function
	{0x10, 0, 0x03, 2, 2, session_ctrl_svc},
	{0x11, 0, 0x03, 2, 2, reset_svc},
	{0x3E, 0, 0x03, 2, 2, tester_present_svc},
	{0x31, 1, 0x02, 4, 12, routine_ctrl_svc},
	{0x34, 1, 0x02, 4, 10, download_req_svc},
	{0x36, 1, 0x02, 3, 1500, xfer_data_svc},
	{0x37, 1, 0x02, 1, 1, exit_xfer_svc},
	{0x27, 0, 0x03, 2, 6, sec_access_svc},
	{0x2E, 1, 0x02, 4, 1500, write_data_by_id_svc},
	{0x22, 0, 0x03, 3, 3, read_data_by_id_svc},
};

int check_flash_address_valid(uint32_t addr, uint32_t size)
{
	const uint32_t seg_base[]    = {0x00F8C000, 0x01001000};
	const uint32_t seg_size[]    = {   0x54000,   0x57F000};
	const uint8_t seg_mem_type[] = {         1,          1};
	int ret = 0; // 0 - invalid address, 1 - P-Flash
	int i;
	for (i=0; i<sizeof(seg_base)/sizeof(seg_base[0]); i++)
	{
		if ((addr >= seg_base[i]) && (addr + size <= seg_base[i] + seg_size[i]) && (size != 0) && (size <= seg_size[i]))
		{
			ret = seg_mem_type[i];
			break;
		}
	}
	return ret;
}

void build_crypt_msg(uint8_t *src, uint32_t src_len, uint8_t *dest, uint32_t *dest_len, uint8_t mask)
{
	uint32_t i;
	uint8_t check = 0;
	
	for (i = 0; i < src_len; i++)
	{
		src[i] ^= mask;
		check += src[i];
	}
	
	dest[0] = 0x7E;
	dest[1] = src[0];
	dest[2] = (uint8_t)(src_len >> 8);
	dest[3] = (uint8_t)(src_len);
	if(src_len > 1){
		memcpy(&dest[4], &src[1], src_len-1);
	}
	dest[3+src_len] = check;
	dest[4+src_len] = 0x7E;
	
	*dest_len = src_len + 5;
}

void decrypt_msg(uint8_t *src, uint32_t src_len, uint8_t *dest, int *dest_len, uint8_t mask)
{
	uint32_t i;
	int len = 1;
	
	dest[0] = src[1];
	if(src_len > 6){
		len = ((src[2]<<8)&0xFF00) + src[3];
		memcpy(&dest[1], &src[4], len-1);
	}
	
	for (i = 0; i < len; i++)
	{
		dest[i] ^= mask;
	}
	
	*dest_len = len;
}

static void boot_service_data_init(boot_service_data_t *data)
{
	data->session = 0x01;
	data->unlocked = 0;
	data->seed = 0;
	data->reset_req = 0;
	data->xfer_data_rcvd_cnt = 0;
	data->flash_prog_state = 0;
	data->download_req_addr = 0;
	data->download_req_size = 0;
	data->checksum = 0xFFFFFFFF;
	data->expected_xfer_block_sn = 0;
	data->total_xfer_data_cnt = 0;
	data->encrypt_flag = 0;
	data->compress_flag = 0;
}

static int write_data_by_id_svc(boot_service_data_t*state, unsigned char *req, int len)
{
	int ret;
	uint16_t id;
	id = (((uint16_t)req[1] << 8) | req[2]);
	if (id < sizeof(boot_data_table) / sizeof(boot_data_table[0]))
	{
		if (len - 3 == boot_data_table[id].data_len)
		{
			if (boot_data_table[id].access_ctrl & 0x02)
			{
				memcpy(boot_data_table[id].data, &req[3], len - 3);
				req[0] += 0x40;
				ret = 3;
			}
			else
			{
				req[1] = req[0];
				req[0] = 0x7F;
				req[2] = 0x33;
				ret = 3;
			}
		}
		else
		{
			req[1] = req[0];
			req[0] = 0x7F;
			req[2] = 0x13;
			ret = 3;
		}
	}
	else
	{
		req[1] = req[0];
		req[0] = 0x7F;
		req[2] = 0x31;
		ret = 3;
	}
	return ret;	
}

static int read_data_by_id_svc(boot_service_data_t*state, unsigned char *req, int len)
{
	int ret;
	uint16_t id;
	id = (((uint16_t)req[1] << 8) | req[2]);
	if (id < sizeof(boot_data_table) / sizeof(boot_data_table[0]))
	{
		if (boot_data_table[id].access_ctrl & 0x01)
		{
			req[0] += 0x40;
			memcpy(&req[3], boot_data_table[id].data, boot_data_table[id].data_len);
			ret = 3 + boot_data_table[id].data_len;
		}
		else
		{
			req[1] = req[0];
			req[0] = 0x7F;
			req[2] = 0x33;
			ret = 3;
		}
	}
	else
	{
		req[1] = req[0];
		req[0] = 0x7F;
		req[2] = 0x31;
		ret = 3;
	}
	return ret;	
}

static int session_ctrl_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	int ret;
	if ((req[1] != 0) && (req[1] <= 8))
	{
		boot_service_data_init(state);
		state->session = (1 << (req[1] - 1));
		req[0] += 0x40;
		ret = 2;
	}
	else
	{
		req[1] = req[0];
		req[0] = 0x7F;
		req[2] = 0x12;
		ret = 3;
	}
	return ret;
}

static int reset_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	state->reset_req = 1;
	req[0] += 0x40;
	return 2;
}

static int tester_present_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	int ret = 0;
	req[0] += 0x40;
	ret = 2;
	return ret;
}

static int routine_ctrl_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	int ret = 0;
	uint8_t cmd = req[1];
	uint16_t id = (((uint16_t)req[2] << 8) | (req[3]));
	uint32_t tmp_u32[4];
	switch (cmd)
	{
		case 0x01:
			// start routine
			switch (id)
			{
				case ROUTINE_ID_ERASE_MEMORY:
					if (len == 12)
					{
						tmp_u32[0] = (((uint32_t)req[4] << 24) | ((uint32_t)req[5] << 16) | ((uint32_t)req[6] << 8) | ((uint32_t)req[7]));
						tmp_u32[1] = (((uint32_t)req[8] << 24) | ((uint32_t)req[9] << 16) | ((uint32_t)req[10] << 8) | ((uint32_t)req[11]));
						if (check_flash_address_valid(tmp_u32[0], tmp_u32[1]))
						{
							if ((erase_routine_data.state != 1) && (erase_routine_data.req == 0))
							{
								erase_routine_data.result = 0;
								erase_routine_data.address = tmp_u32[0];
								erase_routine_data.size = tmp_u32[1];
								erase_routine_data.req = 1;
								req[0] += 0x40;
								ret = 4;
							}
							else
							{
								req[1] = req[0];
								req[0] = 0x7F;
								req[2] = 0x22;
								ret = 3;
							}
						}
						else
						{
							req[1] = req[0];
							req[0] = 0x7F;
							req[2] = 0x31;
							ret = 3;
						}
					}
					else
					{
						// incorrect message length
						req[1] = req[0];
						req[0] = 0x7F;
						req[2] = 0x13;
						ret = 3;
					}
					break;
				case ROUTINE_ID_CHECKSUM:
					if (len == 8)
					{
						tmp_u32[0] = (((uint32_t)req[4] << 24) | ((uint32_t)req[5] << 16) | ((uint32_t)req[6] << 8) | ((uint32_t)req[7]));
						if (state->total_xfer_data_cnt != 0)
						{		
							if (tmp_u32[0] == state->checksum)
							{
								tmp_u32[0] = APP_VALID_PATTERN;
								tmp_u32[1] = (~APP_VALID_PATTERN);
								tmp_u32[2] = state->total_xfer_data_cnt;
								tmp_u32[3] = state->checksum;
								if (STATUS_SUCCESS == flash_write(APP_VALID_FLAG_ADDR, tmp_u32, 16))
								{
									checksum_routine_data.result = 1;
								}
								else
								{
									// program fail.
									checksum_routine_data.result = 2;
								}
							}
							else
							{
								checksum_routine_data.result = 0;
							}
							checksum_routine_data.state = 2;
							req[0] += 0x40;
							ret = 4;
						}
						else
						{
							req[1] = req[0];
							req[0] = 0x7F;
							req[2] = 0x22;
							ret = 3;
						}
					}
					else
					{
						// incorrect message length
						req[1] = req[0];
						req[0] = 0x7F;
						req[2] = 0x13;
						ret = 3;
					}
					break;
				default:
					req[1] = req[0];
					req[0] = 0x7F;
					req[2] = 0x31;
					ret = 3;
					break;
			}
			break;
		case 0x03:
			// request routine result
			if (len == 4)
			{
				switch (id)
				{
				case ROUTINE_ID_ERASE_MEMORY:
					if (2 == erase_routine_data.state)
					{
						erase_routine_data.state = 0;
						req[0] += 0x40;
						req[4] = erase_routine_data.result;
						ret = 5;
					}
					else if ((1 == erase_routine_data.state) || (1 == erase_routine_data.req))
					{
						req[0] += 0x40;
						req[4] = 0x00;
						ret = 5;
					}
					else
					{
						req[1] = req[0];
						req[0] = 0x7F;
						req[2] = 0x24;
						ret = 3;
					}
					break;
				case ROUTINE_ID_CHECKSUM:
					if (2 == checksum_routine_data.state)
					{
						checksum_routine_data.state = 0;
						req[0] += 0x40;
						req[4] = checksum_routine_data.result;
						ret = 5;
					}
					else if (1 == checksum_routine_data.state)
					{
						checksum_routine_data.state = 0;
						req[0] += 0x40;
						req[4] = 0x00;
						ret = 5;
					}
					else
					{
						req[1] = req[0];
						req[0] = 0x7F;
						req[2] = 0x24;
						ret = 3;
					}
					break;
				default:
					req[1] = req[0];
					req[0] = 0x7F;
					req[2] = 0x31;
					ret = 3;
					break;
				}
			}
			else
			{
				// incorrect message length
				req[1] = req[0];
				req[0] = 0x7F;
				req[2] = 0x13;
				ret = 3;
			}
			break;
		case 0x02:
			//stop routine
			//break;
		default:
			req[1] = req[0];
			req[0] = 0x7F;
			req[2] = 0x12;
			ret = 3;
			break;
	}
	return ret;
}
static int download_req_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	int ret = 0;
	uint8_t fmt_len1 = ((req[1] >> 4) & 0x07);
	uint8_t fmt_len2 = (req[1] & 0x07);
	uint8_t encrypt_flag = (req[1] & 0x80) ? 1 : 0;
	uint8_t compress_flag = (req[1] & 0x08) ? 1 : 0;
	uint32_t addr;
	uint32_t data_size;
	uint8_t tmp_key[16];
	uint8_t i;
	//state->flash_prog_state = 0;
	if ((fmt_len1 != 0) && (fmt_len1 <= 4) && (fmt_len2 != 0) && (fmt_len2 <= 4))
	{
		if (len >= 2 + fmt_len1 + fmt_len2)
		{
			switch (fmt_len1)
			{
			case 1:
				addr = (unsigned int)req[2];
				break;
			case 2:
				addr = (((unsigned int)req[2] << 8) | (unsigned int)req[3]);
				break;
			case 3:
				addr = (((unsigned int)req[2] << 16) | ((unsigned int)req[3] << 8) | (unsigned int)req[4]);
				break;
			case 4:
				addr = (((unsigned int)req[2] << 24) | ((unsigned int)req[3] << 16) | ((unsigned int)req[4] << 8) | (unsigned int)req[5]);
				break;
			default:
				addr = 0;
				break;
			}
			switch (fmt_len2)
			{
			case 1:
				data_size = (unsigned int)req[2 + fmt_len1];
				break;
			case 2:
				data_size = (((unsigned int)req[2 + fmt_len1] << 8) | (unsigned int)req[3 + fmt_len1]);
				break;
			case 3:
				data_size = (((unsigned int)req[2 + fmt_len1] << 16) | ((unsigned int)req[3 + fmt_len1] << 8) | (unsigned int)req[4 + fmt_len1]);
				break;
			case 4:
				data_size = (((unsigned int)req[2 + fmt_len1] << 24) | ((unsigned int)req[3 + fmt_len1] << 16) | ((unsigned int)req[4 + fmt_len1] << 8) | (unsigned int)req[5 + fmt_len1]);
				break;
			default:
				data_size = 0;
				break;
			}
			if (check_flash_address_valid(addr, data_size))
			{
				if (0 == state->flash_prog_state)
				{
					for (i=0; i<sizeof(tmp_key); i++)
					{
						tmp_key[i] = (enc_key[i] ^ enc_header[(i & 7)]);
					}
					rc4_init_key(tmp_key, &rc4_ctx);
				}
				state->expected_xfer_block_sn = 1;
				state->flash_prog_state = 1;
				state->xfer_data_rcvd_cnt = 0;
				state->download_req_addr = addr;
				state->download_req_size = data_size;
				state->encrypt_flag = encrypt_flag;
				state->compress_flag = compress_flag;
				req[0] += 0x40;
				ret = 1;
			}
			else
			{
				state->flash_prog_state = 0;
				req[1] = req[0];
				req[0] = 0x7F;
				req[2] = 0x31;
				ret = 3;
			}
		}
		else
		{
			// incorrect message length
			state->flash_prog_state = 0;
			req[1] = req[0];
			req[0] = 0x7F;
			req[2] = 0x13;
			ret = 3;
		}
	}
	else
	{
		state->flash_prog_state = 0;
		req[1] = req[0];
		req[0] = 0x7F;
		req[2] = 0x12;
		ret = 3;
	}
	return ret;
}
static int xfer_data_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	int ret = 0;
	uint8_t nrc = 0;
	len -=2;
	if ((state->flash_prog_state == 1) && (state->download_req_size >= state->xfer_data_rcvd_cnt + len))
	{
		if (state->expected_xfer_block_sn == req[1])
		{
			if (state->encrypt_flag)
			{
				rc4(&req[2], &req[2], len, &rc4_ctx);
			}
			if (STATUS_SUCCESS == flash_write(state->download_req_addr + state->xfer_data_rcvd_cnt, &req[2], len))
			{
				state->checksum = crc32(state->checksum, (void *)(state->download_req_addr + state->xfer_data_rcvd_cnt), len);
				++state->expected_xfer_block_sn;
				state->xfer_data_rcvd_cnt += len;
				state->total_xfer_data_cnt += len;
				req[0] += 0x40;
				ret = 2;
			}
			else
			{
				nrc = 0x72;
			}
		}
		else
		{
			nrc = 0x24;
		}
	}
	else
	{
		nrc = 0x24;
	}
	if (nrc != 0)
	{
		req[1] = req[0];
		req[0] = 0x7F;
		req[2] = nrc;
		ret = 3;
	}
	return ret;
}

static int exit_xfer_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	int ret = 0;
	if (state->flash_prog_state == 1)
	{
		req[0] += 0x40;
		ret = 1;
	}
	else
	{
		req[1] = req[0];
		req[0] = 0x7F;
		req[2] = 0x24;
		ret = 3;
	}
	state->flash_prog_state = 0;
	return ret;
}

static int sec_access_svc(boot_service_data_t *state, unsigned char *req, int len)
{
	int ret = 0;
	uint32_t key, rx_key;
	switch (req[1])
	{
		case 0x01:
			// seed
			if (len == 2)
			{
				state->seed = Rnd();
				req[0] += 0x40;
				req[2] = (uint8_t)(state->seed >> 24);
				req[3] = (uint8_t)(state->seed >> 16);
				req[4] = (uint8_t)(state->seed >> 8);
				req[5] = (uint8_t)(state->seed);
				ret = 6;
			}
			else
			{
				req[1] = req[0];
				req[0] = 0x7F;
				req[2] = 0x13;
				ret = 3;
			}
			break;
		case 0x02:
			// key
			if (len == 6)
			{
				if (state->seed != 0)
				{
					rx_key = (((uint32_t)req[2] << 24) | ((uint32_t)req[3] << 16) | ((uint32_t)req[4] << 8) | ((uint32_t)req[5]));
					key = LFSR32((state->seed ^ 0x20191028), LFSR_TAP_MASK, state->session * 8);
					if (rx_key == key)
					{
						state->unlocked |= state->session;
						req[0] += 0x40;
						ret = 2;
					}
					else
					{
						state->unlocked &= (~state->session);
						req[1] = req[0];
						req[0] = 0x7F;
						req[2] = 0x35;
						ret = 3;
					}
					state->seed = 0;
				}
				else
				{
					req[1] = req[0];
					req[0] = 0x7F;
					req[2] = 0x24;
					ret = 3;
				}
			}
			else
			{
				req[1] = req[0];
				req[0] = 0x7F;
				req[2] = 0x13;
				ret = 3;
			}
			break;
		default:
			state->seed = 0;
			req[1] = req[0];
			req[0] = 0x7F;
			req[2] = 0x12;
			ret = 3;
	}
	return ret;
}

void erase_routine_task(void *param)
{
	while(1)
	{
		if (erase_routine_data.req)
		{
			erase_routine_data.state = 1;
			erase_routine_data.req = 0;
			erase_routine_data.error_code = flash_erase(erase_routine_data.address, erase_routine_data.size);
			if (STATUS_SUCCESS == erase_routine_data.error_code)
			{
				erase_routine_data.result = 1;
			}
			else
			{
				erase_routine_data.result = 2;
			}
			erase_routine_data.state = 2;
		}
		vTaskDelay(100);
	}
}

void boot_main_task(void *param)
{
	uint8_t dev_id = id_pin_read();
	unsigned char ip_addr[4] = {192, 168, 1, 190};
	unsigned char mac_addr[6] = {0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
	unsigned char net_mask[4] = {255,255,255,0};
	unsigned char gateway[4] = {192, 168, 1, 187};
	unsigned char dns[4] = {114,114,114,114};

	uint8_t buf_crypt[16] = {0};
	uint32_t cryptLen = 0;
	uint8_t buf_decrypt[1280] = {0};
	uint32_t decryptLen = 0;

	Socket_t sock;
	struct freertos_sockaddr local_addr;
	int rx_timeout = 3000; // 3 sec timeout
	int32_t rx_size;
	int32_t tx_size;
	uint8_t *p_rx_data;
	unsigned int i;
	struct freertos_sockaddr addr_remote;
	boot_service_data_t svc_state;

	boot_service_data_init(&svc_state);
	erase_routine_data.state = 0;
	erase_routine_data.req = 0;
	erase_routine_data.result = 0;
	erase_routine_data.error_code = 0;
	erase_routine_data.address = 0;
	erase_routine_data.size = 0;

	checksum_routine_data.state = 0;
	checksum_routine_data.result = 0;

	ip_addr[3] += dev_id;
	mac_addr[5] += dev_id;
	param;
	FreeRTOS_IPInit(ip_addr, net_mask, gateway, dns, mac_addr);
	sock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
	FreeRTOS_setsockopt(sock, 0, FREERTOS_SO_RCVTIMEO, &rx_timeout, 0);
	FreeRTOS_GetAddressConfiguration(&local_addr.sin_addr, NULL, NULL, NULL);
	local_addr.sin_port = FreeRTOS_htons( 14229 );
	FreeRTOS_bind(sock, &local_addr, sizeof(local_addr));
	flash_drv_init();
	while (1)
	{
		rx_size = FreeRTOS_recvfrom(sock, (void *)&p_rx_data, 0, FREERTOS_ZERO_COPY, &addr_remote, NULL, NULL);
		Srnd(xTaskGetTickCount());
		if (rx_size > 0)
		{
			//TODO: serve command.
			decrypt_msg(p_rx_data, rx_size, buf_decrypt, &decryptLen, CPYPT_MASK);
			//memcpy(p_rx_data, buf_decrypt, decryptLen);
			//FreeRTOS_sendto(sock, p_rx_data, decryptLen, FREERTOS_ZERO_COPY, &addr_remote, NULL, NULL);
			
			tx_size = 0;
			for (i = 0; i < sizeof(boot_service_table) / sizeof(boot_service_table[0]); i++)
			{
				if (buf_decrypt[0] == boot_service_table[i].sid)
				{
					if ((decryptLen >= boot_service_table[i].min_len) && (decryptLen <= boot_service_table[i].max_len))
					{
						if (svc_state.session & boot_service_table[i].supported_session_mask)
						{
							if ((boot_service_table[i].unlock_required == 0) || (svc_state.unlocked & svc_state.session))
							{
								if (boot_service_table[i].fn != NULL)
								{
									tx_size = boot_service_table[i].fn(&svc_state, buf_decrypt, decryptLen);
								}
								else
								{
									// general reject
									buf_decrypt[1] = buf_decrypt[0];
									buf_decrypt[0] = 0x7F;
									buf_decrypt[2] = 0x10;
									tx_size = 3;
								}
							}
							else
							{
								// security access required
								buf_decrypt[1] = buf_decrypt[0];
								buf_decrypt[0] = 0x7F;
								buf_decrypt[2] = 0x33;
								tx_size = 3;
							}
						}
						else
						{
							// session not support
							buf_decrypt[1] = buf_decrypt[0];
							buf_decrypt[0] = 0x7F;
							buf_decrypt[2] = 0x7F;
							tx_size = 3;
						}
					}
					else
					{
						// incorrect message length
						buf_decrypt[1] = buf_decrypt[0];
						buf_decrypt[0] = 0x7F;
						buf_decrypt[2] = 0x13;
						tx_size = 3;
					}
					break;
				}
			}
			if (i >= sizeof(boot_service_table) / sizeof(boot_service_table[0]))
			{
				// service not supported
				buf_decrypt[1] = buf_decrypt[0];
				buf_decrypt[0] = 0x7F;
				buf_decrypt[2] = 0x11;
				tx_size = 3;
			}
			if (tx_size > 0)
			{
				build_crypt_msg(buf_decrypt, tx_size, buf_crypt, &cryptLen, CPYPT_MASK);
				memcpy(p_rx_data, buf_crypt, cryptLen);
				FreeRTOS_sendto(sock, p_rx_data, cryptLen, FREERTOS_ZERO_COPY, &addr_remote, NULL, NULL);
			}
		}
		else
		{
			// nothing recved
			boot_service_data_init(&svc_state);
		}
		if (rx_size >= 0)
		{
			/* The buffer *must* be freed once it is no longer needed. */
			FreeRTOS_ReleaseUDPPayloadBuffer(p_rx_data);
		}
		if (svc_state.reset_req)
		{
			svc_state.reset_req = 0;
			vTaskDelay(200);
			SystemSoftwareReset();
		}
	}
}

void app_init(void)
{
	xTaskCreate( boot_main_task, "boot_main", 4096, NULL, 4, NULL );
	xTaskCreate( erase_routine_task, "boot_routine", 2048, NULL, 3, NULL );
	vTaskStartScheduler();
}
