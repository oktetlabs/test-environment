/** @file
 * @brief nginx Test Suite
 *
 * Check that nginx replies on HTTP GET.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "http_get"

#include "te_config.h"

#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_stdio.h"

#define NGINX_NAME      "webserver"
#define SRV_NAME        "dflt"
#define LISTEN_NAME     "1"
#define ADDR_SPEC       "127.0.0.1:8111"
#define HTTP_GET_CMD    "curl " ADDR_SPEC

int
main(int argc, char **argv)
{
    const char         *ta = "Agt_A";
    rcf_rpc_server     *pco_tst = NULL;
    tarpc_pid_t         pid;
    rpc_wait_status     status;

    TEST_START;

    TEST_GET_RPCS(ta, "pco_tst", pco_tst);

    TEST_STEP("Add nginx instance");
    CHECK_RC(tapi_cfg_nginx_add(ta, NGINX_NAME));

    TEST_STEP("Add nginx HTTP server");
    CHECK_RC(tapi_cfg_nginx_http_server_add(ta, NGINX_NAME, SRV_NAME));

    TEST_STEP("Add nginx listening entry");
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(ta, NGINX_NAME, SRV_NAME,
                                                  LISTEN_NAME, ADDR_SPEC));

    TEST_STEP("Start nginx");
    CHECK_RC(tapi_cfg_nginx_enable(ta, NGINX_NAME));

    TEST_STEP("Check that nginx replies on HTTP GET");
    pid = rpc_te_shell_cmd(pco_tst, HTTP_GET_CMD, -1, NULL, NULL, NULL);

    RPC_AWAIT_IUT_ERROR(pco_tst);
    if (rpc_waitpid(pco_tst, pid, &status, 0) != pid)
        TEST_FAIL("Failed to execute '%s' command on agent '%s'",
                  HTTP_GET_CMD, ta);
    else if (status.value != 0)
        TEST_FAIL("Command '%s' failed on agent '%s'", HTTP_GET_CMD, ta);

    TEST_STEP("Stop nginx");
    CHECK_RC(tapi_cfg_nginx_disable(ta, NGINX_NAME));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
