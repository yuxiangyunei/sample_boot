/*******************************************************************************
*    File Name:          rnd.c
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
#include "rnd.h"

static unsigned int RND_Seed = 0x12345678;

unsigned int LFSR32(unsigned int reg, unsigned int mask, unsigned short time)
{
	unsigned short tmp;
	unsigned short i;
	for (i=0; i<time; i++)
	{
		tmp = ((unsigned short)(((unsigned int)(reg & mask)) >> 16)) ^ ((unsigned short)(reg & mask));
		tmp = (tmp >> 8) ^ (tmp & 0xFF);
		tmp = (tmp >> 4) ^ (tmp & 0xF);
		tmp = (tmp >> 2) ^ (tmp & 0x3);
		tmp = (tmp >> 1) ^ (tmp & 1);
		reg = (reg << 1) | (unsigned int)tmp;
	}
	return reg;
}

void Srnd(unsigned int seed)
{
	RND_Seed ^= LFSR32(seed, LFSR_TAP_MASK, 1);
}

unsigned int Rnd(void)
{
	unsigned int ret;
	//characteristic polynomial: x^32 + x^7 + x^5 + x^ 3  x^2 + x^1 + 1
	//tap mask = 0x80000057
	ret = LFSR32(RND_Seed, LFSR_TAP_MASK, (31 + (RND_Seed & 0x0F)));
	RND_Seed ^= ret;
	return (ret);
}

/*****************************************************************************
* Revision history:
*
* Date			SCR No. 		 Author Name	   Description
*
*****************************************************************************/

