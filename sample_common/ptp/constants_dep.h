/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/**
 *\file
 * \brief Plateform-dependent constants definition
 *
 * This header defines all includes and constants which are plateform-dependent
 *
 */

/* platform dependent */

#define IF_NAMESIZE             2
#define INET_ADDRSTRLEN         16

/* #define PTPD_DBGVV */
/* #define PTPD_DBGV */
/* #define PTPD_DBG */
/* #define PTPD_ERR */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <limits.h>

#ifndef stdout
#define stdout 1
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define PTPD_LSBF
#elif BYTE_ORDER == BIG_ENDIAN
#define PTPD_MSBF
#endif

/* pow2ms(a) = round(pow(2,a)*1000) */

#define pow2ms(a) (((a)>0) ? (1000 << (a)) : (1000 >>(-(a))))

#define ADJ_FREQ_MAX  500000

/* UDP/IPv4 dependent */

#define PTP_UUID_LENGTH 6
#define CLOCK_IDENTITY_LENGTH   8
#define FLAG_FIELD_LENGTH    2

#define PACKET_SIZE  300 /* ptpdv1 value kept because of use of TLV... */

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"
#define PEER_PTP_DOMAIN_ADDRESS "224.0.0.107"
#define BROADCAST_DEFAULT_PTP_ADDRESS 0xe0000181UL
#define BROADCAST_PEER_PTP_ADDRESS 0xe000006bUL

#define PBUF_QUEUE_SIZE 8

#define PTP_RX_TASK_STACK_SIZE (1024U)
#define PTP_RX_TASK_PRIO (4U)

#endif /*CONSTANTS_DEP_H_*/

