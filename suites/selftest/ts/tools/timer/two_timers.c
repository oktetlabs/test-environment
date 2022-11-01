/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_timer.h functions
 *
 * Testing a few timers functionality.
 */

/** @page timer-two_timers Two timers test
 *
 * @objective Testing a few timers functionality.
 *
 * Check that it is possible to run a few timers simultaneously and
 * they do not influence to each other.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "timer/two_timers"

#include "te_config.h"
#include "tapi_test.h"
#include "te_timer.h"

#define TIMEOUT_SHORT_S   3
#define TIMEOUT_LONG_S    5

#if TIMEOUT_SHORT_S >= TIMEOUT_LONG_S
#error "Test cannot work properly with such timers' timeouts"
#endif

int
main(int argc, char **argv)
{
    te_timer_t timer_short = TE_TIMER_INIT;
    te_timer_t timer_long = TE_TIMER_INIT;
    te_errno rc_short;
    te_errno rc_long;

    TEST_START;

    TEST_STEP("Start a short timer");
    CHECK_RC(te_timer_start(&timer_short, TIMEOUT_SHORT_S));

    TEST_STEP("Start a long timer");
    CHECK_RC(te_timer_start(&timer_long, TIMEOUT_LONG_S));

    TEST_STEP("Wait until the short timer expires");
    VSLEEP(TIMEOUT_SHORT_S + 1, "waiting until short timer finishes");

    TEST_SUBSTEP("Check that the long timer run while the short one expired");
    rc_long = te_timer_expired(&timer_long);
    if (TE_RC_GET_ERROR(rc_long) == TE_ETIMEDOUT)
        TEST_VERDICT("Long timer unexpectedly expired");
    else if (rc_long == 0)
        RING("Long timer is running");
    else
        CHECK_RC(rc_long);

    rc_short = te_timer_expired(&timer_short);
    if (TE_RC_GET_ERROR(rc_short) == TE_ETIMEDOUT)
        RING("Short timer expired");
    else if (rc_short == 0)
        TEST_VERDICT("Short timer did not expire on time");
    else
        CHECK_RC(rc_short);

    TEST_STEP("Wait until the long timer expires");
    VSLEEP(TIMEOUT_LONG_S - TIMEOUT_SHORT_S + 1,
           "waiting until long timer finishes");
    rc_long = te_timer_expired(&timer_long);
    if (TE_RC_GET_ERROR(rc_long) == TE_ETIMEDOUT)
        RING("Long timer expired");
    else if (rc_long == 0)
        TEST_VERDICT("Long timer did not expire on time");
    else
        CHECK_RC(rc_long);

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(te_timer_stop(&timer_short));
    CLEANUP_CHECK_RC(te_timer_stop(&timer_long));
    TEST_END;
}
