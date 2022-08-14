/** @file
 * @brief TAPI NPtcp test
 *
 * Check TAPI NPtcp
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page tapi-tool-nptcp-np_with_stop Run NPtcp, stop it, and run again
 *
 * @objective Check tapi_nptcp_stop and tapi_nptcp_start after stop
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "nptcp/np_with_stop"

#include "netpipe.h"

int main(int argc, char **argv)
{
    te_errno rc_;

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
    CHECK_RC(tapi_nptcp_create(factory_receiver, factory_transmitter,
                               &opt_receiver, &opt_transmitter, &app));

    TEST_STEP("Start NPtcp");
    CHECK_RC(tapi_nptcp_start(app));

    TEST_STEP("Wait for 15 seconds");
    rc_ = tapi_nptcp_wait(app, TE_SEC2MS(15));
    if (rc_ != 0 && TE_RC_GET_ERROR(rc_) != TE_EINPROGRESS)
        TEST_FAIL("Failed to wait for NPtcp");

    TEST_STEP("Stop NPtcp");
    if (tapi_nptcp_stop(app) != 0)
        TEST_VERDICT("Failed to stop NPtcp");
    else
        TEST_SUBSTEP("NPtcp is stopped successfully");

    TEST_STEP("Get report");
    CHECK_RC(tapi_nptcp_get_report(app, &report));

    TE_VEC_FOREACH(&report, entry)
    {
        RING("Entry %d: %d bytes, %d times,       \
              throughput = %lf Mbps, rtt = %lf usec",
             entry->number, entry->bytes, entry->times,
             entry->throughput, entry->rtt);
    }
    te_vec_reset(&report);

    TEST_STEP("Start NPtcp after stop");
    if (tapi_nptcp_start(app) != 0)
        TEST_VERDICT("Failed to start NPtcp after it was stopped");
    else
        TEST_SUBSTEP("NPtcp was started after stop successfully");

    TEST_STEP("Wait for NPtcp completion again");
    CHECK_RC(tapi_nptcp_wait(app, TE_SEC2MS(120)));

    TEST_STEP("Get report");
    CHECK_RC(tapi_nptcp_get_report(app, &report));

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
