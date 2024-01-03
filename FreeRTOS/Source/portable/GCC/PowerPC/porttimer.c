/*
 * Copyright 2018-2019 NXP
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdint.h>
#include "device_registers.h"
#include "FreeRTOSConfig.h"
#include "interrupt_manager.h"

/* PowerPC specific: pit channel to use 0-15 */
#define configUSE_PIT_CHANNEL (3u)
#define configUSE_SS0_CHANNEL (SS0_IRQn)

/* functions required by port.c */
extern void prvPortTimerSetup(void *paramF, uint32_t coreId, uint32_t tick_interval);
extern void prvPortTimerReset(uint32_t coreId);

/* Workaround for MPC574xP platforms where PIT is PIT_0 defined in header */
#if defined(PIT_0)
#define PIT PIT_0
#endif

void prvPortTimerSetup(void *paramF, uint32_t coreId, uint32_t tick_interval)
{
	switch (coreId)
	{
	case 0U:
		DEV_ASSERT(configUSE_PIT_CHANNEL < PIT_TIMER_COUNT);
		DEV_ASSERT(paramF != NULL);

		/* check if the timer is already running. If it is, we cannot use it. */
		DEV_ASSERT((PIT->TIMER[configUSE_PIT_CHANNEL].TCTRL & PIT_TCTRL_TEN_MASK) == 0u);
		static const IRQn_Type pitIrqId[PIT_INSTANCE_COUNT][PIT_IRQS_CH_COUNT] = PIT_IRQS;
		INT_SYS_InstallHandler(pitIrqId[0U][configUSE_PIT_CHANNEL], (isr_t)paramF, NULL);
		INT_SYS_DisableIRQ_MC_All(pitIrqId[0U][configUSE_PIT_CHANNEL]);
		INT_SYS_DisableIRQ_MC_All(configUSE_SS0_CHANNEL);

		INT_SYS_EnableIRQ(pitIrqId[0U][configUSE_PIT_CHANNEL]);
		INT_SYS_SetPriority(pitIrqId[0U][configUSE_PIT_CHANNEL], 1);
#if FEATURE_OSIF_PIT_FRZ_IN_DEBUG
		//PIT->MCR |= PIT_MCR_FRZ(1u);   /* stop the timer in debug */
#endif								   /* FEATURE_OSIF_PIT_FRZ_IN_DEBUG */
		PIT->MCR &= ~PIT_MCR_MDIS(1u); /* enable the timer */
		/* PIT_LDVAL : tick period */
		PIT->TIMER[configUSE_PIT_CHANNEL].LDVAL = tick_interval;
		/* PIT_RTI_TCTRL: start channel, enable IRQ */
		PIT->TIMER[configUSE_PIT_CHANNEL].TCTRL = PIT_TCTRL_TEN(1u) | PIT_TCTRL_TIE(1u);

#if ((configUSE_PIT_CHANNEL != 0) && (configUSE_PIT_CHANNEL != 1))
		DEV_ASSERT((PIT->TIMER[0].TCTRL & PIT_TCTRL_TEN_MASK) == 0u);
		DEV_ASSERT((PIT->TIMER[1].TCTRL & PIT_TCTRL_TEN_MASK) == 0u);
		PIT->TIMER[1].LDVAL = 0xFFFFFFFF;
		PIT->TIMER[0].LDVAL = 0xFFFFFFFF;
		PIT->TIMER[1].TCTRL = PIT_TCTRL_TEN(1u) | PIT_TCTRL_CHN(1);
		PIT->TIMER[0].TCTRL = PIT_TCTRL_TEN(1u);
#else
#error configUSE_PIT_CHANNEL cannot be 0 or 1, these channels are used as timestamp timer
#endif
		break;
	case 2U:
		INT_SYS_InstallHandler(configUSE_SS0_CHANNEL, (isr_t)paramF, NULL);
		INT_SYS_EnableIRQ(configUSE_SS0_CHANNEL);
		INT_SYS_SetPriority(configUSE_SS0_CHANNEL, 1);
		break;
	default:
		break;
	}
}

void prvPortTimerReset(uint32_t coreId)
{
	switch (coreId)
	{
	case 0U:
		/* clear PIT channel IRQ flag */
		PIT->TIMER[configUSE_PIT_CHANNEL].TFLG = PIT_TFLG_TIF(1u);
		INTC->SSCIR[configUSE_SS0_CHANNEL] = INTC_SSCIR_SET_MASK;
	case 2U:
		/* Clear the interrupt */
		INTC->SSCIR[configUSE_SS0_CHANNEL] = INTC_SSCIR_CLR_MASK;
		break;
	default:
		break;
	}
}

uint32_t vPortGetTimeStampSec(void)
{
	uint64_t ret;
	ret = (((uint64_t)(0xFFFFFFFF - PIT->LTMR64H)) << 32);
	ret += (0xFFFFFFFF - PIT->LTMR64L);
	ret /= (configCPU_CLOCK_HZ);
	return ((uint32_t)ret);
}

uint32_t vPortGetTimeStampMilliSec(void)
{
	uint64_t ret;
	ret = (((uint64_t)(0xFFFFFFFF - PIT->LTMR64H)) << 32);
	ret += (0xFFFFFFFF - PIT->LTMR64L);
	ret /= (configCPU_CLOCK_HZ / 1000);
	return ((uint32_t)ret);
}

uint64_t vPortGetTimeStampMicroSec(void)
{
	uint64_t ret;
	ret = (((uint64_t)(0xFFFFFFFF - PIT->LTMR64H)) << 32);
	ret += (0xFFFFFFFF - PIT->LTMR64L);
	ret /= (configCPU_CLOCK_HZ / 1000000);
	return ret;
}

uint64_t vPortGetTimeStampNanoSec(void)
{
	uint64_t ret;
	ret = (((uint64_t)(0xFFFFFFFF - PIT->LTMR64H)) << 32);
	ret += (0xFFFFFFFF - PIT->LTMR64L);
	ret = ret * (1000000000UL / configCPU_CLOCK_HZ);
	return ret;
}
