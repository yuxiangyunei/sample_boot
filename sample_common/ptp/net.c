/* net.c */

#include "ptpd.h"
#include "usr_timer.h"

/**
 * @brief  Free all pbufs in the queue
 * @param  pQ the queue to be used
 * @retval None
 */
static void netQEmpty(rb_t *prb)
{
    rb_set_empty(prb);
}

/**
 * @brief  Check if something is in the queue
 * @param  pQ the queue to be used
 * @retval Boolean true if success
 */
static Boolean netQCheck(rb_t *prb)
{
    if (rb_is_empty(prb))
    {
        return false;
    }
    else
    {
        return true;
    }
}

static void event_ptp_rx_task(void *param)
{
    NetPath *netPath = (NetPath *)param;
    int32_t rx_size = 0;
    uint8_t *p_rx_data;
    pbuf_t *q = NULL;
    int32_t time_s;
    int32_t time_ns;
    UInteger16 data_len;
    for (;;)
    {
        rx_size = FreeRTOS_recvfrom(netPath->event_ptp_sock, (void *)&p_rx_data, PACKET_SIZE, FREERTOS_ZERO_COPY, NULL, &time_s, &time_ns);
        if (rx_size > 0)
        {
            if (!rb_is_full(&netPath->event_rb))
            {
                q = (pbuf_t *)rb_peek_w_buff(&netPath->event_rb);
                data_len = (UInteger16)rx_size;
                memcpy(q->payload, p_rx_data, data_len);
                q->tot_len = data_len;
                q->time_sec = time_s;
                q->time_nsec = time_ns;
                rb_w_idx_inc(&netPath->event_rb);
            }
        }
        if (rx_size >= 0) /* The buffer *must* be freed once it is no longer needed. */
        {
            FreeRTOS_ReleaseUDPPayloadBuffer(p_rx_data);
        }
    }
}

static void general_ptp_rx_task(void *param)
{
    NetPath *netPath = (NetPath *)param;
    int32_t rx_size = 0;
    uint8_t *p_rx_data;
    pbuf_t *q = NULL;
    int32_t time_s;
    int32_t time_ns;
    UInteger16 data_len;
    for (;;)
    {
        rx_size = FreeRTOS_recvfrom(netPath->general_ptp_sock, (void *)&p_rx_data, PACKET_SIZE, FREERTOS_ZERO_COPY, NULL, &time_s, &time_ns);
        if (rx_size > 0)
        {
            if (!rb_is_full(&netPath->general_rb))
            {
                q = (pbuf_t *)rb_peek_w_buff(&netPath->general_rb);
                data_len = (UInteger16)rx_size;
                memcpy(q->payload, p_rx_data, data_len);
                q->tot_len = data_len;
                q->time_sec = time_s;
                q->time_nsec = time_ns;
                rb_w_idx_inc(&netPath->general_rb);
            }
        }
        if (rx_size >= 0) /* The buffer *must* be freed once it is no longer needed. */
        {
            FreeRTOS_ReleaseUDPPayloadBuffer(p_rx_data);
        }
    }
}

/**
 * @brief  Start all of the UDP stuff
 * @param  netPath network object
 * @param  ptpClock PTP clock object
 * @retval Boolean success
 */
Boolean netInit(NetPath *netPath)
{
    struct freertos_sockaddr local_addr;

    if (0 == netPath->init_flag)
    {
        netPath->event_ptp_sock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
        if (netPath->event_ptp_sock != FREERTOS_INVALID_SOCKET)
        {
            netPath->event_ptp_addr.sin_addr = BROADCAST_DEFAULT_PTP_ADDRESS;
            netPath->event_ptp_addr.sin_port = FreeRTOS_htons(PTP_EVENT_PORT);
            netPath->peer_event_ptp_addr.sin_addr = BROADCAST_PEER_PTP_ADDRESS;
            netPath->peer_event_ptp_addr.sin_port = FreeRTOS_htons(PTP_EVENT_PORT);
            FreeRTOS_GetAddressConfiguration(&local_addr.sin_addr, NULL, NULL, NULL);
            local_addr.sin_port = FreeRTOS_htons(PTP_EVENT_PORT);
            FreeRTOS_bind(netPath->event_ptp_sock, &local_addr, sizeof(local_addr));
            /* Initialize the buffer queues. */
            rb_init(&netPath->event_rb, pbuf_t, netPath->event_buf, PBUF_QUEUE_SIZE);
            xTaskCreate(event_ptp_rx_task, "default_ptp", PTP_RX_TASK_STACK_SIZE, (void *)netPath, PTP_RX_TASK_PRIO, NULL);
        }
        else
        {
            return false;
        }

        netPath->general_ptp_sock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
        if (netPath->general_ptp_sock != FREERTOS_INVALID_SOCKET)
        {
            netPath->general_ptp_addr.sin_addr = BROADCAST_DEFAULT_PTP_ADDRESS;
            netPath->general_ptp_addr.sin_port = FreeRTOS_htons(PTP_GENERAL_PORT);
            netPath->peer_general_ptp_addr.sin_addr = BROADCAST_PEER_PTP_ADDRESS;
            netPath->peer_general_ptp_addr.sin_port = FreeRTOS_htons(PTP_GENERAL_PORT);
            FreeRTOS_GetAddressConfiguration(&local_addr.sin_addr, NULL, NULL, NULL);
            local_addr.sin_port = FreeRTOS_htons(PTP_GENERAL_PORT);
            FreeRTOS_bind(netPath->general_ptp_sock, &local_addr, sizeof(local_addr));
            /* Initialize the buffer queues. */
            rb_init(&netPath->general_rb, pbuf_t, netPath->general_buf, PBUF_QUEUE_SIZE);
            xTaskCreate(general_ptp_rx_task, "peer_ptp", PTP_RX_TASK_STACK_SIZE, (void *)netPath, PTP_RX_TASK_PRIO, NULL);
        }
        else
        {
            return false;
        }
        netPath->init_flag = 1;
    }
    /* Return a success code. */
    return true;
}

/**
 * @brief  Wait for a packet to come in on either port.  For now, there is no wait.
 * Simply check to see if a packet is available on either port and return 1,
 * otherwise return 0.
 * @param  netPath network object
 * @param  timeout not used
 * @retval Integer32 number > 0 if there are some data
 */
Integer32 netSelect(NetPath *netPath)
{
    /* Check the packet queues.  If there is data, return true. */
    if (netQCheck(&netPath->event_rb) || netQCheck(&netPath->general_rb))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief  Delete all waiting packets in Event queue
 * @param  netPath network object
 * @retval None
 */
void netEmptyEventQ(NetPath *netPath)
{
    netQEmpty(&netPath->event_rb);
}
void netEmptyGeneralQ(NetPath *netPath)
{
    netQEmpty(&netPath->general_rb);
}

static ssize_t netRecv(Octet *buf, TimeInternal *time, rb_t *prb)
{
    ssize_t length;
    pbuf_t *p;

    if (!rb_is_empty(prb))
    {
        p = (pbuf_t *)rb_peek_r_buff(prb);

        if (NULL != time)
        {
            time->seconds = p->time_sec;
            time->nanoseconds = p->time_nsec;
        }
        /* Copy the PBUF payload into the buffer. */

        length = p->tot_len;
        memcpy(buf, p->payload, length);

        rb_r_idx_inc(prb);

        return length;
    }
    else
    {
        return 0;
    }
}

ssize_t netRecvEvent(NetPath *netPath, Octet *buf, TimeInternal *time)
{
    return netRecv(buf, time, &netPath->event_rb);
}

ssize_t netRecvGeneral(NetPath *netPath, Octet *buf, TimeInternal *time)
{
    return netRecv(buf, time, &netPath->general_rb);
}

static ssize_t netSend(const Octet *buf, UInteger16 length, TimeInternal *time,
                       struct freertos_sockaddr *addr, Socket_t sock)
{
    /* send the buffer. */
    FreeRTOS_sendto(sock, buf, length, 0, addr, (int32_t *)&time->seconds, (int32_t *)&time->nanoseconds);
    return length;
}

ssize_t netSendEvent(NetPath *netPath, const Octet *buf, UInteger16 length, TimeInternal *time)
{
    return netSend(buf, length, time, &netPath->event_ptp_addr, netPath->event_ptp_sock);
}

ssize_t netSendGeneral(NetPath *netPath, const Octet *buf, UInteger16 length)
{
    return netSend(buf, length, NULL, &netPath->general_ptp_addr, netPath->general_ptp_sock);
}

ssize_t netSendPeerGeneral(NetPath *netPath, const Octet *buf, UInteger16 length)
{
    return netSend(buf, length, NULL, &netPath->peer_general_ptp_addr, netPath->general_ptp_sock);
}

ssize_t netSendPeerEvent(NetPath *netPath, const Octet *buf, UInteger16 length, TimeInternal *time)
{
    return netSend(buf, length, time, &netPath->peer_event_ptp_addr, netPath->event_ptp_sock);
}
