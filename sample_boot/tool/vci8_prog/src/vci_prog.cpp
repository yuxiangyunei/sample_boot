#include "vci_prog.h"
#include "boot_comm.h"
#include "SRecMem.h"

#define ERASE_APP_FLASH_START (0x01001000)
#define ERASE_APP_FLLASH_SIZE (5564 * 1024)

static void calculate_flash_address_range(SRecordMem &srec, uint32_t *start, uint32_t *end, uint32_t *actual_total_size)
{
	unsigned int i;
	uint32_t start_addr, end_addr, tmp, size, total_size;
	*start = 0;
	*end = 0;
	start_addr = 0xFFFFFFFF;
	end_addr = 0;
    total_size = 0;
	for (i=0; i<srec.GetSegmentNumber(); i++)
	{
		if (srec.GetSegmentInfo(i, &tmp, &size))
		{
			if ((tmp >= ERASE_APP_FLASH_START) && (size <= ERASE_APP_FLLASH_SIZE))
			{
                total_size += size;
				if (tmp < start_addr)
				{
					start_addr = tmp;
				}
				tmp += (size - 1);
				if (tmp > end_addr)
				{
					end_addr = tmp;
				}
			}
		}
	}
	*start = start_addr;
	*end = end_addr;
    *actual_total_size = total_size;
}
int vci_prog(char *ip_addr, char *file_name, vci_prog_callback_t callback)
{
	int ret;
	unsigned int i;
	SOCKET sock;
	SRecordMem srec;
	uint32_t crc, addr, size, total_size, progress;
	struct sockaddr_in vci_addr;
	uint8_t enc_header[8];
	uint8_t enc_enable;
	if ((ip_addr != NULL) && (file_name != NULL))
	{
		if (true == srec.ParseFile(file_name))
		{
			sock = boot_sock_init();
			if (sock != INVALID_SOCKET)
			{
#ifdef WIN32
				vci_addr.sin_addr.S_un.S_addr = inet_addr(ip_addr);
#else
				vci_addr.sin_addr.s_addr = inet_addr(ip_addr);
#endif
				if (5 == enter_boot_req(sock, &vci_addr))
				{
					printf("Enter boot request OK.\n");
					DelayMs(1000); // delay for waiting MCU reset
					ret = enter_session(sock, &vci_addr, 0x02);
					if (0 == ret)
					{
						ret = security_access(sock, &vci_addr, 0x01);
						if (0 == ret)
						{
							if (8 == srec.GetData(0x00000000, 8, enc_header, 0xFF))
							{
								enc_enable = 1;
								crc = (((uint32_t)enc_header[4] << 24) | ((uint32_t)enc_header[5] << 16) | ((uint32_t)enc_header[6] << 8) | ((uint32_t)enc_header[7]));
							}
							else
							{
								enc_enable = 0;
								crc = 0xFFFFFFFF;
							}
							if (enc_enable)
							{
								ret = write_data_by_id(sock, &vci_addr, 0x0000, enc_header, sizeof(enc_header));
							}
							else
							{
								ret = 0;
							}
							if (0 == ret)
							{
								calculate_flash_address_range(srec, &addr, &size, &total_size);
								size = size - addr + 1;
								ret = erase_flash_memory(sock, &vci_addr, addr, size);
								if (0 == ret)
								{
                                    progress = 0;
									for (i = 0; i < srec.GetSegmentNumber(); i++)
									{
										if (srec.GetSegmentInfo(i, &addr, &size))
										{
											if ((addr >= ERASE_APP_FLASH_START) && (size != 0) && (size <= ERASE_APP_FLLASH_SIZE) && (addr + size <= ERASE_APP_FLASH_START + ERASE_APP_FLLASH_SIZE))
											{
												ret = download_data(sock, &vci_addr, addr, size, srec.GetSegmentDataPointer(i, &size), &crc, enc_enable);
												if (0 != ret)
												{
													ret = VCI_PROG_ERR_DOWNLOAD_DATA_FAIL;
													break;
												}
												else
												{
                                                    if (callback != NULL)
                                                    {
                                                        progress += size;
                                                        callback(total_size, progress);
                                                    }
												}
											}
											//else
											//{
												// ignore memory data
											//}
										}
										else
										{
											ret = VCI_PROG_ERR_READ_SREC_FAIL;
											printf("read srecord fail.\n");
											break;
										}
									}
									if (0 == ret)
									{
										ret = exit_download_data(sock, &vci_addr);
										if (0 == ret)
										{
											printf("Exit Download OK.\n");
											ret = data_checksum_validate(sock, &vci_addr, crc);
											if (0 == ret)
											{
												printf("CRC Validate OK.\n");
												ret = reset_device(sock, &vci_addr, 0x01);
												if (0 == ret)
												{
													printf("VCI8 Programe Complete Successfully.\n");
												}
												else
												{
													printf("Reset device fail. 0x%X\n", ret);
													ret = VCI_PROG_ERR_RESET_DEVICE_FAIL;
												}
											}
											else
											{
												printf("Checksum validate fail. 0x%X\n", ret);
												ret = VCI_PROG_ERR_CHECKSUM_VALIDATE_FAIL;
											}
										}
										else
										{
											printf("Exit download fail. 0x%X\n", ret);
											ret = VCI_PROG_ERR_EXIT_DOWNLOAD_FAIL;
										}
									}
								}
								else
								{
									printf("Erase flash memory fail. 0x%X\n", ret);
									ret = VCI_PROG_ERR_ERASE_MEMORY_FAIL;
								}
							}
							else
							{
								printf("Write enc key fail. 0x%X\n", ret);
								ret = VCI_PROG_ERR_WRITE_ENC_KEY_FAIL;
							}
						}
						else
						{
							printf("Security Access fail. 0x%X\n", ret);
							ret = VCI_PROG_ERR_SEC_ACCESS_FAIL;
						}
					}
					else
					{
						printf("Enter prog session fail. 0x%X\n", ret);
						ret = VCI_PROG_ERR_ENTER_PROG_SESSION_FAIL;
					}
				}
				else
				{
					ret = VCI_PROG_ERR_ENTER_BOOT_FAIL;
					printf("Enter boot fail.\n");
				}

				boot_sock_deinit(sock);
			}
			else
			{
				ret = VCI_PROG_ERR_OPEN_SOCKET_FAIL;
				//printf("Open socket error.\n");
			}
		}
		else
		{
			ret = VCI_PROG_ERR_OPEN_FILE_FAIL;
			//printf("Open file %s fail.\n", argv[2]);
		}
	}
	else
	{
		ret = VCI_PROG_ERR_INVALID_ARG;
	}
	return ret;
}