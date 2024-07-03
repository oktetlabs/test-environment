/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI memtier test.
 *
 * Demonstrate the usage of TAPI memtier.
 */

/** @page tapi-tool-memcached-memtier Using memtier_benchmark
 *
 * @objective Use memtier TAPI to run memtier_benchmark tool and
 *            test memcached server.
 *
 * @par Scenario:
 */

#define TE_TEST_NAME "memtier"

#include <arpa/inet.h>

#include "memcached.h"
#include "tapi_memtier.h"
#include "tapi_memcached.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_sockaddr.h"
#include "tapi_test.h"
#include "tapi_env.h"

/* How long test checks that memcached is running in seconds. */
#define MEMCACHED_WAIT_TIMEOUT 5
/* How long memtier test runs in seconds. */
#define MEMTIER_RUN_TIME 30
/* How long to wait for memtier_benchmark after test time expires. */
#define MEMTIER_WAIT_TIMEOUT 5
/* Default path to memtier_benchmark. */
#define MEMTIER_DEF_PATH "memtier_benchmark"

int
main(int argc, char **argv)
{
    rcf_rpc_server *iut_rpcs = NULL;
    const struct sockaddr *iut_addr = NULL;

    tapi_job_factory_t *factory = NULL;
    tapi_memcached_app *memcached_app = NULL;
    tapi_memtier_app *memtier_app = NULL;
    tapi_memcached_opt memcached_opts = tapi_memcached_default_opt;
    tapi_memtier_opt memtier_opts = tapi_memtier_default_opt;
    tapi_memtier_report memtier_report = tapi_memtier_default_report;
    const char *memtier_path = NULL;

    te_errno rc_wait = 0;

    TEST_START;

    TEST_GET_PCO(iut_rpcs);
    TEST_GET_ADDR(iut_rpcs, iut_addr);

    memcached_opts.tcp_port = iut_addr;
    memcached_opts.username = "root";

    memtier_opts.server = iut_addr;
    memtier_opts.clients = TAPI_JOB_OPT_UINT_VAL(100);
    memtier_opts.threads = TAPI_JOB_OPT_UINT_VAL(4);
    memtier_opts.test_time = TAPI_JOB_OPT_UINT_VAL(MEMTIER_RUN_TIME);
    memtier_opts.key_maximum = TAPI_JOB_OPT_UINT_VAL(1000);
    memtier_opts.ratio = "1:1";
    memtier_opts.key_pattern = "S:R";
    memtier_opts.protocol = TAPI_MEMTIER_PROTO_MEMCACHE_TEXT;
    memtier_opts.hide_histogram = true;

    TEST_STEP("Check that memtier_benchmark application can be "
              "found on IUT.");

    memtier_path = getenv("TE_IUT_MEMTIER_PATH");
    if (te_str_is_null_or_empty(memtier_path))
    {
        TEST_SKIP("Path to memtier_benchmark is not specified "
                  "in environment");
    }

    if (rpc_te_file_check_executable(iut_rpcs, memtier_path) != 0)
        TEST_SKIP("memtier_benchmark is not available");

    memtier_opts.memtier_path = memtier_path;

    TEST_STEP("Create RPC factory on IUT for running apps on it.");
    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &factory));

    TEST_STEP("Create memcached app on IUT.");
    CHECK_RC(tapi_memcached_create(factory, &memcached_opts,
                                   &memcached_app));

    TEST_STEP("Create memtier_benchmark app on IUT.");
    CHECK_RC(tapi_memtier_create(factory, &memtier_opts, &memtier_app));

    TEST_STEP("Start memcached on IUT.");
    CHECK_RC(tapi_memcached_start(memcached_app));

    TEST_STEP("Start memtier_benchmark on IUT.");
    CHECK_RC(tapi_memtier_start(memtier_app));

    TEST_STEP("Wait for memtier_benchmark completion.");
    rc = tapi_memtier_wait(
                     memtier_app,
                     TE_SEC2MS(MEMTIER_RUN_TIME + MEMTIER_WAIT_TIMEOUT));
    if (rc != 0)
    {
        TEST_VERDICT("Failed to wait for memtier_benchmark completion: %r",
                     rc);
    }

    TEST_STEP("Check that memcached is still running.");
    rc_wait = tapi_memcached_wait(memcached_app,
                                  TE_SEC2MS(MEMCACHED_WAIT_TIMEOUT));

    if (rc_wait != 0 && TE_RC_GET_ERROR(rc_wait) != TE_EINPROGRESS)
        TEST_FAIL("memcached is not running");

    TEST_STEP("Stop memcached on IUT.");
    CHECK_RC(tapi_memcached_stop(memcached_app));

    TEST_STEP("Get memtier_benchmark report on IUT.");
    CHECK_RC(tapi_memtier_get_report(memtier_app, &memtier_report));

    TEST_STEP("Print MI log of obtained report.");
    CHECK_RC(tapi_memtier_report_mi_log(&memtier_report));

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_memcached_destroy(memcached_app));

    tapi_memtier_destroy_report(&memtier_report);
    CLEANUP_CHECK_RC(tapi_memtier_destroy(memtier_app));

    tapi_job_factory_destroy(factory);

    TEST_END;
}
