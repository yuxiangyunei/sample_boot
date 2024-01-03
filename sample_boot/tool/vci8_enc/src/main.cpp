#include <stdio.h>
#include "SRecMem.h"
#include "crc32.h"
#include "rc4.h"

#define ERASE_APP_FLASH_START (0x01001000)
#define ERASE_APP_FLLASH_SIZE (5564 * 1024)

int main(int argc, char *argv[])
{
	unsigned int i;
	unsigned char *p;
	SRecordMem srec;
	uint32_t crc, addr, size, count;
	rc4_key rc4_ctx;
	uint8_t enc_key[16] = {'k','U','n','Y','i','@','V','a','R','v','C','i',0x20,0x19,0x10,0x28};
	uint8_t header_buf[8];
	unsigned int seg_num;
	if (argc != 3)
	{
		printf("USAGE: %s input_hex_file output_srec_file\n", argv[0]);
	}
	else
	{
		if (true == srec.ParseFile(argv[1]))
		{
			if (0 == srec.GetData(0x00000000, 8, header_buf, 0xFF))
			{
				count = 0;
				crc = 0xFFFFFFFF;
				seg_num = srec.GetSegmentNumber();
				for (i = 0; i < seg_num; i++)
				{
					srec.GetSegmentInfo(i, &addr, &size);
					if ((addr >= ERASE_APP_FLASH_START) && (size != 0) && (size <= ERASE_APP_FLLASH_SIZE) && (addr + size <= ERASE_APP_FLASH_START + ERASE_APP_FLLASH_SIZE))
					{
						p = srec.GetSegmentDataPointer(i, &size);
						count += size;
						crc = crc32(crc, p, size);
					}
					else
					{
						// ignore memory data
					}
				}
				header_buf[0] = (uint8_t)(count >> 24);
				header_buf[1] = (uint8_t)(count >> 16);
				header_buf[2] = (uint8_t)(count >> 8);
				header_buf[3] = (uint8_t)(count);
				header_buf[4] = (uint8_t)(crc >> 24);
				header_buf[5] = (uint8_t)(crc >> 16);
				header_buf[6] = (uint8_t)(crc >> 8);
				header_buf[7] = (uint8_t)(crc);
				for (i = 0; i < sizeof(enc_key); i++)
				{
					enc_key[i] ^= header_buf[(i & 0x07)];
				}
				rc4_init_key(enc_key, &rc4_ctx);
				for (i = 0; i < seg_num; i++)
				{
					srec.GetSegmentInfo(i, &addr, &size);
					if ((addr >= ERASE_APP_FLASH_START) && (size != 0) && (size <= ERASE_APP_FLLASH_SIZE) && (addr + size <= ERASE_APP_FLASH_START + ERASE_APP_FLLASH_SIZE))
					{
						p = srec.GetSegmentDataPointer(i, &size);
						rc4(p, p, size, &rc4_ctx);
					}
					else
					{
						// ignore memory data
					}
				}
				srec.AddSegment(0x00000000, header_buf, 8);
				if (!srec.WriteFile(argv[2]))
				{
					printf("write file %s fail.\n", argv[2]);
				}
			}
			else
			{
				printf("invalid or encrypted hex file.\n");
			}
			
		}
		else
		{
			printf("open file %s fail.\n", argv[1]);
		}
	}
	return 0;
}
