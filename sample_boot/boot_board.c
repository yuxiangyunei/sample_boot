/*
 * hal.c
 *
 *  Created on: 2019��2��14��
 *      Author: Fred
 */
#include "boot_board.h"
//#include "Cpu.h"
#include "drivers.h"
#include "clockMan.h"
#include "pin_mux.h"
#include "hwio.h"

void board_hw_init(void)
{
	//status_t ret;
	//1. Clock
	CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT, g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
	CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);
	//SEMA42_DRV_Init(0);
	//2. Pin Mux
	PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr);
	//3. CAN - be initialized by can.c
	//4. LIN - be initialized by lin.c
	//5. ADC
	//6. Eth - be initialized in FreeRTOS-TCP stack.
	//7. EEE
}

void can_set_transciever_mode(unsigned char channel, bool power_enable, bool trans_enable, bool stbn_enable)
{
	static const DioIdxType pwr_en[8] = {PM_EN0, PM_EN1, PM_EN2, PM_EN3, PM_EN4, PM_EN5, PM_EN6, PM_EN7};
	static const DioIdxType en[8] = {CAN_EN0, CAN_EN1, CAN_EN2, CAN_EN3, CAN_EN4, CAN_EN5, CAN_EN6, CAN_EN7};
	static const DioIdxType stbn[8] = {CAN_STBN0, CAN_STBN1, CAN_STBN2, CAN_STBN3, CAN_STB4, CAN_STB5, CAN_STB6, CAN_STB7};
	if (channel < 8)
	{
		DioWrite(pwr_en[channel], power_enable);
		DioWrite(en[channel], trans_enable ? 0 : 1); //inverted logic by circuit
		DioWrite(stbn[channel], stbn_enable ? 1 : 0); // always normal, not sleep
	}
}
unsigned char id_pin_read(void)
{
	unsigned char ret;
	DioRead(ID_CHECK, &ret);
	return ret;
}

unsigned char hw_rev_pin_read(void)
{
	// pu = 1 and pd = 0 : hw rev 3.0
	// pu = 1 and pd = 1 : hw rev 3.1
	unsigned char pu, pd, ret;
	DioRead(SYNC_PU, &pu);
	DioRead(SYNC_PD, &pd);
	ret = ((pu | (pd << 1)) & 0x03);
	switch (ret)
	{
	case 0x01:
		ret = 0x30;
		break;
	case 0x03:
		ret = 0x31;
		break;
	default:
		ret = 0x30;
		break;
	}
	return ret;
}
