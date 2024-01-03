/* ptpd.c */

#include "ptpd.h"
#include "rtos.h"
#include "usr_timer.h"
#include "vci_server.h"
#include "board.h"

#define PTP_TASK_STACK_SIZE (1024U)
#define PTP_TASK_PRIORITY (4U)
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */

RunTimeOpts rtOpts; /* statically allocated run-time configuration data */
PtpClock ptpClock;
ForeignMasterRecord ptpForeignRecords[DEFAULT_MAX_FOREIGN_RECORDS];

void ptp_run_control_task(void *param)
{
	(void)(param);
	TickType_t xLastWakeTime;
	PTPTimerInit();
	xLastWakeTime = xTaskGetTickCount();
	while (1)
	{
		ptpd_Periodic_Handle(LocalTime);
		vTaskDelayUntil(&xLastWakeTime, 7);
		LocalTime += 7;
	}
}

int PTPd_Init(void)
{
	Integer16 ret;

	/* initialize run-time options to default values */
	rtOpts.announceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	rtOpts.syncInterval = DEFAULT_SYNC_INTERVAL;
	rtOpts.clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	rtOpts.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	rtOpts.clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE; /* 7.6.3.3 */
	rtOpts.priority1 = DEFAULT_PRIORITY1;
	rtOpts.priority2 = DEFAULT_PRIORITY2;
	rtOpts.domainNumber = DEFAULT_DOMAIN_NUMBER;
	rtOpts.slaveOnly = SLAVE_ONLY;
	rtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
	rtOpts.servo.noResetClock = DEFAULT_NO_RESET_CLOCK;
	rtOpts.servo.noAdjust = NO_ADJUST;
	rtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	rtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	rtOpts.servo.sDelay = DEFAULT_DELAY_S;
	rtOpts.servo.sOffset = DEFAULT_OFFSET_S;
	rtOpts.servo.ap = DEFAULT_AP;
	rtOpts.servo.ai = DEFAULT_AI;
	rtOpts.maxForeignRecords = sizeof(ptpForeignRecords) / sizeof(ptpForeignRecords[0]);
	rtOpts.stats = PTP_TEXT_STATS;
	rtOpts.delayMechanism = DEFAULT_DELAY_MECHANISM;

	/* Initialize run time options with command line arguments*/
	ret = ptpdStartup(&ptpClock, &rtOpts, ptpForeignRecords);

	if (ret != 0)
		return ret;

	xTaskCreate(ptp_run_control_task, "ptp_run", PTP_TASK_STACK_SIZE, NULL, PTP_TASK_PRIORITY, NULL);
	return 0;
}

/* loop forever. doState() has a switch for the actions and events to be
 checked for 'port_state'. the actions and events may or may not change
 'port_state' by calling toState(), but once they are done we loop around
 again and perform the actions required for the new 'port_state'. */
void ptpd_Periodic_Handle(__IO UInteger32 localtime)
{
	static UInteger32 old_localtime = 0;

	catchAlarm(abs(localtime - old_localtime));
	old_localtime = localtime;
	/* at least once run stack */

	do
	{
		doState(&ptpClock);
		/* if there are still some packets - run stack again */
	} while (netSelect(&ptpClock.netPath) > 0);
}
