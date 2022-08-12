/** @file
 * @brief nginx Test Suite
 *
 * Check that nginx config is updated when some parameter is changed
 * for running instance.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Marina Maslova <Marina.Maslova@oktetlabs.ru>
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "conf_change"

#include "te_config.h"

#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"
#include "tapi_file.h"

#define NGINX_NAME      "webserver"
#define SRV_NAME        "dflt"
#define LISTEN_NAME     "1"
#define ADDR_SPEC       "8111"

#define NGINX_WRK_PS_NUM_1    2
#define NGINX_WRK_PS_NUM_2    4

int
main(int argc, char **argv)
{
    const char *ta = "Agt_A";
    char       *config_path = NULL;
    char       *config_output1 = NULL;
    char       *config_output2 = NULL;
    unsigned    ps_num;

    TEST_START;

    TEST_STEP("Add nginx instance");
    CHECK_RC(tapi_cfg_nginx_add(ta, NGINX_NAME));

    TEST_STEP("Add nginx HTTP server");
    CHECK_RC(tapi_cfg_nginx_http_server_add(ta, NGINX_NAME, SRV_NAME));

    TEST_STEP("Add nginx listening entry");
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(ta, NGINX_NAME, SRV_NAME,
                                                  LISTEN_NAME, ADDR_SPEC));
    TEST_STEP("Set number of worker processes");
    CHECK_RC(tapi_cfg_nginx_wrk_ps_num_set(ta, NGINX_NAME,
                                           NGINX_WRK_PS_NUM_1));

    TEST_STEP("Start nginx");
    CHECK_RC(tapi_cfg_nginx_enable(ta, NGINX_NAME));

    TEST_STEP("Get nginx config");
    CHECK_RC(tapi_cfg_nginx_config_path_get(ta, NGINX_NAME, &config_path));
    CHECK_RC(tapi_file_read_ta(ta, config_path, &config_output1));
    RING("Nginx config: %s", config_output1);

    TEST_STEP("Change number of worker processes");
    CHECK_RC(tapi_cfg_nginx_wrk_ps_num_set(ta, NGINX_NAME,
                                           NGINX_WRK_PS_NUM_2));

    TEST_STEP("Check that number of worker processes has changed");
    CHECK_RC(tapi_cfg_nginx_wrk_ps_num_get(ta, NGINX_NAME, &ps_num));
    if (ps_num != NGINX_WRK_PS_NUM_2)
        TEST_FAIL("Number of worker processes is not changed");

    TEST_STEP("Get nginx config after parameter change");
    CHECK_RC(tapi_file_read_ta(ta, config_path, &config_output2));
    RING("Nginx config: %s", config_output2);

    TEST_STEP("Compare old and new configs");
    if (strcmp(config_output1, config_output2) == 0)
        TEST_FAIL("Config is not changed");

    TEST_STEP("Stop nginx");
    CHECK_RC(tapi_cfg_nginx_disable(ta, NGINX_NAME));

    TEST_SUCCESS;

cleanup:
    tapi_cfg_nginx_del(ta, NGINX_NAME);
    free(config_path);
    free(config_output1);
    free(config_output2);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
