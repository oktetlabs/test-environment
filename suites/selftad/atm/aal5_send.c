/** @file
 * @brief Test Environment
 *
 * AAL5 CSAP create and send.
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

/** @page aal5_send Create AAL5.ATM over Socket CSAP and send one SDU
 *
 * @objective Check possibility of CSAP AAL5 layer creation and sending
 *            data using created CSAP.
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#define TE_TEST_NAME    "aal5_send"

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
#include "tapi_tcp.h"
#include "tapi_atm.h"

#include "tapi_test.h"


int
main(int argc, char *argv[])
{
    tapi_env_host          *iut_host = NULL;
    rcf_rpc_server         *pco_tst = NULL;

    const struct sockaddr  *iut_addr = NULL;
    socklen_t               iut_addrlen;

    ndn_atm_type            type = NDN_ATM_UNI; /* PARAM */
    uint16_t                vpi;
    uint16_t                vci;
    te_bool                 congestion;
    te_bool                 clp;
    uint8_t                 gfc;
    uint8_t                 cpcs_uu;
    uint8_t                 cpi;

    csap_handle_t           tcp_srv_csap = CSAP_INVALID_HANDLE;
    int                     tst_s = -1;
    int                     iut_s = -1;
    asn_value              *csap_spec = NULL;
    csap_handle_t           csap = CSAP_INVALID_HANDLE;
    void                   *payload = NULL;
    size_t                  payload_len;
    asn_value              *tmpl = NULL;
    uint8_t                 cell[ATM_CELL_LEN];
    uint8_t                 idle[ATM_CELL_LEN] = { 0, };
    ssize_t                 r;
    size_t                  received;


    TEST_START;

    TEST_GET_HOST(iut_host);
    TEST_GET_PCO(pco_tst);
    TEST_GET_ADDR(iut_addr, iut_addrlen);
    TEST_GET_INT_PARAM(vpi);
    TEST_GET_INT_PARAM(vci);
    TEST_GET_BOOL_PARAM(congestion);
    TEST_GET_BOOL_PARAM(clp);
    TEST_GET_INT_PARAM(gfc);
    TEST_GET_INT_PARAM(cpcs_uu);
    TEST_GET_INT_PARAM(cpi);

    CHECK_NOT_NULL(payload = te_make_buf(0, 0xff, &payload_len));

    CHECK_RC(tapi_tcp_server_csap_create(iut_host->ta, 0, 
                                         SIN(iut_addr)->sin_addr.s_addr,
                                         sockaddr_get_port(iut_addr),
                                         &tcp_srv_csap));

    tst_s = rpc_socket(pco_tst, rpc_socket_domain_by_addr(iut_addr),
                       RPC_SOCK_STREAM, RPC_PROTO_DEF);
    rpc_connect(pco_tst, tst_s, iut_addr, iut_addrlen);

    CHECK_RC(tapi_tcp_server_recv(iut_host->ta, 0, tcp_srv_csap,
                                  1000000, &iut_s));

    CHECK_RC(rcf_ta_csap_destroy(iut_host->ta, 0, tcp_srv_csap));
    tcp_srv_csap = CSAP_INVALID_HANDLE;

    CHECK_RC(tapi_atm_aal5_add_csap_layer(&csap_spec, &cpcs_uu, &cpi));
    CHECK_RC(tapi_atm_add_csap_layer(&csap_spec,
                                     type, &vpi, &vci, &congestion, &clp));
    CHECK_RC(tapi_tad_socket_add_csap_layer(&csap_spec, iut_s));
    CHECK_RC(tapi_tad_csap_create(iut_host->ta, 0, "aal5.atm.socket",
                                  csap_spec, &csap));

    CHECK_RC(tapi_atm_aal5_add_pdu(&tmpl, FALSE, NULL, NULL));
    CHECK_RC(tapi_atm_add_pdu(&tmpl, FALSE, &gfc, NULL, NULL, NULL, NULL));
    CHECK_RC(asn_write_value_field(tmpl, payload, payload_len,
                                   "payload.#bytes"));
    CHECK_RC(tapi_tad_trsend_start(iut_host->ta, 0, csap, tmpl,
                                   RCF_MODE_BLOCKING));

    RING("Sent %u bytes as AAL5 payload, it is expected to receive %u "
         "cells", payload_len,
         (payload_len + AAL5_TRAILER_LEN + ATM_PAYLOAD_LEN - 1) /
             ATM_PAYLOAD_LEN);
    SLEEP(1);

    received = 0;
    while ((r = rpc_recv(pco_tst, tst_s, cell, sizeof(cell),
                         RPC_MSG_DONTWAIT)) > 0)
    {
        ssize_t useful = MAX(MIN((ssize_t)payload_len - (ssize_t)received,
                                 ATM_PAYLOAD_LEN), 0);
        ssize_t rest = ATM_PAYLOAD_LEN - useful;

        RING("Received cell is %Tm", cell, r);

        if (r != ATM_CELL_LEN)
            TEST_FAIL("Unexpected number of bytes received");

        if (useful > 0 &&
            memcmp(cell + ATM_HEADER_LEN, payload + received, useful) != 0)
        {
            TEST_FAIL("Unexpected payload in received cell.\n"
                      "Expected:%Tm\nGot:%Tm\n",
                      payload + received, useful,
                      cell + ATM_HEADER_LEN, useful);
        }

        if (rest > 0)
        {
            if (rest < AAL5_TRAILER_LEN)
            {
                if (memcmp(idle, cell + ATM_HEADER_LEN + useful,
                           rest) != 0)
                {
                    TEST_FAIL("Unexpected padding%Tm",
                              cell + ATM_HEADER_LEN + useful, rest);
                }
            }
            else if (rest > AAL5_TRAILER_LEN)
            {
                if (memcmp(idle, cell + ATM_HEADER_LEN + useful,
                           rest - AAL5_TRAILER_LEN) != 0)
                {
                    TEST_FAIL("Unexpected padding%Tm",
                              cell + ATM_HEADER_LEN + useful,
                              rest - AAL5_TRAILER_LEN);
                }
            }
        }

        received += (r - ATM_HEADER_LEN);

        RPC_AWAIT_IUT_ERROR(pco_tst);
    }

    TEST_SUCCESS;

cleanup:
    asn_free_value(tmpl);
    asn_free_value(csap_spec);

    CLEANUP_RPC_CLOSE(pco_tst, tst_s);

    if (iut_host != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(iut_host->ta, 0,
                                             tcp_srv_csap));

    TEST_END;
}
