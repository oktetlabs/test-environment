/** @file
 * @brief Test Environment
 *
 * TCP CSAP and TAPI test
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_TEST_NAME    "ipstack/tcp_conn"

#define TE_LOG_LEVEL 0xff

#include "config.h"

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
#include "tapi_ip.h"
#include "tapi_udp.h"
#include "tapi_tcp.h"
#include "tapi_cfg.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


int
main(int argc, char *argv[]) 
{ 
    tapi_env_host *host_csap;
    const struct if_nameindex *sock_if;
    const struct if_nameindex *csap_if;

    int syms = 0;
    int pld_len;

    tapi_tcp_handler_t conn_hand;
    asn_value *tcp_template;

    rcf_rpc_server *sock_pco = NULL;

    char    *agt_a;
    uint8_t  flags;

    int socket = -1;
    int acc_sock = -1;
    int opt_val = 1;

    const struct sockaddr *csap_addr;
    const struct sockaddr *sock_addr;

    // uint8_t csap_mac[6] = {0x00, 0x05, 0x5D, 0x74, 0xAB, 0xB4};
    uint8_t csap_mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t sock_mac[6];
    size_t  sock_mac_len = sizeof(sock_mac);

    TEST_START; 

    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(sock_pco);
    TEST_GET_IF(sock_if);
    TEST_GET_IF(csap_if);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_addr);
    TEST_GET_INT_PARAM(pld_len);

    CHECK_RC(tapi_cfg_get_hwaddr(sock_pco->ta, sock_if->if_name, 
                                 sock_mac, &sock_mac_len));

    rc = asn_parse_value_text(
              "{ arg-sets { simple-for:{begin 0, end 10000} }," 
              "  pdus { tcp:{flags plain:8}, "
              "         ip4:{}, eth:{}}"
              "}",
              ndn_traffic_template, &tcp_template, &syms);
    if (rc != 0)
        TEST_FAIL("parse complex template failed %r syms %d", rc, syms); 

    
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



    rc = rpc_listen(sock_pco, socket, 1);
    if (rc != 0)
        TEST_FAIL("listen failed");

    rc = tapi_tcp_init_connection(agt_a, TAPI_TCP_CLIENT, 
                                  csap_addr, sock_addr, 
                                  csap_if->if_name, csap_mac, sock_mac,
                                  1000, &conn_hand);
    if (rc != 0)
        TEST_FAIL("init connection failed: %r", rc); 


    rc = tapi_tcp_wait_open(conn_hand, 2000);
    if (rc != 0)
        TEST_FAIL("open connection failed: %r", rc); 

    RING("connection inited, handle %d", conn_hand);

    acc_sock = rpc_accept(sock_pco, socket, NULL, NULL);

    rpc_close(sock_pco, socket);
    socket = acc_sock;
    acc_sock = -1;

    opt_val = 1;
    rpc_setsockopt(sock_pco, socket, RPC_SO_REUSEADDR, &opt_val);

    opt_val = 200000;
    rpc_setsockopt(sock_pco, socket, RPC_SO_RCVBUF, &opt_val);


    {
        char seqn_expr[100];
        tapi_tcp_pos_t seqn = tapi_tcp_next_seqn(conn_hand);
        uint64_t received = 0;

        sprintf(seqn_expr, "expr:(%u + ($0 * %d))", seqn, pld_len);

        rc = asn_write_string(tcp_template, seqn_expr,
                              "pdus.0.#tcp.seqn.#script");
        if (rc != 0)
            TEST_FAIL("write seqn script failed %X", rc);

        rc = asn_write_int32(tcp_template, pld_len, "payload.#length"); 
        if (rc != 0)
            TEST_FAIL("write arg len failed %X", rc);

        rc = tapi_tcp_send_template(conn_hand, tcp_template,
                                    RCF_MODE_NONBLOCKING);
        if (rc != 0)
            TEST_FAIL("send template failed %X", rc);

        rc = rpc_simple_receiver(sock_pco, socket, 0, &received);

        tapi_tcp_update_sent_seq(conn_hand, received);
    }

#define GOOD_CLOSE 0
#if GOOD_CLOSE
    rc = tapi_tcp_send_fin(conn_hand, 1000);
    if (rc != 0)
        TEST_FAIL("wait for ACK to our FIN failed: %r", rc); 
#endif


    rpc_close(sock_pco, socket);
    socket = -1;

#if GOOD_CLOSE
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
#endif

    TEST_SUCCESS;

cleanup:

    if (acc_sock > 0)
        rpc_close(sock_pco, acc_sock);

    if (socket > 0)
        rpc_close(sock_pco, socket);


    TEST_END;
}
