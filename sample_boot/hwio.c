/*
 * hwio.c
 *
 *  Created on: 2020��1��13��
 *      Author: fredl
 */
#include "drivers.h"
#include "hwio.h"
#include "rtos.h"

typedef struct
{
	dspi_instance_t spi_instance;
	void *tx_buff;
	void *rx_buff;
	uint16_t data_buff_size;
	GPIO_Type *cs_port;
	uint16_t cs_pin_mask;
	uint8_t cs_active_state;
	uint8_t xfer_req;
	mutex_t mutex;
	dspi_master_config_t *cfg;
	dspi_state_t *drv_state;
} SpiCfgType;

typedef struct
{
	uint8_t type; // b0 - pin di; b1 - pin do; b2 - spi di; b3 - spi do
	SpiCfgType *spi_cfg;
	void *gpio; // if dio is gpio, this is a pointer to GPIO_Type, if it is a spi dio, it is a pointer to spi buffer
	uint16_t pin_mask;
} DioCfgType;

static uint8_t spi_io_data_buff[3] = {0xFF, 0x07, 0xFF};
static uint8_t spi_rx_buff[3];
static const dspi_master_config_t spi_595_cfg = {
	.bitsPerSec = 10000000,
	.pcsPolarity = DSPI_ACTIVE_HIGH,
	.bitcount = 8,
	.clkPhase = DSPI_CLOCK_PHASE_2ND_EDGE,
	.clkPolarity = DSPI_ACTIVE_LOW,
	.lsbFirst = false,
	.transferType = DSPI_USING_INTERRUPTS,
	.rxDMAChannel = 255U,
	.txDMAChannel = 255U,
	.callback = NULL,
	.callbackParam = NULL,
	.continuousPCS = false,
	.whichPCS = 0,
	.core = 0};

NOINIT_DATA_SECTION static dspi_state_t spi_state[2];

static SpiCfgType SpiIoConfig[] =
	{
		{SPI0_INSTANCE, &spi_io_data_buff[0], &spi_rx_buff[0], 1, PTG, (1u << 11), 0, 1, NULL, &spi_595_cfg, &spi_state[0]},
		{SPI3_INSTANCE, &spi_io_data_buff[1], &spi_rx_buff[1], 2, PTH, (1u << 5), 0, 1, NULL, &spi_595_cfg, &spi_state[1]}};

static const DioCfgType DioConfig[] =
	{
		{0x02, NULL, PTA, (1u << 0)},  //CAN_EN0
		{0x02, NULL, PTA, (1u << 1)},  //CAN_EN1
		{0x02, NULL, PTA, (1u << 2)},  //CAN_EN2
		{0x02, NULL, PTA, (1u << 3)},  //CAN_EN3
		{0x02, NULL, PTA, (1u << 4)},  //CAN_STBN0
		{0x02, NULL, PTA, (1u << 5)},  //CAN_STBN1
		{0x02, NULL, PTA, (1u << 6)},  //CAN_STBN2
		{0x02, NULL, PTA, (1u << 7)},  //CAN_STBN3
		{0x01, NULL, PTB, (1u << 4)},  //ID_CHECK
		{0x02, NULL, PTB, (1u << 12)}, //GPO_0 NOT USED
		{0x02, NULL, PTB, (1u << 13)}, //GPO_1 NOT USED
		{0x02, NULL, PTB, (1u << 14)}, //GPO_2 NOT USED
		{0x02, NULL, PTB, (1u << 15)}, //GPO_3 NOT USED
		{0x01, NULL, PTE, (1u << 12)}, //ENET_INTn NOT USED
		{0x02, NULL, PTE, (1u << 13)}, //ENET_RST NOT USED
		{0x02, NULL, PTE, (1u << 14)}, //MCU_OFF NOT USED
		{0x02, NULL, PTE, (1u << 15)}, //MCU_TRIG NOT USED
		{0x02, NULL, PTF, (1u << 2)},  //CAN_STB4
		{0x02, NULL, PTF, (1u << 3)},  //CAN_STB5
		{0x02, NULL, PTF, (1u << 4)},  //CAN_STB6 NOT USED
		{0x02, NULL, PTF, (1u << 5)},  //CAN_STB7 NOT USED
		{0x01, NULL, PTH, (1u << 7)},  //GPI_0 NOT USED
		{0x01, NULL, PTH, (1u << 8)},  //GPI_1 NOT USED
		{0x01, NULL, PTH, (1u << 12)}, //GPI_2 NOT USED
		{0x01, NULL, PTH, (1u << 13)}, //GPI_3 NOT USED
		{0x02, NULL, PTH, (1u << 15)}, //TRA_LED NOT USED
		{0x02, NULL, PTI, (1u << 4)},  //CAN_EN4
		{0x02, NULL, PTI, (1u << 5)},  //CAN_EN5
		{0x02, NULL, PTI, (1u << 6)},  //CAN_EN6 NOT USED
		{0x02, NULL, PTI, (1u << 7)},  //CAN_EN7 NOT USED
		{0x02, NULL, PTI, (1u << 12)}, //FRA_EN NOT USED
		{0x02, NULL, PTI, (1u << 13)}, //FRA_STBN NOT USED
		{0x02, NULL, PTI, (1u << 14)}, //FRB_EN NOT USED
		{0x02, NULL, PTI, (1u << 15)}, //FRB_STBN NOT USED
		{0x02, NULL, PTJ, (1u << 1)},  //GPO_4 NOT USED
		{0x02, NULL, PTJ, (1u << 2)},  //GPO_5 NOT USED
		{0x01, NULL, PTJ, (1u << 3)},  //SYNC_PU
		{0x01, NULL, PTJ, (1u << 4)},  //SYNC_PD

		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 0)}, //LIN_SLP0
		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 1)}, //LIN_SLP1
		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 2)}, //LIN_SLP2
		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 3)}, //LIN_SLP3
		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 4)}, //LIN_SLP4
		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 5)}, //LIN_SLP5
		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 6)}, //LIN_SLP6
		{0x08, &SpiIoConfig[0], &spi_io_data_buff[0], (1u << 7)}, //LIN_SLP7

		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 0)}, //PM_EN0
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 1)}, //PM_EN1
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 2)}, //PM_EN2
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 3)}, //PM_EN3
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 4)}, //PM_EN4
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 5)}, //PM_EN5
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 6)}, //PM_EN6
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[2], (1u << 7)}, //PM_EN7
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[1], (1u << 0)}, //FRA_EN
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[1], (1u << 1)}, //FRB_EN
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[1], (1u << 2)}, //LOCAL_WU_EN
		{0x08, &SpiIoConfig[1], &spi_io_data_buff[1], (1u << 3)}, //SOC_OFF
		{0x02, NULL, PTF, (1u << 1)},							  //IO_WUP_EN NOT USED
};

HwIoStatus SpiTransfer(SpiCfgType *spi)
{
	//static unsigned char first_transfer = 1;
	HwIoStatus ret = HWIO_INVALID_PARAM;
	dspi_transfer_status_t spi_sts;
	if ((spi != NULL) && (spi->mutex != NULL))
	{
		if (spi->xfer_req)
		{
			if (STATUS_SUCCESS == OSIF_MutexLock(&spi->mutex, OSIF_WAIT_FOREVER))
			{

				if (spi->cs_active_state)
				{
					PINS_DRV_SetPins(spi->cs_port, spi->cs_pin_mask);
				}
				else
				{
					PINS_DRV_ClearPins(spi->cs_port, spi->cs_pin_mask);
				}
				if (STATUS_SUCCESS == DSPI_MasterTransfer(spi->spi_instance, spi->tx_buff, spi->rx_buff, spi->data_buff_size))
				{
					do
					{
						DSPI_GetTransferStatus(spi->spi_instance, &spi_sts);
					} while (spi_sts == DSPI_IN_PROGRESS);
					if (spi_sts == DSPI_TRANSFER_OK)
					{
						spi->xfer_req = 0;
						ret = HWIO_STS_OK;
					}
					else
					{
						ret = HWIO_ERROR_IO;
					}
				}
				else
				{
					ret = HWIO_ERROR_IO;
				}
				PINS_DRV_TogglePins(spi->cs_port, spi->cs_pin_mask);
				OSIF_MutexUnlock(&spi->mutex);
			}
			else
			{
				ret = HWIO_ERROR_OS;
			}
		}
		else
		{
			ret = HWIO_ERROR_NOT_REQ;
		}
	}
	return ret;
}

int SpiInit(void)
{
	unsigned int i;
	status_t rv;
	for (i = 0; i < sizeof(SpiIoConfig) / sizeof(SpiIoConfig[0]); i++)
	{
		if (SpiIoConfig[i].cs_active_state)
		{
			PINS_DRV_ClearPins(SpiIoConfig[i].cs_port, SpiIoConfig[i].cs_pin_mask);
		}
		else
		{
			PINS_DRV_SetPins(SpiIoConfig[i].cs_port, SpiIoConfig[i].cs_pin_mask);
		}
		rv = DSPI_MasterInit(SpiIoConfig[i].spi_instance, SpiIoConfig[i].drv_state, SpiIoConfig[i].cfg);
		if (rv == STATUS_SUCCESS)
		{
			rv = OSIF_MutexCreate(&SpiIoConfig[i].mutex);
			if (rv != STATUS_SUCCESS)
			{
				i = -2;
				break;
			}
			else
			{
				SpiTransfer(&SpiIoConfig[i]);
			}
		}
		else
		{
			i = -1;
			break;
		}
	}
	return i;
}
void SpiMainFunction(void)
{
	unsigned int i;
	for (i = 0; i < sizeof(SpiIoConfig) / sizeof(SpiIoConfig[0]); i++)
	{
		SpiTransfer(&SpiIoConfig[i]);
	}
}

HwIoStatus DioWrite(DioIdxType idx, uint8_t value)
{
	HwIoStatus ret = HWIO_INVALID_PARAM;
	//uint8_t old_val;
	if (idx < sizeof(DioConfig) / sizeof(DioConfig[0]))
	{
		if (DioConfig[idx].type & 0x02)
		{
			if (value)
			{
				PINS_DRV_SetPins((GPIO_Type *)DioConfig[idx].gpio, DioConfig[idx].pin_mask);
			}
			else
			{
				PINS_DRV_ClearPins((GPIO_Type *)DioConfig[idx].gpio, DioConfig[idx].pin_mask);
			}
			ret = HWIO_STS_OK;
		}
		else if (DioConfig[idx].type & 0x08)
		{
			OSIF_MutexLock(&DioConfig[idx].spi_cfg->mutex, OSIF_WAIT_FOREVER);
			if (value)
			{
				*((uint8_t *)DioConfig[idx].gpio) |= ((uint8_t)DioConfig[idx].pin_mask);
			}
			else
			{
				*((uint8_t *)DioConfig[idx].gpio) &= ((uint8_t)(~DioConfig[idx].pin_mask));
			}
			DioConfig[idx].spi_cfg->xfer_req = 1;
			OSIF_MutexUnlock(&DioConfig[idx].spi_cfg->mutex);
			ret = HWIO_STS_OK;
		}
		else
		{
			// do nothing
		}
	}
	return ret;
}

HwIoStatus DioRead(DioIdxType idx, uint8_t *value)
{
	HwIoStatus ret = HWIO_INVALID_PARAM;
	pins_channel_type_t tmp;
	if ((idx < sizeof(DioConfig) / sizeof(DioConfig[0])) && (value != NULL))
	{
		if (DioConfig[idx].type & 0x02)
		{
			tmp = PINS_DRV_GetPinsOutput(DioConfig[idx].gpio);
			if (tmp & DioConfig[idx].pin_mask)
			{
				*value = 1;
			}
			else
			{
				*value = 0;
			}
			ret = HWIO_STS_OK;
		}
		else if (DioConfig[idx].type & 0x01)
		{
			tmp = PINS_DRV_ReadPins(DioConfig[idx].gpio);
			if (tmp & DioConfig[idx].pin_mask)
			{
				*value = 1;
			}
			else
			{
				*value = 0;
			}
			ret = HWIO_STS_OK;
		}
		else if (DioConfig[idx].type & 0x0C)
		{
			*value = (*((uint8_t *)DioConfig[idx].gpio) & (uint8_t)DioConfig[idx].pin_mask) ? 1 : 0;
			ret = HWIO_STS_OK;
		}
		else
		{
			// do nothing
		}
	}
	return ret;
}
