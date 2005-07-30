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

#define TE_TEST_NAME    "ipstack/tcp_data"

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
#include "tapi_tcp.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"

uint8_t buffer[10000];

int
main(int argc, char *argv[])

{ 
    int syms = 0;
    asn_value *tcp_template;

    csap_handle_t csap = CSAP_INVALID_HANDLE;

    rcf_rpc_server *rpc_srv = NULL;

    char  ta[32];
    char *agt_a = ta;
    char *agt_b;

    size_t  len = sizeof(ta);

    int socket = -1;
    int acc_sock;
    int opt_val = 1;

    struct sockaddr_in sock_addr;
    struct sockaddr_in csap_addr;


    in_addr_t csap_ip_addr = inet_addr("192.168.72.28");
    in_addr_t sock_ip_addr = inet_addr("192.168.72.38");

    TEST_START; 

    {
        struct timeval now;
        gettimeofday(&now, NULL);
        srand(now.tv_usec);
    }

    rc = asn_parse_value_text(
              "{ pdus { tcp:{}, ip4:{}} }",
              ndn_traffic_template, &tcp_template, &syms);
    if (rc != 0)
        TEST_FAIL("parse template failed %X syms %d", rc, syms); 

    
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
    
    rcf_rpc_setlibname(rpc_srv, NULL); 

    memset(&sock_addr, 0, sizeof(sock_addr)); 

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = sock_ip_addr;
    sock_addr.sin_port = htons(rand_range(20000, 21000));

    memset(&csap_addr, 0, sizeof(csap_addr)); 

    csap_addr.sin_family = AF_INET;
    csap_addr.sin_addr.s_addr = csap_ip_addr;
    csap_addr.sin_port = htons(20000); 

    if ((socket = rpc_socket(rpc_srv, RPC_AF_INET, RPC_SOCK_STREAM, 
                                  RPC_IPPROTO_TCP)) < 0 ||
        rpc_srv->_errno != 0)
        TEST_FAIL("Calling of RPC socket() failed %x", rpc_srv->_errno);

    opt_val = 1;

    rc = rpc_bind(rpc_srv, socket, SA(&sock_addr), sizeof(sock_addr));
    if (rc != 0)
        TEST_FAIL("bind failed");



    rc = tapi_tcp_server_csap_create(agt_a, 0, csap_ip_addr, 
                                     csap_addr.sin_port, &csap);
    if (rc != 0)
        TEST_FAIL("server csap create failed: %X", rc); 

    rc = rpc_connect(rpc_srv, socket,
                     SA(&csap_addr), sizeof(csap_addr));
    if (rc != 0)
        TEST_FAIL("connect() 'call' failed: %X", rc); 


    rc = tapi_tcp_server_recv(agt_a, 0, csap, 1000, &acc_sock);
    if (rc != 0)
        TEST_FAIL("recv accepted socket failed: %X", rc); 


    RING("acc socket: %d", acc_sock);
    opt_val = 1;

    /*
     * Send data
     */

    rc = rpc_send(rpc_srv, socket, buffer, 200, 0);


#if 0
    rc = tapi_tcp_send_msg(conn_hand, buffer, 50, TAPI_TCP_AUTO, 0, 
                           TAPI_TCP_QUIET, 0, NULL, 0);
    if (rc != 0)
        TEST_FAIL("tapi_tcp_send_msg() failed: %X", rc); 

    rc = rpc_recv(rpc_srv, socket, buffer, sizeof(buffer), 0);
#endif


    TEST_SUCCESS;

cleanup:

    if (csap == CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(agt_a, 0, csap);

    if (socket > 0)
        rpc_close(rpc_srv, socket);

    if (rpc_srv && (rcf_rpc_server_destroy(rpc_srv) != 0))
    {
        WARN("Cannot delete dst RPC server\n");
    }
    


    TEST_END;
}
