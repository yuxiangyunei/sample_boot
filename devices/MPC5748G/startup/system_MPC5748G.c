/*
** ###################################################################
**     Processor:           MPC5748G
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright (c) 2015 Freescale Semiconductor, Inc.
**     Copyright 2016-2019 NXP
**     All rights reserved.
**
**     THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
**     IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
**     OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**     IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
**     INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
**     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
**     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
**     STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
**     IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
**     THE POSSIBILITY OF SUCH DAMAGE.
**
**
** ###################################################################
*/

/**
 * @page misra_violations MISRA-C:2012 violations
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 11.4, Conversion between a pointer and
 * integer type.
 * The cast is required to initialize a pointer with an unsigned int define,
 * representing a memory-mapped address.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.7, External could be made static.
 * Function is defined for usage by application code.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 11.1, Conversions shall not be performed
 * between a pointer to a function and any other type.
 * This is required in order to write the prefix of the interrupt vector table.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.9, An object should be defined at block
 * scope if its identifier only appears in a single function.
 * All variables with this problem are defined in the linker files.
 *
 * @section [global]
 * Violates MISRA 2012 Required Rule 11.6, Cast from pointer to unsigned int.
 * The cast is required to initialize a pointer with an unsigned int define,
 * representing a memory-mapped address.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.7, External could be made static.
 * Function is defined for usage by application code.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 2.5, local macro not referenced.
 * KEY_VALUE1 and KEY_VALUE2 are used for enabling cores.
 *
 */

/*!
 * @file MPC5748G
 * @version 1.0
 * @date 2017-02-14
 * @brief Device specific configuration file for MPC5748G (implementation file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "system_MPC5748G.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/
/*!
 * @brief Defines the init table layout
 */
typedef struct
{
    uint8_t *rom_start; /*!< Start address of section in ROM */
    uint8_t *ram_start; /*!< Start address of section in RAM */
    uint8_t *rom_end;   /*!< End address of section in ROM */
} copy_layout_t;

/*!
 * @brief Defines the zero table layout
 */
typedef struct
{
    uint8_t *ram_start; /*!< Start address of section in RAM */
    uint8_t *ram_end;   /*!< End address of section in RAM */
} zero_layout_t;

extern uint32_t __COPY_TABLE[];
extern uint32_t __ZERO_TABLE[];
/*========================================================================*/
/*                      EXTERNAL PROTOTYPES                               */
/*========================================================================*/
extern void VTABLE(void);
extern uint32_t __VECTOR_RAM[(uint32_t)(FEATURE_INTERRUPT_IRQ_MAX) + 1U];
static volatile uint32_t *const s_vectors[NUMBER_OF_CORES] = FEATURE_INTERRUPT_INT_VECTORS;
/*========================================================================*/
/*                          FUNCTIONS                                     */
/*========================================================================*/
/**************************************************************************/
/* FUNCTION     : SetIVPR                                                 */
/* PURPOSE      : Initialise Core IVPR                                    */
/**************************************************************************/
void SetIVPR(register unsigned int x)
{
    __asm__("mtIVPR %0 \n\t" ::"r"(x));
}

/**************************************************************************/
/* FUNCTION     : InitINTC                                                */
/* PURPOSE      : This function intializes the INTC for software vector   */
/*                mode.                                                   */
/**************************************************************************/
void InitINTC(void)
{
    const copy_layout_t *copy_layout;
    const zero_layout_t *zero_layout;
    const uint8_t *rom;
    uint8_t *ram;
    uint16_t coreId = GET_CORE_ID();
    uint32_t len = 0U;
    uint32_t size = 0U;
    uint32_t i = 0U;
    uint32_t j = 0U;
    const uint32_t *initTable = (uint32_t *)__COPY_TABLE;
    const uint32_t *zeroTable = (uint32_t *)__ZERO_TABLE;

    switch (coreId)
    {
    case 0U:
        /* Copy initialized table */
        len = *initTable;
        copy_layout = (copy_layout_t *)(initTable + 1U);
        for (i = 0; i < len; i++)
        {
            rom = copy_layout[i].rom_start;
            ram = copy_layout[i].ram_start;
            size = (uint32_t)copy_layout[i].rom_end - (uint32_t)copy_layout[i].rom_start;

            for (j = 0UL; j < size; j++)
            {
                ram[j] = rom[j];
            }
        }

        /* Clear zero table */
        len = *zeroTable;
        zero_layout = (zero_layout_t *)(zeroTable + 1U);
        for (i = 0; i < len; i++)
        {
            ram = zero_layout[i].ram_start;
            size = (uint32_t)zero_layout[i].ram_end - (uint32_t)zero_layout[i].ram_start;

            for (j = 0UL; j < size; j++)
            {
                ram[j] = 0U;
            }
        }
        /* Software vector mode used for core 0 */
        INTC->BCR &= ~(INTC_BCR_HVEN0_MASK);

        /**************************************************************************/
        /* GRANT ACCESS TO PERIPHERALS FOR DMA MASTER */
        /**************************************************************************/
#if ENABLE_DMA_ACCESS_TO_PERIPH
        /* DMA trusted for read/writes in supervisor & user modes on peripheral bridge A */
        AIPS_A->MPRA |= AIPS_MPRA_MTW4_MASK;
        AIPS_A->MPRA |= AIPS_MPRA_MTR4_MASK;
        AIPS_A->MPRA |= AIPS_MPRA_MPL4_MASK;
        /* DMA trusted for read/writes in supervisor & user modes on peripheral bridge B */
        AIPS_B->MPRA |= AIPS_MPRA_MPL4_MASK;
        AIPS_B->MPRA |= AIPS_MPRA_MTW4_MASK;
        AIPS_B->MPRA |= AIPS_MPRA_MTR4_MASK;
        AXBS_0->PORT[6].PRS = 0x70654321; /* PBridge 0:    gives highest priority to DMA */
        AXBS_0->PORT[5].PRS = 0x70654321; /* PBridge 1:    gives highest priority to DMA */
        AXBS_0->PORT[2].PRS = 0x70654321; /* Flash Port 2: gives highest priority to DMA */
#endif
        break;
    case 1U:
        /* Software vector mode used for core 1 */
        INTC->BCR &= ~(INTC_BCR_HVEN1_MASK);
        break;
    case 2U:
        /* Software vector mode used for core 2 */
        INTC->BCR &= ~(INTC_BCR_HVEN2_MASK);
        break;
    default:
        /* invalid core number */
        DEV_ASSERT(false);
        break;
    }
    *s_vectors[coreId] = (uint32_t)__VECTOR_RAM;
}

/**************************************************************************/
/* FUNCTION     : enableIrq                                               */
/* PURPOSE      : This function sets INTC's current priority to 0.        */
/*                External interrupts to the core are enabled.            */
/**************************************************************************/
void enableIrq(void)
{
    uint16_t coreId = GET_CORE_ID();
    switch (coreId)
    {
    case 0U:
        /* Lower core 0's INTC current priority to 0 */
        INTC->CPR0 = 0U;
        break;
    case 1U:
        /* Lower core 1's INTC current priority to 0 */
        INTC->CPR1 = 0U;
        break;
    case 2U:
        /* Lower core 2's INTC current priority to 1 */
        INTC->CPR2 = 0U;
        break;
    default:
        /* invalid core number */
        DEV_ASSERT(false);
        break;
    }
    /* Enable external interrupts */
    __asm__(" wrteei 1");
}

/**************************************************************************/
/* FUNCTION     : xcptn_xmpl                                              */
/* PURPOSE      : This function sets up the necessary functions to raise  */
/*                and handle a Interrupt in software vector mode          */
/**************************************************************************/
void xcptn_xmpl(void (*vtable)(void))
{

    /* Initialise Core IVPR */
    SetIVPR((unsigned int)vtable);

    /* Initialize INTC for SW vector mode */
    InitINTC();

    /* Enable interrupts */
    enableIrq();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SystemSoftwareReset
 * Description   : This function is used to initiate a 'functional' reset event
 * to the microcontroller. The reset module will do a state machine from
 * PHASE1->PHASE2->PHASE3->IDLE.
 *
 * Implements    : SystemSoftwareReset_Activity
 *END**************************************************************************/
void SystemSoftwareReset(void)
{
    MC_ME->MCTL = FEATURE_MC_ME_KEY;
    MC_ME->MCTL = MC_ME_MCTL_TARGET_MODE(0x00) | FEATURE_MC_ME_KEY_INV;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
