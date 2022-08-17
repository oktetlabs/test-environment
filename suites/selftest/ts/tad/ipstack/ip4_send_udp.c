/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Check UDP/IP4/ETH CSAP data-sending behaviour
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page ipstack-ip4_send_udp Send UDP/IP4 datagram via udp.ip4.eth CSAP and receive it via DGRAM socket
 *
 * @objective Check that udp.ip4.eth CSAP can send UDP datagrams
 *            with user-specified ports and checksum.
 *
 * @param host_csap     TA with CSAP
 * @param pco           TA with RAW socket
 * @param csap_addr     CSAP local IPv4 address
 * @param sock_addr     CSAP remote IPv4 address
 * @param csap_hwaddr   CSAP local MAC address
 * @param sock_hwaddr   CSAP remote MAC address
 * @param pld_len       Datagram's payload length
 * @param chksum        Datagram's checksum (correct or
 *                      corrupted by user)
 *
 * @par Scenario:
 *
 * -# Create udp.ip4.eth CSAP on @p pco_csap.
 * -# Create UDP socket on @p pco_sock.
 * -# Send UDP/IP4 datagrem with specified payload length and checksum.
 * -# If @p chksum is 'correct' receive datagram via socket.
 * -# In other cases check that no datagram is received.
 * -# Destroy CSAP and close socket
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "ipstack/ip4_send_udp"

#define TEST_START_VARS         TEST_START_ENV_VARS
#define TEST_START_SPECIFIC     TEST_START_ENV
#define TEST_END_SPECIFIC       TEST_END_ENV

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif

#include "tad_common.h"
#include "rcf_rpc.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "ndn_ipstack.h"
#include "tapi_ndn.h"
#include "tapi_udp.h"
#include "tapi_env.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_test.h"
#include "tapi_rpc_params.h"
#include "tapi_sockaddr.h"

int
main(int argc, char *argv[])
{
    int                         pld_len;

    tapi_env_host              *host_csap = NULL;
    rcf_rpc_server             *pco = NULL;
    rcf_rpc_server             *pco_a = NULL;
    const struct sockaddr      *csap_addr;
    const struct sockaddr      *sock_addr;
    const struct sockaddr      *csap_hwaddr;
    const struct sockaddr      *sock_hwaddr;
    const struct if_nameindex  *csap_if;
    const char                 *chksum;

    csap_handle_t               udp_ip4_send_csap = CSAP_INVALID_HANDLE;
    int                         recv_socket = -1;

    asn_value                  *template = NULL;

    void                       *send_buf = NULL;
    void                       *recv_buf = NULL;

    te_bool                     sum_ok;

    TEST_START;

    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(pco);
    TEST_GET_PCO(pco_a);
    TEST_GET_ADDR(pco_a, csap_addr);
    TEST_GET_ADDR(pco, sock_addr);
    TEST_GET_LINK_ADDR(csap_hwaddr);
    TEST_GET_LINK_ADDR(sock_hwaddr);
    TEST_GET_IF(csap_if);
    TEST_GET_INT_PARAM(pld_len);
    TEST_GET_STRING_PARAM(chksum);

    /* Create send-recv buffers */
    send_buf = te_make_buf_by_len(pld_len);
    recv_buf = te_make_buf_by_len(pld_len);

    /* Create UDP socket */
    recv_socket = rpc_socket(pco,
                             RPC_PF_INET, RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);
    /* Bind socket */
    CHECK_RC(rpc_bind(pco, recv_socket, sock_addr));

    /* Create CSAP */
    CHECK_RC(tapi_udp_ip4_eth_csap_create(host_csap->ta,
                                          0,
                                          csap_if->if_name,
                                          TAD_ETH_RECV_NO,
                                          (const uint8_t *)csap_hwaddr->sa_data,
                                          (const uint8_t *)sock_hwaddr->sa_data,
                                          SIN(csap_addr)->sin_addr.s_addr,
                                          SIN(sock_addr)->sin_addr.s_addr,
                                          SIN(csap_addr)->sin_port,
                                          SIN(sock_addr)->sin_port,
                                          &udp_ip4_send_csap));

    /* Prepare data-sending template */
    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(&template, FALSE,
                                          ndn_udp_header,
                                          "#udp", NULL));
    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(&template, FALSE,
                                          ndn_ip4_header,
                                          "#ip4", NULL));
    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(&template, FALSE,
                                          ndn_eth_header,
                                          "#eth", NULL));
    CHECK_RC(tapi_tad_tmpl_ptrn_set_payload_plain(&template, FALSE,
                                                  send_buf, pld_len));

    if (strcmp(chksum, "correct") == 0)
    {
        sum_ok = TRUE;
    }
    else if (chksum[0] == '+')
    {
        char           *end;
        unsigned long   v = strtoul(chksum + 1, &end, 0);

        if (end == chksum + 1 || *end != '\0')
            TEST_FAIL("Invalide 'chksum' parameter value '%s'", chksum);

        CHECK_RC(asn_write_int32(template, v,
                                 "pdus.1.#ip4.pld-checksum.#diff"));
        sum_ok = FALSE;
    }
    else
    {
        TEST_FAIL("Invalid 'chksum' parameter value '%s'", chksum);
    }

    /* Start sending data */
    CHECK_RC(tapi_tad_trsend_start(host_csap->ta, 0, udp_ip4_send_csap,
                                   template, RCF_MODE_BLOCKING));

    MSLEEP(100);

    /* Start receiving data */
    RPC_AWAIT_IUT_ERROR(pco);
    rc = rpc_recv(pco, recv_socket, recv_buf, pld_len, RPC_MSG_DONTWAIT);

    if (!sum_ok && rc != -1)
        TEST_FAIL("Datadgram was received despite of incorrect checksum");
    else if (sum_ok && rc != pld_len)
        TEST_FAIL("Numbers of sent and received bytes differ");
    else if (sum_ok && memcmp(send_buf, recv_buf, pld_len) != 0)
        TEST_FAIL("UDP payload corrupted");

    TEST_SUCCESS;

cleanup:

    CLEANUP_RPC_CLOSE(pco, recv_socket);

    asn_free_value(template);

    if (host_csap != NULL)
        CLEANUP_CHECK_RC(rcf_ta_csap_destroy(host_csap->ta, 0,
                                             udp_ip4_send_csap));

    free(send_buf);
    free(recv_buf);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
