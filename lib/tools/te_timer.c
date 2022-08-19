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
#define TE_TIMER_SIGNO      SIGRTMIN

static void
te_timer_handler(int signo, siginfo_t *info, void *context)
{
    int overrun;
    te_timer_t *timer;

    VERB("debug: signal details: signal (%d), code (%d)",
         info->si_signo, info->si_code);

    if (info->si_code != SI_TIMER)
        return;

    timer = info->si_value.sival_ptr;
    timer->expire++;
    VERB("debug: timer-id (kernel) = %d, timerid = %p, overrun = %d",
         info->si_timerid, timer->id, info->si_overrun);

    overrun = timer_getoverrun(timer->id);
    if (overrun >= 0)
        timer->expire += overrun;
    else
        ERROR("%s(): timer_getoverrun failed: %s", __func__, strerror(errno));
}

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
    sigset_t mask;
    struct sigaction sa;
    struct sigevent sev;
    struct itimerspec trigger;

    if (timer->id != 0)
    {
        ERROR("Timer is already in progress or initialized incorrectly");
        return TE_EINPROGRESS;
    }

    /* Establish handler for timer signal */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = te_timer_handler;
    TE_TIMER_CHECK_ERRNO(sigemptyset(&sa.sa_mask));
    TE_TIMER_CHECK_ERRNO(sigaction(TE_TIMER_SIGNO, &sa, NULL));

    /* Block timer signal temporarily */
    TE_TIMER_CHECK_ERRNO(sigemptyset(&mask));
    TE_TIMER_CHECK_ERRNO(sigaddset(&mask, TE_TIMER_SIGNO));
    TE_TIMER_CHECK_ERRNO(sigprocmask(SIG_SETMASK, &mask, NULL));

    /* Create the timer */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = TE_TIMER_SIGNO;
    sev.sigev_value.sival_ptr = timer;
    TE_TIMER_CHECK_ERRNO(timer_create(TE_TIMER_CLOCKID, &sev, &timer->id));

    /* Start the timer */
    trigger.it_value.tv_sec = timeout_s;
    trigger.it_value.tv_nsec = 0;
    trigger.it_interval.tv_sec = 0;
    trigger.it_interval.tv_nsec = 0;

    TE_TIMER_CHECK_ERRNO(timer_settime(timer->id, 0, &trigger, NULL));

    timer->expire = 0;

    /* Unlock the timer signal, so that timer notification can be delivered */
    TE_TIMER_CHECK_ERRNO(sigemptyset(&mask));
    TE_TIMER_CHECK_ERRNO(sigaddset(&mask, TE_TIMER_SIGNO));
    TE_TIMER_CHECK_ERRNO(sigprocmask(SIG_UNBLOCK, &mask, NULL));

    return 0;
}

/* See description in te_timer.h */
te_errno
te_timer_expired(te_timer_t *timer)
{
    if (timer->expire == 0)
    {
        if (timer->id == 0)
        {
            ERROR("Timer is not runnig or initialized incorrectly");
            return TE_EINVAL;
        }

        return 0;
    }

    if (timer->id != 0)
    {
        TE_TIMER_CHECK_ERRNO(timer_delete(timer->id));
        timer->id = 0;
    }

    return TE_ETIMEDOUT;
}
