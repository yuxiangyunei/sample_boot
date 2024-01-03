#include "usr_timer.h"
#include "rtos.h"
#include "stdlib.h"
#include "drivers.h"
#include "portable.h"

#define PTP_ENET_INSTANCE 0
#define S_TO_NS_COUNT (1000000000UL)
#define ENET_CLOCK_FRE (40000000UL)
#define NS_INC_IN_TICK (S_TO_NS_COUNT / ENET_CLOCK_FRE)

#define ADJUST_BASE_TIMES_VALUE (50000000UL)
#define ADJUST_INC_PERIOD_VALUE (1u)

#define PTP_USE_PIT_CHANNEL_IRQn (PIT_Ch5_IRQn)
#define PTP_USE_PIT_CHANNEL (5u)

static volatile uint32_t s_correctionInc = 0;

enet_timer_config_t ptp_timer_config_struct;
static volatile uint32_t time_s = 0;
static volatile uint32_t last_visit_time_ns = 0;
static volatile uint32_t last_visit_time_s = 0;

void get_current_time(uint32_t *s, uint32_t *ns, uint8_t irq_flag)
{
    uint32_t temp_ns = 0;
    uint32_t temp_s = 0;
    if (USE_OUT_IRQ_FLAG == irq_flag)
    {
        taskENTER_CRITICAL();
    }
    temp_s = time_s;
    ENET_DRV_TimerGet(PTP_ENET_INSTANCE, &temp_ns);

    if (temp_s == last_visit_time_s)
    {
        if ((last_visit_time_ns > 990000000) && (temp_ns < 1000000))
        {
            temp_ns += (uint32_t)S_TO_NS_COUNT;
        }
        else
        {
            last_visit_time_ns = temp_ns;
        }
    }
    else
    {
        last_visit_time_s = temp_s;
        last_visit_time_ns = temp_ns;
    }

    if (USE_OUT_IRQ_FLAG == irq_flag)
    {
        taskEXIT_CRITICAL();
    }
    *s = temp_s;
    *ns = temp_ns;
}

void enet_arriver_callback(uint8_t instance, uint8_t channel)
{
    (void)(instance);
    (void)(channel);
    time_s++;
}

static void ptp_timer_close_irqhandler(void)
{
    ENET_DRV_TimerSetCorrection(PTP_ENET_INSTANCE, 0, 0);
    PIT->TIMER[PTP_USE_PIT_CHANNEL].TFLG = PIT_TFLG_TIF(1u);
    PIT->TIMER[PTP_USE_PIT_CHANNEL].TCTRL = PIT_TCTRL_TEN(0u) | PIT_TCTRL_TIE(0u);
}

void PTPTimerInit(void)
{
    ptp_timer_config_struct.timerPeriod = S_TO_NS_COUNT;
    ptp_timer_config_struct.timerInc = NS_INC_IN_TICK;
    ptp_timer_config_struct.correctionPeriod = 0;
    ptp_timer_config_struct.correctionInc = 0;
    ptp_timer_config_struct.irqEnable = true;
    ptp_timer_config_struct.callback = enet_arriver_callback;
    s_correctionInc = NS_INC_IN_TICK;
    ENET_DRV_TimerInit(PTP_ENET_INSTANCE, &ptp_timer_config_struct);
    ENET_DRV_TimerStart(PTP_ENET_INSTANCE);
    INT_SYS_InstallHandler(PTP_USE_PIT_CHANNEL_IRQn, ptp_timer_close_irqhandler, NULL);
    INT_SYS_EnableIRQ(PTP_USE_PIT_CHANNEL_IRQn);
    INT_SYS_SetPriority(PTP_USE_PIT_CHANNEL_IRQn, 9);
}

void setPTPUsrTime(const int32_t second, const int32_t nanoSecond)
{
    ENET_DRV_TimerSet(PTP_ENET_INSTANCE, (uint32_t)nanoSecond);
    time_s = second;
}

void getPTPUsrTime(int32_t *const second, int32_t *const nanoSecond)
{
    uint32_t temp_s;
    uint32_t temp_ns;

    get_current_time(&temp_s, &temp_ns, 0);
    if (temp_ns > S_TO_NS_COUNT)
    {
        *second = (int32_t)(temp_s) + 1;
        *nanoSecond = (int32_t)(temp_ns)-S_TO_NS_COUNT;
    }
    else
    {
        *second = (int32_t)(temp_s);
        *nanoSecond = (int32_t)(temp_ns);
    }
}

// 负值代表当前时间比实际时间慢，需要将时间加快
// 正值代表当前时间比实际时间块，需要将时间降低
// 按周期时间为1s进行微差校时
void updatePTPTimerOffest(int32_t adj)
{
    if (abs(adj) > 400)
    {
        if (abs(adj) > 4000000)
        {
            s_correctionInc = (uint32_t)((float)NS_INC_IN_TICK * (1.0 + ((float)adj / (float)ADJUST_BASE_TIMES_VALUE) * 1.0));
            ENET_DRV_TimerSetCorrection(PTP_ENET_INSTANCE, s_correctionInc, 1);
        }
        else if (abs(adj) > 40000)
        {
            s_correctionInc = (uint32_t)((float)NS_INC_IN_TICK * (1.0 + ((float)adj / (float)ADJUST_BASE_TIMES_VALUE) * 10.0));
            ENET_DRV_TimerSetCorrection(PTP_ENET_INSTANCE, s_correctionInc, 10);
        }
        else if (abs(adj) > 4000)
        {
            s_correctionInc = (uint32_t)((float)NS_INC_IN_TICK * (1.0 + ((float)adj / (float)ADJUST_BASE_TIMES_VALUE) * 125.0));
            ENET_DRV_TimerSetCorrection(PTP_ENET_INSTANCE, s_correctionInc, 125);
        }
        else if (abs(adj) > 400)
        {
            s_correctionInc = (uint32_t)((float)NS_INC_IN_TICK * (1.0 + ((float)adj / (float)ADJUST_BASE_TIMES_VALUE) * 1250.0));
            ENET_DRV_TimerSetCorrection(PTP_ENET_INSTANCE, s_correctionInc, 1250);
        }
        else
        {
            s_correctionInc = (uint32_t)((float)NS_INC_IN_TICK * (1.0 + ((float)adj / (float)ADJUST_BASE_TIMES_VALUE) * 12500.0));
            ENET_DRV_TimerSetCorrection(PTP_ENET_INSTANCE, s_correctionInc, 12500);
        }
        PIT->TIMER[PTP_USE_PIT_CHANNEL].LDVAL = 1999999; //2000000;
        PIT->TIMER[PTP_USE_PIT_CHANNEL].TCTRL = PIT_TCTRL_TEN(1u) | PIT_TCTRL_TIE(1u);
    }
    else
    {
        ENET_DRV_TimerSetCorrection(PTP_ENET_INSTANCE, 0, 0);
    }
}

extern volatile uint8_t ptp_adjust_flag;

uint8_t ptp_check(void)
{
    return ptp_adjust_flag;
}

uint32_t vPortGetPTPTimeStampSec(uint8_t *ptp_flag, uint8_t irq_flag)
{
    if (ptp_flag != NULL)
    {
        *ptp_flag = ptp_adjust_flag;
    }
    return time_s;
}

uint64_t vPortGetPTPTimeStampMilliSec(uint8_t *ptp_flag, uint8_t irq_flag)
{
    uint64_t ret;
    uint32_t temp_s;
    uint32_t temp_ns;
#ifndef NO_PTP
    if (ptp_adjust_flag == 1)
    {
        get_current_time(&temp_s, &temp_ns, irq_flag);

        ret = (uint64_t)((uint64_t)temp_s * 1000) + ((uint64_t)temp_ns) / 1000000;

        if (ptp_flag != NULL)
        {
            *ptp_flag = 1;
        }
        return ret;
    }
    else
#endif
    {
        if (ptp_flag != NULL)
        {
            *ptp_flag = 0;
        }
        return vPortGetTimeStampMilliSec();
    }
}

uint64_t vPortGetPTPTimeStampMicroSec(uint8_t *ptp_flag, uint8_t irq_flag)
{
    uint64_t ret;
    uint32_t temp_s;
    uint32_t temp_ns;

#ifndef NO_PTP
    if (ptp_adjust_flag == 1)
    {
        get_current_time(&temp_s, &temp_ns, irq_flag);

        ret = (uint64_t)((uint64_t)temp_s * 1000000) + ((uint64_t)temp_ns) / 1000;

        if (ptp_flag != NULL)
        {
            *ptp_flag = 1;
        }
        return ret;
    }
    else
#endif
    {
        if (ptp_flag != NULL)
        {
            *ptp_flag = 0;
        }
        return vPortGetTimeStampMicroSec();
    }
}

uint64_t vPortGetPTPTimeStampNanoSec(uint8_t *ptp_flag, uint8_t irq_flag)
{
    uint64_t ret;
    uint32_t temp_s;
    uint32_t temp_ns;

#ifndef NO_PTP
    if (ptp_adjust_flag == 1)
    {
        get_current_time(&temp_s, &temp_ns, irq_flag);

        ret = (uint64_t)((uint64_t)temp_s * 1000000000) + ((uint64_t)temp_ns);

        if (ptp_flag != NULL)
        {
            *ptp_flag = 1;
        }
        return ret;
    }
    else
#endif
    {
        if (ptp_flag != NULL)
        {
            *ptp_flag = 0;
        }
        return vPortGetTimeStampNanoSec();
    }
}
