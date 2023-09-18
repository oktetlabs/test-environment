/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to operate the Timer service
 *
 * Functions to check if time expired
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_timer.h"
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include "logger_api.h"
#include "te_time.h"

#define TE_TIMER_CLOCKID    CLOCK_MONOTONIC

/**
 * Check an expression passed as the argument against zero.
 * If the expression is something not zero the macro reports an
 * error based on set errno, and returns errno converted to TE error code
 *
 * @param _expr     Expression to be checked
 */
#define TE_TIMER_CHECK_ERRNO(_expr) \
    do {                                                    \
        int _res = (_expr);                                 \
                                                            \
        if (_res != 0)                                      \
        {                                                   \
            te_errno _rc = te_rc_os2te(errno);              \
                                                            \
            ERROR("%s(): %s failed: %r",                    \
                  __func__, #_expr, _rc);                   \
            return _rc;                                     \
        }                                                   \
    } while (0)

/* See description in te_timer.h */
te_errno
te_timer_start(te_timer_t *timer, unsigned int timeout_s)
{
    struct sigevent sev = { .sigev_notify = SIGEV_NONE, };
    struct itimerspec trigger;

    if (timer->is_valid)
    {
        ERROR("Timer is already in progress or initialized incorrectly");
        return TE_EINPROGRESS;
    }

    /* Create the timer */
    TE_TIMER_CHECK_ERRNO(timer_create(TE_TIMER_CLOCKID, &sev, &timer->id));
    timer->is_valid = TRUE;

    /* Start the timer */
    trigger.it_value.tv_sec = timeout_s;
    trigger.it_value.tv_nsec = 0;
    trigger.it_interval.tv_sec = 0;
    trigger.it_interval.tv_nsec = 0;

    TE_TIMER_CHECK_ERRNO(timer_settime(timer->id, 0, &trigger, NULL));

    return 0;
}

/* See description in te_timer.h */
te_errno
te_timer_restart(te_timer_t *timer, unsigned int timeout_s)
{
    struct itimerspec trigger;

    if (!timer->is_valid)
    {
        ERROR("Timer is not running or initialized incorrectly");
        return TE_EINVAL;
    }

    /* Rearm the timer */
    trigger.it_value.tv_sec = timeout_s;
    trigger.it_value.tv_nsec = 0;
    trigger.it_interval.tv_sec = 0;
    trigger.it_interval.tv_nsec = 0;

    TE_TIMER_CHECK_ERRNO(timer_settime(timer->id, 0, &trigger, NULL));

    return 0;
}

/* See description in te_timer.h */
te_errno
te_timer_stop(te_timer_t *timer)
{
    if (timer->is_valid)
    {
        TE_TIMER_CHECK_ERRNO(timer_delete(timer->id));
        timer->is_valid = FALSE;
    }

    return 0;
}

/* See description in te_timer.h */
te_errno
te_timer_expired(te_timer_t *timer)
{
    struct itimerspec remaining;

    if (!timer->is_valid)
    {
        ERROR("Timer is not running or initialized incorrectly");
        return TE_EINVAL;
    }

    TE_TIMER_CHECK_ERRNO(timer_gettime(timer->id, &remaining));
    if (remaining.it_value.tv_sec == 0 && remaining.it_value.tv_nsec == 0)
        return TE_ETIMEDOUT;

    return 0;
}
