/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief nginx Test Suite
 *
 * Check that nginx instance can be run with command line prefix.
 *
 * Copyright (C) 2019-2023 OKTET Labs Ltd. All rights reserved.
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "prefix"

#include "te_config.h"

#include "nginx_suite.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"
#include "tapi_env.h"

#define NGINX_NAME      "webserver"

int
main(int argc, char **argv)
{
    rcf_rpc_server *iut_rpcs = NULL;

    TEST_START;

    TEST_STEP("Get parameters from environment");
    TEST_GET_PCO(iut_rpcs);

    TEST_STEP("Add nginx instance");
    CHECK_RC(tapi_cfg_nginx_add(iut_rpcs->ta, NGINX_NAME));

    TEST_STEP("Set command line wrapper");
    CHECK_RC(tapi_cfg_nginx_cmd_prefix_set(iut_rpcs->ta,
                                           NGINX_NAME, "strace"));

    TEST_STEP("Start nginx");
    CHECK_RC(tapi_cfg_nginx_enable(iut_rpcs->ta, NGINX_NAME));

    TEST_STEP("Stop nginx");
    CHECK_RC(tapi_cfg_nginx_disable(iut_rpcs->ta, NGINX_NAME));

    TEST_STEP("Delete nginx instance");
    CHECK_RC(tapi_cfg_nginx_del(iut_rpcs->ta, NGINX_NAME));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
