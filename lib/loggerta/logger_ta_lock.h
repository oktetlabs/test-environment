/** @file
 * @brief Logger subsystem API - TA side
 *
 * Macros and functions for logger data locking.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_TA_LOCK_H__
#define __TE_LOGGER_TA_LOCK_H__

#include <stdio.h>

#include "te_errno.h"
#include "te_defs.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef int ta_log_lock_key;

/*
 * Following macro should be used to protect the
 * log buffer/file during processing
 */
#if HAVE_PTHREAD_H

#include <pthread.h>

extern pthread_mutex_t  ta_log_mutex;

static inline int
ta_log_lock_init(void)
{
    int rc = pthread_mutex_init(&ta_log_mutex, NULL);

    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_init() failed: errno=%d\n",
                __FUNCTION__, errno);
    }
    return rc;
}

static inline int
ta_log_lock_destroy(void)
{
    return 0;
}

static inline int
ta_log_lock(ta_log_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_lock(&ta_log_mutex);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_lock() failed: errno=%d",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_log_unlock(ta_log_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_unlock(&ta_log_mutex);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_unlock() failed: errno=%d",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_log_trylock(ta_log_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_trylock(&ta_log_mutex);
    if ((rc != 0) && (errno != EBUSY))
    {
        fprintf(stderr, "%s(): pthread_mutex_trylock() failed: errno=%d\n",
                __FUNCTION__, errno);
    }

    return rc;
}

#elif HAVE_SEMAPHORE_H

#error We only have semaphore.h
#include <semaphore.h>

extern sem_t    ta_log_sem;

static inline int
ta_log_lock_init(void)
{
    int rc = sem_init(&ta_log_sem, 0, 1);

    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_init() failed: errno=%d\n",
                __FUNCTION__, errno);
    }
    return rc;
}

static inline int
ta_log_lock_destroy(void)
{
    int rc = sem_destroy(&ta_log_sem);

    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_destroy() failed: errno=%d\n",
                __FUNCTION__, errno);
    }
    return rc;
}


static inline int
ta_log_lock(ta_log_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = sem_lock(&ta_log_sem);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_lock() failed: errno=%d\n",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_log_unlock(ta_log_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = sem_unlock(&ta_log_sem);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): sem_unlock() failed: errno=%d\n",
                __FUNCTION__, errno);
    }

    return rc;
}

static inline int
ta_log_trylock(ta_log_lock_key key)
{
    int rc;

    UNUSED(key);
    rc = sem_trylock(&ta_log_sem);
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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TA_LOCK_H__ */
