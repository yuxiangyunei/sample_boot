#ifndef RC4_H
#define RC_H
#include <stdint.h>
#include <string.h>
//#include "rtos.h"

#define RC4_KEY_SIZE (16) /* shall be 2^n */
#define rc4_memcmp memcmp
#define rc4_memcpy memcpy


typedef struct rc4_key 
{  
  uint8_t key[RC4_KEY_SIZE];
  uint8_t x;
  uint8_t y;
  uint32_t step;
  uint8_t state[256];
} rc4_key;

void rc4_init_key(uint8_t key[16], rc4_key *rc4_ctx);
void rc4(uint8_t* in_buffer_ptr, uint8_t* out_buffer_ptr, uint32_t buffer_len, rc4_key *key); 

#endif
