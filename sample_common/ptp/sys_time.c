/* sys.c */

#include "ptpd.h"
#include "usr_timer.h"

void getTime(TimeInternal *time)
{
    struct ptptime_t timestamp;

    getPTPUsrTime(&timestamp.tv_sec, &timestamp.tv_nsec);
    time->seconds = timestamp.tv_sec;
    time->nanoseconds = timestamp.tv_nsec;
}

void setTime(const TimeInternal *time)
{
    setPTPUsrTime(time->seconds, time->nanoseconds);
}

void updateTime(const TimeInternal *time)
{
    setPTPUsrTime(time->seconds, time->nanoseconds);
}

UInteger32 getRand(UInteger32 randMax)
{
    return rand() % randMax;
}

Boolean adjFreq(Integer32 adj)
{
    if (adj > ADJ_FREQ_MAX)
    {
        adj = ADJ_FREQ_MAX;
    }
    else if (adj < -ADJ_FREQ_MAX)
    {
        adj = -ADJ_FREQ_MAX;
    }
    updatePTPTimerOffest(adj);
    return true;
}
