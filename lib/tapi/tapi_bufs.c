/** @file
 * @brief Test API to deal with buffers
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Buffers"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_PTHREAD_H
#include <pthread.h>
#else
#error Required pthread.h not found
#endif

#include "te_defs.h"
#include "logger_api.h"

/** The number of characters in a sinle log buffer. */
#define LOG_BUF_LEN (1024 * 10)
/** The number of buffers in log buffer pool. */
#define LOG_BUF_NUM 10

/** Internal presentation of log buffer. */
typedef struct tapi_log_buf {
    te_bool used; /**< Whether this buffer is already in use */
    char    ptr[LOG_BUF_LEN]; /**< Buffer data */
    size_t  cur_len; /**< The number of bytes currently used in buffer */
} tapi_log_buf;

/** Statically allocated pool of log buffers. */
static tapi_log_buf tapi_log_bufs[LOG_BUF_NUM];

/** The index of the last freed buffer, or -1 if unknown. */
static int tapi_log_buf_last_freed = 0;

/** Mutex used to protect shred log buffer pool. */
static pthread_mutex_t tapi_log_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Check that 'ptr_' is a valid log buffer */
#define VALIDATE_LOG_BUF(ptr_) \
    do {                                                           \
        assert((ptr_)->used == TRUE);                              \
        assert(((ptr_) >= tapi_log_bufs) &&                        \
               ((ptr_) - tapi_log_bufs) /                          \
                   sizeof(tapi_log_bufs[0]) <=                     \
               sizeof(tapi_log_bufs) / sizeof(tapi_log_bufs[0]) && \
               ((ptr_) - tapi_log_bufs) %                          \
                   sizeof(tapi_log_bufs[0]) == 0);                 \
    } while (0)


/* See description in tapi_bufs.h */
void
tapi_fill_buf(void *buf, size_t len)
{
    size_t  i;

    for (i = 0; i < len / sizeof(int); ++i)
    {
        ((int *)buf)[i] = rand();
    }
    for (i = (len / sizeof(int)) * sizeof(int); i < len; ++i)
    {
        ((unsigned char *)buf)[i] = rand();
    }
}


/* See description in tapi_bufs.h */
void *
tapi_make_buf(size_t min, size_t max, size_t *p_len)
{
    *p_len = rand_range(min, max);
    if (*p_len > 0)
    {
        void   *buf;

        buf = malloc(*p_len);
        if (buf == NULL)
        {
            ERROR("Memory allocation failure - EXIT");
            return NULL;
        }
        tapi_fill_buf(buf, *p_len);
        return buf;
    }
    else
    {
        return NULL;
    }
}

/* See description in tapi_bufs.h */
tapi_log_buf *
tapi_log_buf_alloc()
{
    int i;
    int id;

    pthread_mutex_lock(&tapi_log_buf_mutex);

    if (tapi_log_buf_last_freed != -1)
    {
        assert(tapi_log_bufs[tapi_log_buf_last_freed].used == FALSE &&
               tapi_log_bufs[tapi_log_buf_last_freed].cur_len == 0);

        id = tapi_log_buf_last_freed;
        tapi_log_buf_last_freed = -1;
        tapi_log_bufs[id].used = TRUE;
        pthread_mutex_unlock(&tapi_log_buf_mutex);

        return &tapi_log_bufs[id];
    }
    
    for (i = 0; i < LOG_BUF_NUM; i++)
    {
        if (!tapi_log_bufs[i].used)
        {
            assert(tapi_log_bufs[i].cur_len == 0);

            tapi_log_bufs[i].used = TRUE;
            pthread_mutex_unlock(&tapi_log_buf_mutex);

            return &tapi_log_bufs[i];
        }
    }
    pthread_mutex_unlock(&tapi_log_buf_mutex);

    /* There is no available buffer, wait until one freed */
    do {
        pthread_mutex_lock(&tapi_log_buf_mutex);
        if (tapi_log_buf_last_freed != -1)
        {
            assert(tapi_log_bufs[tapi_log_buf_last_freed].used == FALSE &&
                   tapi_log_bufs[tapi_log_buf_last_freed].cur_len == 0);

            id = tapi_log_buf_last_freed;
            tapi_log_buf_last_freed = -1;
            tapi_log_bufs[id].used = TRUE;
            pthread_mutex_unlock(&tapi_log_buf_mutex);
            
            return &tapi_log_bufs[id];
        }
        pthread_mutex_unlock(&tapi_log_buf_mutex);
        
        RING("Waiting for a tapi log buffer");
        sleep(1);
    } while (TRUE);

    assert(0);
    return NULL;
}

/* See description in tapi_bufs.h */
int
tapi_log_buf_append(tapi_log_buf *buf, const char *fmt, ...)
{
    int      rc;
    va_list  ap;
    char    *ptr;
    size_t   size;

    VALIDATE_LOG_BUF(buf);

    ptr = buf->ptr + buf->cur_len;
    size = sizeof(buf->ptr) - buf->cur_len;

    va_start(ap, fmt);
    rc = vsnprintf(ptr, size, fmt, ap);
    buf->cur_len += rc;
    va_end(ap);

    return rc;
}

const char *
tapi_log_buf_get(tapi_log_buf *buf)
{
    VALIDATE_LOG_BUF(buf);

    if (buf->cur_len == 0)
        buf->ptr[0] = '\0';

    return buf->ptr;
}

/* See description in tapi_bufs.h */
void
tapi_log_buf_free(tapi_log_buf *buf)
{
    VALIDATE_LOG_BUF(buf);

    pthread_mutex_lock(&tapi_log_buf_mutex);
    buf->used = FALSE;
    buf->cur_len = 0;
    tapi_log_buf_last_freed = 
        (buf - tapi_log_bufs) / sizeof(tapi_log_bufs[0]);
    pthread_mutex_unlock(&tapi_log_buf_mutex);
}
