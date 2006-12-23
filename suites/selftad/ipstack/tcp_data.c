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
#include "tapi_socket.h"

#include "te_bufs.h"

#if 0
#include "ndn_eth.h"
#include "ndn_ipstack.h"
#endif

uint8_t tx_buffer[0x10000];
uint8_t rx_buffer[0x10000];


int
main(int argc, char *argv[]) 
{ 
    tapi_env_host *host_csap = NULL;
    csap_handle_t csap = CSAP_INVALID_HANDLE;
    csap_handle_t acc_csap = CSAP_INVALID_HANDLE;

    rcf_rpc_server *sock_pco = NULL;

    int    socket = -1;
    int    acc_sock;
    size_t len;

    const struct sockaddr *csap_addr;
    const struct sockaddr *sock_addr;


    TEST_START; 

    TEST_GET_HOST(host_csap);
    TEST_GET_PCO(sock_pco);
    TEST_GET_ADDR(sock_addr);
    TEST_GET_ADDR(csap_addr);


    if ((socket = rpc_socket(sock_pco, RPC_AF_INET, RPC_SOCK_STREAM, 
                                  RPC_IPPROTO_TCP)) < 0 ||
        sock_pco->_errno != 0)
        TEST_FAIL("Calling of RPC socket() failed %r", sock_pco->_errno);


    rc = rpc_bind(sock_pco, socket, sock_addr);
    if (rc != 0)
        TEST_FAIL("bind failed");


    rc = tapi_tcp_server_csap_create(host_csap->ta, 0, csap_addr, &csap);
    if (rc != 0)
        TEST_FAIL("server csap create failed: %r", rc); 
    rc = rpc_connect(sock_pco, socket, csap_addr);
    if (rc != 0)
        TEST_FAIL("connect() 'call' failed: %r", rc); 

    rc = tapi_tcp_server_recv(host_csap->ta, 0, csap, 1000, &acc_sock);
    if (rc != 0)
        TEST_FAIL("recv accepted socket failed: %r", rc); 


    RING("acc socket: %d", acc_sock);

    rc = tapi_tcp_socket_csap_create(host_csap->ta, 0, acc_sock, &acc_csap);
    if (rc != 0)
        TEST_FAIL("create CSAP over accepted socket failed, %r", rc);

    /*
     * Send data
     */

    memset(tx_buffer, 0, sizeof(tx_buffer));
    len = 20000;

    te_fill_buf(tx_buffer, len);
    INFO("+++++++++++ Prepared data: %Tm", tx_buffer, len);
    rc = rpc_send(sock_pco, socket, tx_buffer, len, 0); 
    RING("%d bytes sent from RPC socket", rc);

    memset(rx_buffer, 0, sizeof(rx_buffer));
    rc = tapi_socket_recv(host_csap->ta, 0, acc_csap, 2000, 
                              CSAP_INVALID_HANDLE, TRUE, 
                              rx_buffer, &len);
    if (rc != 0)
        TEST_FAIL("recv on CSAP failed: %r", rc); 

    INFO("+++++++++++ Received data: %Tm", rx_buffer, len);

    rc = memcmp(tx_buffer, rx_buffer, len);
    if (rc != 0)
        TEST_FAIL("RPC->CSAP: sent and received data differ, rc = %d", rc);

    len = 20000;

    te_fill_buf(tx_buffer, len);
    INFO("+++++++++++ Prepared data: %Tm", tx_buffer, len);
    rc = tapi_socket_send(host_csap->ta, 0, acc_csap, 
                              tx_buffer, len);
    if (rc != 0)
        TEST_FAIL("recv on CSAP failed: %r", rc); 


    memset(rx_buffer, 0, sizeof(rx_buffer));
    rc = rpc_recv(sock_pco, socket, rx_buffer, sizeof(rx_buffer), 0);
    if (rc != (int)len)
        TEST_FAIL("CSAP->RPC: len received %d, expected %d", rc, len);

    rc = memcmp(tx_buffer, rx_buffer, len);
    if (rc != 0)
        TEST_FAIL("CSAP->RPC: sent and received data differ, rc = %d", rc);

    rpc_close(sock_pco, socket);
    socket = -1;

    memset(rx_buffer, 0, sizeof(rx_buffer));
    rc = tapi_socket_recv(host_csap->ta, 0, acc_csap, 2000, 
                              CSAP_INVALID_HANDLE, TRUE, 
                              rx_buffer, &len);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ETADENDOFDATA)
            RING("CSAP detected that connection was closed");
        else 
            TEST_FAIL("recv on CSAP failed: %r", rc); 
    }
    else
        TEST_FAIL("recv on TCP CSAP have detect that connection closed"); 

    TEST_SUCCESS;

cleanup:

    if (host_csap != NULL)
        rcf_ta_csap_destroy(host_csap->ta, 0, csap);

    if (host_csap != NULL)
        rcf_ta_csap_destroy(host_csap->ta, 0, acc_csap);

    if (socket > 0)
        rpc_close(sock_pco, socket);


    TEST_END;
}
