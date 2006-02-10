/** @file
 * @brief Test Environment
 *
 * Tests on generic TAD functionality.
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 * 
 * $Id: example.c 18619 2005-09-22 16:47:08Z artem $
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
#include "tapi_tcp.h"


int
main(int argc, char *argv[])
{
    tapi_env_host      *iut_host = NULL;
    te_bool             zero_timeout;
    uint16_t            port;
    csap_handle_t       tcp_srv_csap = CSAP_INVALID_HANDLE;
    rcf_trpoll_csap     csapd;

    TEST_START;
    TEST_GET_HOST(iut_host);
    TEST_GET_BOOL_PARAM(zero_timeout);

    CHECK_RC(tapi_allocate_port_htons(&port));
    CHECK_RC(tapi_tcp_server_csap_create(iut_host->ta, 0, 
                                         htonl(INADDR_ANY), port,
                                         &tcp_srv_csap));
#if 0
    CHECK_RC(tapi_tcp_server_recv(iut_host->ta, 0, tcp_srv_csap,
                                  1000000, &iut_s));
#else
    {
        asn_value *pattern = NULL;

        int rc = 0, syms, num;

        rc = asn_parse_value_text("{ { pdus { socket:{} } } }",
                                  ndn_traffic_pattern, &pattern, &syms);
        if (rc != 0)
        {
            TEST_FAIL("%s(): parse ASN csap_spec failed %X, sym %d", 
                      __FUNCTION__, rc, syms);
            return rc;
        }

        rc = tapi_tad_trrecv_start(iut_host->ta, 0, tcp_srv_csap,
                                   pattern, 2000, 1, RCF_TRRECV_PACKETS);
        if (rc != 0)
        {
            TEST_FAIL("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        }
    }
#endif

    csapd.ta = iut_host->ta;
    csapd.csap_id = tcp_srv_csap;
    csapd.status = 0;

    rc = rcf_trpoll(&csapd, 1, zero_timeout ? 0 : rand_range(1, 1000));
    if (rc != 0)
    {
        TEST_FAIL("rcf_trpoll() unexpectedly failed: %r", rc);
    }
    if (TE_RC_GET_ERROR(csapd.status) != TE_ETADCSAPNOTEX)
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
