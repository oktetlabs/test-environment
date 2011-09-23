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

/** @page atm-simple_recv Create ATM over Socket CSAP and receive one cell
 *
 * @objective Check possibility of CSAP ATM layer creation and
 *            receiving/matching cells using created CSAP.
 *
 * @reference @ref ITU-T-Std-I361
 *
 * @param iut_host      Host with TA with tested TAD implementation
 * @param iut_addr      IPv4 address assigned to some interface
 *                      of the host @p iut_host
 * @param pco_tst       Auxiliary RPC server
 * @param csap_*        Whether corresponding CSAP parameter should
 *                      be unspecified, match or do not match sent
 *                      data
 * @param ptrn_*        Whether corresponding traffic pattern parameter
 *                      should be unspecified, match or do not match sent
 *                      data
 *
 * @par Scenario:
 *
 * -# Create socket CSAP with TCP listening socket on @p iut_host bound
 *    to @p iut_addr IPv4 address and some @p port.
 * -# Create TCP over IPv4 socket on @p pco_tst and connect it to
 *    @p iut_addr : @p port.
 * -# Receive accepted socket from CSAP with listening socket and close
 *    csap with listening socket.
 * \n @htmlonly &nbsp; @endhtmlonly
 * -# Create atm.socket CSAP over accepted socket using @p type and
 *    @p csap_* parameters passed to the test.
 * -# Prepare ATM layer pattern using @p ptrn_* parameters of the test
 *    and start receive operation on created CSAP using this pattern
 *    with 1 second timeout.
 * -# Send 53-bytes of prepared ATM cell to the socket on @p pco_tst
 *    using @b write() function.
 * -# Wait for receive operation completion. If any cell is received,
 *    check that its fields match fields of sent cell. Check that no
 *    cells are received, if at least or @p ptrn_* parameter is
 *    @c nomatch, or @p ptrn_* parameter is @c unspec and corresponding
 *    @p csap_* parameter is @c nomatch.
 * \n @htmlonly &nbsp; @endhtmlonly
 * -# Close socket on @p pco_tst.
 * -# Destroy all created CSAPs.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "atm/simple_recv"

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


/** CSAP parameter specification type */
typedef enum csap_param_spec_type {
    CSAP_PARAM_UNSPEC,  /**< Unspecified */
    CSAP_PARAM_MATCH,   /**< Match with value to be sent */
    CSAP_PARAM_NOMATCH, /**< No match with value to be sent */
} csap_param_spec_type;

/**
 * The list of values allowed for parameter of type 'rpc_socket_domain'
 */
#define CSAP_PARAM_MAPPING_LIST \
    { "unspec",   CSAP_PARAM_UNSPEC },  \
    { "match",    CSAP_PARAM_MATCH },   \
    { "nomatch",  CSAP_PARAM_NOMATCH }

/**
 * Get the value of parameter of type 'csap_param_spec_type'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'csap_param_spec_type'
 *                   (OUT)
 */
#define TEST_GET_CSAP_PARAM(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, CSAP_PARAM_MAPPING_LIST)


static const char             *type;
static csap_param_spec_type    csap_vpi;
static csap_param_spec_type    csap_vci;
static csap_param_spec_type    csap_congestion;
static csap_param_spec_type    csap_clp;
static csap_param_spec_type    ptrn_gfc;
static csap_param_spec_type    ptrn_vpi;
static csap_param_spec_type    ptrn_vci;
static csap_param_spec_type    ptrn_payload_type;
static csap_param_spec_type    ptrn_congestion;
static csap_param_spec_type    ptrn_clp;
static uint8_t                 gfc;
static uint16_t                vpi;
static uint16_t                vci;
static uint8_t                 payload_type;
static te_bool                 congestion;
static te_bool                 clp;

static void
my_callback(asn_value *packet, void *user_data)
{
    int32_t tmp;

    UNUSED(user_data);

    CHECK_RC(asn_read_int32(packet, &tmp, "pdus.0.#atm.gfc.#plain"));
    if (tmp != gfc)
        TEST_FAIL("Invalid GFC in received packet");

    CHECK_RC(asn_read_int32(packet, &tmp, "pdus.0.#atm.vpi.#plain"));
    if (tmp != vpi)
        TEST_FAIL("Invalid VPI in received packet");

    CHECK_RC(asn_read_int32(packet, &tmp, "pdus.0.#atm.vci.#plain"));
    if (tmp != vci)
        TEST_FAIL("Invalid VCI in received packet");

    CHECK_RC(asn_read_int32(packet, &tmp, "pdus.0.#atm.payload-type.#plain"));
    if (tmp != payload_type)
        TEST_FAIL("Invalid payload-type in received packet");

    CHECK_RC(asn_read_int32(packet, &tmp, "pdus.0.#atm.clp.#plain"));
    if (tmp != clp)
        TEST_FAIL("Invalid CLP in received packet");

    RING("Packet verification - OK");
}

int
main(int argc, char *argv[])
{
    tapi_env_host          *iut_host = NULL;
    rcf_rpc_server         *pco_tst = NULL;
    rcf_rpc_server         *pco_iut = NULL;

    const struct sockaddr  *iut_addr = NULL;

    uint8_t                 gfc_nomatch;
    uint16_t                vpi_nomatch;
    uint16_t                vci_nomatch;
    uint8_t                 payload_type_nomatch;
    te_bool                 congestion_nomatch;
    te_bool                 clp_nomatch;

    csap_handle_t           tcp_srv_csap = CSAP_INVALID_HANDLE;
    int                     tst_s = -1;
    int                     enable = 1;
    int                     iut_s = -1;
    asn_value              *csap_spec = NULL;
    csap_handle_t           csap = CSAP_INVALID_HANDLE;
    asn_value              *ptrn = NULL;

    uint32_t                cell_hdr = 0;
    uint8_t                 cell[ATM_CELL_LEN];

    ssize_t                 r;
    unsigned int            got = 0;


    TEST_START;

    TEST_GET_HOST(iut_host);
    TEST_GET_PCO(pco_tst);
    TEST_GET_PCO(pco_iut);
    TEST_GET_ADDR(pco_iut, iut_addr);

    TEST_GET_STRING_PARAM(type);

    TEST_GET_CSAP_PARAM(csap_vpi);
    TEST_GET_CSAP_PARAM(csap_vci);
    TEST_GET_CSAP_PARAM(csap_congestion);
    TEST_GET_CSAP_PARAM(csap_clp);

    TEST_GET_CSAP_PARAM(ptrn_gfc);
    TEST_GET_CSAP_PARAM(ptrn_vpi);
    TEST_GET_CSAP_PARAM(ptrn_vci);
    TEST_GET_CSAP_PARAM(ptrn_payload_type);
    TEST_GET_CSAP_PARAM(ptrn_congestion);
    TEST_GET_CSAP_PARAM(ptrn_clp);

    if (strcmp(type, "uni") == 0)
    {
        TEST_GET_INT_PARAM(gfc);
        gfc_nomatch = (gfc + 1) & 0xf;
    }
    TEST_GET_INT_PARAM(vpi);
    if (vpi >= (1 << ((strcmp(type, "uni") == 0) ? 8 : 12)))
        TEST_FAIL("Too big VPI parameter");
    vpi_nomatch = (vpi + 1) & 0xff;
    TEST_GET_INT_PARAM(vci);
#if 0
    if (vci >= (1 << 16))
        TEST_FAIL("Too big VCI parameter");
#endif
    vci_nomatch = (vci + 1) & 0xffff;
    TEST_GET_INT_PARAM(payload_type);
    if (payload_type >= (1 << 3))
        TEST_FAIL("Too big payload-type parameter");
    payload_type_nomatch = (payload_type + 1) & 0x7;
    TEST_GET_BOOL_PARAM(congestion);
    payload_type |= !!congestion << 1;
    congestion_nomatch = !congestion;
    TEST_GET_BOOL_PARAM(clp);
    clp_nomatch = !clp;

    if (strcmp(type, "uni") == 0)
        cell_hdr |= gfc << 28;
    cell_hdr |= vpi << 20;
    cell_hdr |= vci << 4;
    cell_hdr |= payload_type << 1;
    cell_hdr |= (clp) ? 1 : 0;
    cell_hdr = htonl(cell_hdr);

    /* Copy cell header */
    memcpy(cell, &cell_hdr, sizeof(cell_hdr));
    /* Initialize HEC as zero */
    cell[ATM_HEADER_LEN - 1] = 0;
    /* Fill in payload */
    te_fill_buf(cell + ATM_HEADER_LEN, ATM_PAYLOAD_LEN);

    CHECK_RC(tapi_tcp_server_csap_create(iut_host->ta, 0, iut_addr,
                                         &tcp_srv_csap));

    tst_s = rpc_socket(pco_tst, rpc_socket_domain_by_addr(iut_addr),
                       RPC_SOCK_STREAM, RPC_PROTO_DEF);
    rpc_setsockopt(pco_tst, tst_s, RPC_TCP_NODELAY, &enable);
    rpc_connect(pco_tst, tst_s, iut_addr);

    CHECK_RC(tapi_tcp_server_recv(iut_host->ta, 0, tcp_srv_csap,
                                  1000000, &iut_s));

    CHECK_RC(tapi_tad_csap_destroy(iut_host->ta, 0, tcp_srv_csap));
    tcp_srv_csap = CSAP_INVALID_HANDLE;

    CHECK_RC(tapi_atm_add_csap_layer(&csap_spec, NULL,
                 (strcmp(type, "uni") == 0) ?  NDN_ATM_UNI : NDN_ATM_NNI,
                 (csap_vpi == CSAP_PARAM_UNSPEC) ?  NULL :
                 (csap_vpi == CSAP_PARAM_MATCH) ?  &vpi : &vpi_nomatch,
                 (csap_vci == CSAP_PARAM_UNSPEC) ?  NULL :
                 (csap_vci == CSAP_PARAM_MATCH) ?  &vci : &vci_nomatch,
                 (csap_congestion == CSAP_PARAM_UNSPEC) ?  NULL :
                 (csap_congestion == CSAP_PARAM_MATCH) ?  &congestion :
                     &congestion_nomatch,
                 (csap_clp == CSAP_PARAM_UNSPEC) ?  NULL :
                 (csap_clp == CSAP_PARAM_MATCH) ?  &clp : &clp_nomatch));
    CHECK_RC(tapi_tad_socket_add_csap_layer(&csap_spec, iut_s));
    CHECK_RC(tapi_tad_csap_create(iut_host->ta, 0, "atm.socket",
                                  csap_spec, &csap));

    CHECK_RC(tapi_atm_add_pdu(&ptrn, TRUE,
                 (ptrn_gfc == CSAP_PARAM_UNSPEC ||
                     strcmp(type, "uni") != 0) ?  NULL :
                 (ptrn_gfc == CSAP_PARAM_MATCH) ?  &gfc : &gfc_nomatch,
                 (ptrn_vpi == CSAP_PARAM_UNSPEC) ?  NULL :
                 (ptrn_vpi == CSAP_PARAM_MATCH) ?  &vpi : &vpi_nomatch,
                 (ptrn_vci == CSAP_PARAM_UNSPEC) ?  NULL :
                 (ptrn_vci == CSAP_PARAM_MATCH) ?  &vci : &vci_nomatch,
                 (ptrn_payload_type == CSAP_PARAM_UNSPEC) ?  NULL :
                 (ptrn_payload_type == CSAP_PARAM_MATCH) ?  &payload_type :
                     &payload_type_nomatch,
                 (ptrn_clp == CSAP_PARAM_UNSPEC) ?  NULL :
                 (ptrn_clp == CSAP_PARAM_MATCH) ?  &clp : &clp_nomatch));
    CHECK_RC(tapi_tad_trrecv_start(iut_host->ta, 0, csap, ptrn, 1000, 1,
                                   RCF_TRRECV_PACKETS));

    r = rpc_write(pco_tst, tst_s, cell, sizeof(cell));
    if (r != (ssize_t)sizeof(cell))
        TEST_FAIL("Failed to send ATM cell via socket");
    RING("Sent ATM cell is %Tm", cell, sizeof(cell));

    rc = tapi_tad_trrecv_wait(iut_host->ta, 0, csap,
                              tapi_tad_trrecv_make_cb_data(my_callback,
                                                           NULL),
                              &got);
    if ((rc != 0) && (TE_RC_GET_ERROR(rc) != TE_ETIMEDOUT))
        TEST_FAIL("Unexpected result of the trrecv_wait operation: %r", rc);

    /* TODO: Check that packet have (not) to be catched */

    TEST_SUCCESS;

cleanup:
    asn_free_value(ptrn);
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
