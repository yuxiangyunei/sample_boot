/*
 * hwio.h
 *
 *  Created on: 2020��1��13��
 *      Author: fredl
 */

#ifndef HWIO_H_
#define HWIO_H_
#include <stdint.h>

typedef enum
{
	HWIO_STS_OK = 0,
	HWIO_INVALID_PARAM,
	HWIO_ERROR_OS,
	HWIO_ERROR_IO,
	HWIO_ERROR_NOT_REQ,
} HwIoStatus;

typedef enum
{
	CAN_EN0 = 0,
	CAN_EN1,
	CAN_EN2,
	CAN_EN3,
	CAN_STBN0,
	CAN_STBN1,
	CAN_STBN2,
	CAN_STBN3,
	ID_CHECK,
	GPO_0,
	GPO_1,
	GPO_2,
	GPO_3,
	ENET_INTn,
	ENET_RST,
	MCU_OFF,
	MCU_TRIG,
	CAN_STB4,
	CAN_STB5,
	CAN_STB6,
	CAN_STB7,
	GPI_0,
	GPI_1,
	GPI_2,
	GPI_3,
	TRA_LED,
	CAN_EN4,
	CAN_EN5,
	CAN_EN6,
	CAN_EN7,
	FRA_EN,
	FRA_STBN,
	FRB_EN,
	FRB_STBN,
	GPO_4,
	GPO_5,
	SYNC_PU,
	SYNC_PD,
	LIN_SLP0,
	LIN_SLP1,
	LIN_SLP2,
	LIN_SLP3,
	LIN_SLP4,
	LIN_SLP5,
	LIN_SLP6,
	LIN_SLP7,
	PM_EN0,
	PM_EN1,
	PM_EN2,
	PM_EN3,
	PM_EN4,
	PM_EN5,
	PM_EN6,
	PM_EN7,
	FRA_WU_EN,
	FRB_WU_EN,
	LOCAL_WU_EN,
	SOC_OFF,
	IO_WUP_EN
} DioIdxType;

HwIoStatus DioWrite(DioIdxType idx, uint8_t value);
HwIoStatus DioRead(DioIdxType idx, uint8_t *value);

int SpiInit(void);
void SpiMainFunction(void);

#endif /* HWIO_H_ */
