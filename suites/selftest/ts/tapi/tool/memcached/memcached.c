/** @file
 * @brief TAPI memcached test
 *
 * Demonstrate the usage of TAPI memcached.
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 *
 * @author Daniil Byshenko <Daniil.Byshenko@oktetlabs.ru>
 */

#define TE_TEST_NAME "memcached"

#include "memcached.h"
#include "tapi_memcached.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_sockaddr.h"
#include "tapi_test.h"
#include "tapi_env.h"

/* How long test checks that memcached is running in seconds. */
#define WAIT_TIMEOUT 15

int
main(int argc, char **argv)
{
    te_errno                rc_wait     = 0;
    rcf_rpc_server         *iut_rpcs    = NULL;
    tapi_job_factory_t     *factory_iut = NULL;
    const struct sockaddr  *iut_addr    = NULL;

    tapi_memcached_app *app_iut = NULL;
    tapi_memcached_opt  opt_iut = tapi_memcached_default_opt;

    TEST_START;

    TEST_STEP("Configure and start memcahed on iut");

    TEST_GET_PCO(iut_rpcs);
    TEST_GET_ADDR(iut_rpcs, iut_addr);

    TEST_SUBSTEP("Initialize memcahed params on iut");

    opt_iut.tcp_port = (const struct sockaddr *)iut_addr;

    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &factory_iut));

    TEST_SUBSTEP("Create memcached app on iut");
    CHECK_RC(tapi_memcached_create(factory_iut, &opt_iut, &app_iut));

    TEST_SUBSTEP("Start memcached on iut");
    CHECK_RC(tapi_memcached_start(app_iut));

    TEST_STEP("Check that memcached is running");
    rc_wait = tapi_memcached_wait(app_iut, TE_SEC2MS(15));

    if (rc_wait != 0 && TE_RC_GET_ERROR(rc_wait) != TE_EINPROGRESS)
        TEST_FAIL("memcached is not running");

    TEST_STEP("Stop memcached on iut");
    CHECK_RC(tapi_memcached_stop(app_iut));

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_memcached_destroy(app_iut));
    tapi_job_factory_destroy(factory_iut);

    TEST_END;
}
