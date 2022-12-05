/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI redis-server test
 *
 * Demonstrate the usage of TAPI redis-server.
 *
 * Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME "redis_srv"

#include "redis_srv.h"
#include "tapi_redis_srv.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_test.h"
#include "tapi_sockaddr.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    te_errno rc_wait = 0;

    rcf_rpc_server *iut_rpcs = NULL;
    tapi_job_factory_t *factory = NULL;
    const struct sockaddr *iut_addr = NULL;

    tapi_redis_srv_opt opt = tapi_redis_srv_default_opt;
    tapi_redis_srv_app *app = NULL;

    TEST_START;

    TEST_GET_PCO(iut_rpcs);
    TEST_GET_ADDR(iut_rpcs, iut_addr);

    TEST_STEP("Set server option for redis-server");
    opt.server = iut_addr;
    opt.protected_mode = TE_BOOL3_FALSE;
    opt.loglevel = TAPI_REDIS_SRV_LOGLEVEL_VERBOSE;
    opt.databases = TAPI_JOB_OPT_UINT_VAL(1);
    opt.io_threads = TAPI_JOB_OPT_UINT_VAL(2);

    TEST_STEP("Create redis-server app handle.");
    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &factory));
    CHECK_RC(tapi_redis_srv_create(factory, &opt, &app));

    TEST_STEP("Start redis-server on IUT.");
    CHECK_RC(tapi_redis_srv_start(app));

    TEST_STEP("Check that redis-server is running.");
    rc_wait = tapi_redis_srv_wait(app, TE_SEC2MS(15));

    if (rc_wait != 0 && TE_RC_GET_ERROR(rc_wait) != TE_EINPROGRESS)
        TEST_FAIL("Redis-server is not running");

    TEST_STEP("Stop redis-server on IUT.");
    CHECK_RC(tapi_redis_srv_stop(app));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_redis_srv_destroy(app));
    tapi_job_factory_destroy(factory);

    TEST_END;
}
