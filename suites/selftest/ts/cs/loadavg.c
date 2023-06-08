/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing change tracking API
 *
 * Testing change tracking API
 */
/** @page cs-loadavg Testing TAPI for getting loadavg information
 *
 * @objective Check that TAPI functions get loadavg information from Configurator.
 *
 * @par Scenario:
 */
#define TE_TEST_NAME "cs/cpu_info"

#include "te_config.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"

static te_errno
check_getting_loadavg(const char *agent, void *data)
{
    tapi_cfg_base_loadavg loadavg;
    uint64_t runnable;
    uint64_t total;
    uint64_t latest_pid;

    UNUSED(data);

    RING("Loadavg statistics for agent %s", agent);

    TEST_STEP("Check getting loadavg");

    CHECK_RC(tapi_cfg_base_get_loadavg(agent, &loadavg));

    RING("min1: %.2f", loadavg.min1);
    RING("min5: %.2f", loadavg.min5);
    RING("min15: %.2f", loadavg.min15);

    TEST_STEP("Check getting kernel scheduling entities counters");

    CHECK_RC(tapi_cfg_base_get_proc_number(agent, &runnable, &total));

    RING("runnable: %d", runnable);
    RING("total: %d", total);

    TEST_STEP("Check getting latest PID");

    CHECK_RC(tapi_cfg_base_get_latest_pid(agent, &latest_pid));

    RING("latest_pid: %d", latest_pid);

    return 0;
}

int
main(int argc, char **argv)
{
    TEST_START;

    rcf_foreach_ta(check_getting_loadavg, NULL);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
