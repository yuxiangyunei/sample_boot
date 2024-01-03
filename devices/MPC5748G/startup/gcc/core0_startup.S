#define START _start
#define BSP_CORE 1
#define DISABLE_SWT0 1
#define MAIN main
#define __SP_INIT __SP_INIT0
#undef I_CACHE
#undef D_CACHE


#include "startup_MPC5748G.S"
