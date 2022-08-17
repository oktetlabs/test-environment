/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to operate the Stopwatch service
 *
 * Functions for time measurement
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_stopwatch.h"
#include "logger_api.h"

/* See description in te_stopwatch.h */
te_errno
te_stopwatch_start(te_stopwatch_t *stopwatch)
{
    if (stopwatch->is_running)
        INFO("Stopwatch is already running");

    stopwatch->is_running = TRUE;
    return te_gettimeofday(&stopwatch->start, NULL);
}

/* See description in te_stopwatch.h */
te_errno
te_stopwatch_stop(te_stopwatch_t *stopwatch, struct timeval *lap)
{
    struct timeval now;
    te_errno rc = 0;

    assert(lap != NULL);

    if (!stopwatch->is_running)
    {
        ERROR("Stopwatch is not running");
        return TE_EFAULT;
    }

    rc = te_gettimeofday(&now, NULL);
    if (rc == 0)
    {
        te_timersub(&now, &stopwatch->start, lap);
        stopwatch->is_running = FALSE;
    }

    return rc;
}
