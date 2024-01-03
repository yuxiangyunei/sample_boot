/*
 * FreeRTOS+TCP V2.0.11
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_DNS.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

#include "enet_driver.h"
#include "phy.h"

#define ETH_INSTANCE (0)
#define ETH_USED_RING_CNT (1)

#define PHY_ADDRESS (1)

#define ETH_RXBUFNB (8)
#define ETH_TXBUFNB (8)

#define ETH_RX_BUF_SIZE (ENET_BUFF_ALIGN(ENET_FRAME_MAX_FRAMELEN))
#define ETH_TX_BUF_SIZE (ENET_BUFF_ALIGN(ENET_FRAME_MAX_FRAMELEN))

#define niEMAC_HANDLER_TASK_PRIORITY (configMAX_PRIORITIES - 1)

/* Default the size of the stack used by the EMAC deferred handler task to twice
the size of the stack used by the idle task - but allow this to be overridden in
FreeRTOSConfig.h as configMINIMAL_STACK_SIZE is a user definable constant. */
#define configEMAC_TASK_STACK_SIZE (2 * configMINIMAL_STACK_SIZE)

//#define ipconfigUSE_RMII 1
/*-----------------------------------------------------------*/

/*
 * A deferred interrupt handler task that processes
 */
static void prvEMACHandlerTask(void *pvParameters);

/*
 * See if there is a new packet and forward it to the IP-task.
 */
static void prvNetworkInterfaceInput(void);

/* After packets have been sent, the network
	buffers will be released. */
//static void vClearTXBuffers(void);

/*-----------------------------------------------------------*/

NOINIT_DATA_SECTION static uint32_t RND_Seed; // noinit data to keep the random initial value

/* Ethernet handle. */
NOINIT_DATA_SECTION static enet_config_t xETH_Config;
NOINIT_DATA_SECTION static enet_state_t xETH_State;
NOINIT_DATA_SECTION static enet_buffer_config_t xETH_Buffer_Config[ETH_USED_RING_CNT];

static SemaphoreHandle_t xTxMutexLock = NULL;
/*
 * Note: it is adviced to define both
 *
 *     #define  ipconfigZERO_COPY_RX_DRIVER   1
 *     #define  ipconfigZERO_COPY_TX_DRIVER   1
 *
 * The method using memcpy is slower and probaly uses more RAM memory.
 * The possibility is left in the code just for comparison.
 *
 * It is adviced to define ETH_TXBUFNB at least 4. Note that no
 * TX buffers are allocated in a zero-copy driver.
 */
/* MAC buffers: ---------------------------------------------------------*/

/* Put the DMA descriptors in '.first_data'.
This is important for STM32F7, which has an L1 data cache.
The first 64KB of the SRAM is not cached. */

/* Ethernet Rx MA Descriptor */
ALIGNED(FEATURE_ENET_BUFFDESCR_ALIGNMENT)
NOINIT_DATA_SECTION static enet_buffer_descriptor_t DMARxDscrTab[ETH_RXBUFNB];
ALIGNED(FEATURE_ENET_BUFFDESCR_ALIGNMENT)
NOINIT_DATA_SECTION static enet_buffer_descriptor_t DMATxDscrTab[ETH_TXBUFNB];

/* Holds the handle of the task used as a deferred interrupt processor.  The
handle is used so direct notifications can be sent to the task for all EMAC/DMA
related interrupts. */
static TaskHandle_t xEMACTaskHandle = NULL;

//static enet_buffer_descriptor_t *xTxBufferDescQueue[ETH_TXBUFNB+1];
//static int xTxBufferDescQueueReadIdx = 0;
//static int xTxBufferDescQueueWriteIdx = 0;

uint32_t ENET_RxFrameCnt = 0;
uint32_t ENET_DroppedFrameCnt = 0;
uint32_t ENET_ProcessedFrameCnt = 0;
uint32_t ENET_LostFrameCnt = 0;
uint32_t ENET_TxFrameCnt = 0;
uint32_t ENET_TxFailFrameCnt = 0;

const unsigned char filter_vci_mac_sddr[4] = {0x22, 0x33, 0x44, 0x55};
const unsigned char default_ptp_mac_addr[6] = {0x01, 0x00, 0x5e, 0x00, 0x01, 0x81};
const unsigned char peer_ptp_mac_addr[6] = {0x01, 0x00, 0x5e, 0x00, 0x00, 0x6b};

void __attribute__((weak)) EthTxBufferFreeHook(enet_buffer_descriptor_t *bd)
{
}

void __attribute__((weak)) EthTxBufferOutHook(enet_buffer_t *buff)
{
}

static uint8_t *EthBufferAlloc(int size)
{
	uint8_t *ret = NULL;
	NetworkBufferDescriptor_t *desc = NULL;
	const TickType_t xDescriptorWaitTime = pdMS_TO_TICKS(250);
	desc = pxGetNetworkBufferWithDescriptor(size, xDescriptorWaitTime);
	if (desc != NULL)
	{
		ret = desc->pucEthernetBuffer;
	}
	return ret;
}

static void EthBufferFree(uint8_t *data)
{
	NetworkBufferDescriptor_t *desc;
	if (data != NULL)
	{
		desc = pxPacketBuffer_to_NetworkBuffer(data);
		if (desc != NULL)
		{
			vReleaseNetworkBufferAndDescriptor(desc);
		}
	}
}

void HAL_ETH_RxCpltCallback(uint8_t instance, enet_event_t event, uint8_t ring)
{
	(void)(instance);
	(void)(ring);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xTaskNotifyFromISR(xEMACTaskHandle, 1u<<event, eSetBits, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*-----------------------------------------------------------*/
//TODO:


static void vClearCurrentTxBuffer(void)
{
	enet_buffer_descriptor_t *bd = ENET_DRV_GetCurrentTxBuffDesc(ETH_INSTANCE, 0);
	if (ENET_DRV_GetTxBuffDescStatus(bd) == STATUS_SUCCESS)
	{
		EthTxBufferFreeHook(bd);
		EthBufferFree(bd->buffer);
		bd->buffer = NULL;
	}
}
/*
static void vClearTXBuffers(void)
{
	enet_buffer_descriptor_t *bd;
	status_t sts;
	while (xTxBufferDescQueueReadIdx != xTxBufferDescQueueWriteIdx)
	{
#if (ipconfigZERO_COPY_TX_DRIVER != 0)
		bd = xTxBufferDescQueue[xTxBufferDescQueueReadIdx];
		sts = ENET_DRV_GetTxBuffDescStatus(bd);
		if (sts == STATUS_SUCCESS)
		{
			EthBufferFree(bd->buffer);
			bd->buffer = NULL;
		}
		else if (sts == STATUS_BUSY)
		{
			break;
		}
		else
		{
			//should never here
		}
#endif
		xTxBufferDescQueueReadIdx = (xTxBufferDescQueueReadIdx + 1) % (sizeof(xTxBufferDescQueue) / sizeof(xTxBufferDescQueue[0]));
	}
}
*/
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceInitialise(void)
{
	if (xEMACTaskHandle == NULL)
	{
		if (xTxMutexLock == NULL)
		{
			xTxMutexLock = xSemaphoreCreateMutex();
			configASSERT(xTxMutexLock);
		}

		/* Initialise ETH */
		xETH_Config.interrupts = ENET_RX_FRAME_INTERRUPT;
		xETH_Config.maxFrameLen = ENET_FRAME_MAX_FRAMELEN;
		xETH_Config.rxAccelerConfig = ENET_RX_ACCEL_ENABLE_MAC_CHECK;
		xETH_Config.txAccelerConfig = 0;//ENET_TX_ACCEL_INSERT_IP_CHECKSUM;// | ENET_TX_ACCEL_INSERT_PROTO_CHECKSUM;
		xETH_Config.callback = HAL_ETH_RxCpltCallback;
		xETH_Config.miiSpeed = ENET_MII_SPEED_100M;
		xETH_Config.miiDuplex = ENET_MII_FULL_DUPLEX;
		xETH_Config.ringCount = ETH_USED_RING_CNT;
		xETH_Config.rxConfig = ENET_RX_CONFIG_STRIP_CRC_FIELD;
		xETH_Config.txConfig = 0;
#if (ipconfigUSE_RMII != 0)
		{
			xETH_Config.miiMode = ENET_RMII_MODE;
		}
#else
		{
			xETH_Config.miiMode = ENET_MII_MODE;
		}
#endif /* ipconfigUSE_RMII */
		xETH_Buffer_Config[0].rxRingSize = ETH_RXBUFNB;
		xETH_Buffer_Config[0].txRingSize = ETH_TXBUFNB;
		xETH_Buffer_Config[0].rxRingAligned = DMARxDscrTab;
		xETH_Buffer_Config[0].txRingAligned = DMATxDscrTab;
		xETH_Buffer_Config[0].rxBufferAligned = NULL;
		xETH_Buffer_Config[0].rxBufferAllocator = EthBufferAlloc;

		ENET_DRV_Init(ETH_INSTANCE, &xETH_State, &xETH_Config, xETH_Buffer_Config, FreeRTOS_GetMACAddress());
		ENET_DRV_SetMulticastForward(ETH_INSTANCE, default_ptp_mac_addr, true);
		ENET_DRV_SetMulticastForward(ETH_INSTANCE, peer_ptp_mac_addr, true);
		ENET_DRV_EnableMDIO(ETH_INSTANCE, false);
		xTaskCreate(prvEMACHandlerTask, "EMAC", configEMAC_TASK_STACK_SIZE, NULL, niEMAC_HANDLER_TASK_PRIORITY, &xEMACTaskHandle);
	} /* if( xEMACTaskHandle == NULL ) */
	return pdPASS;
}

BaseType_t xNetworkInterfaceOutput(NetworkBufferDescriptor_t *const pxDescriptor, BaseType_t bReleaseAfterSend)
{
	BaseType_t xReturn = pdFAIL;
	enet_buffer_t buff;
	buff.length = pxDescriptor->xDataLength;
	buff.data = pxDescriptor->pucEthernetBuffer;
	xSemaphoreTake(xTxMutexLock, portMAX_DELAY);
	vClearCurrentTxBuffer();
	//vClearTXBuffers();
	//xTxBufferDescQueue[xTxBufferDescQueueWriteIdx] = ENET_DRV_GetCurrentTxBuffDesc(ETH_INSTANCE, 0);
	EthTxBufferOutHook(&buff);
	if (STATUS_SUCCESS == ENET_DRV_SendFrame(ETH_INSTANCE, 0, &buff, NULL))
	{
		bReleaseAfterSend = pdFALSE;
		xReturn = pdPASS;
		//xTxBufferDescQueueWriteIdx = (xTxBufferDescQueueWriteIdx + 1) % (sizeof(xTxBufferDescQueue) / sizeof(xTxBufferDescQueue[0]));
	}
	else
	{
		bReleaseAfterSend = pdTRUE;
	}
	xSemaphoreGive(xTxMutexLock);
	if (bReleaseAfterSend != pdFALSE)
	{
		vReleaseNetworkBufferAndDescriptor(pxDescriptor);
	}
	return xReturn;
}
/*-----------------------------------------------------------*/

void prvNetworkInterfaceInput(void)
{
	uint8_t instance = ETH_INSTANCE;
	uint8_t ring = 0;
	status_t sts;
	enet_buffer_t buff;
	enet_rx_enh_info_t info;
	NetworkBufferDescriptor_t * nb_rcvd;
	NetworkBufferDescriptor_t * nb_new;
	uint8_t *tmp_p;
	xIPStackEvent_t rx_event;
	sts = ENET_DRV_ReadFrame(instance, ring, &buff, &info);
	while (sts == STATUS_SUCCESS)
	{
		ENET_RxFrameCnt++;
		if ((buff.length > ipSIZE_OF_ETH_HEADER) && (buff.length <= ipTOTAL_ETHERNET_FRAME_SIZE))
		{
			if (eConsiderFrameForProcessing(buff.data) == eProcessBuffer)
			{
				nb_new = pxGetNetworkBufferWithDescriptor(ETH_RX_BUF_SIZE, 0);
				if (nb_new != NULL)
				{
					tmp_p = buff.data;
					buff.data = nb_new->pucEthernetBuffer;
					nb_rcvd = pxPacketBuffer_to_NetworkBuffer(tmp_p);
					nb_rcvd->xDataLength = buff.length;
					rx_event.eEventType = eNetworkRxEvent;
					rx_event.pvData = ( void * ) nb_rcvd;
					if( xSendEventStructToIPTask( &rx_event, 0 ) == pdFALSE )
					{
						vReleaseNetworkBufferAndDescriptor(nb_rcvd);
						ENET_LostFrameCnt++;
						iptraceETHERNET_RX_EVENT_LOST();
					}
					else
					{
						ENET_ProcessedFrameCnt++;
						iptraceNETWORK_INTERFACE_RECEIVE();
					}
				}
				else
				{
					ENET_LostFrameCnt++;
					iptraceETHERNET_RX_EVENT_LOST();
				}
			}
			else
			{
				ENET_DroppedFrameCnt++;
			}
		}
		else
		{
			ENET_DroppedFrameCnt++;
		}
		ENET_DRV_ProvideRxBuff(instance, ring, &buff);
		sts = ENET_DRV_ReadFrame(instance, ring, &buff, &info);
	}
}

/* Uncomment this in case BufferAllocation_1.c is used. */

#define niBUFFER_1_PACKET_SIZE (1536+64)

void vNetworkInterfaceAllocateRAMToBuffers(NetworkBufferDescriptor_t pxNetworkBuffers[ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS])
{
	ALIGNED(FEATURE_ENET_BUFF_ALIGNMENT) NOINIT_DATA_SECTION static uint8_t ucNetworkPackets[ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS * niBUFFER_1_PACKET_SIZE];
	uint8_t *ucRAMBuffer = ucNetworkPackets;
	uint32_t ul;

	for (ul = 0; ul < ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS; ul++)
	{
		pxNetworkBuffers[ul].pucEthernetBuffer = ucRAMBuffer + ipBUFFER_PADDING;
		*((unsigned *)ucRAMBuffer) = (unsigned)(&(pxNetworkBuffers[ul]));
		ucRAMBuffer += niBUFFER_1_PACKET_SIZE;
	}
}
/*-----------------------------------------------------------*/

static void prvEMACHandlerTask(void *pvParameters)
{
	uint32_t flags;
	const TickType_t ulMaxBlockTime = portMAX_DELAY;//pdMS_TO_TICKS(100UL);
	/* Remove compiler warnings about unused parameters. */
	(void)pvParameters;
	for (;;)
	{
		//if (pdTRUE == xTaskNotifyWait( 0x00, 0xFFFFFFFF, &flags, ulMaxBlockTime ))
		xTaskNotifyWait( 0x00, 0xFFFFFFFF, &flags, ulMaxBlockTime );
		{
			RND_Seed ^= xTaskGetTickCount();
			//if (flags & )
			//{
			prvNetworkInterfaceInput();
			//}
			//else
			//{
			//}
		}
	}
}

static uint32_t LFSR32(uint32_t reg, uint32_t mask, uint16_t time)
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

uint32_t uxRand( void )
{
	uint32_t ret = LFSR32(RND_Seed, 0x80000057U, (31+((uint8_t)RND_Seed & 0x0F)));
	RND_Seed ^= ret;
	return ret;
}

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
	(void)(eNetworkEvent);
}

uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
													uint16_t usSourcePort,
													uint32_t ulDestinationAddress,
													uint16_t usDestinationPort )
{
	uint32_t ret = xTaskGetTickCount();
	return (ret ^ ulSourceAddress ^ ulDestinationAddress ^ usDestinationPort ^ usSourcePort);
}
/*-----------------------------------------------------------*/

//void ETH_IRQHandler(void)
//{
//	HAL_ETH_IRQHandler(&xETH);
//}
