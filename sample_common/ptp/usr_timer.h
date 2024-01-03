#ifndef TIME_H
#define TIME_H
#include <stdint.h>

#define USE_IN_IRQ_FLAG (1u)
#define USE_OUT_IRQ_FLAG (0u)

void setPTPUsrTime(const int32_t second, const int32_t nanoSecond);
void getPTPUsrTime(int32_t *const second, int32_t *const nanoSecond);
void PTPTimerInit(void);
void updatePTPTimerOffest(int32_t adj);

uint8_t ptp_check(void);
uint32_t vPortGetPTPTimeStampSec(uint8_t *ptp_flag, uint8_t irq_flag);
uint64_t vPortGetPTPTimeStampMilliSec(uint8_t *ptp_flag, uint8_t irq_flag);
uint64_t vPortGetPTPTimeStampMicroSec(uint8_t *ptp_flag, uint8_t irq_flag);
uint64_t vPortGetPTPTimeStampNanoSec(uint8_t *ptp_flag, uint8_t irq_flag);

#endif
