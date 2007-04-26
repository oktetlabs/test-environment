/** @file
 * @brief Logger subsystem API - TA side
 *
 * Macros and functions for internal using by Logger subsystem itself.
 *
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_TA_INTERNAL_H__
#define __TE_LOGGER_TA_INTERNAL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_raw_log.h"
#include "logger_defs.h"
#include "te_errno.h"
#include "logger_ta_lock.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Maximum arguments processed in this implementation.
 * Implementation of the fast log has a lot of dependencies
 * from this number.
 */
#define TA_LOG_ARGS_MAX     12

#ifndef TA_LOG_FORCE_NEW
/* 
 * Drop a new or the oldest message, if no enough space:
 * 0 - drop a new message, !0 - drop the oldest message.
 */
#define TA_LOG_FORCE_NEW    0
#endif


/*
 * Following macros provide the means for ring buffer processing.
 */

/* Length of separate element into the ring buffer  */
#define LGR_RB_ELEMENT_LEN      sizeof(struct lgr_mess_header)

/* Maximum message length in bytes TODO: 3595*/
#define LGR_RB_BIG_MESSAGE_LEN  3597

/* Maximum number of big messages to be logged into Ring Buffer */
#if 0
#define LGR_MAX_BIG_MESSAGES    100
#else
#define LGR_MAX_BIG_MESSAGES    1000
#endif

/* Total of the ring buffer elements */
#define LGR_TOTAL_RB_EL \
    (uint32_t)((LGR_RB_BIG_MESSAGE_LEN * \
                LGR_MAX_BIG_MESSAGES) / LGR_RB_ELEMENT_LEN)

/* Total of the ring buffer bytes */
#define LGR_TOTAL_RB_BYTES (uint32_t)(LGR_TOTAL_RB_EL * LGR_RB_ELEMENT_LEN)

/** This macro corrects head/tail value on ring buffer physical border */
#define LGR_RB_CORRECTION(_val, _res) \
    do {                                        \
        if ((_val) < LGR_TOTAL_RB_EL)           \
            (_res) = (_val);                    \
        else                                    \
            (_res) = (_val) - LGR_TOTAL_RB_EL;  \
    } while (0)

/** Get ring buffer unused elements */
#define LGR_RB_UNUSED(_rb)  ((_rb)->unused)

/** Get ring buffer head element */
#define LGR_RB_HEAD(_rb)    ((_rb)->head)

/** Get ring buffer tail element */
#define LGR_RB_TAIL(_rb)    ((_rb)->tail)

/** Get/Set header argument */
#define LGR_GET_ARG(_hdr, _narg)        ((_hdr).args[_narg])
#define LGR_SET_ARG(_hdr, _narg, _val)  ((_hdr).args[_narg] = (_val))

/** This macro for using in following macros only */
#define LGR_GET_MESSAGE_ADDR(_rb, _pos) \
    ((struct lgr_mess_header *)((_rb)->rb) + (_pos))

/** Get message memory address in ring buffer */
#define LGR_GET_MESSAGE_ARRAY(_rb, _pos)  \
    (uint8_t *)LGR_GET_MESSAGE_ADDR((_rb), (_pos))

/** Get/Set message separate fields */
#define LGR_GET_ELEMENTS_FIELD(_rb, _pos) \
    LGR_GET_MESSAGE_ADDR((_rb), (_pos))->elements
#define LGR_SET_ELEMENTS_FIELD(_rb, _pos, _val) \
    LGR_GET_MESSAGE_ADDR((_rb), (_pos))->elements = (_val)

#define LGR_GET_MARK_FIELD(_rb, _pos) \
    LGR_GET_MESSAGE_ADDR((_rb), (_pos))->mark
#define LGR_SET_MARK_FIELD(_rb, _pos, _val) \
    LGR_GET_MESSAGE_ADDR((_rb), (_pos))->mark = (_val)

#define LGR_GET_SEQUENCE_FIELD(_rb, _pos) \
    LGR_GET_MESSAGE_ADDR((_rb), (_pos))->sequence
#define LGR_SET_SEQUENCE_FIELD(_rb, _pos, _val) \
    LGR_GET_MESSAGE_ADDR((_rb), (_pos))->sequence = (_val)


/** Type of argument native for a stack */
typedef long ta_log_arg;

/*
 * It defines the structure of the message header.
 * In the case of using fast logging the maximum logging value
 * is a length of this structure. So, the length of this structure
 * will be the length of ring buffer element.
 * In the case of using slow logging the logged message can consist of
 * the number of consequent ring buffer elements.
 */
typedef struct lgr_mess_header {
    uint32_t        elements;       /**< Number of consequent ring buffer
                                         elements in message */
    uint32_t        sequence;       /**< Sequence number for this message */
    uint32_t        mark;           /**< Flag: message is marked as
                                         processed at this time */

    te_log_ts_sec   sec;            /**< Seconds of the timestamp */
    te_log_ts_usec  usec;           /**< Microseconds of the timestamp */
    te_log_level    level;          /**< Log level mask to be passed 
                                         in raw log*/
    const char     *user;           /**< User_name string location */
    const char     *fmt;            /**< Format string location */

    unsigned int    n_args;                 /**< Number of arguments */
    ta_log_arg      args[TA_LOG_ARGS_MAX];  /**< Arguments */

} lgr_mess_header;


/**
 * Get timestamp for a log message.
 *
 * @param sec       Location for seconds
 * @param usec      Location for microseconds
 */
static inline void
ta_log_timestamp(te_log_ts_sec *sec, te_log_ts_usec *usec)
{
    struct timeval  tv;

    assert(sec != NULL);
    assert(usec != NULL);

    gettimeofday(&tv, NULL);

    *sec  = tv.tv_sec;
    *usec = tv.tv_usec;
}


/**
 * The main ring buffer structure.
 * Element of the ring buffer is multiple to the struct lgr_mess_header
 */
struct lgr_rb {
    uint32_t head;    /**< Head ring buffer element number */
    uint32_t tail;    /**< Tail ring buffer element number */
    uint32_t unused;  /**< Number of unused ring buffer elements */
    uint8_t *rb;      /**< Pointer to the ring buffer location */
};

extern struct lgr_rb log_buffer;
extern uint32_t      log_sequence;


/**
 * Initialize ring buffer.
 *
 * @param ring_buffer Ring buffer location.
 *
 * @retval  0 Success.
 * @retval -1 Failure.
 */
static inline int
lgr_rb_init(struct lgr_rb *ring_buffer)
{
    memset(ring_buffer, 0, sizeof(struct lgr_rb));

    ring_buffer->rb =
        (uint8_t *)calloc(1, LGR_TOTAL_RB_BYTES * sizeof(uint8_t));
    if (ring_buffer->rb == NULL)
        return -1;

    ring_buffer->unused = LGR_TOTAL_RB_EL;
    ring_buffer->head = ring_buffer->tail = 0;

    return 0;
}

/**
 * Destroy ring buffer.
 *
 * @param ring_buffer Ring buffer location.
 *
 * @retval  0  Success.
 * @retval -1  Failure.
 */
static inline int
lgr_rb_destroy(struct lgr_rb *ring_buffer)
{

    if (ring_buffer->rb == NULL)
        return -1;

    free(ring_buffer->rb);
    return 0;
}

static inline void
lgr_rb_view_head(struct lgr_rb *ring_buffer, uint32_t position)
{
    printf("unused:%d, head:%d, tail:%d elements:%d, mark:%d, "
           "sequence:%d\n",
           (int)LGR_RB_UNUSED(ring_buffer), 
           (int)LGR_RB_HEAD(ring_buffer),
           (int)LGR_RB_TAIL(ring_buffer),
           (int)LGR_GET_ELEMENTS_FIELD(ring_buffer, position),
           (int)LGR_GET_MARK_FIELD(ring_buffer, position),
           (int)LGR_GET_SEQUENCE_FIELD(ring_buffer, position));
}

/**
 * Remove oldest message.
 *
 * @param ring_buffer Ring buffer location.
 *
 * @retval  Unused ring buffer elements.
 */
static inline uint32_t
lgr_rb_remove_oldest(struct lgr_rb *ring_buffer)
{
    uint32_t mess_len;
    uint32_t head;

    if (ring_buffer->unused == LGR_TOTAL_RB_EL)
        return LGR_TOTAL_RB_EL;

    head = LGR_RB_HEAD(ring_buffer);
    mess_len = LGR_GET_ELEMENTS_FIELD(ring_buffer, head);
    head += mess_len;

    LGR_RB_CORRECTION(head, LGR_RB_HEAD(ring_buffer));

    ring_buffer->unused += mess_len;
    return ring_buffer->unused;
}


/**
 * Allocate ring buffer space for nmbr elements.
 *
 * @param ring_buffer Ring buffer location.
 * @param nmbr        Number of elements need to be allocated.
 * @param position    Allocated space first element number.
 *
 * @retval Number of allocated elements.
 */
static inline uint32_t
lgr_rb_allocate_space(struct lgr_rb *ring_buffer,
                      uint32_t nmbr, uint32_t *position)
{
    uint32_t tail;

    if (ring_buffer->unused < nmbr)
    {
        return 0;
    }

    tail = *position = ring_buffer->tail;
    tail += nmbr;

    LGR_RB_CORRECTION(tail, ring_buffer->tail);
    ring_buffer->unused -= nmbr;

    return nmbr;
}

/**
 * Allocate ring buffer space for head element of message.
 * If force flag is turn on and ring buffer does not have an unused space
 * the oldest message will be removed to allocate new one.
 *
 * @param ring_buffer Ring buffer location.
 * @param force       Remove oldest message if unused space is absent.
 * @param position    Number of the first element of allocated space.
 *
 * @retval  Number of allocated elements.
 */
static inline uint32_t
lgr_rb_allocate_head(struct lgr_rb *ring_buffer,
                     uint32_t force, uint32_t *position)
{
    log_sequence++;

    if ((ring_buffer->unused == 0) &&
        ((force == 0) ||
         (LGR_GET_MARK_FIELD(ring_buffer, ring_buffer->head) == 1)))
    {
        return 0;
    }

    if (ring_buffer->unused == 0)
        lgr_rb_remove_oldest(ring_buffer);

    lgr_rb_allocate_space(ring_buffer, 1, position);

    LGR_SET_ELEMENTS_FIELD(ring_buffer, *position, 1);
    LGR_SET_MARK_FIELD(ring_buffer, *position, 0);
    LGR_SET_SEQUENCE_FIELD(ring_buffer, *position, log_sequence);

    return 1;
}

/**
 * Allocate ring buffer space and copy length bytes from start address.
 * Only 256 bytes can be copied if ring buffer unused space is enought.
 *
 * @param ring_buffer    Ring buffer location.
 * @param start          Byte array start address.
 * @param length         The length of copied space.
 * @param arg_addr       Argument address (memory dump
 *                       address or string address).
 *
 * @retval  Number of allocated elements.
 */
static inline uint32_t
lgr_rb_allocate_and_copy(struct lgr_rb *ring_buffer,
                         void *start, uint32_t length,
                         uint8_t **arg_addr)
{
    uint32_t start_pos = 0;
    uint32_t tail;
    uint32_t need_elements;

    if (length > TE_LOG_FIELD_MAX)
        length = TE_LOG_FIELD_MAX;

    need_elements = (length % LGR_RB_ELEMENT_LEN) ?
                     (uint32_t)(length / LGR_RB_ELEMENT_LEN) + 1 :
                     (uint32_t)(length / LGR_RB_ELEMENT_LEN);

    if (lgr_rb_allocate_space(ring_buffer, need_elements, &start_pos) !=
            need_elements)
    {
        return 0;
    }

    *arg_addr = LGR_GET_MESSAGE_ARRAY(ring_buffer, start_pos);

    tail = start_pos = ring_buffer->tail;


    if (tail < LGR_TOTAL_RB_EL)
    {
        memcpy(*arg_addr, start, length);
    }
    else
    {
        uint32_t  length_aux;
        uint8_t  *start_aux = start;

        length_aux = (LGR_TOTAL_RB_EL - start_pos) * LGR_RB_ELEMENT_LEN;
        memcpy(*arg_addr, start_aux, length_aux);

        *start_aux += length_aux;
        length_aux = length - length_aux;
        memcpy(LGR_GET_MESSAGE_ARRAY(ring_buffer, 0),
               start_aux, length_aux);
    }

    return need_elements;
}

/**
 * Get ring buffer elements
 *
 * @param ring_buffer    Ring buffer location.
 * @param position       Obtained element position.
 * @param length         Number of bytes need to be obtained.
 * @param destination    Destination location.
 */
static inline void
lgr_rb_get_elements(struct lgr_rb *ring_buffer, uint32_t position,
                    uint32_t length, uint8_t *destination)
{
    uint32_t  pos_aux = position;
    uint32_t  length_aux;

    length_aux = length * LGR_RB_ELEMENT_LEN;
    pos_aux += length;

    if (pos_aux <= LGR_TOTAL_RB_EL)
    {
        memcpy(destination, LGR_GET_MESSAGE_ARRAY(ring_buffer, position),
               length_aux);
    }
    else
    {
        uint8_t  *start_aux = LGR_GET_MESSAGE_ARRAY(ring_buffer, position);

        length_aux = (LGR_TOTAL_RB_EL - position) * LGR_RB_ELEMENT_LEN;

        memcpy(destination, start_aux, length_aux);

        start_aux = LGR_GET_MESSAGE_ARRAY(ring_buffer, 0);

        destination += length_aux;

        length_aux = (pos_aux - LGR_TOTAL_RB_EL) * LGR_RB_ELEMENT_LEN; 

        memcpy(destination, start_aux, length_aux);
   }
}


extern void logfork_log_message(const char *file, unsigned int line,
                                te_log_ts_sec sec, te_log_ts_usec usec,
                                unsigned int level,
                                const char *entity, const char *user,
                                const char *fmt, va_list ap); 

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TA_INTERNAL_H__ */
