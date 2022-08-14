/** @file
 * @brief TAPI NPtcp test
 *
 * Check TAPI NPtcp
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page tapi-tool-nptcp-np_with_destroy Run NPtcp and destroy it
 *
 * @objective Check tapi_nptcp_destroy
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "nptcp/np_with_destroy"

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

    TEST_STEP("Destroy NPtcp");
    if (tapi_nptcp_destroy(app) != 0)
        TEST_VERDICT("Failed to destroy NPtcp");
    else
        TEST_SUBSTEP("NPtcp was destroyed successfully");

    TEST_SUCCESS;

cleanup:
    tapi_job_factory_destroy(factory_receiver);
    tapi_job_factory_destroy(factory_transmitter);

    TEST_END;
}
