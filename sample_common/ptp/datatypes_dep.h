#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

#include <stdint.h>
#include "drivers.h"
#include "rtos.h"
#include "tcpip.h"

#include "rb.h"

/**
 *\file
 * \brief Implementation specific datatype

 */
#include "constants_dep.h"
/**
 * \brief 5.2 Primitive data types specification
 */
typedef bool Boolean; /**< Boolean primitive */
/* typedef enum {FALSE=0, TRUE} Boolean; */
typedef unsigned char Enumeration4; /**< 4-bit enumeration */
typedef unsigned char Enumeration8; /**< 8-bit enumeration */
typedef unsigned short Enumeration16; /**< 16-bit enumeration */
typedef unsigned char UInteger4; /**< 4-bit unsigned integer */
typedef signed char Integer8; /**< 8-bit signed integer */
typedef unsigned char UInteger8; /**< 8-bit unsigned integer */
typedef signed short Integer16; /**< 16-bit signed integer */
typedef unsigned short UInteger16; /**< 16-bit unsigned integer */
typedef signed int Integer32; /**< 32-bit signed integer */
typedef unsigned int UInteger32; /**< 32-bit unsigned integer */

struct ptptime_t {
	int32_t tv_sec;
	int32_t tv_nsec;
};

/**
 * \struct UInteger48
 * \brief 48-bit unsigned integer
 */
typedef struct {
	unsigned int lsb;
	unsigned short msb;
} UInteger48;

/**
 * \brief 64-bit signed integer
 */

/*
 typedef struct
 {
 unsigned int lsb;
 signed int msb;
 } Integer64;
 */

typedef int64_t Integer64;

typedef unsigned char Nibble; /**< 4-bit data without numerical representation */
typedef char Octet; /**< 8-bit data without numerical representation */

/**
 * \brief Struct used to average the offset from master and the one way delay
 *
 * Exponencial smoothing
 * 
 * alpha = 1/2^s
 * y[1] = x[0]
 * y[n] = alpha * x[n-1] + (1-alpha) * y[n-1]
 */

typedef struct {
	Integer32 y_prev, y_sum;
	Integer16 s;
	Integer16 s_prev;
	Integer32 n;
} Filter;

typedef struct
{
	/** pointer to the actual data in the buffer */
	uint8_t payload[PACKET_SIZE];
	UInteger16 tot_len;
	int32_t time_sec;
	int32_t time_nsec;
} pbuf_t;

/**
 * \brief Struct used to store network datas
 */

typedef struct
{
	rb_t event_rb;
	rb_t general_rb;
	pbuf_t event_buf[PBUF_QUEUE_SIZE];
	pbuf_t general_buf[PBUF_QUEUE_SIZE];

	Socket_t event_ptp_sock;
	Socket_t general_ptp_sock;
	struct freertos_sockaddr event_ptp_addr;
	struct freertos_sockaddr general_ptp_addr;
	struct freertos_sockaddr peer_event_ptp_addr;
	struct freertos_sockaddr peer_general_ptp_addr;
	uint32_t init_flag;
} NetPath;

/* define compiler specific symbols */
#if defined   ( __CC_ARM   )
typedef long ssize_t;
#elif defined ( __ICCARM__ )
typedef long ssize_t;
#elif defined (  __GNUC__  )
//typedef long ssize_t;
#elif defined   (  __TASKING__  )
typedef long ssize_t;
#endif

#endif /*DATATYPES_DEP_H_*/
