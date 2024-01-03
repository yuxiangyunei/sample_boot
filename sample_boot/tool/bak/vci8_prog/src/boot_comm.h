#ifndef BOOT_COMM_H
#define BOOT_COMM_H

#include <stdint.h>
#ifdef WIN32
#include <winsock2.h>
#define DelayMs(ms) Sleep(ms)
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define INVALID_SOCKET (-1)
typedef int SOCKET;
#define DelayMs(ms) usleep((ms)*1000)
#endif
int boot_req(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t *req, int req_len, uint8_t *resp_buf, int resp_buf_size);

SOCKET boot_sock_init(void);
void boot_sock_deinit(SOCKET sock);

int enter_boot_req(SOCKET sock, struct sockaddr_in *remote_addr);
int enter_session(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t session);
int security_access(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t level);
int erase_flash_memory(SOCKET sock, struct sockaddr_in *remote_addr, uint32_t addr, uint32_t size);
int download_data(SOCKET sock, struct sockaddr_in *remote_addr, uint32_t addr, uint32_t size, uint8_t *data, uint32_t *crc, uint8_t enc_enable);
int exit_download_data(SOCKET sock, struct sockaddr_in *remote_addr);
int data_checksum_validate(SOCKET sock, struct sockaddr_in *remote_addr, uint32_t chksum);
int reset_device(SOCKET sock, struct sockaddr_in *remote_addr, uint8_t mode);
int write_data_by_id(SOCKET sock, struct sockaddr_in *remote_addr, uint16_t id, uint8_t *data, uint8_t data_len);
int read_data_by_id(SOCKET sock, struct sockaddr_in *remote_addr, uint16_t id, uint8_t *data, uint8_t data_size);

#endif
