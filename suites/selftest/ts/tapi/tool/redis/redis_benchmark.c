/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI redis_benchmark test
 *
 * Demonstrate the usage of TAPI redis_benchmark.
 *
 * Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME "redis_benchmark"

#include "redis_srv.h"
#include "tapi_cfg_memory.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_redis_srv.h"
#include "tapi_redis_benchmark.h"
#include "tapi_sockaddr.h"
#include "tapi_env.h"
#include "tapi_test.h"

/* How long test checks that redis is running in seconds. */
#define REDIS_SRV_WAIT_TIMEOUT 5

/* Benchmarking ALL tests requires about 2GB of memory  */
#define REDIS_BENCHMARK_ALL_REQUIRED_MEMORY_MB 2048

int
main(int argc, char **argv)
{
    te_errno rc_wait = 0;

    rcf_rpc_server *iut_rpcs = NULL;
    const struct sockaddr *iut_addr = NULL;

    te_mi_logger *logger = NULL;
    tapi_job_factory_t *srv_factory = NULL;
    tapi_job_factory_t *bm_factory = NULL;

    tapi_redis_srv_app *redis_srv_app = NULL;
    tapi_redis_srv_opt redis_srv_opt = tapi_redis_srv_default_opt;

    tapi_redis_benchmark_app *redis_bm_app = NULL;
    tapi_redis_benchmark_opt redis_bm_opt = tapi_redis_benchmark_default_opt;
    tapi_redis_benchmark_report redis_bm_report =
                                    SLIST_HEAD_INITIALIZER(redis_bm_report);

    unsigned int clients;
    unsigned int requests;
    unsigned int size;
    unsigned int keyspacelen;
    unsigned int pipelines;
    unsigned int threads;
    const char *tests;

    TEST_START;
    TEST_GET_PCO(iut_rpcs);
    TEST_GET_ADDR(iut_rpcs, iut_addr);
    TEST_GET_UINT_PARAM(clients);
    TEST_GET_UINT_PARAM(requests);
    TEST_GET_UINT_PARAM(size);
    TEST_GET_UINT_PARAM(keyspacelen);
    TEST_GET_UINT_PARAM(pipelines);
    TEST_GET_UINT_PARAM(threads);
    TEST_GET_STRING_PARAM(tests);

    if (strcmp(tests, "-") == 0)
    {
        uint64_t memory;

        TEST_STEP("Check if there is enough RAM to run all benchmark tests.");
        CHECK_RC(tapi_cfg_get_memory(iut_rpcs->ta, 0, &memory));
        memory /= (1024 * 1024);
        if (memory < REDIS_BENCHMARK_ALL_REQUIRED_MEMORY_MB)
        {
            ERROR("Total memory %luMB, while is required %luMB",
                  memory, REDIS_BENCHMARK_ALL_REQUIRED_MEMORY_MB);
            TEST_SKIP("Not enough RAM to run all benchmark tests");
        }
    }

    TEST_STEP("Configure and start redis-server on IUT.");
    redis_srv_opt.server = (const struct sockaddr *)iut_addr;
    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &srv_factory));
    CHECK_RC(tapi_redis_srv_create(srv_factory, &redis_srv_opt,
                                   &redis_srv_app));
    CHECK_RC(tapi_redis_srv_start(redis_srv_app));

    TEST_STEP("Check that redis-server is running.");
    rc_wait = tapi_redis_srv_wait(redis_srv_app,
                                  TE_SEC2MS(REDIS_SRV_WAIT_TIMEOUT));
    if (rc_wait != 0 && TE_RC_GET_ERROR(rc_wait) != TE_EINPROGRESS)
        TEST_FAIL("Redis-server is not running");

    TEST_STEP("Configure and start redis-benchmark on IUT.");
    redis_bm_opt.server = (const struct sockaddr *)iut_addr;
    redis_bm_opt.clients = TAPI_JOB_OPT_UINT_VAL(clients);
    redis_bm_opt.requests = TAPI_JOB_OPT_UINT_VAL(requests);
    redis_bm_opt.size = TAPI_JOB_OPT_UINT_VAL(size);
    redis_bm_opt.keyspacelen = TAPI_JOB_OPT_UINT_VAL(keyspacelen);
    redis_bm_opt.pipelines = TAPI_JOB_OPT_UINT_VAL(pipelines);
    redis_bm_opt.threads = TAPI_JOB_OPT_UINT_VAL(threads);
    if (strcmp(tests, "-") != 0)
        redis_bm_opt.tests = tests;

    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &bm_factory));
    CHECK_RC(tapi_redis_benchmark_create(bm_factory, &redis_bm_opt,
                                         &redis_bm_app));
    CHECK_RC(tapi_redis_benchmark_start(redis_bm_app));

    TEST_STEP("Wait for redis-benchmark completion.");
    CHECK_RC(tapi_redis_benchmark_wait(redis_bm_app, -1));

    TEST_STEP("Stop redis-server on IUT.");
    CHECK_RC(tapi_redis_srv_stop(redis_srv_app));

    TEST_STEP("Get redis-benchmark report on IUT.");
    CHECK_RC(tapi_redis_benchmark_get_report(redis_bm_app, &redis_bm_report));

    TEST_STEP("Log redis-benchmark statistics in MI format.");
    CHECK_RC(te_mi_logger_meas_create("redis-benchmark", &logger));
    CHECK_RC(tapi_redis_benchmark_report_mi_log(logger, &redis_bm_report));

    TEST_STEP("Stop redis-benchmark on IUT.");
    CHECK_RC(tapi_redis_benchmark_stop(redis_bm_app));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_redis_srv_destroy(redis_srv_app));
    CLEANUP_CHECK_RC(tapi_redis_benchmark_destroy(redis_bm_app));
    te_mi_logger_destroy(logger);
    tapi_redis_benchmark_destroy_report(&redis_bm_report);
    tapi_job_factory_destroy(srv_factory);
    tapi_job_factory_destroy(bm_factory);

    TEST_END;
}
