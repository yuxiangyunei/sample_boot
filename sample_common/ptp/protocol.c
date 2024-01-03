/* protocol.c */

#include "ptpd.h"

static void handle(PtpClock *);
static void handleAnnounce(PtpClock *, Boolean);
static void handleSync(PtpClock *, TimeInternal *, Boolean);
static void handleFollowUp(PtpClock *, Boolean);
static void handlePDelayReq(PtpClock *, TimeInternal *, Boolean);
static void handleDelayReq(PtpClock *, TimeInternal *, Boolean);
static void handlePDelayResp(PtpClock *, TimeInternal *, Boolean);
static void handleDelayResp(PtpClock *, Boolean);
static void handlePDelayRespFollowUp(PtpClock *, Boolean);
//static void handleManagement(PtpClock *, Boolean);
//static void handleSignaling(PtpClock *, Boolean);

static void issueDelayReqTimerExpired(PtpClock *);
static void issueAnnounce(PtpClock *);
static void issueSync(PtpClock *);
static void issueFollowup(PtpClock *, const TimeInternal *);
static void issueDelayReq(PtpClock *);
static void issueDelayResp(PtpClock *, const TimeInternal *, const MsgHeader *);
static void issuePDelayReq(PtpClock *);
static void issuePDelayResp(PtpClock *, TimeInternal *, const MsgHeader *);
static void issuePDelayRespFollowUp(PtpClock *, const TimeInternal *, const MsgHeader *);
//static void issueManagement(const MsgHeader*,MsgManagement*,PtpClock*);

static Boolean doInit(PtpClock *);

/* perform actions required when leaving 'port_state' and entering 'state' */
void toState(PtpClock *ptpClock, UInteger8 state)
{
    ptpClock->messageActivity = true;

    /* leaving state tasks */
    switch (ptpClock->portDS.portState)
    {
    case PTP_MASTER:
        initClock(ptpClock);

        timerStop(SYNC_INTERVAL_TIMER, ptpClock->itimer);
        timerStop(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer);
        timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
        break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
        if (state == PTP_UNCALIBRATED || state == PTP_SLAVE)
        {
            break;
        }

        timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);

        switch (ptpClock->portDS.delayMechanism)
        {
        case E2E:
            timerStop(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
            break;
        case P2P:
            timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
            break;
        default:
            /* none */
            break;
        }

        initClock(ptpClock);

        break;

    case PTP_PASSIVE:
        initClock(ptpClock);

        timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
        timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
        break;

    case PTP_LISTENING:
        initClock(ptpClock);

        timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
        break;

    case PTP_PRE_MASTER:
        initClock(ptpClock);

        timerStop(QUALIFICATION_TIMEOUT, ptpClock->itimer);
        break;

    default:
        break;
    }

    /* entering state tasks */

    switch (state)
    {
    case PTP_INITIALIZING:
        //state PTP_INITIALIZING
        ptpClock->portDS.portState = PTP_INITIALIZING;
        ptpClock->recommendedState = PTP_INITIALIZING;
        break;

    case PTP_FAULTY:
        //state PTP_FAULTY
        ptpClock->portDS.portState = PTP_FAULTY;
        break;

    case PTP_DISABLED:
        //state PTP_DISABLED
        ptpClock->portDS.portState = PTP_DISABLED;
        break;

    case PTP_LISTENING:
        //state PTP_LISTENING
        timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->portDS.announceReceiptTimeout) * (pow2ms(ptpClock->portDS.logAnnounceInterval)), ptpClock->itimer);
        ptpClock->portDS.portState = PTP_LISTENING;
        ptpClock->recommendedState = PTP_LISTENING;
        break;

    case PTP_PRE_MASTER:
    case PTP_MASTER:
        //state PTP_MASTER
        ptpClock->portDS.logMinDelayReqInterval = DEFAULT_DELAYREQ_INTERVAL; /* it may change during slave state */
        timerStart(SYNC_INTERVAL_TIMER, pow2ms(ptpClock->portDS.logSyncInterval), ptpClock->itimer);
        //SYNC INTERVAL TIMER
        timerStart(ANNOUNCE_INTERVAL_TIMER, pow2ms(ptpClock->portDS.logAnnounceInterval), ptpClock->itimer);

        switch (ptpClock->portDS.delayMechanism)
        {
        case E2E:
            /* none */
            break;
        case P2P:
            timerStart(PDELAYREQ_INTERVAL_TIMER, getRand(pow2ms(ptpClock->portDS.logMinPdelayReqInterval) + 1), ptpClock->itimer);
            break;
        default:
            break;
        }

        ptpClock->portDS.portState = PTP_MASTER;

        break;

    case PTP_PASSIVE:
        //state PTP_PASSIVE
        timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->portDS.announceReceiptTimeout) * (pow2ms(ptpClock->portDS.logAnnounceInterval)), ptpClock->itimer);

        if (ptpClock->portDS.delayMechanism == P2P)
        {
            timerStart(PDELAYREQ_INTERVAL_TIMER, getRand(pow2ms(ptpClock->portDS.logMinPdelayReqInterval + 1)), ptpClock->itimer);
        }

        ptpClock->portDS.portState = PTP_PASSIVE;

        break;

    case PTP_UNCALIBRATED:
        //state PTP_UNCALIBRATED
        timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->portDS.announceReceiptTimeout) * (pow2ms(ptpClock->portDS.logAnnounceInterval)), ptpClock->itimer);

        switch (ptpClock->portDS.delayMechanism)
        {
        case E2E:
            timerStart(DELAYREQ_INTERVAL_TIMER, getRand(pow2ms(ptpClock->portDS.logMinDelayReqInterval + 1)), ptpClock->itimer);
            break;
        case P2P:
            timerStart(PDELAYREQ_INTERVAL_TIMER, getRand(pow2ms(ptpClock->portDS.logMinPdelayReqInterval + 1)), ptpClock->itimer);
            break;
        default:
            /* none */
            break;
        }

        ptpClock->portDS.portState = PTP_UNCALIBRATED;

        break;

    case PTP_SLAVE:
        //state PTP_SLAVE
        ptpClock->portDS.portState = PTP_SLAVE;
        break;

    default:
        //toState: unrecognized
        break;
    }
}

static Boolean doInit(PtpClock *ptpClock)
{
    if (!netInit(&ptpClock->netPath))
    {
        //doInit: failed to initialize network
        return false;
    }
    else
    {
        /* initialize other stuff */
        initData(ptpClock);
        initTimer();
        initClock(ptpClock);
        m1(ptpClock);
        msgPackHeader(ptpClock, ptpClock->msgObuf);
        return true;
    }
}

/* handle actions and events for 'port_state' */
void doState(PtpClock *ptpClock)
{
    ptpClock->messageActivity = false;

    switch (ptpClock->portDS.portState)
    {
    case PTP_LISTENING:
    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
    case PTP_PRE_MASTER:
    case PTP_MASTER:
    case PTP_PASSIVE:
        /*State decision Event*/
        if (getFlag(ptpClock->events, STATE_DECISION_EVENT))
        {
            //event STATE_DECISION_EVENT
            clearFlag(ptpClock->events, STATE_DECISION_EVENT);
            ptpClock->recommendedState = bmc(ptpClock);

            switch (ptpClock->recommendedState)
            {
            case PTP_MASTER:
            case PTP_PASSIVE:

                if (ptpClock->defaultDS.slaveOnly || ptpClock->defaultDS.clockQuality.clockClass == 255)
                {
                    ptpClock->recommendedState = PTP_LISTENING;
                }

                break;

            default:
                break;
            }
        }

        break;

    default:
        break;
    }

    switch (ptpClock->recommendedState)
    {
    case PTP_MASTER:
        switch (ptpClock->portDS.portState)
        {
        case PTP_PRE_MASTER:
            if (timerExpired(QUALIFICATION_TIMEOUT, ptpClock->itimer))
            {
                toState(ptpClock, PTP_MASTER);
            }

            break;

        case PTP_MASTER:
            break;
        default:
            toState(ptpClock, PTP_PRE_MASTER);
            break;
        }

        break;

    case PTP_PASSIVE:
        if (ptpClock->portDS.portState != ptpClock->recommendedState)
        {
            toState(ptpClock, PTP_PASSIVE);
        }

        break;

    case PTP_SLAVE:
        switch (ptpClock->portDS.portState)
        {
        case PTP_UNCALIBRATED:

            if (getFlag(ptpClock->events, MASTER_CLOCK_SELECTED))
            {
                //event MASTER_CLOCK_SELECTED
                clearFlag(ptpClock->events, MASTER_CLOCK_SELECTED);
                toState(ptpClock, PTP_SLAVE);
            }

            if (getFlag(ptpClock->events, MASTER_CLOCK_CHANGED))
            {
                //event MASTER_CLOCK_CHANGED
                clearFlag(ptpClock->events, MASTER_CLOCK_CHANGED);
            }

            break;

        case PTP_SLAVE:
            if (getFlag(ptpClock->events, SYNCHRONIZATION_FAULT))
            {
                //event SYNCHRONIZATION_FAULT
                clearFlag(ptpClock->events, SYNCHRONIZATION_FAULT);
                toState(ptpClock, PTP_UNCALIBRATED);
            }

            if (getFlag(ptpClock->events, MASTER_CLOCK_CHANGED))
            {
                //event MASTER_CLOCK_CHANGED
                clearFlag(ptpClock->events, MASTER_CLOCK_CHANGED);
                toState(ptpClock, PTP_UNCALIBRATED);
            }

            break;

        default:
            toState(ptpClock, PTP_UNCALIBRATED);
            break;
        }

        break;

    case PTP_LISTENING:
        if (ptpClock->portDS.portState != ptpClock->recommendedState)
        {
            toState(ptpClock, PTP_LISTENING);
        }

        break;

    case PTP_INITIALIZING:
        break;
    default:
        //doState: unrecognized recommended state
        break;
    }

    switch (ptpClock->portDS.portState)
    {
    case PTP_INITIALIZING:
        if (true == doInit(ptpClock))
        {
            toState(ptpClock, PTP_LISTENING);
        }
        else
        {
            toState(ptpClock, PTP_FAULTY);
        }

        break;

    case PTP_FAULTY:
        /* imaginary troubleshooting */

        //event FAULT_CLEARED
        toState(ptpClock, PTP_INITIALIZING);
        return;

    case PTP_DISABLED:
        handle(ptpClock);
        break;

    case PTP_LISTENING:
    case PTP_UNCALIBRATED:
    case PTP_SLAVE:
    case PTP_PASSIVE:
        if (timerExpired(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer))
        {
            //event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES
            ptpClock->foreignMasterDS.count = 0;
            ptpClock->foreignMasterDS.i = 0;

            if (!(ptpClock->defaultDS.slaveOnly || ptpClock->defaultDS.clockQuality.clockClass == 255))
            {
                m1(ptpClock);
                ptpClock->recommendedState = PTP_MASTER;
                toState(ptpClock, PTP_MASTER);
            }
            else if (ptpClock->portDS.portState != PTP_LISTENING)
            {
                toState(ptpClock, PTP_LISTENING);
            }

            break;
        }

        handle(ptpClock);

        break;

    case PTP_MASTER:
        if (timerExpired(SYNC_INTERVAL_TIMER, ptpClock->itimer))
        {
            //event SYNC_INTERVAL_TIMEOUT_EXPIRES
            issueSync(ptpClock);
        }

        if (timerExpired(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer))
        {
            //event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES
            issueAnnounce(ptpClock);
        }

        handle(ptpClock);

        issueDelayReqTimerExpired(ptpClock);

        break;
    default:
        //doState: do unrecognized state
        break;
    }
}

/* check and handle received messages */
static void handle(PtpClock *ptpClock)
{

    int ret;
    Boolean isFromSelf;
    TimeInternal time = {
        0, 0};

    if (false == ptpClock->messageActivity)
    { //检查是否接收到数据
        ret = netSelect(&ptpClock->netPath);

        if (ret < 0)
        { //接收出错了
            //handle: failed to poll sockets
            toState(ptpClock, PTP_FAULTY);
            return;
        }
        else if (!ret)
        { //没有接收到数据，直接返回
            //handle: nothing
            return;
        }
    }

    //handle: something

    ptpClock->msgIbufLength = netRecvEvent(&ptpClock->netPath, ptpClock->msgIbuf, &time);
    /* local time is not UTC, we can calculate UTC on demand, otherwise UTC time is not used */
    /* time.seconds += ptpClock->timePropertiesDS.currentUtcOffset; */

    if (ptpClock->msgIbufLength < 0)
    {
        //handle: failed to receive on the event socket
        toState(ptpClock, PTP_FAULTY);
        return;
    }
    else if (!ptpClock->msgIbufLength)
    { //读取数据，接收时MAC层加入的时间戳保存在time内，数据保存在msgIbuf
        ptpClock->msgIbufLength = netRecvGeneral(&ptpClock->netPath, ptpClock->msgIbuf, &time);

        if (ptpClock->msgIbufLength < 0)
        {
            //handle: failed to receive on the general socket
            toState(ptpClock, PTP_FAULTY);
            return;
        }
        else if (!ptpClock->msgIbufLength)
            return;
    }

    ptpClock->messageActivity = true;

    if (ptpClock->msgIbufLength < HEADER_LENGTH)
    {
        //handle: message shorter than header length
        toState(ptpClock, PTP_FAULTY);
        return;
    }
    //将数据解析，并放在msgTmpHeader中
    msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->msgTmpHeader);

    if (ptpClock->msgTmpHeader.versionPTP != ptpClock->portDS.versionNumber)
    {
        //handle: ignore version %d message\n", ptpClock->msgTmpHeader.versionPTP);
        return;
    }

    if (ptpClock->msgTmpHeader.domainNumber != ptpClock->defaultDS.domainNumber)
    {
        //handle: ignore message from domainNumber %d\n", ptpClock->msgTmpHeader.domainNumber);
        return;
    }

    /*Spec 9.5.2.2*/
    isFromSelf = isSamePortIdentity(&ptpClock->portDS.portIdentity, &ptpClock->msgTmpHeader.sourcePortIdentity); //�ж������Ƿ�Ϊͬһ������

    /* subtract the inbound latency adjustment if it is not a loop back and the
     time stamp seems reasonable */
    if (!isFromSelf && time.seconds > 0)
        subTime(&time, &time, &ptpClock->inboundLatency);

    switch (ptpClock->msgTmpHeader.messageType)
    {

    case ANNOUNCE:
        handleAnnounce(ptpClock, isFromSelf);
        break;

    case SYNC:
        handleSync(ptpClock, &time, isFromSelf);
        break;

    case FOLLOW_UP:
        handleFollowUp(ptpClock, isFromSelf);
        break;

    case DELAY_REQ:
        handleDelayReq(ptpClock, &time, isFromSelf);
        break;

    case PDELAY_REQ:
        handlePDelayReq(ptpClock, &time, isFromSelf);
        break;

    case DELAY_RESP:
        handleDelayResp(ptpClock, isFromSelf);
        break;

    case PDELAY_RESP:
        handlePDelayResp(ptpClock, &time, isFromSelf);
        break;

    case PDELAY_RESP_FOLLOW_UP:
        handlePDelayRespFollowUp(ptpClock, isFromSelf);
        break;

    case MANAGEMENT:
        //handleManagement(ptpClock, isFromSelf);
        break;

    case SIGNALING:
        //handleSignaling(ptpClock, isFromSelf);
        break;

    default:
        //handle: unrecognized message
        break;
    }
}

/*spec 9.5.3*/
static void handleAnnounce(PtpClock *ptpClock, Boolean isFromSelf)
{
    Boolean isFromCurrentParent = false;

    //handleAnnounce: received

    if (ptpClock->msgIbufLength < ANNOUNCE_LENGTH)
    {
        //handleAnnounce: short message
        toState(ptpClock, PTP_FAULTY);
        return;
    }

    if (isFromSelf)
    {
        //handleAnnounce: ignore from self
        return;
    }

    switch (ptpClock->portDS.portState)
    {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:

        //handleAnnounce: disreguard
        break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:

        /* Valid announce message is received : BMC algorithm will be executed */
        setFlag(ptpClock->events, STATE_DECISION_EVENT);

        isFromCurrentParent = isSamePortIdentity(&ptpClock->parentDS.parentPortIdentity, &ptpClock->msgTmpHeader.sourcePortIdentity);

        msgUnpackAnnounce(ptpClock->msgIbuf, &ptpClock->msgTmp.announce);

        if (isFromCurrentParent)
        {
            s1(ptpClock, &ptpClock->msgTmpHeader, &ptpClock->msgTmp.announce);

            /*Reset Timer handling Announce receipt timeout*/
            timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->portDS.announceReceiptTimeout) * (pow2ms(ptpClock->portDS.logAnnounceInterval)), ptpClock->itimer);
        }
        else
        {
            //handleAnnounce: from another foreign master

            /*addForeign takes care of AnnounceUnpacking*/
            addForeign(ptpClock, &ptpClock->msgTmpHeader, &ptpClock->msgTmp.announce);
        }

        break;

    case PTP_PASSIVE:
        timerStart(ANNOUNCE_RECEIPT_TIMER, (ptpClock->portDS.announceReceiptTimeout) * (pow2ms(ptpClock->portDS.logAnnounceInterval)), ptpClock->itimer);
    case PTP_MASTER:
    case PTP_PRE_MASTER:
    case PTP_LISTENING:
    default:

        //handleAnnounce: from another foreign master

        msgUnpackAnnounce(ptpClock->msgIbuf, &ptpClock->msgTmp.announce);

        /* Valid announce message is received : BMC algorithm will be executed */
        setFlag(ptpClock->events, STATE_DECISION_EVENT);

        addForeign(ptpClock, &ptpClock->msgTmpHeader, &ptpClock->msgTmp.announce);
        break;

    } //switch on (portState)
}

static void handleSync(PtpClock *ptpClock, TimeInternal *time, Boolean isFromSelf)
{
    TimeInternal originTimestamp;
    TimeInternal correctionField;

    Boolean isFromCurrentParent = false;
    //handleSync: received

    if (ptpClock->msgIbufLength < SYNC_LENGTH)
    {
        //handleSync: short message
        toState(ptpClock, PTP_FAULTY);
        return;
    }

    switch (ptpClock->portDS.portState)
    {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:

        //handleSync: disreguard
        break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:

        if (isFromSelf)
        {
            //handleSync: ignore from self
            break;
        }

        isFromCurrentParent = isSamePortIdentity(&ptpClock->parentDS.parentPortIdentity, &ptpClock->msgTmpHeader.sourcePortIdentity);

        if (!isFromCurrentParent)
        {
            //handleSync: ignore from another master
            break;
        }

        ptpClock->timestamp_syncRecieve = *time; //保存接收sync报文时的时间戳

        scaledNanosecondsToInternalTime(&ptpClock->msgTmpHeader.correctionfield, &correctionField);

        if (getFlag(ptpClock->msgTmpHeader.flagField[0], FLAG0_TWO_STEP))
        {
            ptpClock->waitingForFollowUp = true;
            ptpClock->recvSyncSequenceId = ptpClock->msgTmpHeader.sequenceId;
            /* Save correctionField of Sync message for future use */
            ptpClock->correctionField_sync = correctionField;
        }
        else
        {
            msgUnpackSync(ptpClock->msgIbuf, &ptpClock->msgTmp.sync);
            ptpClock->waitingForFollowUp = false;
            /* Synchronize  local clock */
            toInternalTime(&originTimestamp, &ptpClock->msgTmp.sync.originTimestamp);
            /* use correctionField of Sync message for future use */
            updateOffset(ptpClock, &ptpClock->timestamp_syncRecieve, &originTimestamp, &correctionField);
            updateClock(ptpClock);

            issueDelayReqTimerExpired(ptpClock);
        }

        break;

    case PTP_MASTER:

        if (!isFromSelf)
        {
            //handleSync: from another master
            break;
        }
        else
        {
            //handleSync: ignore from self
            break;
        }

        //      if waitingForLoopback && TWO_STEP_FLAG
        //        {
        //            /*Add latency*/
        //            addTime(time, time, &rtOpts->outboundLatency);
        //
        //            issueFollowup(ptpClock, time);
        //            break;
        //        }
    case PTP_PASSIVE:
        issueDelayReqTimerExpired(ptpClock);
        //handleSync: disreguard
        break;

    default:
        //handleSync: disreguard

        break;
    }
}

static void handleFollowUp(PtpClock *ptpClock, Boolean isFromSelf)
{
    TimeInternal preciseOriginTimestamp;
    TimeInternal correctionField;
    Boolean isFromCurrentParent = false;

    //handleFollowup: received

    if (ptpClock->msgIbufLength < FOLLOW_UP_LENGTH)
    {
        //handleFollowup: short message
        toState(ptpClock, PTP_FAULTY);
        return;
    }

    if (isFromSelf)
    {
        //handleFollowup: ignore from self
        return;
    }

    switch (ptpClock->portDS.portState)
    {
    case PTP_INITIALIZING:
    case PTP_FAULTY:
    case PTP_DISABLED:
    case PTP_LISTENING:

        //handleFollowup: disreguard
        break;

    case PTP_UNCALIBRATED:
    case PTP_SLAVE:

        isFromCurrentParent = isSamePortIdentity(&ptpClock->parentDS.parentPortIdentity, &ptpClock->msgTmpHeader.sourcePortIdentity);

        if (!ptpClock->waitingForFollowUp)
        {
            //handleFollowup: not waiting a message
            break;
        }

        if (!isFromCurrentParent)
        {
            //handleFollowup: not from current parent
            break;
        }

        if (ptpClock->recvSyncSequenceId != ptpClock->msgTmpHeader.sequenceId)
        {
            //handleFollowup: SequenceID doesn't match with last Sync message
            break;
        }

        msgUnpackFollowUp(ptpClock->msgIbuf, &ptpClock->msgTmp.follow);

        ptpClock->waitingForFollowUp = false;
        /* synchronize local clock */
        toInternalTime(&preciseOriginTimestamp, &ptpClock->msgTmp.follow.preciseOriginTimestamp);
        scaledNanosecondsToInternalTime(&ptpClock->msgTmpHeader.correctionfield, &correctionField);
        addTime(&correctionField, &correctionField, &ptpClock->correctionField_sync);
        updateOffset(ptpClock, &ptpClock->timestamp_syncRecieve, &preciseOriginTimestamp, &correctionField);
        updateClock(ptpClock);

        issueDelayReqTimerExpired(ptpClock);
        break;

    case PTP_MASTER:
        //handleFollowup: from another master

        break;

    case PTP_PASSIVE:
        issueDelayReqTimerExpired(ptpClock);
        //handleFollowup: disreguard
        break;

    default:
        //handleFollowup: unrecognized state

        break;
    } //Switch on (port_state)
}

static void handleDelayReq(PtpClock *ptpClock, TimeInternal *time, Boolean isFromSelf)
{
    (void)(isFromSelf);
    switch (ptpClock->portDS.delayMechanism)
    {
    case E2E:
        //handleDelayReq: received

        if (ptpClock->msgIbufLength < DELAY_REQ_LENGTH)
        {
            //handleDelayReq: short message
            toState(ptpClock, PTP_FAULTY);
            return;
        }

        switch (ptpClock->portDS.portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:
            //handleDelayReq: disreguard
            return;

        case PTP_SLAVE:
            //handleDelayReq: disreguard
            //            if (isFromSelf)
            //            {
            //    /* waitingForLoopback? */
            //                /* Get sending timestamp from IP stack with So_TIMESTAMP*/
            //                ptpClock->delay_req_send_time = *time;

            //                /*Add latency*/
            //                addTime(&ptpClock->delay_req_send_time, &ptpClock->delay_req_send_time, &rtOpts->outboundLatency);
            //                break;
            //            }
            break;

        case PTP_MASTER:
            /* TODO: manage the value of ptpClock->logMinDelayReqInterval form logSyncInterval to logSyncInterval + 5 */
            issueDelayResp(ptpClock, time, &ptpClock->msgTmpHeader);
            break;

        default:
            //handleDelayReq: unrecognized state
            break;
        }

        break;

    case P2P:
        //handleDelayReq: disreguard in P2P mode
        break;
    default:
        /* none */
        break;
    }
}

static void handleDelayResp(PtpClock *ptpClock, Boolean isFromSelf)
{
    (void)(isFromSelf);
    Boolean isFromCurrentParent = false;
    Boolean isCurrentRequest = false;
    TimeInternal correctionField;

    switch (ptpClock->portDS.delayMechanism)
    {
    case E2E:
        //handleDelayResp: received

        if (ptpClock->msgIbufLength < DELAY_RESP_LENGTH)
        {
            //handleDelayResp: short message
            toState(ptpClock, PTP_FAULTY);
            return;
        }

        switch (ptpClock->portDS.portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_LISTENING:
            //handleDelayResp: disreguard
            return;
        case PTP_UNCALIBRATED:
        case PTP_SLAVE:

            msgUnpackDelayResp(ptpClock->msgIbuf, &ptpClock->msgTmp.resp);

            isFromCurrentParent = isSamePortIdentity(&ptpClock->parentDS.parentPortIdentity, &ptpClock->msgTmpHeader.sourcePortIdentity);

            isCurrentRequest = isSamePortIdentity(&ptpClock->portDS.portIdentity, &ptpClock->msgTmp.resp.requestingPortIdentity);

            if (((ptpClock->sentDelayReqSequenceId - 1) == ptpClock->msgTmpHeader.sequenceId) && isCurrentRequest && isFromCurrentParent)
            {
                /* TODO: revisit 11.3 */
                toInternalTime(&ptpClock->timestamp_delayReqRecieve, &ptpClock->msgTmp.resp.receiveTimestamp);

                scaledNanosecondsToInternalTime(&ptpClock->msgTmpHeader.correctionfield, &correctionField);
                updateDelay(ptpClock, &ptpClock->timestamp_delayReqSend, &ptpClock->timestamp_delayReqRecieve, &correctionField);

                ptpClock->portDS.logMinDelayReqInterval = ptpClock->msgTmpHeader.logMessageInterval;
            }
            else
            {
                //handleDelayResp: doesn't match with the delayReq
                break;
            }
        }

        break;

    case P2P:
        //handleDelayResp: disreguard in P2P mode
        break;
    default:
        break;
    }
}

static void handlePDelayReq(PtpClock *ptpClock, TimeInternal *time, Boolean isFromSelf)
{
    switch (ptpClock->portDS.delayMechanism)
    {
    case E2E:
        //handlePDelayReq: disreguard in E2E mode
        break;
    case P2P:
        //handlePDelayReq: recieved

        if (ptpClock->msgIbufLength < PDELAY_REQ_LENGTH)
        {
            //handlePDelayReq: short message
            toState(ptpClock, PTP_FAULTY);
            return;
        }

        switch (ptpClock->portDS.portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:
            //handlePDelayReq: disreguard
            return;

        case PTP_PASSIVE:
        case PTP_SLAVE:
        case PTP_MASTER:

            if (isFromSelf)
            {
                //handlePDelayReq: ignore from self
                break;
            }

            //            if (isFromSelf) /* && loopback mode */
            //            {
            //                /* Get sending timestamp from IP stack with So_TIMESTAMP*/
            //                ptpClock->pdelay_req_send_time = *time;
            //
            //                /*Add latency*/
            //                addTime(&ptpClock->pdelay_req_send_time, &ptpClock->pdelay_req_send_time, &rtOpts->outboundLatency);
            //                break;
            //            }
            //            else
            //            {
            //ptpClock->PdelayReqHeader = ptpClock->msgTmpHeader;

            issuePDelayResp(ptpClock, time, &ptpClock->msgTmpHeader);

            if ((time->seconds != 0) && getFlag(ptpClock->msgTmpHeader.flagField[0], FLAG0_TWO_STEP)) /* not loopback mode */
            {
                issuePDelayRespFollowUp(ptpClock, time, &ptpClock->msgTmpHeader);
            }

            break;

            //            }

        default:

            //handlePDelayReq: unrecognized state
            break;
        }

        break;

    default:
        /* nothing */
        break;
    }
}

static void handlePDelayResp(PtpClock *ptpClock, TimeInternal *time, Boolean isFromSelf)
{
    TimeInternal requestReceiptTimestamp;
    TimeInternal correctionField;
    Boolean isCurrentRequest;

    switch (ptpClock->portDS.delayMechanism)
    {
    case E2E:
        //handlePDelayReq: disreguard in E2E mode
        break;
    case P2P:
        //handlePDelayReq: received

        if (ptpClock->msgIbufLength < PDELAY_RESP_LENGTH)
        {
            //handlePDelayReq: short message
            toState(ptpClock, PTP_FAULTY);
            return;
        }

        switch (ptpClock->portDS.portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
        case PTP_LISTENING:

            //handlePDelayReq: disreguard
            return;

        case PTP_MASTER:
        case PTP_SLAVE:

            if (isFromSelf)
            {
                //handlePDelayReq: ignore from self
                break;
            }

            //            if (isFromSelf)  && loopback mode
            //            {
            //                addTime(time, time, &rtOpts->outboundLatency);
            //                issuePDelayRespFollowUp(time, ptpClock);
            //                break;
            //            }

            msgUnpackPDelayResp(ptpClock->msgIbuf, &ptpClock->msgTmp.presp);

            isCurrentRequest = isSamePortIdentity(&ptpClock->portDS.portIdentity, &ptpClock->msgTmp.presp.requestingPortIdentity);

            if (((ptpClock->sentPDelayReqSequenceId - 1) == ptpClock->msgTmpHeader.sequenceId) && isCurrentRequest)

            {
                if (getFlag(ptpClock->msgTmpHeader.flagField[0], FLAG0_TWO_STEP))
                {
                    ptpClock->waitingForPDelayRespFollowUp = true;

                    /*Store t4 (Fig 35)*/
                    ptpClock->pdelay_t4 = *time;

                    /*store t2 (Fig 35)*/
                    toInternalTime(&requestReceiptTimestamp, &ptpClock->msgTmp.presp.requestReceiptTimestamp);
                    ptpClock->pdelay_t2 = requestReceiptTimestamp;

                    scaledNanosecondsToInternalTime(&ptpClock->msgTmpHeader.correctionfield, &correctionField);
                    ptpClock->correctionField_pDelayResp = correctionField;
                } //Two Step Clock

                else //One step Clock
                {
                    ptpClock->waitingForPDelayRespFollowUp = false;

                    /*Store t4 (Fig 35)*/
                    ptpClock->pdelay_t4 = *time;

                    scaledNanosecondsToInternalTime(&ptpClock->msgTmpHeader.correctionfield, &correctionField);
                    updatePeerDelay(ptpClock, &correctionField, false);
                }
            }
            else
            {
                //handlePDelayReq: PDelayResp doesn't match with the PDelayReq.
            }

            break;

        default:

            //handlePDelayReq: unrecognized state
            break;
        }

        break;

    default:
        /* nothing */
        break;
    }
}

static void handlePDelayRespFollowUp(PtpClock *ptpClock, Boolean isFromSelf)
{
    (void)(isFromSelf);
    TimeInternal responseOriginTimestamp;
    TimeInternal correctionField;

    switch (ptpClock->portDS.delayMechanism)
    {
    case E2E:
        //handlePDelayRespFollowUp: disreguard in E2E mode
        break;
    case P2P:
        //handlePDelayRespFollowUp: received

        if (ptpClock->msgIbufLength < PDELAY_RESP_FOLLOW_UP_LENGTH)
        {
            //handlePDelayRespFollowUp: short message
            toState(ptpClock, PTP_FAULTY);
            return;
        }

        switch (ptpClock->portDS.portState)
        {
        case PTP_INITIALIZING:
        case PTP_FAULTY:
        case PTP_DISABLED:
        case PTP_UNCALIBRATED:
            //handlePDelayRespFollowUp: disreguard
            return;

        case PTP_SLAVE:
        case PTP_MASTER:

            if (!ptpClock->waitingForPDelayRespFollowUp)
            {
                //handlePDelayRespFollowUp: not waiting a message
                break;
            }

            if (ptpClock->msgTmpHeader.sequenceId == ptpClock->sentPDelayReqSequenceId - 1)
            {
                msgUnpackPDelayRespFollowUp(ptpClock->msgIbuf, &ptpClock->msgTmp.prespfollow);
                toInternalTime(&responseOriginTimestamp, &ptpClock->msgTmp.prespfollow.responseOriginTimestamp);
                ptpClock->pdelay_t3 = responseOriginTimestamp;
                scaledNanosecondsToInternalTime(&ptpClock->msgTmpHeader.correctionfield, &correctionField);
                addTime(&correctionField, &correctionField, &ptpClock->correctionField_pDelayResp);

                updatePeerDelay(ptpClock, &correctionField, true);

                ptpClock->waitingForPDelayRespFollowUp = false;
                break;
            }
        }

        break;

    default:
        /* nothing */
        break;
    }
}

// static void handleManagement(PtpClock *ptpClock, Boolean isFromSelf)
// {
//     /* ENABLE_PORT -> DESIGNATED_ENABLED -> toState(PTP_INITIALIZING) */
//     /* DISABLE_PORT -> DESIGNATED_DISABLED -> toState(PTP_DISABLED) */
// }

// static void handleSignaling(PtpClock *ptpClock, Boolean isFromSelf)
// {
// }
static void issueDelayReqTimerExpired(PtpClock *ptpClock)
{
    switch (ptpClock->portDS.delayMechanism)
    {
    case E2E:

        if (ptpClock->portDS.portState != PTP_SLAVE)
        {
            break;
        }

        if (timerExpired(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer))
        {
            timerStart(DELAYREQ_INTERVAL_TIMER, getRand(pow2ms(ptpClock->portDS.logMinDelayReqInterval + 1)), ptpClock->itimer);
            //event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES
            issueDelayReq(ptpClock);
        }

        break;

    case P2P:

        if (timerExpired(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer))
        {
            timerStart(PDELAYREQ_INTERVAL_TIMER, getRand(pow2ms(ptpClock->portDS.logMinPdelayReqInterval + 1)), ptpClock->itimer);
            //event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES
            issuePDelayReq(ptpClock);
        }
        break;
    default:
        break;
    }
}

/*Pack and send on general multicast ip adress an Announce message*/
static void issueAnnounce(PtpClock *ptpClock)
{
    msgPackAnnounce(ptpClock, ptpClock->msgObuf);

    if (!netSendGeneral(&ptpClock->netPath, ptpClock->msgObuf, ANNOUNCE_LENGTH))
    {
        //issueAnnounce: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        //issueAnnounce
        ptpClock->sentAnnounceSequenceId++;
    }
}

/*Pack and send on event multicast ip adress a Sync message*/
static void issueSync(PtpClock *ptpClock)
{
    Timestamp originTimestamp;
    TimeInternal internalTime;

    /* try to predict outgoing time stamp */
    getTime(&internalTime);
    fromInternalTime(&internalTime, &originTimestamp);

    msgPackSync(ptpClock, ptpClock->msgObuf, &originTimestamp);

    if (!netSendEvent(&ptpClock->netPath, ptpClock->msgObuf, SYNC_LENGTH, &internalTime))
    {
        //issueSync: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        //issueSync
        ptpClock->sentSyncSequenceId++;

        /* sync TX timestamp is valid */

        if ((internalTime.seconds != 0) && (ptpClock->defaultDS.twoStepFlag))
        {
            // waitingForLoopback = false;
            addTime(&internalTime, &internalTime, &ptpClock->outboundLatency);
            issueFollowup(ptpClock, &internalTime);
        }
        else
        {
            // waitingForLoopback = ptpClock->twoStepFlag;
        }
    }
}

/*Pack and send on general multicast ip adress a FollowUp message*/
static void issueFollowup(PtpClock *ptpClock, const TimeInternal *time)
{
    Timestamp preciseOriginTimestamp;
    fromInternalTime(time, &preciseOriginTimestamp);

    msgPackFollowUp(ptpClock, ptpClock->msgObuf, &preciseOriginTimestamp);

    if (!netSendGeneral(&ptpClock->netPath, ptpClock->msgObuf, FOLLOW_UP_LENGTH))
    {
        //issueFollowup: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        //issueFollowup
    }
}

/*Pack and send on event multicast ip adress a DelayReq message*/
static void issueDelayReq(PtpClock *ptpClock)
{
    Timestamp originTimestamp;
    TimeInternal internalTime;
    getTime(&internalTime);
    fromInternalTime(&internalTime, &originTimestamp);

    msgPackDelayReq(ptpClock, ptpClock->msgObuf, &originTimestamp);

    if (!netSendEvent(&ptpClock->netPath, ptpClock->msgObuf, DELAY_REQ_LENGTH, &internalTime))
    {
        //issueDelayReq: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        //issueDelayReq
        ptpClock->sentDelayReqSequenceId++;

        /* Delay req TX timestamp is valid */

        if (internalTime.seconds != 0)
        {
            addTime(&internalTime, &internalTime, &ptpClock->outboundLatency);
            ptpClock->timestamp_delayReqSend = internalTime;
        }
    }
}

/*Pack and send on event multicast ip adress a PDelayReq message*/
static void issuePDelayReq(PtpClock *ptpClock)
{
    Timestamp originTimestamp;
    TimeInternal internalTime;
    getTime(&internalTime);
    fromInternalTime(&internalTime, &originTimestamp);

    msgPackPDelayReq(ptpClock, ptpClock->msgObuf, &originTimestamp);

    if (!netSendPeerEvent(&ptpClock->netPath, ptpClock->msgObuf, PDELAY_REQ_LENGTH, &internalTime))
    {
        //issuePDelayReq: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        //issuePDelayReq
        ptpClock->sentPDelayReqSequenceId++;

        /* Delay req TX timestamp is valid */

        if (internalTime.seconds != 0)
        {
            addTime(&internalTime, &internalTime, &ptpClock->outboundLatency);
            ptpClock->pdelay_t1 = internalTime;
        }
    }
}

/*Pack and send on event multicast ip adress a PDelayResp message*/
static void issuePDelayResp(PtpClock *ptpClock,
                            TimeInternal *time,
                            const MsgHeader *pDelayReqHeader)
{
    Timestamp requestReceiptTimestamp;
    fromInternalTime(time, &requestReceiptTimestamp);
    msgPackPDelayResp(ptpClock->msgObuf, pDelayReqHeader, &requestReceiptTimestamp);

    if (!netSendPeerEvent(&ptpClock->netPath, ptpClock->msgObuf, PDELAY_RESP_LENGTH, time))
    {
        //issuePDelayResp: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        if (time->seconds != 0)
        {
            /*Add latency*/
            addTime(time, time, &ptpClock->outboundLatency);
        }

        //issuePDelayResp
    }
}

/*Pack and send on event multicast ip adress a DelayResp message*/
static void issueDelayResp(PtpClock *ptpClock,
                           const TimeInternal *time,
                           const MsgHeader *delayReqHeader)
{
    Timestamp requestReceiptTimestamp;
    fromInternalTime(time, &requestReceiptTimestamp);
    msgPackDelayResp(ptpClock, ptpClock->msgObuf, delayReqHeader, &requestReceiptTimestamp);

    if (!netSendGeneral(&ptpClock->netPath, ptpClock->msgObuf, PDELAY_RESP_LENGTH))
    {
        //issueDelayResp: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        //issueDelayResp
    }
}

static void issuePDelayRespFollowUp(PtpClock *ptpClock,
                                    const TimeInternal *time,
                                    const MsgHeader *pDelayReqHeader)
{
    Timestamp responseOriginTimestamp;
    fromInternalTime(time, &responseOriginTimestamp);

    msgPackPDelayRespFollowUp(ptpClock->msgObuf, pDelayReqHeader, &responseOriginTimestamp);

    if (!netSendPeerGeneral(&ptpClock->netPath, ptpClock->msgObuf, PDELAY_RESP_FOLLOW_UP_LENGTH))
    {
        //issuePDelayRespFollowUp: can't sent
        toState(ptpClock, PTP_FAULTY);
    }
    else
    {
        //issuePDelayRespFollowUp
    }
}

//static void issueManagement(MsgHeader *header,MsgManagement *manage,PtpClock *ptpClock){}
