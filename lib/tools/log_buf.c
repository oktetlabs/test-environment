/** @file
 * @brief API to collect long log messages
 *
 * Implementation of API to collect long log messages.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Oleg Kravtosv <Oleg.Kravtosv@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Log Buffers"

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
typedef struct te_log_bug {
    te_bool used; /**< Whether this buffer is already in use */
    char    ptr[LOG_BUF_LEN]; /**< Buffer data */
    size_t  cur_len; /**< The number of bytes currently used in buffer */
} te_log_bug;

/** Statically allocated pool of log buffers. */
static te_log_bug te_log_bugs[LOG_BUF_NUM];

/** The index of the last freed buffer, or -1 if unknown. */
static int te_log_bug_last_freed = 0;

/** Mutex used to protect shred log buffer pool. */
static pthread_mutex_t te_log_bug_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Check that 'ptr_' is a valid log buffer */
#define VALIDATE_LOG_BUF(ptr_) \
    do {                                                            \
        assert((ptr_)->used == TRUE);                               \
        assert(((ptr_) >= te_log_bugs));                          \
        assert(((ptr_) - te_log_bugs) /                           \
                   sizeof(te_log_bugs[0]) <=                      \
               sizeof(te_log_bugs) / sizeof(te_log_bugs[0]));   \
        assert(((ptr_) - te_log_bugs) %                           \
                   sizeof(te_log_bugs[0]) == 0);                  \
    } while (0)


/* See description in tapi_bufs.h */
te_log_bug *
te_log_bug_alloc()
{
    int i;
    int id;

    pthread_mutex_lock(&te_log_bug_mutex);

    if (te_log_bug_last_freed != -1)
    {
        assert(te_log_bugs[te_log_bug_last_freed].used == FALSE &&
               te_log_bugs[te_log_bug_last_freed].cur_len == 0);

        id = te_log_bug_last_freed;
        te_log_bug_last_freed = -1;
        te_log_bugs[id].used = TRUE;
        pthread_mutex_unlock(&te_log_bug_mutex);

        return &te_log_bugs[id];
    }
    
    for (i = 0; i < LOG_BUF_NUM; i++)
    {
        if (!te_log_bugs[i].used)
        {
            assert(te_log_bugs[i].cur_len == 0);

            te_log_bugs[i].used = TRUE;
            pthread_mutex_unlock(&te_log_bug_mutex);

            return &te_log_bugs[i];
        }
    }
    pthread_mutex_unlock(&te_log_bug_mutex);

    /* There is no available buffer, wait until one freed */
    do {
        pthread_mutex_lock(&te_log_bug_mutex);
        if (te_log_bug_last_freed != -1)
        {
            assert(te_log_bugs[te_log_bug_last_freed].used == FALSE &&
                   te_log_bugs[te_log_bug_last_freed].cur_len == 0);

            id = te_log_bug_last_freed;
            te_log_bug_last_freed = -1;
            te_log_bugs[id].used = TRUE;
            pthread_mutex_unlock(&te_log_bug_mutex);
            
            return &te_log_bugs[id];
        }
        pthread_mutex_unlock(&te_log_bug_mutex);
        
        RING("Waiting for a tapi log buffer");
        sleep(1);
    } while (TRUE);

    assert(0);
    return NULL;
}

/* See description in tapi_bufs.h */
int
te_log_bug_append(te_log_bug *buf, const char *fmt, ...)
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
te_log_bug_get(te_log_bug *buf)
{
    VALIDATE_LOG_BUF(buf);

    if (buf->cur_len == 0)
        buf->ptr[0] = '\0';

    return buf->ptr;
}

/* See description in tapi_bufs.h */
void
te_log_bug_free(te_log_bug *buf)
{
    if (buf == NULL)
        return;

    VALIDATE_LOG_BUF(buf);

    pthread_mutex_lock(&te_log_bug_mutex);
    buf->used = FALSE;
    buf->cur_len = 0;
    te_log_bug_last_freed = 
        (buf - te_log_bugs) / sizeof(te_log_bugs[0]);
    pthread_mutex_unlock(&te_log_bug_mutex);
}
