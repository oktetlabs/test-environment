/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_timer.h functions
 *
 * Testing a timer restart functionality.
 */

/** @page timer-restart_timer Restarting timer test
 *
 * @objective Testing a timer restart functionality.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "timer/restart_timer"

#include "te_config.h"
#include "tapi_test.h"
#include "te_timer.h"

#define TIMEOUT_ORIGINAL_S  1
#define TIMEOUT_RESTART_S   3

#if TIMEOUT_ORIGINAL_S >= TIMEOUT_RESTART_S
#error "Test cannot work properly with such timers' timeouts"
#endif

int
main(int argc, char **argv)
{
    te_timer_t timer = TE_TIMER_INIT;

    TEST_START;

    TEST_STEP("Check that timer cannot restart before starting");
    rc = te_timer_restart(&timer, TIMEOUT_RESTART_S);
    if (TE_RC_GET_ERROR(rc) != TE_EINVAL)
        TEST_VERDICT("Timer can unexpectedly restart before starting");

    TEST_STEP("Start the timer");
    CHECK_RC(te_timer_start(&timer, TIMEOUT_ORIGINAL_S));

    TEST_STEP("Check that timer cannot start again until it expires");
    rc = te_timer_start(&timer, TIMEOUT_ORIGINAL_S);
    if (TE_RC_GET_ERROR(rc) != TE_EINPROGRESS)
        TEST_VERDICT("Timer can start again despite it is running");

    TEST_STEP("Check that timer can stop");
    rc = te_timer_stop(&timer);
    if (rc == 0)
        RING("Timer stopped");
    else
        TEST_VERDICT("Timer cannot stop");

    TEST_STEP("Check that timer can start after stopping");
    rc = te_timer_start(&timer, TIMEOUT_ORIGINAL_S);
    if (rc != 0)
    {
        ERROR_ARTIFACT("Starting timer failed with error %r", rc);
        TEST_VERDICT("Failed to start timer after stopping");
    }

    TEST_STEP("Check that timer can restart with new timeout");
    rc = te_timer_restart(&timer, TIMEOUT_RESTART_S);
    if (rc != 0)
    {
        ERROR_ARTIFACT("Restarting timer failed with error %r", rc);
        TEST_VERDICT("Failed to restart timer with new timeout");
    }

    TEST_SUBSTEP("Wait until the timer expires");
    VSLEEP(TIMEOUT_ORIGINAL_S, "waiting for original (replaced) timeout");
    rc = te_timer_expired(&timer);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
        {
            TEST_VERDICT("Timer expired too early. "
                         "Probably new timeout was not applied");
        }
        else
        {
            TEST_VERDICT("Unexpected timer's status: %r", rc);
        }
    }

    VSLEEP(TIMEOUT_RESTART_S - TIMEOUT_ORIGINAL_S + 1,
           "waiting until timer finishes");
    rc = te_timer_expired(&timer);
    if (rc == 0)
        TEST_VERDICT("Timer did not expire on time");
    else if (TE_RC_GET_ERROR(rc) != TE_ETIMEDOUT)
        TEST_VERDICT("Unexpected timer's status: %r", rc);

    TEST_STEP("Check that timer cannot start again after it expires");
    rc = te_timer_start(&timer, TIMEOUT_ORIGINAL_S);
    if (rc == 0)
    {
        TEST_VERDICT("Timer can unexpectedly start again after it expires");
    }
    else if (TE_RC_GET_ERROR(rc) != TE_EINPROGRESS)
    {
        ERROR_ARTIFACT("Starting timer failed with error %r", rc);
        TEST_VERDICT("Unexpected timer error");
    }

    TEST_STEP("Check that timer can restart after it expires");
    rc = te_timer_restart(&timer, TIMEOUT_ORIGINAL_S);
    if (rc != 0)
    {
        ERROR_ARTIFACT("Starting timer failed with error %r", rc);
        TEST_VERDICT("Failed to start timer again after it expires");
    }

    TEST_SUBSTEP("Wait until the timer expires");
    VSLEEP(TIMEOUT_ORIGINAL_S + 1, "waiting until timer finishes");
    rc = te_timer_expired(&timer);
    if (rc == 0)
        TEST_VERDICT("Timer did not expire on time");
    else if (TE_RC_GET_ERROR(rc) != TE_ETIMEDOUT)
        TEST_VERDICT("Unexpected timer's status: %r", rc);

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(te_timer_stop(&timer));
    TEST_END;
}
