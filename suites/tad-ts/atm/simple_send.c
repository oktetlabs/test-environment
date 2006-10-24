/** @file
 * @brief Test Environment
 *
 * A test example
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
 * $Id$
 */

/** @page atm-simple_send Create ATM over Socket CSAP and send one cell
 *
 * @objective Check possibility of CSAP ATM layer creation and sending
 *            a cell using created CSAP.
 *
 * @par Scenario:
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "atm/simple_send"

#define TEST_START_VARS     TEST_START_ENV_VARS
#define TEST_START_SPECIFIC TEST_START_ENV
#define TEST_END_SPECIFIC   TEST_END_ENV

#include "te_config.h"

#include "logger_api.h"
#include "rcf_api.h"
#include "ndn_atm.h"
#include "te_bufs.h"
#include "tapi_sockaddr.h"
#include "tapi_rpc.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_env.h"
#include "tapi_tad.h"
#include "tapi_socket.h"
#include "tapi_atm.h"

#include "tapi_test.h"


int
main(int argc, char *argv[])
{
    tapi_env_host          *iut_host = NULL;
    rcf_rpc_server         *pco_tst = NULL;

    const struct sockaddr  *iut_addr = NULL;

    ndn_atm_type            type = NDN_ATM_UNI; /* PARAM */
    uint16_t                vpi;
    uint16_t                vci;
    te_bool                 congestion;
    te_bool                 clp;
    uint8_t                 gfc;

    csap_handle_t           tcp_srv_csap = CSAP_INVALID_HANDLE;
    int                     tst_s = -1;
    int                     iut_s = -1;
    asn_value              *csap_spec = NULL;
    csap_handle_t           csap = CSAP_INVALID_HANDLE;
    void                   *payload = NULL;
    size_t                  payload_len;
    asn_value              *tmpl = NULL;
    uint8_t                 cell[ATM_CELL_LEN];
    ssize_t                 r;


    TEST_START;

    TEST_GET_HOST(iut_host);
    TEST_GET_PCO(pco_tst);
    TEST_GET_ADDR(iut_addr);
    TEST_GET_INT_PARAM(vpi);
    TEST_GET_INT_PARAM(vci);
    TEST_GET_BOOL_PARAM(congestion);
    TEST_GET_BOOL_PARAM(clp);
    TEST_GET_INT_PARAM(gfc);

    /* It may be NULL, if payload_len is 0 */
    payload = te_make_buf(0, ATM_PAYLOAD_LEN, &payload_len);

    CHECK_RC(tapi_tcp_server_csap_create(iut_host->ta, 0, 
                                         SIN(iut_addr)->sin_addr.s_addr,
                                         te_sockaddr_get_port(iut_addr),
                                         &tcp_srv_csap));

    tst_s = rpc_socket(pco_tst, rpc_socket_domain_by_addr(iut_addr),
                       RPC_SOCK_STREAM, RPC_PROTO_DEF);
    rpc_connect(pco_tst, tst_s, iut_addr);

    CHECK_RC(tapi_tcp_server_recv(iut_host->ta, 0, tcp_srv_csap,
                                  1000000, &iut_s));

    CHECK_RC(tapi_tad_csap_destroy(iut_host->ta, 0, tcp_srv_csap));
    tcp_srv_csap = CSAP_INVALID_HANDLE;

    CHECK_RC(tapi_atm_add_csap_layer(&csap_spec, NULL,
                                     type, &vpi, &vci, &congestion, &clp));
    CHECK_RC(tapi_tad_socket_add_csap_layer(&csap_spec, iut_s));
    CHECK_RC(tapi_tad_csap_create(iut_host->ta, 0, "atm.socket",
                                  csap_spec, &csap));

    CHECK_RC(tapi_atm_add_pdu(&tmpl, FALSE, &gfc, NULL, NULL, NULL, NULL));
    CHECK_RC(tapi_atm_add_payload(tmpl, payload_len, payload));
    CHECK_RC(tapi_tad_trsend_start(iut_host->ta, 0, csap, tmpl,
                                   RCF_MODE_BLOCKING));

    r = rpc_read(pco_tst, tst_s, cell, sizeof(cell));

    if (memcmp(payload, cell + ATM_HEADER_LEN, payload_len) != 0)
    {
        TEST_FAIL("Payload received in ATM cell%Tm\n"
                  "does not match sent data%Tm",
                  cell + ATM_HEADER_LEN, payload_len,
                  payload, payload_len);
    }

    RING("Sent payload is %Tm\nReceived cell is %Tm",
         payload, payload_len, cell, sizeof(cell));

    TEST_SUCCESS;

cleanup:
    asn_free_value(tmpl);
    asn_free_value(csap_spec);

    CLEANUP_RPC_CLOSE(pco_tst, tst_s);

    if (iut_host != NULL)
    {
        CLEANUP_CHECK_RC(tapi_tad_csap_destroy(iut_host->ta, 0, csap));
        CLEANUP_CHECK_RC(tapi_tad_csap_destroy(iut_host->ta, 0,
                                             tcp_srv_csap));
    }

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
