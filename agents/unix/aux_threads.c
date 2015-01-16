/** @file
 * @brief Unix Test Agent
 *
 * Management module for aux threads which are used to make non-blocking RPC
 * call.
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "Aux threads"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "te_queue.h"
#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "unix_internal.h"

/** Pthread mutex is necessary since a few threads can use the common
 * resources. */
static pthread_mutex_t aux_threads_lock = PTHREAD_MUTEX_INITIALIZER;

/** Lock pthread mutex. */
#define THREADS_LOCK \
do {                                                                       \
    int _rc;                                                               \
    if ((_rc = pthread_mutex_lock(&aux_threads_lock)) != 0)                \
        ERROR("Failed to get lock rc=%d: %s", _rc, strerror(_rc));         \
} while (0)

/** Unlock pthread mutex. */
#define THREADS_UNLOCK \
do {                                                                       \
    int _rc;                                                               \
    if ((_rc = pthread_mutex_unlock(&aux_threads_lock)) != 0)              \
        ERROR("Failed to unlock rc=%d: %s", _rc, strerror(_rc));           \
} while (0)

/** Parent-child thread identifiers couples list. Its using should be
 * protected with mutex. */
typedef struct threads_list {
    SLIST_ENTRY(threads_list)  ent_l;
    pthread_t parent; 
    pthread_t child;
} threads_list;

/** Parent-child couples list header. */
SLIST_HEAD(, threads_list) threads_list_h;

/** Threads counter, its using should be protected with mutex. */
static int threads_counter = 0;

/* See description in unix_internal.h */
te_errno
aux_threads_init(void)
{
    static te_bool initialized = FALSE;
    int rc;

    if (initialized)
    {
        THREADS_LOCK;
        threads_counter++;
        THREADS_UNLOCK;
        return 0;
    }

    if ((rc = pthread_mutex_init(&aux_threads_lock, NULL)) != 0)
    {
        ERROR("Failed to init aux threads mutex rc=%d: %s", rc,
              strerror(rc));
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    SLIST_INIT(&threads_list_h);

    initialized = TRUE;
    threads_counter++;

    return 0;
}

/* See description in unix_internal.h */
void
aux_threads_add(pthread_t tid)
{
    threads_list *thread;
    pthread_t self = pthread_self();

    THREADS_LOCK;
    SLIST_FOREACH(thread, &threads_list_h, ent_l)
    {
        if (thread->parent == self)
            break;
    }

    if (thread == NULL)
    {
        thread = malloc(sizeof(*thread));
        thread->parent = self;
        SLIST_INSERT_HEAD(&threads_list_h, thread, ent_l);
    }
    thread->child = tid;
    THREADS_UNLOCK;
}

/**
 * Try to cancel the aux thread if it was started. It can be useful if RPC
 * function hangs.
 * 
 * @param tid   Thread identifier
 * 
 * @return Status code
 */
static te_errno
aux_threads_cancel_child(pthread_t tid)
{
    int rc;

    if (tid == 0)
        return 0;

    if ((rc = pthread_cancel(tid)) == 0)
    {
        if ((rc = pthread_join(tid, NULL)) != 0)
        {
            ERROR("Failed to stop aux thread with non-blocking call, "
                  "pthread_join rc=%d", rc);
            return TE_RC(TE_TA_UNIX, TE_EFAIL);
        }

        RING("Aux thread with a non-blocking RPC was canceled");
        return 0;
    }
    else if (rc == ESRCH)
        return 0;

    ERROR("pthread_cancel to stop aux thread with non-blocking call "
          "returned unexpected code %d", rc);

    return TE_RC(TE_TA_UNIX, TE_EFAIL);
}

/* See description in unix_internal.h */
te_errno
aux_threads_cleanup(void)
{
    threads_list *thread;
    pthread_t     self = pthread_self();
    te_bool       last_thread = FALSE;
    te_errno      rc = 0;
    int           res = 0;

    THREADS_LOCK;
    threads_counter--;
    if (threads_counter == 0)
        last_thread = TRUE;

    SLIST_FOREACH(thread, &threads_list_h, ent_l)
    {
        if (thread->parent == self)
            break;
    }

    if (thread != NULL)
    {
        rc = aux_threads_cancel_child(thread->child);
        SLIST_REMOVE(&threads_list_h, thread, threads_list, ent_l);
        free(thread);

    }
    THREADS_UNLOCK;

    if (!last_thread)
        return rc;

    if ((res = pthread_mutex_destroy(&aux_threads_lock)) != 0)
    {
        ERROR("Failed to destroy aux threads mutex rc=%d: %s", res,
              strerror(res));
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    return rc;
}
