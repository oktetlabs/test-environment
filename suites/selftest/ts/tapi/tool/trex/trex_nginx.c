/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI TRex with Nginx test
 *
 * Demonstrate the usage of TAPI TRex with Nginx server.
 */

#define TE_TEST_NAME "trex_nginx"

#include "trex.h"
#include "tapi_trex.h"
#include "tapi_cfg_nginx.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_sockaddr.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_cfg.h"

#include "te_file.h"

#define NGINX_NAME "webserver"
#define SRV_NAME "dflt"
#define LISTEN_NAME "1"

/** Driver name for DPDK port bounding. */
#define TE_TREX_PCI_DRIVER "uio_pci_generic"

/* Path to nginx exec. */
const char *nginx_path = "nginx";

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_tst = NULL;

    tapi_job_factory_t *factory_iut = NULL;
    tapi_job_factory_t *factory_tst = NULL;

    tapi_trex_app *trex_app = NULL;
    tapi_trex_opt trex_opt = tapi_trex_default_opt;
    tapi_trex_report trex_report = { 0 };

    const struct sockaddr *iut_addr;
    const struct sockaddr *tst_addr;

    const struct if_nameindex *tst_if = NULL;

    const char *astf_template_path;
    char astf_template[RCF_RPC_MAX_BUF];

    double trex_duration;
    unsigned trex_multiplier;

    char *port = NULL;
    unsigned nginx_port;

    TEST_START;

    TEST_STEP("Get parameters from environment");

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);

    TEST_GET_ADDR(pco_iut, iut_addr);
    TEST_GET_ADDR(pco_tst, tst_addr);

    TEST_GET_IF(tst_if);

    TEST_GET_UINT_PARAM(nginx_port);
    TEST_GET_DOUBLE_PARAM(trex_duration);
    TEST_GET_UINT_PARAM(trex_multiplier);

    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory_iut));
    CHECK_RC(tapi_job_factory_rpc_create(pco_tst, &factory_tst));

    TEST_STEP("Configure nginx and TRex instances");

    TEST_SUBSTEP("Check that nginx exec exists");
    if (rpc_te_file_check_executable(pco_iut, nginx_path) != 0)
        TEST_SKIP("There is no nginx app on iut");

    TEST_SUBSTEP("Initialize TRex params on tst");

    astf_template_path = getenv("TE_TREX_ASTF_TEMPLATE_PATH");
    if (te_str_is_null_or_empty(astf_template_path))
        TEST_SKIP("Path to TRex ASTF template is not specified in environment");

    trex_opt.trex_exec = getenv("TE_TREX_EXEC");
    if (te_str_is_null_or_empty(trex_opt.trex_exec))
        TEST_SKIP("Path to TRex exec is not specified in environment");

    CHECK_RC(te_file_read_text(astf_template_path, astf_template,
                               sizeof(astf_template)));

    trex_opt.driver = TE_TREX_PCI_DRIVER;
    trex_opt.astf_template = astf_template;
    trex_opt.lro_disable = TRUE;
    trex_opt.no_monitors = TRUE;
    trex_opt.duration = TAPI_JOB_OPT_DOUBLE_VAL(trex_duration);
    trex_opt.rate_multiplier = TAPI_JOB_OPT_UINT_VAL(trex_multiplier);

    trex_opt.clients = TAPI_TREX_CLIENTS(
        TAPI_TREX_CLIENT(
            .common.interface = TAPI_TREX_PCI_BY_IFACE(pco_tst->ta,
                                                       tst_if->if_name),
            .common.ip = tst_addr,
            .common.gw = iut_addr,
            .common.ip_range_beg = tst_addr,
            .common.ip_range_end = tst_addr,
            .common.port = TAPI_JOB_OPT_UINT_VAL(nginx_port),
            .common.payload = "GET /3384 HTTP/1.1\r\nHo"
        )
    );

    trex_opt.servers = TAPI_TREX_SERVERS(
        TAPI_TREX_SERVER(
            .common.ip = tst_addr,
            .common.gw = iut_addr,
            .common.ip_range_beg = iut_addr,
            .common.ip_range_end = iut_addr,
            .common.port = TAPI_JOB_OPT_UINT_VAL(nginx_port)
        )
    );

    TEST_STEP("Create TRex and nginx instances");

    TEST_SUBSTEP("Add nginx instance on iut");
    CHECK_RC(tapi_cfg_nginx_add(pco_iut->ta, NGINX_NAME));

    TEST_SUBSTEP("Add nginx HTTP server on iut");
    CHECK_RC(tapi_cfg_nginx_http_server_add(pco_iut->ta, NGINX_NAME,
                                            SRV_NAME));

    TEST_SUBSTEP("Add nginx listening entry on iut");
    CHECK_NOT_NULL(port = te_string_fmt("%u", nginx_port));
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(pco_iut->ta, NGINX_NAME,
                                                  SRV_NAME, LISTEN_NAME,
                                                  port));

    TEST_SUBSTEP("Create TRex instance on tst");
    CHECK_RC(tapi_trex_create(factory_tst, &trex_opt, &trex_app));

    TEST_STEP("Start TRex and nginx instances");

    TEST_SUBSTEP("Start nginx instance on iut");
    CHECK_RC(tapi_cfg_nginx_enable(pco_iut->ta, NGINX_NAME));

    TEST_SUBSTEP("Start TRex instance on tst");
    CHECK_RC(tapi_trex_start(trex_app));

    TEST_STEP("Wait for TRex and nginx apps to finish");

    TEST_STEP("Wait for TRex instance completion");
    CHECK_RC(tapi_trex_wait(trex_app, -1));

    TEST_STEP("Stop TRex and nginx instances");

    TEST_SUBSTEP("Stop TRex instance on tst");
    CHECK_RC(tapi_trex_stop(trex_app));

    TEST_SUBSTEP("Stop nginx instance on iut");
    CHECK_RC(tapi_cfg_nginx_disable(pco_iut->ta, NGINX_NAME));

    TEST_STEP("Get TRex instance report on tst");
    CHECK_RC(tapi_trex_get_report(trex_app, &trex_report));
    CHECK_RC(tapi_trex_report_mi_log(&trex_report));

    TEST_STEP("Delete nginx and TRex instances");

    TEST_SUBSTEP("Delete nginx instance on iut");
    CHECK_RC(tapi_cfg_nginx_del(pco_iut->ta, NGINX_NAME));

    TEST_SUBSTEP("Delete TRex instance on tst");
    CHECK_RC(tapi_trex_destroy(pco_tst->ta, trex_app, &trex_opt));
    CHECK_RC(tapi_trex_destroy_report(&trex_report));

    TEST_SUCCESS;

cleanup:
    tapi_job_factory_destroy(factory_iut);
    tapi_job_factory_destroy(factory_tst);

    free(port);

    TEST_END;
}
