/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI TRex with wrk test
 *
 * Demonstrate the usage of TAPI TRex with wrk tool.
 */

#define TE_TEST_NAME "trex_wrk"

#include "trex.h"
#include "tapi_wrk.h"
#include "tapi_trex.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_sockaddr.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_cfg.h"

#include "te_file.h"

/** Driver name for DPDK port bounding. */
#define TE_TREX_PCI_DRIVER "uio_pci_generic"

/* Path to wrk exec. */
const char *wrk_path = "wrk";

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

    tapi_wrk_app *wrk_app = NULL;
    tapi_wrk_opt wrk_opt = tapi_wrk_default_opt;

    const struct sockaddr *iut_addr = NULL;
    const struct sockaddr *tst_addr = NULL;

    const struct if_nameindex *tst_if = NULL;

    const char *astf_template_path;
    char astf_template[RCF_RPC_MAX_BUF];

    char *trex_host = NULL;
    char *trex_addr = NULL;

    double trex_duration;
    unsigned wrk_connections;

    TEST_START;

    TEST_STEP("Get parameters from environment");

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);

    TEST_GET_ADDR(pco_iut, iut_addr);
    TEST_GET_ADDR(pco_tst, tst_addr);

    TEST_GET_IF(tst_if);

    TEST_GET_DOUBLE_PARAM(trex_duration);
    TEST_GET_UINT_PARAM(wrk_connections);

    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory_iut));
    CHECK_RC(tapi_job_factory_rpc_create(pco_tst, &factory_tst));

    TEST_STEP("Configure TRex and wrk instances");

    TEST_SUBSTEP("Check that wrk exec exists on iut");
    if (rpc_te_file_check_executable(pco_iut, wrk_path) != 0)
        TEST_SKIP("There is no nginx app on iut");

    TEST_SUBSTEP("Initialize TRex instances params on tst");

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
    trex_opt.force_close_at_end = TRUE;
    trex_opt.no_monitors = TRUE;
    trex_opt.astf_server_only = TRUE;
    trex_opt.lro_disable = TRUE;
    trex_opt.duration = TAPI_JOB_OPT_DOUBLE_VAL(trex_duration);

    trex_opt.servers = TAPI_TREX_SERVERS(
        TAPI_TREX_SERVER(
            .common.interface = TAPI_TREX_PCI_BY_IFACE(pco_tst->ta,
                                                       tst_if->if_name),
            .common.ip = tst_addr,
            .common.gw = iut_addr,
            .common.ip_range_beg = tst_addr,
            .common.ip_range_end = tst_addr,
            .common.payload = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\nConnection: keep-alive\r\n"
                    "Content-Length: 18\r\n\r\n<html>Hello</html>"
        )
    );

    trex_addr = te_ip2str(tst_addr);
    trex_host = te_string_fmt("http://%s:80", trex_addr);

    TEST_SUBSTEP("Initialize wrk params on iut");
    wrk_opt.duration_s = trex_duration - 2;
    wrk_opt.connections = wrk_connections;
    wrk_opt.host = trex_host;

    TEST_STEP("Create and start TRex instance");

    TEST_SUBSTEP("Create TRex instance on tst");
    CHECK_RC(tapi_trex_create(factory_tst, &trex_opt, &trex_app));

    TEST_SUBSTEP("Start TRex instance on tst");
    CHECK_RC(tapi_trex_start(trex_app));

    TEST_STEP("Create and start wrk instance");

    TEST_SUBSTEP("Create wrk instance on iut");
    CHECK_RC(tapi_wrk_create(factory_iut, &wrk_opt, &wrk_app));

    TEST_SUBSTEP("Start wrk instance on iut");
    CHECK_RC(tapi_wrk_start(wrk_app));

    TEST_STEP("Wait for TRex and wrk instances to finish");

    TEST_STEP("Wait for wrk instance completion");
    CHECK_RC(tapi_wrk_wait(wrk_app, -1));

    TEST_STEP("Wait for TRex instance completion");
    CHECK_RC(tapi_trex_wait(trex_app, -1));

    TEST_STEP("Stop TRex instance on tst");
    CHECK_RC(tapi_trex_stop(trex_app));

    TEST_STEP("Get TRex report on tst");
    CHECK_RC(tapi_trex_get_report(trex_app, &trex_report));
    CHECK_RC(tapi_trex_report_mi_log(&trex_report));

    TEST_STEP("Delete TRex instance on tst");
    CHECK_RC(tapi_trex_destroy(pco_tst->ta, trex_app, &trex_opt));
    CHECK_RC(tapi_trex_destroy_report(&trex_report));

    TEST_STEP("Delete wrk instance on tst");
    CHECK_RC(tapi_wrk_destroy(wrk_app));

    TEST_SUCCESS;

cleanup:
    tapi_job_factory_destroy(factory_tst);
    tapi_job_factory_destroy(factory_iut);

    free(trex_addr);
    free(trex_host);

    TEST_END;
}
