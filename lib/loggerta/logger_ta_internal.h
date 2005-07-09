/** @file
 * @brief Logger subsystem API - TA side
 *
 * Macros and functions for internal using by Logger subsystem itself.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_TA_INTERNAL_H__
#define __TE_LOGGER_TA_INTERNAL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdio.h>
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_raw_log.h"
#include "logger_defs.h"
#include "logger_int.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef int ta_lgr_lock_key;

/*
 * Following macro should be used to protect the
 * log buffer/file during processing
 */
#if HAVE_PTHREAD_H

#include <pthread.h>

extern pthread_mutex_t  ta_lgr_mutex;

static inline int
ta_lgr_lock_init(void)
{
    int rc = pthread_mutex_init(&ta_lgr_mutex, NULL);

    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_init() failed: errno=%d\n",
                __FUNCTION__, errno);
    }
    return rc;
}

static inline int
ta_lgr_lock_destroy(void)
{
    return 0;
}

static inline int
ta_lgr_lock(ta_lgr_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_lock(&ta_lgr_mutex);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_lock() failed: errno=%d",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_lgr_unlock(ta_lgr_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_unlock(&ta_lgr_mutex);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_unlock() failed: errno=%d",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_lgr_trylock(ta_lgr_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_trylock(&ta_lgr_mutex);
    if ((rc != 0) && (errno != EBUSY))
    {
        fprintf(stderr, "%s(): pthread_mutex_trylock() failed: errno=%d",
                __FUNCTION__, errno);
    }

    return rc;
}

#elif HAVE_SEMAPHORE_H

#error We only have semaphore.h
#include <semaphore.h>

extern sem_t    ta_lgr_sem;

static inline int
ta_lgr_lock_init(void)
{
    int rc = sem_init(&ta_lgr_sem, 0, 1);

    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_init() failed: errno=%d\n",
                __FUNCTION__, errno);
    }
    return rc;
}

static inline int
ta_lgr_lock_destroy(void)
{
    int rc = sem_destroy(&ta_lgr_sem);

    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_destroy() failed: errno=%d\n",
                __FUNCTION__, errno);
    }
    return rc;
}


static inline int
ta_lgr_lock(ta_lgr_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = sem_lock(&ta_lgr_sem);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_lock() failed: errno=%d\n",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_lgr_unlock(ta_lgr_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = sem_unlock(&ta_lgr_sem);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_unlock() failed: errno=%d\n",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_lgr_trylock(ta_lgr_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = sem_trylock(&ta_lgr_sem);
    if ((rc != 0) && (errno != EBUSY))
    {
        fprintf(stderr, "%s(): sem_trylock() failed: errno=%d\n",
                __FUNCTION__, errno);
    }

    return rc;
}

#elif HAVE_VXWORKS_H

#error "We only have vxworks.h"
#define LGR_LOCK(lock)    lock = intLock()
#define LGR_UNLOCK(lock)  intUnlock(lock)

#else

#error "We have no any locks"
#define LGR_LOCK(lock)    lock = 1  
#define LGR_UNLOCK(lock)  lock = 0

#endif


/** Create timestamp */
#define LGR_TIMESTAMP(_tval)    gettimeofday(_tval, NULL)


/** Maximum arguments processed in this implementation */
#define LGR_MAX_ARGS  12

/**
 * These macros carry out preliminary argument processing
 * at compilation time to make logging as quick as possible.
 * Each macro processes only one argument and calls following macro.
 * This way only limited number of arguments can be processed
 * (twelve arguments for this implementation).
 * Each macro produces one pair of elements:
 *   - "4, (uint32_t)(arg + 0)"  , if argument exists
 *   or
 *   - "0, (uint32_t)(+0)"      , if argument is not specified.
 * This is done by following way: #a[0] converts argument to string;
 * if argument is not present, first symbol of this string is equal
 * to 0, otherwise it is not; the obtained symbol is converted
 * to 0 or 1 by means of !! and multiplied with 4 ( = sizeof(uint32_t)).
 * So, this way  valid numbers of arguments can be known in advance.
 */
#define LARG1(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0)
#define LARG2(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG1(args)
#define LARG3(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG2(args)
#define LARG4(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG3(args)
#define LARG5(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG4(args)
#define LARG6(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG5(args)
#define LARG7(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG6(args)
#define LARG8(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG7(args)
#define LARG9(a, args...)  !!(#a[0]) * 4, (uint32_t)(a +0), LARG8(args)
#define LARG10(a, args...) !!(#a[0]) * 4, (uint32_t)(a +0), LARG9(args)
#define LARG11(a, args...) !!(#a[0]) * 4, (uint32_t)(a +0), LARG10(args)
#define LARG12(a, args...) !!(#a[0]) * 4, (uint32_t)(a +0), LARG11(args)

#if TALOGDEBUG
#define LOGF_PUT(_us, _fs...) log_message_print(_us, _fs)
#else
#define LOGF_MESS(_lvl, _us, _fs, _args...) \
    log_message_fast(_lvl, _us, _fs, LARG12(_args))
#define LOGF_PUT(_us, _fs, _args...) LOGF_MESS(0x0, _us, _fs, _args)
#endif

/*
 * Following macros provide the means for ring buffer processing.
 */

/* Wait until unused resources in ring buffer */
#define LGR_RB_FORCE            0

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
#define LGR_GET_ARG(_hdr, _narg)        *(&((_hdr).arg1) + (_narg))
#define LGR_SET_ARG(_hdr, _narg, _val)  *(&((_hdr).arg1) + (_narg)) = (_val)

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

/*
 * Following macros are auxiliary for routine providing the debugging print.
 */
#define LGR_AUX_STR_LEN  10

#define LGR_DEBUG_PRT(_b, _e, _t) \
    do {                                           \
        memset(tmp_str, 0, TE_LOG_FIELD_MAX);  \
        strncpy(tmp_str, (_b), ((_e) - (_b) + 1)); \
        fprintf(stdout, tmp_str, va_arg(ap, _t));  \
        (_b) = (_e);                               \
        (_b)++;                                    \
    } while (0)

#define LGR_GET_DIGITS(_aux, _b, _e) \
    do {                                      \
        int res;                              \
        do {                                  \
            ++(_e);                           \
            res = *(_e) - 0x30;               \
        } while ((res >= 0) && (res <= 9));   \
        memset((_aux), 0, LGR_AUX_STR_LEN);   \
        strncpy((_aux), (_b), ((_e) - (_b))); \
        (_b) = (_e); (_b)++;                  \
    } while (0)

/*
 * Following macros are auxiliary for routine providing slow logging.
 */

#define LGR_PUT_MD_LIST(_mdl, _narg, _addr, _len) \
    do {                                                          \
            md_list *tmp_list =                                   \
                (struct md_list *)malloc(sizeof(struct md_list)); \
            (_mdl).last->next = tmp_list;                         \
            (_mdl).last = tmp_list;                               \
            tmp_list->narg = (_narg);                             \
            tmp_list->addr = (_addr);                             \
            tmp_list->length = (_len);                            \
            tmp_list->next = &(_mdl);                             \
    } while (0)

#define LGR_FREE_MD_LIST(_mdl) \
    do {                                       \
        while ((_mdl).next != &(_mdl))         \
        {                                      \
            (_mdl).last = (_mdl).next;         \
            (_mdl).next = (_mdl).next->next;   \
            free((_mdl).last);                 \
        }                                      \
        (_mdl).next = &(_mdl);                 \
        (_mdl).last = &(_mdl);                 \
    } while (0)

typedef struct md_list {
    struct md_list *next;
    struct md_list *last;
    uint32_t        narg;
    uint8_t        *addr;
    uint32_t        length;
} md_list;

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
    struct timeval  timestamp;
    te_log_level_t  level;          /**< Log level mask to be passed 
                                         in raw log*/
    const char     *user_name;      /**< User_name string location */
    const char     *fs;             /**< Format string location */
    uint32_t        arg1;
    uint32_t        arg2;
    uint32_t        arg3;
    uint32_t        arg4;
    uint32_t        arg5;
    uint32_t        arg6;
    uint32_t        arg7;
    uint32_t        arg8;
    uint32_t        arg9;
    uint32_t        arg10;
    uint32_t        arg11;
    uint32_t        arg12;
} lgr_mess_header;

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

extern int log_entries_fast;
extern int log_entries_slow;


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
                         uint8_t *start, uint32_t length,
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


/**
 * Register message in the raw log (fast mode).
 *
 * @param  user_name      Arbitrary "user name";
 * @param  form_str       Raw log format string. This string should contain
 *                        conversion specifiers if some arguments following;
 * @param  argl1..argl12  Auxiliary args containing information about
 *                        appropriate arg;
 * @param  arg1..arg12    Arguments passed into the function according to
 *                        raw log format string description.
 */
static inline void
log_message_fast(uint16_t level, const char *user_name,
                 const char *form_str,
                 int argl1,  uint32_t arg1,  int argl2,  uint32_t arg2,
                 int argl3,  uint32_t arg3,  int argl4,  uint32_t arg4,
                 int argl5,  uint32_t arg5,  int argl6,  uint32_t arg6,
                 int argl7,  uint32_t arg7,  int argl8,  uint32_t arg8,
                 int argl9,  uint32_t arg9,  int argl10, uint32_t arg10,
                 int argl11, uint32_t arg11, int argl12, uint32_t arg12)
{
    ta_lgr_lock_key key;
    uint32_t        position;
    int             res;

    struct lgr_mess_header *message;

    log_entries_fast++;

    if (log_buffer.rb == NULL)
        return;

    if (ta_lgr_lock(key) != 0)
        return;
    res = lgr_rb_allocate_head(&log_buffer, LGR_RB_FORCE, &position);
    if (res == 0)
    {
        (void)ta_lgr_unlock(key);
        return;
    }

    message = (struct lgr_mess_header *)LGR_GET_MESSAGE_ARRAY(&log_buffer,
                                                              position);

    LGR_TIMESTAMP(&(message->timestamp));
    message->user_name = user_name;
    message->level = level;
    message->fs = form_str;

    if (argl1 > 0)
    {
        message->arg1 = arg1;
        if (argl2 > 0)
        {
            message->arg2 = arg2;
            if (argl3 > 0)
            {
                message->arg3 = arg3;
                if (argl4 > 0)
                {
                    message->arg4 = arg4;
                    if (argl5 > 0)
                    {
                        message->arg5 = arg5;
                        if (argl6 > 0)
                        {
                            message->arg6 = arg6;
                            if (argl7 > 0)
                            {
                                message->arg7 = arg7;
                                if (argl8 > 0)
                                {
                                    message->arg8 = arg8;
                                    if (argl9 > 0)
                                    {
                                        message->arg9 = arg9;
                                        if (argl10 > 0)
                                        {
                                            message->arg10 = arg10;
                                            if (argl11 > 0)
                                            {
                                                message->arg11 = arg11;
                                                if (argl12 > 0)
                                                    message->arg12 = arg12;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#if 0
    lgr_rb_view_head(&log_buffer, position);
#endif
    (void)ta_lgr_unlock(key);
}

#if TALOGDEBUG
/*
 * Print message in stdout (debug mode).
 *
 * @param  user_name      Arbitrary "user name";
 * @param  form_str       Raw log format string. This string should contain
 *                        conversion specifiers if some arguments following;
 * @param  ...            Arguments passed into the function according to
 *                        raw log format string description.
 */
extern void log_message_print(const char *us, const char *fs, ...);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TA_INTERNAL_H__ */
