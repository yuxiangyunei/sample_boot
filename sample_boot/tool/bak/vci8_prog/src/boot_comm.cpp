#include <stdio.h>
#include <string.h>
#include "boot_comm.h"
#include "crc32.h"

#define LFSR_TAP_MASK (0x80000057U)
uint32_t LFSR32(uint32_t reg, uint32_t mask, uint16_t time)
{
	uint16_t tmp;
	uint16_t i;
	for (i=0; i<time; i++)
	{
		tmp = ((uint16_t)(((uint32_t)(reg & mask)) >> 16)) ^ ((uint16_t)(reg & mask));
		tmp = (tmp >> 8) ^ (tmp & 0xFF);
		tmp = (tmp >> 4) ^ (tmp & 0xF);
		tmp = (tmp >> 2) ^ (tmp & 0x3);
		tmp = (tmp >> 1) ^ (tmp & 1);
		reg = (reg << 1) | (uint32_t)tmp;
	}
	return reg;
}

SOCKET boot_sock_init(void)
{
#ifdef WIN32
    WSADATA ws_data;//这个结构被用来存储被WSAStartup函数调用后返回的Windows Sockets数据
#endif
    SOCKET ret = INVALID_SOCKET;
	//struct timeval timeOut;
#ifdef WIN32
    if(WSAStartup(MAKEWORD(2,2), &ws_data) == 0)
#endif
    {
		ret = socket(AF_INET, SOCK_DGRAM, 0);
		if (ret != INVALID_SOCKET)
		{
			//timeOut.tv_sec = 5;
			//timeOut.tv_usec = 0;
			//setsockopt(ret, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeOut, sizeof(timeOut));
		}
	}
	return ret;
}

void boot_sock_deinit(SOCKET sock)
{
#ifdef WIN32
    closesocket(sock);
    WSACleanup();
#else
	close(sock);
#endif
}

/*
void print_hex(uint8_t *data, int len)
{
	int i;
	for (i=0; i<len; i++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");
}
*/

int boot_req(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t *req, int req_len, uint8_t *resp_buf, int resp_buf_size)
{
    int ret;
    struct sockaddr_in my_addr;
#ifdef WIN32
    int addr_len;
#else
	socklen_t addr_len;
#endif
	ret = sendto(sock, (const char *)req, req_len, 0, (struct sockaddr *)remote_addr, sizeof(*remote_addr));
	if (req_len == ret)
	{
		if ((resp_buf != NULL) && (resp_buf_size > 0))
		{
            addr_len = sizeof(my_addr);
            ret = recvfrom(sock, (char *)resp_buf, resp_buf_size, 0, (struct sockaddr *)&my_addr, &addr_len);
			/*
			if (ret > 0)
			{
				print_hex(resp_buf, ret);
			}
			else
			{
				printf("ret = %d\n", ret);
			}
			*/
        }
	}
	else
    {
        ret = -1;
    }
    return ret;
}

int enter_boot_req(SOCKET sock, struct sockaddr_in *remote_addr)
{
	int ret;
	uint8_t buf[5];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(8183);
	buf[0] = 0x00;
	buf[1] = 0x03;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0xFB;
	ret = sendto(sock, (const char *)buf, 5, 0, (struct sockaddr *)remote_addr, sizeof(*remote_addr));
	return ret;
}
int enter_session(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t session)
{
	int ret;
	uint8_t sid = 0x10;
	uint8_t buf[16];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	buf[0] = sid;
	buf[1] = session;
	ret = boot_req(sock, remote_addr, buf, 2, buf, sizeof(buf));
	if ((ret >= 2) && (buf[0] == 0x40 + sid))
	{
		ret = 0;
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}

int security_access(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t level)
{
	int ret;
	uint8_t sid = 0x27;
	uint8_t buf[16];
	uint32_t seed, key;
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	buf[0] = sid;
	buf[1] = level;
	ret = boot_req(sock, remote_addr, buf, 2, buf, sizeof(buf));
	if ((ret >= 6) && (buf[0] == 0x40 + sid) && (buf[1] == level))
	{
		seed = (((uint32_t)buf[2] << 24) | ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | ((uint32_t)buf[5]));
		key = LFSR32((seed ^ 0x20191028), LFSR_TAP_MASK, 16);
		buf[0] = sid;
		buf[1] = level + 1;
		buf[2] = (uint8_t)(key >> 24);
		buf[3] = (uint8_t)(key >> 16);
		buf[4] = (uint8_t)(key >> 8);
		buf[5] = (uint8_t)(key);
		ret = boot_req(sock, remote_addr, buf, 6, buf, sizeof(buf));
		if ((ret >= 2) && (buf[0] == 0x40 + sid) && (buf[1] == level + 1))
		{
			ret = 0;
		}
		else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
		{
			ret = buf[2];
		}
		else
		{
			ret = -2;
		}
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}

int erase_flash_memory(SOCKET sock, struct sockaddr_in *remote_addr, uint32_t addr, uint32_t size)
{
	int ret;
	uint8_t sid = 0x31;
	uint8_t buf[16];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	buf[0] = sid;
	buf[1] = 0x01;
	buf[2] = 0xFF;
	buf[3] = 0x00;
	buf[4] = (uint8_t)(addr >> 24);
	buf[5] = (uint8_t)(addr >> 16);
	buf[6] = (uint8_t)(addr >> 8);
	buf[7] = (uint8_t)(addr);
	buf[8] = (uint8_t)(size >> 24);
	buf[9] = (uint8_t)(size >> 16);
	buf[10] = (uint8_t)(size >> 8);
	buf[11] = (uint8_t)(size);
	ret = boot_req(sock, remote_addr, buf, 12, buf, sizeof(buf));
	if ((ret == 4) && (buf[0] == 0x40 + sid) && (buf[1] == 0x01) && (buf[2] == 0xFF) && (buf[3] == 0x00))
	{
		while(1)
		{
			buf[0] = sid;
			buf[1] = 0x03;
			buf[2] = 0xFF;
			buf[3] = 0x00;
			ret = boot_req(sock, remote_addr, buf, 4, buf, sizeof(buf));
			if ((ret == 5) && (buf[0] == 0x40+sid) &&  (buf[1] == 0x03) && (buf[2] == 0xFF) && (buf[3] == 0x00))
			{
				if (buf[4] == 0x00)
				{
					DelayMs(500);
				}
				else if (buf[4] == 0x01)
				{
					ret = 0;
					break;
				}
				else
				{
					ret = -3;
					break;
				}
			}
			else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
			{
				ret = buf[2];
				break;
			}
			else
			{
				ret = -2;
				break;
			}
		}
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}

int read_data_by_id(SOCKET sock, struct sockaddr_in *remote_addr, uint16_t id, uint8_t *data, uint8_t data_size)
{
	int ret;
	uint8_t sid = 0x22;
	uint8_t buf[16];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	buf[0] = sid;
	buf[1] = (uint8_t)(id >> 8);
	buf[2] = (uint8_t)id;
	ret = boot_req(sock, remote_addr, buf, 3, buf, sizeof(buf));
	if ((ret > 3) && (buf[0] == 0x40 + sid) && (buf[1] == (uint8_t)(id >> 8)) && (buf[2] == (uint8_t)id))
	{
		data_size = (data_size > (ret - 3)) ? (ret - 3) : data_size;
		memcpy(data, &buf[3], data_size);
		ret = 0;
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}
int write_data_by_id(SOCKET sock, struct sockaddr_in *remote_addr, uint16_t id, uint8_t *data, uint8_t data_len)
{
	int ret;
	uint8_t sid = 0x2E;
	uint8_t buf[16];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	data_len = (data_len > (sizeof(buf) - 3)) ? ((sizeof(buf)) - 3) : data_len;
	buf[0] = sid;
	buf[1] = (uint8_t)(id >> 8);
	buf[2] = (uint8_t)id;
	memcpy(&buf[3], data, data_len);
	ret = boot_req(sock, remote_addr, buf, 3 + data_len, buf, sizeof(buf));
	if ((ret == 3) && (buf[0] == 0x40 + sid) && (buf[1] == (uint8_t)(id >> 8)) && (buf[2] == (uint8_t)id))
	{
		ret = 0;
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}

int download_data(SOCKET sock, struct sockaddr_in *remote_addr, uint32_t addr, uint32_t size, uint8_t *data, uint32_t *crc, uint8_t enc_enable)
{
	int ret;
	uint8_t buf[1026];
	uint8_t sn, enc_flag;
	uint32_t i, tx_len;
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	if (enc_enable)
	{
		enc_flag = 0x80;
	}
	else
	{
		enc_flag = 0x00;
	}
	buf[0] = 0x34;
	buf[1] = (0x44 | enc_flag);
	buf[2] = (uint8_t)(addr >> 24);
	buf[3] = (uint8_t)(addr >> 16);
	buf[4] = (uint8_t)(addr >> 8);
	buf[5] = (uint8_t)(addr);
	buf[6] = (uint8_t)(size >> 24);
	buf[7] = (uint8_t)(size >> 16);
	buf[8] = (uint8_t)(size >> 8);
	buf[9] = (uint8_t)(size);
	ret = boot_req(sock, remote_addr, buf, 10, buf, sizeof(buf));
	if ((ret >= 1) && (buf[0] == 0x74))
	{
		sn = 1;
		i = 0;
		ret = 0;
		while (i < size)
		{
			tx_len = size - i;
			if (tx_len > sizeof(buf) - 2)
			{
				tx_len = sizeof(buf) - 2;
			}
			buf[0] = 0x36;
			buf[1] = sn;
			memcpy(&buf[2], &data[i], tx_len);
			if (enc_flag == 0x00)
			{
				*crc = crc32(*crc, &data[i], tx_len);
			}
			ret = boot_req(sock, remote_addr, buf, tx_len + 2, buf, sizeof(buf));
			if ((ret == 2) && (buf[0] == 0x76) && (buf[1] == sn))
			{
				i += tx_len;
				++sn;
				ret = 0;
			}
			else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == 0x36))
			{
				ret = buf[2];
				break;
			}
			else
			{
				ret = -2;
				break;
			}
		}

	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == 0x34))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}

int exit_download_data(SOCKET sock, struct sockaddr_in *remote_addr)
{
	int ret;
	uint8_t sid = 0x37;
	uint8_t buf[16];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	buf[0] = sid;
	ret = boot_req(sock, remote_addr, buf, 1, buf, sizeof(buf));
	if ((ret >= 1) && (buf[0] == 0x40 + sid))
	{
		ret = 0;
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}

int data_checksum_validate(SOCKET sock, struct sockaddr_in *remote_addr, uint32_t chksum)
{
	int ret;
	uint8_t sid = 0x31;
	uint8_t buf[16];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	buf[0] = sid;
	buf[1] = 0x01;
	buf[2] = 0xFF;
	buf[3] = 0x01;
	buf[4] = (uint8_t)(chksum >> 24);
	buf[5] = (uint8_t)(chksum >> 16);
	buf[6] = (uint8_t)(chksum >> 8);
	buf[7] = (uint8_t)(chksum);
	ret = boot_req(sock, remote_addr, buf, 8, buf, sizeof(buf));
	if ((ret == 4) && (buf[0] == 0x40 + sid) && (buf[1] == 0x01) && (buf[2] == 0xFF) && (buf[3] == 0x01))
	{
		while(1)
		{
			buf[0] = sid;
			buf[1] = 0x03;
			buf[2] = 0xFF;
			buf[3] = 0x01;
			ret = boot_req(sock, remote_addr, buf, 4, buf, sizeof(buf));
			if ((ret == 5) && (buf[0] == 0x40 + sid) &&  (buf[1] == 0x03) && (buf[2] == 0xFF) && (buf[3] == 0x01))
			{
				if (buf[4] == 0x00)
				{
					DelayMs(500);
				}
				else if (buf[4] == 0x01)
				{
					ret = 0;
					break;
				}
				else
				{
					ret = -3;
					break;
				}
			}
			else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
			{
				ret = buf[2];
				break;
			}
			else
			{
				ret = -2;
				break;
			}
		}
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}

int reset_device(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t mode)
{
	int ret;
	uint8_t sid = 0x11;
	uint8_t buf[16];
	remote_addr->sin_family = AF_INET;
	remote_addr->sin_port = htons(14229);
	buf[0] = sid;
	buf[1] = mode;
	ret = boot_req(sock, remote_addr, buf, 2, buf, sizeof(buf));
	if ((ret == 2) && (buf[0] == 0x40 + sid) && (buf[1] == mode))
	{
		ret = 0;
	}
	else if ((ret == 3) && (buf[0] == 0x7F) && (buf[1] == sid))
	{
		ret = buf[2];
	}
	else
	{
		ret = -1;
	}
	return ret;
}