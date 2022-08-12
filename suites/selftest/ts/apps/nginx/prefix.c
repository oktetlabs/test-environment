/** @file
 * @brief nginx Test Suite
 *
 * Check that nginx instance can be run with command line prefix.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "prefix"

#include "te_config.h"

#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"

#define NGINX_NAME      "webserver"

int
main(int argc, char **argv)
{
    const char *ta = "Agt_A";

    TEST_START;

    TEST_STEP("Add nginx instance");
    CHECK_RC(tapi_cfg_nginx_add(ta, NGINX_NAME));

    TEST_STEP("Set command line wrapper");
    CHECK_RC(tapi_cfg_nginx_cmd_prefix_set(ta, NGINX_NAME, "strace"));

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
