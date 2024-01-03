/*rc4.c */
#include "rc4.h"

void rc4_init_key(uint8_t key[16], rc4_key *rc4_ctx)
{
	uint8_t index1;
	uint8_t index2;
	uint8_t tmp;
	uint8_t* state;
	uint16_t counter;
	rc4_memcpy(rc4_ctx->key, key, sizeof(rc4_ctx->key));
	rc4_ctx->step = 0;
	state = &rc4_ctx->state[0];
	for (counter = 0; counter < 256; counter++)
	{
		state[counter] = counter;
	}
	rc4_ctx->x = 0;
	rc4_ctx->y = 0;
	index1 = 0;
	index2 = 0;
	for (counter = 0; counter < 256; counter++)
	{
		index2 = ((key[index1] + state[counter] + index2) & 0xFF);
		tmp = state[counter];
		state[counter] = state[index2];
		state[index2] = tmp;
		index1 = ((index1 + 1) & (RC4_KEY_SIZE - 1));
	}
}

void rc4(uint8_t* in_buffer_ptr, uint8_t* out_buffer_ptr, uint32_t buffer_len, rc4_key *rc4_ctx)
{
	uint8_t x;
	uint8_t y;
	uint8_t* state;
	uint8_t xorIndex;
	uint8_t tmp;
	uint32_t counter;
	x = rc4_ctx->x;
	y = rc4_ctx->y;
	state = &rc4_ctx->state[0];
	for(counter = 0; counter < buffer_len; counter ++)
	{
		x = ((x + 1) & 0xFF);
		y = ((state[x] + y) & 0xFF);
		tmp = state[x];
		state[x] = state[y];
		state[y] = tmp;
		if ((out_buffer_ptr != NULL) && (in_buffer_ptr != NULL))
		{
			xorIndex = ((state[x] + state[y]) & 0xFF);
			out_buffer_ptr[counter] = (state[xorIndex] ^ in_buffer_ptr[counter]);
		}
		rc4_ctx->step ++;
	}
	rc4_ctx->x = x;
	rc4_ctx->y = y;
}


