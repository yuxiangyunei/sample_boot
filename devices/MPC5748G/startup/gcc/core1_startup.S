#if defined(TURN_ON_CPU1)
#define START _startcore1
#define MAIN main1
#define __SP_INIT __SP_INIT1

#undef DISABLE_SWT0
#undef START_FROM_FLASH
#undef I_CACHE
#undef D_CACHE


#include "startup_MPC5748G.S"
#endif