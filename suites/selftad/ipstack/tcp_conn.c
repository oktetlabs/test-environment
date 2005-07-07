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


#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "conf_api.h"
#include "tapi_rpc.h"

#include "tapi_test.h"
#include "tapi_ip.h"
#include "tapi_udp.h"
#include "tapi_tcp.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"

uint8_t buffer[10000];

int
main(int argc, char *argv[])

{ 
    tapi_tcp_handler_t conn_hand;

    rcf_rpc_server *rpc_srv = NULL;

    char  ta[32];
    char *agt_a = ta;
    char *agt_b;

    size_t  len = sizeof(ta);
    uint8_t flags;

    int socket = -1;
    int acc_sock = -1;
    int opt_val = 1;

    te_bool is_server;
    te_bool init_close;

    struct sockaddr_in sock_addr;
    struct sockaddr_in csap_addr;

    // uint8_t csap_mac[6] = {0x00, 0x05, 0x5D, 0x74, 0xAB, 0xB4};
    uint8_t csap_mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t sock_mac[6] = {0x00, 0x0D, 0x88, 0x4F, 0x55, 0xAF};

    in_addr_t csap_ip_addr = inet_addr("192.168.72.28");
    in_addr_t sock_ip_addr = inet_addr("192.168.72.38");

    TEST_START; 

    TEST_GET_BOOL_PARAM(is_server);
    TEST_GET_BOOL_PARAM(init_close);

    {
        struct timeval now;
        gettimeofday(&now, NULL);
        srand(now.tv_usec);
    }
    
    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %X", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len) 
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;

    INFO("Found second TA: %s", agt_b, len);

    if ((rc = rcf_rpc_server_create(agt_b, "FIRST", &rpc_srv)) != 0)
        TEST_FAIL("Cannot create server %x", rc);

    rpc_srv->def_timeout = 5000;
    
    rpc_setlibname(rpc_srv, NULL); 

    memset(&sock_addr, 0, sizeof(sock_addr)); 

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = sock_ip_addr;
    sock_addr.sin_port = htons(rand_range(20000, 21000));

    memset(&csap_addr, 0, sizeof(csap_addr)); 

    csap_addr.sin_family = AF_INET;
    csap_addr.sin_addr.s_addr = csap_ip_addr;
    csap_addr.sin_port = htons(20000); 

#if 1    
    if ((socket = rpc_socket(rpc_srv, RPC_AF_INET, RPC_SOCK_STREAM, 
                                  RPC_IPPROTO_TCP)) < 0 ||
        rpc_srv->_errno != 0)
        TEST_FAIL("Calling of RPC socket() failed %x", rpc_srv->_errno);

    opt_val = 1;
    rpc_setsockopt(rpc_srv, socket, RPC_SOL_SOCKET, RPC_SO_REUSEADDR,
                   &opt_val, sizeof(opt_val));


    rc = rpc_bind(rpc_srv, socket, SA(&sock_addr), sizeof(sock_addr));
    if (rc != 0)
        TEST_FAIL("bind failed");



    if (!is_server) /*Csap is server, socket is client */
    {
        rc = rpc_listen(rpc_srv, socket, 1);
        if (rc != 0)
            TEST_FAIL("listen failed");
    }

    rc = tapi_tcp_init_connection(agt_a,
                                  is_server ? TAPI_TCP_SERVER :
                                              TAPI_TCP_CLIENT, 
                                  SA(&csap_addr), SA(&sock_addr), 
                                  "eth2", csap_mac, sock_mac,
                                  1000, &conn_hand);
    if (rc != 0)
        TEST_FAIL("init connection failed: %X", rc); 

    if (is_server)
    {
        rpc_srv->op = RCF_RPC_CALL;
        rc = rpc_connect(rpc_srv, socket,
                         SA(&csap_addr), sizeof(csap_addr));
        if (rc != 0)
            TEST_FAIL("connect() 'call' failed: %X", rc); 
    } 

    rc = tapi_tcp_wait_open(conn_hand, 2000);
    if (rc != 0)
        TEST_FAIL("open connection failed: %X", rc); 

    RING("connection inited, handle %d", conn_hand);

    if (is_server)
    {
        rpc_srv->op = RCF_RPC_WAIT;
        rc = rpc_connect(rpc_srv, socket,
                         SA(&csap_addr), sizeof(csap_addr));
        if (rc != 0)
            TEST_FAIL("connect() 'wait' failed: %X", rc); 
    }
    else
    { 
        acc_sock = rpc_accept(rpc_srv, socket, NULL, NULL);

        rpc_close(rpc_srv, socket);
        socket = acc_sock;
        acc_sock = -1;
    } 

    opt_val = 1;
    rpc_setsockopt(rpc_srv, socket, RPC_SOL_SOCKET, RPC_SO_REUSEADDR,
                   &opt_val, sizeof(opt_val));

    /*
     * Send data
     */

    rc = rpc_send(rpc_srv, socket, buffer, 200, 0);

    {
        tapi_tcp_pos_t seq, ack;
        len = sizeof(buffer);
        rc = tapi_tcp_recv_msg(conn_hand, 2000, TAPI_TCP_AUTO,
                               buffer, &len, &seq, &ack, &flags);
        if (rc != 0)
            TEST_FAIL("recv_msg() failed: %X", rc); 

        RING("msg received: %d bytes, seq %u", len, seq);
    }
#if 0
    rc = tapi_tcp_send_msg(conn_hand, buffer, 50, TAPI_TCP_AUTO, 0, 
                           TAPI_TCP_QUIET, 0, NULL, 0);
    if (rc != 0)
        TEST_FAIL("recv_msg() failed: %X", rc); 

    rc = rpc_recv(rpc_srv, socket, buffer, sizeof(buffer), 0);
#endif

    /*
     * Closing connection
     */
    if (!init_close)
    {
        rpc_close(rpc_srv, socket);
        socket = -1;
    }

    rc = tapi_tcp_send_fin(conn_hand, 1000);
    if (rc != 0)
        TEST_FAIL("wait for ACK to our FIN failed: %X", rc); 

    if (init_close)
    {
        rpc_close(rpc_srv, socket);
        socket = -1;
    }

    do {
        rc = tapi_tcp_recv_msg(conn_hand, 2000, TAPI_TCP_AUTO,
                               NULL, 0, NULL, NULL, &flags);
        if (rc != 0)
            TEST_FAIL("close connection failed: %X", rc); 

        if (flags & TCP_FIN_FLAG)
        {
            RING("FIN received!");
        }
    } while (!(flags & TCP_FIN_FLAG) && !(flags & TCP_RST_FLAG));

#endif
    TEST_SUCCESS;

cleanup:

    if (acc_sock > 0)
        rpc_close(rpc_srv, acc_sock);

    if (socket > 0)
        rpc_close(rpc_srv, socket);

    if (rpc_srv && (rcf_rpc_server_destroy(rpc_srv) != 0))
    {
        WARN("Cannot delete dst RPC server\n");
    }
    


    TEST_END;
}
