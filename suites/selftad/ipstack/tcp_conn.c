/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * TCP CSAP and TAPI test
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_TEST_NAME    "ipstack/tcp_conn"

#define TE_LOG_LEVEL 0xff

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ipstack-ts.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "conf_api.h"
#include "tapi_rpc.h"

#include "tapi_env.h"
#include "tapi_test.h"
#include "tapi_ip4.h"
#include "tapi_udp.h"
#include "tapi_tcp.h"
#include "tapi_cfg.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"

uint8_t buffer[10000];

int
main(int argc, char *argv[])
{
    tapi_env_host *host_csap;
    const struct if_nameindex *sock_if;
    const struct if_nameindex *csap_if;

    int syms = 0;
    tapi_tcp_handler_t conn_hand;
    asn_value *tcp_template;

    rcf_rpc_server *sock_pco = NULL;
    rcf_rpc_server *pco_a = NULL;

    char    *agt_a;
    size_t   len;
    uint8_t  flags;

    int socket = -1;
    int acc_sock = -1;
    int opt_val = 1;

    te_bool is_server;
    te_bool init_close;

    const struct sockaddr *csap_addr;
    const struct sockaddr *sock_addr;

    // uint8_t csap_mac[6] = {0x00, 0x05, 0x5D, 0x74, 0xAB, 0xB4};
    uint8_t csap_mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t sock_mac[6];
    size_t  sock_mac_len = sizeof(sock_mac);

    TEST_START;

    TEST_GET_BOOL_PARAM(is_server);
    TEST_GET_BOOL_PARAM(init_close);
    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(sock_pco);
    TEST_GET_PCO(pco_a);
    TEST_GET_IF(sock_if);
    TEST_GET_IF(csap_if);
    TEST_GET_ADDR(sock_pco, sock_addr);
    TEST_GET_ADDR(pco_a, csap_addr);

    CHECK_RC(tapi_cfg_get_hwaddr(sock_pco->ta, sock_if->if_name,
                                 sock_mac, &sock_mac_len));

    rc = asn_parse_value_text(
              "{ arg-sets { ints:{0}, ints-assoc:{0} },"
              "  pdus { tcp:{seqn script:\"expr:$0\"}, "
              "         ip4:{}, eth:{}},"
              "  payload stream:{offset script:\"expr:$0\", "
              "                  length script:\"expr:$1\", "
              "                  function \"arithm_progr\""
              "                 },"
              "}",
              ndn_traffic_template, &tcp_template, &syms);
    if (rc != 0)
        TEST_FAIL("parse complex template failed %X syms %d", rc, syms);


    agt_a = host_csap->ta;

    if ((socket = rpc_socket(sock_pco, RPC_AF_INET, RPC_SOCK_STREAM,
                                  RPC_IPPROTO_TCP)) < 0 ||
        sock_pco->_errno != 0)
        TEST_FAIL("Calling of RPC socket() failed %r", sock_pco->_errno);

    opt_val = 1;
    rpc_setsockopt(sock_pco, socket, RPC_SO_REUSEADDR, &opt_val);


    rc = rpc_bind(sock_pco, socket, sock_addr);
    if (rc != 0)
        TEST_FAIL("bind failed");



    if (!is_server) /*Csap is server, socket is client */
    {
        rc = rpc_listen(sock_pco, socket, 1);
        if (rc != 0)
            TEST_FAIL("listen failed");
    }

    rc = tapi_tcp_init_connection(agt_a,
                                  is_server ? TAPI_TCP_SERVER :
                                              TAPI_TCP_CLIENT,
                                  csap_addr, sock_addr,
                                  csap_if->if_name, csap_mac, sock_mac,
                                  1000, &conn_hand);
    if (rc != 0)
        TEST_FAIL("init connection failed: %r", rc);

    if (is_server)
    {
        sock_pco->op = RCF_RPC_CALL;
        rc = rpc_connect(sock_pco, socket, csap_addr);
        if (rc != 0)
            TEST_FAIL("connect() 'call' failed: %r", rc);
    }

    rc = tapi_tcp_wait_open(conn_hand, 2000);
    if (rc != 0)
        TEST_FAIL("open connection failed: %r", rc);

    RING("connection inited, handle %d", conn_hand);

    if (is_server)
    {
        sock_pco->op = RCF_RPC_WAIT;
        rc = rpc_connect(sock_pco, socket, csap_addr);
        if (rc != 0)
            TEST_FAIL("connect() 'wait' failed: %r", rc);
    }
    else
    {
        acc_sock = rpc_accept(sock_pco, socket, NULL, NULL);

        rpc_close(sock_pco, socket);
        socket = acc_sock;
        acc_sock = -1;
    }

    opt_val = 1;
    rpc_setsockopt(sock_pco, socket, RPC_SO_REUSEADDR, &opt_val);

    /*
     * Send data
     */

    rc = rpc_send(sock_pco, socket, buffer, 200, 0);

    {
        tapi_tcp_pos_t seq, ack;
        len = sizeof(buffer);
        rc = tapi_tcp_recv_msg(conn_hand, 2000, TAPI_TCP_AUTO,
                               buffer, &len, &seq, &ack, &flags);
        if (rc != 0)
            TEST_FAIL("recv_msg() failed: %r", rc);

        RING("msg received: %d bytes, seq %u", len, seq);
    }
#if 1
    rc = tapi_tcp_send_msg(conn_hand, buffer, 50, TAPI_TCP_AUTO, 0,
                           TAPI_TCP_QUIET, 0, NULL, 0);
    if (rc != 0)
        TEST_FAIL("tapi_tcp_send_msg() failed: %r", rc);

    rc = rpc_recv(sock_pco, socket, buffer, sizeof(buffer), 0);
#endif
    {
        tapi_tcp_pos_t seqn = tapi_tcp_next_seqn(conn_hand);
        uint32_t length = 120;


        rc = asn_write_int32(tcp_template, 1400,
                        "pdus.0.#tcp.options.0.#mss.mss.#plain");
        if (rc != 0)
            WARN("write MSS failed %r", rc);

        rc = asn_write_int32(tcp_template, seqn, "arg-sets.0.#ints.0");
        if (rc != 0)
            TEST_FAIL("write arg seqn failed %X", rc);
        rc = asn_write_int32(tcp_template, length,
                             "arg-sets.1.#ints-assoc.0");
        if (rc != 0)
            TEST_FAIL("write arg len failed %X", rc);

        rc = tapi_tcp_send_template(conn_hand, tcp_template,
                                    RCF_MODE_BLOCKING);
        if (rc != 0)
            TEST_FAIL("send template failed %X", rc);

        rpc_recv(sock_pco, socket, buffer, sizeof(buffer), 0);

        tapi_tcp_update_sent_seq(conn_hand, length);
    }
    asn_free_value(tcp_template);
    tcp_template = NULL;

    {
        uint32_t length = 200;

        tapi_tcp_conn_template(conn_hand, buffer, length, &tcp_template);
        rc = asn_write_int32(tcp_template, 1025,
                             "pdus.0.#tcp.options.0.#mss.mss.#plain");
        if (rc != 0)
            WARN("write MSS failed %r", rc);

        asn_write_value_field(tcp_template, NULL, 0,
                             "pdus.0.#tcp.options.1.#nop");
        asn_write_value_field(tcp_template, NULL, 0,
                             "pdus.0.#tcp.options.2.#nop");
        asn_write_int32(tcp_template, 1234567,
                     "pdus.0.#tcp.options.3.#timestamp.value.#plain");
        asn_write_int32(tcp_template, 0,
                     "pdus.0.#tcp.options.3.#timestamp.echo-reply.#plain");
        rc = tapi_tcp_send_template(conn_hand, tcp_template,
                                    RCF_MODE_BLOCKING);
        if (rc != 0)
            TEST_FAIL("send template failed %X", rc);
        tapi_tcp_update_sent_seq(conn_hand, length);

        rpc_recv(sock_pco, socket, buffer, sizeof(buffer), 0);
    }

    /*
     * Closing connection
     */
    if (!init_close)
    {
        rpc_close(sock_pco, socket);
        socket = -1;
    }

    rc = tapi_tcp_send_fin(conn_hand, 1000);
    if (rc != 0)
        TEST_FAIL("wait for ACK to our FIN failed: %r", rc);

    if (init_close)
    {
        rpc_close(sock_pco, socket);
        socket = -1;
    }

    do {
        rc = tapi_tcp_recv_msg(conn_hand, 2000, TAPI_TCP_AUTO,
                               NULL, 0, NULL, NULL, &flags);
        if (rc != 0)
            TEST_FAIL("close connection failed: %r", rc);

        if (flags & TCP_FIN_FLAG)
        {
            RING("FIN received!");
        }
    } while (!(flags & TCP_FIN_FLAG) && !(flags & TCP_RST_FLAG));

    TEST_SUCCESS;

cleanup:

    if (acc_sock > 0)
        rpc_close(sock_pco, acc_sock);

    if (socket > 0)
        rpc_close(sock_pco, socket);


    TEST_END;
}
