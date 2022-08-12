/** @file
 * @brief TAPI memaslap test
 *
 * Demonstrate the usage of TAPI memaslap.
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_TEST_NAME "memaslap"

#include <arpa/inet.h>

#include "memcached.h"
#include "tapi_memaslap.h"
#include "tapi_memcached.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_sockaddr.h"
#include "tapi_test.h"
#include "tapi_env.h"

/* How long test checks that memcached is running in seconds. */
#define MEMCACHED_WAIT_TIMEOUT 5
/* How long memaslap test runs in seconds. */
#define MEMASLAP_RUN_TIMEOUT 30

int
main(int argc, char **argv)
{
    te_errno                rc_wait             = 0;
    rcf_rpc_server         *iut_rpcs            = NULL;
    tapi_job_factory_t     *memcached_factory   = NULL;
    tapi_job_factory_t     *memaslap_factory    = NULL;

    tapi_memcached_app     *memcached_app       = NULL;
    tapi_memaslap_app      *memaslap_app        = NULL;
    tapi_memcached_opt      memcached_opts      = tapi_memcached_default_opt;
    tapi_memaslap_opt       memaslap_opts       = tapi_memaslap_default_opt;
    tapi_memaslap_report    memaslap_report     = { 0 };
    const struct sockaddr  *iut_addr            = NULL;

    TEST_START;

    TEST_STEP("Configure and start memcached and memaslap on iut");

    TEST_GET_PCO(iut_rpcs);
    TEST_GET_ADDR(iut_rpcs, iut_addr);

    TEST_SUBSTEP("Initialize memcached params on iut");

    memcached_opts.tcp_port = (const struct sockaddr *)iut_addr;

    TEST_SUBSTEP("Initialize memaslap params on iut");

    /* Set work time for memaslap. */
    memaslap_opts.time.value   = MEMASLAP_RUN_TIMEOUT;
    memaslap_opts.time.defined = TRUE;

    /* Set servers = 127.0.0.1:11212 and number of servers = 1. */
    memaslap_opts.servers[0]   = (const struct sockaddr *)iut_addr;
    memaslap_opts.n_servers    = 1;

    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &memcached_factory));
    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &memaslap_factory));

    TEST_SUBSTEP("Create memcached app on iut");
    CHECK_RC(tapi_memcached_create(memcached_factory, &memcached_opts,
                                   &memcached_app));

    TEST_SUBSTEP("Create memaslap app on iut");
    CHECK_RC(tapi_memaslap_create(memaslap_factory, &memaslap_opts,
                                  &memaslap_app));

    TEST_SUBSTEP("Start memcached on iut");
    CHECK_RC(tapi_memcached_start(memcached_app));

    TEST_SUBSTEP("Start memaslap on iut");
    CHECK_RC(tapi_memaslap_start(memaslap_app));

    TEST_STEP("Wait for memaslap completion");
    CHECK_RC(tapi_memaslap_wait(memaslap_app, -1));

    TEST_STEP("Check that memcached is running");
    rc_wait = tapi_memcached_wait(memcached_app,
                                  TE_SEC2MS(MEMCACHED_WAIT_TIMEOUT));

    if (rc_wait != 0 && TE_RC_GET_ERROR(rc_wait) != TE_EINPROGRESS)
        TEST_FAIL("memcached is not running");

    TEST_STEP("Stop memcached on iut");
    CHECK_RC(tapi_memcached_stop(memcached_app));

    TEST_STEP("Get memaslap report on iut");
    CHECK_RC(tapi_memaslap_get_report(memaslap_app, &memaslap_report));

    TEST_STEP("MI log memaslap report on iut");
    CHECK_RC(tapi_memaslap_report_mi_log(&memaslap_report));

    TEST_STEP("Stop memaslap on iut");
    CHECK_RC(tapi_memaslap_stop(memaslap_app));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_memcached_destroy(memcached_app));
    tapi_job_factory_destroy(memcached_factory);
    CLEANUP_CHECK_RC(tapi_memaslap_destroy(memaslap_app));
    tapi_job_factory_destroy(memaslap_factory);
    CLEANUP_CHECK_RC(tapi_memaslap_destroy_report(&memaslap_report));

    TEST_END;
}
