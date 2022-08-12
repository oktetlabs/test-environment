/** @file
 * @brief TAPI NPtcp test
 *
 * Check TAPI NPtcp
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

/** @page tapi-tool-nptcp-np Run NPtcp and get report
 *
 * @objective Check tapi_nptcp_create/start/wait/get_report
 *
 * @par Scenario:
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME "nptcp/np"

#include "netpipe.h"

int main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_tst = NULL;

    const struct sockaddr *iut_addr = NULL;
    const char *iut_ip = NULL;

    tapi_job_factory_t *factory_receiver = NULL;
    tapi_job_factory_t *factory_transmitter = NULL;

    tapi_nptcp_app *app = NULL;
    tapi_nptcp_opt opt_receiver = tapi_nptcp_default_opt;
    tapi_nptcp_opt opt_transmitter = tapi_nptcp_default_opt;
    te_vec report = TE_VEC_INIT(tapi_nptcp_report_entry);
    tapi_nptcp_report_entry *entry;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);

    TEST_STEP("Initialize tapi_job_factory on pco_iut");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory_receiver));
    TEST_STEP("Initialize tapi_job_factory on pco_tst");
    CHECK_RC(tapi_job_factory_rpc_create(pco_tst, &factory_transmitter));

    TEST_STEP("Get IUT ip");
    TEST_GET_ADDR(pco_iut, iut_addr);
    iut_ip = te_sockaddr_get_ipstr(iut_addr);
    if (iut_ip == NULL)
        TEST_FAIL("Failed to get pco_iut ip address");

    RING("IUT ip is: %s", iut_ip);
    opt_transmitter.host = iut_ip;

    TEST_STEP("Initialize tapi_nptcp_app");
    if (tapi_nptcp_create(factory_receiver, factory_transmitter,
                          &opt_receiver, &opt_transmitter, &app) != 0)
        TEST_VERDICT("Failed to initialize tapi_nptcp_app");
    else
        TEST_SUBSTEP("tapi_nptcp_app is initialized successfully");

    TEST_STEP("Start NPtcp");
    if (tapi_nptcp_start(app) != 0)
        TEST_VERDICT("Failed to start NPtcp");
    else
        TEST_SUBSTEP("NPtcp is started successfully");

    TEST_STEP("Wait for NPtcp completion");
    if (tapi_nptcp_wait(app, TE_SEC2MS(120)) != 0)
        TEST_VERDICT("Failed to wait for NPtcp completion");
    else
        TEST_SUBSTEP("NPtcp completed successfully");

    TEST_STEP("Get report");
    if (tapi_nptcp_get_report(app, &report) != 0)
        TEST_VERDICT("Failed to get the report");
    else
        TEST_SUBSTEP("Got the report successfully");

    TE_VEC_FOREACH(&report, entry)
    {
        RING("Entry %d: %d bytes, %d times,       \
              throughput = %lf Mbps, rtt = %lf usec",
             entry->number, entry->bytes, entry->times,
             entry->throughput, entry->rtt);
    }

    TEST_SUCCESS;

cleanup:
    te_vec_free(&report);

    CLEANUP_CHECK_RC(tapi_nptcp_destroy(app));

    tapi_job_factory_destroy(factory_receiver);
    tapi_job_factory_destroy(factory_transmitter);

    TEST_END;
}
