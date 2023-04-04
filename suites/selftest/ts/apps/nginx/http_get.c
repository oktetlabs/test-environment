/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief nginx Test Suite
 *
 * Check that nginx replies on HTTP GET.
 *
 * Copyright (C) 2019-2023 OKTET Labs Ltd. All rights reserved.
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "http_get"

#include "te_config.h"

#include "nginx_suite.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_stdio.h"
#include "tapi_env.h"

#define NGINX_NAME      "webserver"
#define SRV_NAME        "dflt"
#define LISTEN_NAME     "1"
#define ADDR_SPEC       "127.0.0.1:8111"
#define HTTP_GET_CMD    "curl " ADDR_SPEC

int
main(int argc, char **argv)
{
    rcf_rpc_server     *iut_rpcs = NULL;
    tarpc_pid_t         pid;
    rpc_wait_status     status;

    TEST_START;

    TEST_STEP("Get parameters from environment");
    TEST_GET_PCO(iut_rpcs);

    TEST_STEP("Add nginx instance");
    CHECK_RC(tapi_cfg_nginx_add(iut_rpcs->ta, NGINX_NAME));

    TEST_STEP("Add nginx HTTP server");
    CHECK_RC(tapi_cfg_nginx_http_server_add(iut_rpcs->ta,
                                            NGINX_NAME, SRV_NAME));

    TEST_STEP("Add nginx listening entry");
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(iut_rpcs->ta,
                                                  NGINX_NAME, SRV_NAME,
                                                  LISTEN_NAME, ADDR_SPEC));

    TEST_STEP("Start nginx");
    CHECK_RC(tapi_cfg_nginx_enable(iut_rpcs->ta, NGINX_NAME));

    TEST_STEP("Check that nginx replies on HTTP GET");
    pid = rpc_te_shell_cmd(iut_rpcs, HTTP_GET_CMD, -1, NULL, NULL, NULL);

    RPC_AWAIT_IUT_ERROR(iut_rpcs);
    if (rpc_waitpid(iut_rpcs, pid, &status, 0) != pid)
        TEST_FAIL("Failed to execute '%s' command on agent '%s'",
                  HTTP_GET_CMD, iut_rpcs->ta);
    else if (status.value != 0)
        TEST_FAIL("Command '%s' failed on agent '%s'",
                  HTTP_GET_CMD, iut_rpcs->ta);

    TEST_STEP("Stop nginx");
    CHECK_RC(tapi_cfg_nginx_disable(iut_rpcs->ta, NGINX_NAME));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
