/** @file
 * @brief Test Environment
 *
 * Send/receive test for IEEE Std 802.3 frames with Ethernet2 and LLC
 * encapsulation, 802.1Q tagged/untagged.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "eth/send_recv"

#define TEST_START_VARS         TEST_START_ENV_VARS
#define TEST_START_SPECIFIC     TEST_START_ENV
#define TEST_END_SPECIFIC       TEST_END_ENV

#include "te_config.h"

#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_eth.h"


static const uint16_t tst_eth_type = 0xf0f0;
static const uint8_t  tst_priority = 1;

static void
test_send_eth_frame(const char *ta, csap_handle_t csap,
                    te_bool3 tagged, te_bool3 llc_snap)
{
    asn_value  *tmpl = NULL;
    asn_value  *pdu = NULL;

    CHECK_RC(tapi_eth_add_pdu(&tmpl, &pdu, FALSE, NULL, NULL,
                              &tst_eth_type, tagged, llc_snap));

    if (tagged == TE_BOOL3_TRUE)
        CHECK_RC(tapi_eth_pdu_tag_header(pdu, &tst_priority, NULL));

    if (llc_snap == TE_BOOL3_TRUE)
        CHECK_RC(tapi_eth_pdu_llc_snap(pdu));

    CHECK_RC(tapi_tad_trsend_start(ta, 0, csap, tmpl, RCF_MODE_BLOCKING));

    asn_free_value(tmpl);
}

int
main(int argc, char **argv)
{
    tapi_env_host              *host_send = NULL;
    tapi_env_host              *host_recv = NULL;
    const struct if_nameindex  *if_send = NULL;
    const struct if_nameindex  *if_recv = NULL;
    const void                 *hwaddr_send = NULL;
    const void                 *hwaddr_recv = NULL;

    te_bool3    tagged;
    te_bool3    llc_snap;
    te_bool     ethertype;

    csap_handle_t   send_csap = CSAP_INVALID_HANDLE;
    csap_handle_t   recv_csap = CSAP_INVALID_HANDLE;

    asn_value  *csap_spec = NULL;
    asn_value  *pattern = NULL;
    asn_value  *pdu = NULL;

    unsigned int    num;


    TEST_START;
    TEST_GET_HOST(host_send);
    TEST_GET_IF(if_send);
    TEST_GET_LINK_ADDR(hwaddr_send);
    TEST_GET_HOST(host_recv);
    TEST_GET_IF(if_recv);
    TEST_GET_LINK_ADDR(hwaddr_recv);
    TEST_GET_BOOL3_PARAM(tagged);
    TEST_GET_BOOL3_PARAM(llc_snap);
    TEST_GET_BOOL_PARAM(ethertype);

    /* Create send CSAP */
    CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, if_send->if_name,
                                     TAD_ETH_RECV_NO,
                                     hwaddr_recv, hwaddr_send, NULL,
                                     TE_BOOL3_ANY /* tagged/untagged */,
                                     TE_BOOL3_ANY /* Ethernet2/LLC */));
    CHECK_RC(tapi_tad_csap_create(host_send->ta, 0, "eth", csap_spec,
                                  &send_csap));
    asn_free_value(csap_spec); csap_spec = NULL;

    /* Create receive CSAP */
    CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, if_recv->if_name,
                                     TAD_ETH_RECV_ALL,
                                     hwaddr_send, hwaddr_recv, NULL,
                                     TE_BOOL3_ANY /* tagged/untagged */,
                                     TE_BOOL3_ANY /* Ethernet2/LLC */));
    CHECK_RC(tapi_tad_csap_create(host_recv->ta, 0, "eth", csap_spec,
                                  &recv_csap));
    asn_free_value(csap_spec); csap_spec = NULL;

    /* Prepare receive pattern and start receiver */
    CHECK_RC(tapi_eth_add_pdu(&pattern, &pdu, TRUE, NULL, NULL,
                              &tst_eth_type, tagged, llc_snap));
    if (tagged == TE_BOOL3_TRUE)
        CHECK_RC(tapi_eth_pdu_tag_header(pdu, &tst_priority, NULL));
    if (llc_snap == TE_BOOL3_TRUE)
        CHECK_RC(tapi_eth_pdu_llc_snap(pdu));

    CHECK_RC(tapi_tad_trrecv_start(host_recv->ta, 0, recv_csap,
                                   pattern, 1000, 0, RCF_TRRECV_PACKETS));

    /* Send various frames */
    test_send_eth_frame(host_send->ta, send_csap,
                        TE_BOOL3_FALSE, TE_BOOL3_FALSE);
    test_send_eth_frame(host_send->ta, send_csap,
                        TE_BOOL3_TRUE, TE_BOOL3_FALSE);
    test_send_eth_frame(host_send->ta, send_csap,
                        TE_BOOL3_FALSE, TE_BOOL3_TRUE);
    test_send_eth_frame(host_send->ta, send_csap,
                        TE_BOOL3_TRUE, TE_BOOL3_TRUE);

    rc = tapi_tad_trrecv_wait(host_recv->ta, 0, recv_csap, NULL, &num);
    if (TE_RC_GET_ERROR(rc) != TE_ETIMEDOUT)
        TEST_FAIL("Unexpected status of wait operation: %r", rc);

    if ((int)num != 1 + (tagged == TE_BOOL3_ANY) +
                    (llc_snap == TE_BOOL3_ANY) +
                    (tagged == TE_BOOL3_ANY && llc_snap == TE_BOOL3_ANY))
    {
        TEST_FAIL("Unexpected number of packets is received");
    }

    TEST_SUCCESS;

cleanup:

    asn_free_value(csap_spec); csap_spec = NULL;
    asn_free_value(pattern); pattern = NULL;

    if (host_send != NULL)
        CLEANUP_CHECK_RC(tapi_tad_csap_destroy(host_send->ta, 0,
                                               send_csap));
    if (host_recv != NULL)
        CLEANUP_CHECK_RC(tapi_tad_csap_destroy(host_recv->ta, 0,
                                               recv_csap));

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
