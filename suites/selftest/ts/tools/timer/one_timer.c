/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_timer.h functions
 *
 * Testing a single timer functionality.
 */

/** @page timer-one_timer Single timer test
 *
 * @objective Testing a single timer functionality.
 *
 * @param timeout_s     Timer's timeout (in seconds).
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "timer/one_timer"

#include "te_config.h"
#include "tapi_test.h"
#include "te_timer.h"

int
main(int argc, char **argv)
{
    te_timer_t timer = TE_TIMER_INIT;
    uint32_t timeout_s;

    TEST_START;
    TEST_GET_UINT_PARAM(timeout_s);

    TEST_STEP("Check that timer's status is unavailable before starting");
    rc = te_timer_expired(&timer);
    if (TE_RC_GET_ERROR(rc) != TE_EINVAL)
        TEST_VERDICT("Timer's status unexpectedly available before starting");

    TEST_STEP("Check that timer can start");
    rc = te_timer_start(&timer, timeout_s);
    if (rc != 0)
    {
        ERROR_ARTIFACT("Starting timer failed with error %r", rc);
        TEST_VERDICT("Failed to start timer");
    }

    TEST_STEP("Check that timer can expire on time");
    if (timeout_s > 1)
        VSLEEP(timeout_s - 1, "waiting until timer is closer to the end");
    rc = te_timer_expired(&timer);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
            TEST_VERDICT("Timer expired too early");
        else
            TEST_VERDICT("Unexpected timer's status: %r", rc);
    }

    VSLEEP(2, "waiting until timer finishes");
    rc = te_timer_expired(&timer);
    if (rc == 0)
        TEST_VERDICT("Timer did not expire on time");
    else if (TE_RC_GET_ERROR(rc) != TE_ETIMEDOUT)
        TEST_VERDICT("Unexpected timer's status: %r", rc);

    TEST_STEP("Check that timer can keep expired status");
    rc = te_timer_expired(&timer);
    if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
        RING("Timer keeps expired status");
    else
        TEST_VERDICT("Timer did not save expired status");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(te_timer_stop(&timer));
    TEST_END;
}
