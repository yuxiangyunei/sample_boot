#ifndef PTPD_DEP_H_
#define PTPD_DEP_H_

/**
 *\file
 * \brief Functions used in ptpdv2 which are platform-dependent
 */

/** \name Endian corrections*/
/**\{*/

#if defined(PTPD_MSBF)
#define shift8(x, y) ((x) << ((3 - y) << 3))
#define shift16(x, y) ((x) << ((1 - y) << 4))
#elif defined(PTPD_LSBF)
#define shift8(x, y) ((x) << ((y) << 3))
#define shift16(x, y) ((x) << ((y) << 4))
#endif

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define htons(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))
#define htonl(x) (((x)&0xff) << 24) |          \
					 (((x)&0xff00) << 8) |     \
					 (((x)&0xff0000UL) >> 8) | \
					 (((x)&0xff000000UL) >> 24)

#define flip16(x) (x) //htons(x)
#define flip32(x) (x) //htonl(x)

/** \}*/

/** \name Bit array manipulations*/
/**\{*/
#define getFlag(flagField, mask) (Boolean)(((flagField) & (mask)) == (mask))
#define setFlag(flagField, mask) (flagField) |= (mask)
#define clearFlag(flagField, mask) (flagField) &= ~(mask)
/* #define getFlag(x,y)  (Boolean)!!( *(UInteger8*)((x)+((y)<8?1:0)) &   (1<<((y)<8?(y):(y)-8)) ) */
/* #define setFlag(x,y)    ( *(UInteger8*)((x)+((y)<8?1:0)) |=   1<<((y)<8?(y):(y)-8)  ) */
/* #define clearFlag(x,y)  ( *(UInteger8*)((x)+((y)<8?1:0)) &= ~(1<<((y)<8?(y):(y)-8)) ) */
/** \}*/

/** \name msg.c
 *-Pack and unpack PTP messages */
/**\{*/

void msgUnpackHeader(const Octet *, MsgHeader *);
void msgUnpackAnnounce(const Octet *, MsgAnnounce *);
void msgUnpackSync(const Octet *, MsgSync *);
void msgUnpackFollowUp(const Octet *, MsgFollowUp *);
void msgUnpackDelayReq(const Octet *, MsgDelayReq *);
void msgUnpackDelayResp(const Octet *, MsgDelayResp *);
void msgUnpackPDelayReq(const Octet *, MsgPDelayReq *);
void msgUnpackPDelayResp(const Octet *, MsgPDelayResp *);
void msgUnpackPDelayRespFollowUp(const Octet *, MsgPDelayRespFollowUp *);
void msgUnpackManagement(const Octet *, MsgManagement *);
/* UInteger8 msgUnloadManagement(void*, MsgManagement*, PtpClock*); */
void msgUnpackManagementPayload(const Octet *buf, MsgManagement *manage);
void msgPackHeader(const PtpClock *, Octet *);
void msgPackAnnounce(const PtpClock *, Octet *);
void msgPackSync(const PtpClock *, Octet *, const Timestamp *);
void msgPackFollowUp(const PtpClock *, Octet *, const Timestamp *);
void msgPackDelayReq(const PtpClock *, Octet *, const Timestamp *);
void msgPackDelayResp(const PtpClock *, Octet *, const MsgHeader *, const Timestamp *);
void msgPackPDelayReq(const PtpClock *, Octet *, const Timestamp *);
void msgPackPDelayResp(Octet *, const MsgHeader *, const Timestamp *);
void msgPackPDelayRespFollowUp(Octet *, const MsgHeader *, const Timestamp *);
UInteger16 msgPackManagement(const PtpClock *, Octet *, const MsgManagement *);
UInteger16 msgPackManagementResponse(const PtpClock *, Octet *, MsgHeader *, const MsgManagement *);
/** \}*/

/** \name net.c (Linux API dependent)
 * -Init network stuff, send and receive datas*/
/**\{*/

Boolean netInit(NetPath *);
Integer32 netSelect(NetPath *);
ssize_t netRecvEvent(NetPath *, Octet *, TimeInternal *);
ssize_t netRecvGeneral(NetPath *, Octet *, TimeInternal *);
ssize_t netSendEvent(NetPath *, const Octet *, UInteger16, TimeInternal *);
ssize_t netSendGeneral(NetPath *, const Octet *, UInteger16);
ssize_t netSendPeerGeneral(NetPath *, const Octet *, UInteger16);
ssize_t netSendPeerEvent(NetPath *, const Octet *, UInteger16, TimeInternal *);
void netEmptyEventQ(NetPath *netPath);
void netEmptyGeneralQ(NetPath *netPath);
/** \}*/

/** \name servo.c
 * -Clock servo*/
/**\{*/

void initClock(PtpClock *);
void updatePeerDelay(PtpClock *, const TimeInternal *, Boolean);
void updateDelay(PtpClock *, const TimeInternal *, const TimeInternal *, const TimeInternal *);
void updateOffset(PtpClock *, const TimeInternal *, const TimeInternal *, const TimeInternal *);
void updateClock(PtpClock *);
/** \}*/

/** \name startup.c (Linux API dependent)
 * -Handle with runtime options*/
/**\{*/
Integer16 ptpdStartup(PtpClock *, RunTimeOpts *, ForeignMasterRecord *);
/** \}*/

/** \name sys.c (Linux API dependent)
 * -Manage timing system API*/
/**\{*/
void displayStats(const PtpClock *ptpClock);
Boolean nanoSleep(const TimeInternal *);
void getTime(TimeInternal *);
void setTime(const TimeInternal *);
void updateTime(const TimeInternal *);
Boolean adjFreq(Integer32);
UInteger32 getRand(UInteger32);
/** \}*/

/** \name timer.c (Linux API dependent)
 * -Handle with timers*/
/**\{*/
void catchAlarm(UInteger32);
void initTimer(void);
void timerUpdate(IntervalTimer *);
void timerStop(UInteger16, IntervalTimer *);
void timerStart(UInteger16, UInteger32, IntervalTimer *);
Boolean timerExpired(UInteger16, IntervalTimer *);
/** \}*/

/*Test functions*/

#endif /*PTPD_DEP_H_*/
