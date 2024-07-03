/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to operate the Timer service
 *
 * @defgroup te_tools_te_timer Timer
 * @ingroup te_tools
 * @{
 *
 * Functions to check if time expired
 *
 * @section te_tools_te_timer_example Example of usage
 *
 * Let's assume we need to do some routine until certain time has expired
 *
 * @code
 * #include "te_timer.h"
 *
 * te_timer_t timer = TE_TIMER_INIT;
 * te_errno rc;
 *
 * CHECK_RC(te_timer_start(&timer, 3));
 * do {
 *     VSLEEP(1, "repeat some routine until the timer expires");
 * } while ((rc = te_timer_expired(&timer)) == 0);
 * CHECK_RC(te_timer_stop(&timer));
 * if (rc != TE_ETIMEDOUT)
 *     CHECK_RC(rc);
 * @endcode
 */
#ifndef __TE_TIMER_H__
#define __TE_TIMER_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Timer context */
typedef struct te_timer_t {
    bool is_valid;   /**< @c true if timer is created */
    timer_t id;         /**< POSIX.1 timer ID */
} te_timer_t;

/** On-stack timer context initializer */
#define TE_TIMER_INIT { .is_valid = false }

/**
 * Start timer
 *
 * @param timer         Timer handle
 * @param timeout_s     Timeout for triggering timer
 *
 * @return Status code
 */
extern te_errno te_timer_start(te_timer_t *timer, unsigned int timeout_s);

/**
 * Restart already running or expired timer with a new timeout.
 *
 * @param timer         Timer handle.
 * @param timeout_s     Timeout for triggering timer.
 *
 * @return Status code.
 */
extern te_errno te_timer_restart(te_timer_t *timer, unsigned int timeout_s);

/**
 * Stop the timer and free its resources.
 *
 * @param timer         Timer handle.
 *
 * @return Status code.
 */
extern te_errno te_timer_stop(te_timer_t *timer);

/**
 * Check whether timeout has expired or not.
 *
 * @note It does not stop the timer if it expires, so te_timer_stop()
 * should be called to free timer's resources.
 *
 * @param timer         Timer handle
 *
 * @return Status code
 * @retval 0                Time has not expired yet
 * @retval TE_ETIMEDOUT     Time has expired
 * @retval Others           Internal error, there is no way to use this timer
 *                          any more
 */
extern te_errno te_timer_expired(te_timer_t *timer);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TIMER_H__ */
/**@} <!-- END te_tools_te_timer --> */
