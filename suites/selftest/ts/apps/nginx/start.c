/** @file
 * @brief nginx Test Suite
 *
 * Check that nginx instance can be created, started, stopped and deleted.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "start"

#include "te_config.h"

#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"

#define NGINX_NAME      "webserver"
#define SRV_NAME        "dflt"
#define LISTEN_NAME     "1"
#define ADDR_SPEC       "8111"

int
main(int argc, char **argv)
{
    const char *ta = "Agt_A";

    TEST_START;

    TEST_STEP("Add nginx instance");
    CHECK_RC(tapi_cfg_nginx_add(ta, NGINX_NAME));

    TEST_STEP("Add nginx HTTP server");
    CHECK_RC(tapi_cfg_nginx_http_server_add(ta, NGINX_NAME, SRV_NAME));

    TEST_STEP("Add nginx listening entry");
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(ta, NGINX_NAME, SRV_NAME,
                                                  LISTEN_NAME, ADDR_SPEC));

    TEST_STEP("Start nginx");
    CHECK_RC(tapi_cfg_nginx_enable(ta, NGINX_NAME));

    TEST_STEP("Stop nginx");
    CHECK_RC(tapi_cfg_nginx_disable(ta, NGINX_NAME));

    TEST_STEP("Delete nginx instance");
    CHECK_RC(tapi_cfg_nginx_del(ta, NGINX_NAME));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
