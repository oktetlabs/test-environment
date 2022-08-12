/** @file
 * @brief Test Environment
 *
 * Tests on generic TAD functionality.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 */

/** @page common-poll_one Call traffic poll operation for one CSAP
 *
 * @objective Check @b rcf_trpoll() behaviour with one CSAP only and
 *            different scenarious.
 *
 * @par Scenario:
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "common/poll_one"

#define TEST_START_VARS     TEST_START_ENV_VARS
#define TEST_START_SPECIFIC TEST_START_ENV
#define TEST_END_SPECIFIC   TEST_END_ENV

#include "te_config.h"

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "tapi_test.h"

#include "asn_usr.h"
#include "ndn.h"
#include "tapi_sockaddr.h"
#include "tapi_env.h"
#include "tapi_socket.h"


int
main(int argc, char *argv[])
{
    tapi_env_host      *iut_host = NULL;
    rcf_rpc_server     *pco_iut = NULL;
    te_bool             zero_timeout;
    csap_handle_t       tcp_srv_csap = CSAP_INVALID_HANDLE;
    rcf_trpoll_csap     csapd;
    struct sockaddr_in  addr;

    TEST_START;
    TEST_GET_HOST(iut_host);
    TEST_GET_PCO(pco_iut);
    TEST_GET_BOOL_PARAM(zero_timeout);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    CHECK_RC(tapi_allocate_port_htons(pco_iut, &addr.sin_port));
    CHECK_RC(tapi_tcp_server_csap_create(iut_host->ta, 0,
                                         SA(&addr), &tcp_srv_csap));

    CHECK_RC(tapi_tad_trrecv_start(iut_host->ta, 0, tcp_srv_csap,
                                   NULL, 2000, 1, RCF_TRRECV_PACKETS));

    csapd.ta = iut_host->ta;
    csapd.csap_id = tcp_srv_csap;
    csapd.status = 0;

    rc = rcf_trpoll(&csapd, 1, zero_timeout ? 0 : rand_range(1, 1000));
    if (rc != 0)
    {
        TEST_FAIL("rcf_trpoll() unexpectedly failed: %r", rc);
    }
    if (TE_RC_GET_ERROR(csapd.status) != TE_ETIMEDOUT)
    {
        TEST_FAIL("rcf_trpoll() with request set status to %r instead "
                  "of %r", csapd.status, TE_ETADCSAPNOTEX);
    }

    TEST_SUCCESS;

cleanup:

    if (iut_host != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(iut_host->ta, 0,
                                             tcp_srv_csap));

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
