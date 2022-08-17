/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Logger subsystem API - TA side
 *
 * Macros and functions for logger data locking.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
        fprintf(stderr, "%s(): pthread_mutex_init() failed: error=%s\n",
                __FUNCTION__, te_rc_err2str(te_rc_os2te(rc)));
    }
    return rc;
}

static inline int
ta_log_lock_destroy(void)
{
    return 0;
}

static inline int
ta_log_lock(ta_log_lock_key *key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_lock(&ta_log_mutex);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_lock() failed: error=%s",
                __FUNCTION__, te_rc_err2str(te_rc_os2te(rc)));
    }

    return rc;
}

static inline int
ta_log_unlock(ta_log_lock_key *key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_unlock(&ta_log_mutex);
    if (rc != 0)
    {
        fprintf(stderr, "%s(): pthread_mutex_unlock() failed: error=%s",
                __FUNCTION__, te_rc_err2str(te_rc_os2te(rc)));
    }

    return rc;
}

static inline int
ta_log_trylock(ta_log_lock_key *key)
{
    int rc;

    UNUSED(key);
    rc = pthread_mutex_trylock(&ta_log_mutex);
    if ((rc != 0) && (rc != EBUSY))
    {
        fprintf(stderr, "%s(): pthread_mutex_trylock() failed: error=%s\n",
                __FUNCTION__, te_rc_err2str(te_rc_os2te(rc)));
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
ta_log_lock(ta_log_lock_key *key)
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
ta_log_unlock(ta_log_lock_key *key)
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
ta_log_trylock(ta_log_lock_key *key)
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

#else

#error "We have no any locks"
#define LGR_LOCK(lock)    lock = 1
#define LGR_UNLOCK(lock)  lock = 0

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TA_LOCK_H__ */
