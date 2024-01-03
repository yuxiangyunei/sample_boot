#ifndef RND_H
#define RND_H
/*******************************************************************************
*    File Name:          rnd.h
*
*    $Rev: $
*
*    $Date: $
*
*    Description:        This file contains implementation of the random number generator
*                        module in HF01BL project.
*
*    Classification:     ShiningView Confidential     
*
*    (c) Copyright ShiningView Electronics Technology (SH) Co., Ltd. 2012
*    All rights reserved.
*******************************************************************************/

/*******************************************************************************
*	Include Files
*******************************************************************************/
#define LFSR_TAP_MASK (0x80000057U)

void Srnd(unsigned int seed);
unsigned int Rnd(void);
unsigned int LFSR32(unsigned int reg, unsigned int mask, unsigned short time);


/*****************************************************************************
* Revision history:
*
* Date			SCR No. 		 Author Name	   Description
*
*****************************************************************************/
#endif

