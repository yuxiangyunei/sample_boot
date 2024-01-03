/* startup.c */

#include "ptpd.h"

Integer16 ptpdStartup(PtpClock * ptpClock, RunTimeOpts *rtOpts, ForeignMasterRecord* foreign)
{
	ptpClock->rtOpts = rtOpts;

	ptpClock->foreignMasterDS.records = foreign;

	ptpClock->netPath.init_flag = 0;

	/* 9.2.2 */

	if (rtOpts->slaveOnly)
		rtOpts->clockQuality.clockClass = DEFAULT_CLOCK_CLASS_SLAVE_ONLY;

	/* no negative or zero attenuation */
	if (rtOpts->servo.ap < 1)
		rtOpts->servo.ap = 1;

	if (rtOpts->servo.ai < 1)
		rtOpts->servo.ai = 1;

	//event POWER UP

	toState(ptpClock, PTP_INITIALIZING);

	return 0;
}
