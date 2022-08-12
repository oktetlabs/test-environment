/** @file
 * @brief Test API to operate the Timer service
 *
 * @defgroup te_tools_te_timer Timer
 * @ingroup te_tools
 * @{
 *
 * Functions to check if time expired
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
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
 * if (rc != TE_ETIMEDOUT)
 *     CHECK_RC(rc);
 * @endcode
 *
 * The next example shows how a few timers could be used in parallel
 *
 * @todo This code could be reused in selftests
 *
 * @code
 * #include "te_timer.h"
 *
 * te_timer_t timer_a = TE_TIMER_INIT;
 * te_timer_t timer_b = TE_TIMER_INIT;
 * te_errno rc_a, rc_b;
 *
 * TEST_STEP("Use a single timer_a");
 * CHECK_RC(te_timer_start(&timer_a, 3));
 * do {
 *     VSLEEP(1, "waiting for timer_a");
 * } while ((rc_a = te_timer_expired(&timer_a)) == 0);
 * if (rc_a == TE_ETIMEDOUT)
 *     RING("timer_a has expired");
 * else
 *     CHECK_RC(rc_a);
 *
 * TEST_STEP("Reuse timer_a one more time and start timer_b in parallel");
 * CHECK_RC(te_timer_start(&timer_a, 5));
 * CHECK_RC(te_timer_start(&timer_b, 15));
 * do {
 *     VSLEEP(1, "waiting for any timer_a or timer_b");
 * } while ((rc_a = te_timer_expired(&timer_a)) == 0 &&
 *          (rc_b = te_timer_expired(&timer_b)) == 0);
 * if (rc_a == TE_ETIMEDOUT)
 *     RING("timer_a has expired");
 * else
 *     CHECK_RC(rc_a);
 * if (rc_b == TE_ETIMEDOUT)
 *     RING("timer_b has expired");
 * else
 *     CHECK_RC(rc_b);
 *
 * TEST_SUBSTEP("Try to continue with timer_a");
 * while ((rc_a = te_timer_expired(&timer_a)) == 0)
 *     VSLEEP(1, "waiting for rest timer_a");
 * if (rc_a == TE_ETIMEDOUT)
 *     RING("timer_a has expired");
 * else
 *     CHECK_RC(rc_a);
 *
 * TEST_SUBSTEP("Try to continue with timer_b");
 * while ((rc_b = te_timer_expired(&timer_b)) == 0)
 *     VSLEEP(1, "waiting for rest timer_b");
 * if (rc_b == TE_ETIMEDOUT)
 *     RING("timer_b has expired");
 * else
 *     CHECK_RC(rc_b);
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
    timer_t id;         /**< POSIX.1 timer ID */
    int expire;         /**< Counter of timer overruns */
} te_timer_t;

/** On-stack timer context initializer */
#define TE_TIMER_INIT { .id = 0, .expire = 0 }

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
 * Check whether timeout set with te_timer_start() has expired or not
 *
 * @note It stops the timer if time is expired
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