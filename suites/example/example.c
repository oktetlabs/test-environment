/** @file
 * @brief Test Environment
 *
 * A test example
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 * 
 * $Id$
 */

/** @page example An example test
 *
 *  @objective Example smells example whether it's called example or not.
 *
 */

#include "te_config.h"

#define ETHER_ADDR_LEN 6
#define TE_TEST_NAME "example"

#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "tapi_test.h"
#include "tapi_tcp.h"
#include "tapi_iscsi.h"


#define NET_DUMP INFO

#define INCOMMING_PACKET_TIMEOUT 2000
#define OUTGOING_PACKET_TIMEOUT 2000

#define GET_INCOMMING_PACKET_FROM_NET(agent_, conn_, \
                                      buffer_, len_) \
    do {                                                        \
        memset((buffer_), 0, (len_));                           \
        rc = tapi_tcp_buffer_recv((agent_), 0, (conn_),         \
                                  INCOMMING_PACKET_TIMEOUT,     \
                                  CSAP_INVALID_HANDLE, FALSE,   \
                                  (buffer_), &(len_));          \
        if (rc != 0)                                            \
            TEST_FAIL("recv from NET failed: %r", rc);          \
        NET_DUMP("received %d bytes from NET", len);            \
    } while(0)

#define SEND_INCOMMING_PACKET_TO_TARGET(agent_, target_csap_, \
                                        buffer_, len_)         \
    do {                                                            \
        rc = tapi_iscsi_send_pkt((agent_), 0, (target_csap_), NULL, \
                                 (buffer_), (len_));                \
        if (rc != 0)                                                \
            TEST_FAIL("send to TARGET failed: %r", rc);             \
    } while(0)

#define GET_OUTGOING_PACKET_FROM_TARGET(agent_, target_csap_, \
                                        buffer_, len_) \
    do {                                                      \
        memset((buffer_), 0, (len_));                         \
        rc = tapi_iscsi_recv_pkt((agent_), 0, (target_csap_), \
                                 OUTGOING_PACKET_TIMEOUT,     \
                                 CSAP_INVALID_HANDLE, NULL,   \
                                 (buffer_), &(len_));         \
        if (rc != 0)                                          \
            TEST_FAIL("recv from TARGET failed: %r", rc);     \
        NET_DUMP("+++ data from TARGET to NET: %tm",          \
                 (buffer_), (len_));                          \
    }while(0)

#define SEND_OUTGOING_PACKET_TO_NET(agent_, target_csap_, \
                                    buffer_, len_)         \
    do {                                                         \
        rc = tapi_tcp_buffer_send((agent_), 0, (target_csap_),   \
                                 (buffer_), (len_));             \
        if (rc != 0)                                             \
            TEST_FAIL("send to NET failed: %r", rc);             \
    } while(0)


int
main(int argc, char *argv[])
{
    char ta[256];
    uint8_t rx_buffer[10000];
    uint8_t tx_buffer[10000];
    int len;
    
    int acc_sock;
    csap_handle_t iscsi_csap = CSAP_INVALID_HANDLE;
    csap_handle_t listen_csap = CSAP_INVALID_HANDLE;
    csap_handle_t acc_csap   = CSAP_INVALID_HANDLE;

    TEST_START;

    len = sizeof(ta);
    CHECK_RC(rcf_get_ta_list(ta, &len));
    RING("Agent is %s", ta);

    CHECK_RC(tapi_tcp_server_csap_create(ta, 0, 
                                         0,
                                         htons(3260),
                                         &listen_csap));

    CHECK_RC(tapi_tcp_server_recv(ta, 0, listen_csap, 100000, &acc_sock));

    RING("acc socket: %d", acc_sock);

    CHECK_RC(tapi_tcp_socket_csap_create(ta, 0, acc_sock, &acc_csap));

    CHECK_RC(tapi_iscsi_csap_create(ta, 0, &iscsi_csap));

    RING("Starting the Login Phase");
    do {
        /* I->T */
        len = sizeof(rx_buffer);
        GET_INCOMMING_PACKET_FROM_NET(ta, acc_csap, rx_buffer, len);

        SEND_INCOMMING_PACKET_TO_TARGET(ta, iscsi_csap, rx_buffer, len);
        /* T->I */
        len = sizeof(tx_buffer);
        GET_OUTGOING_PACKET_FROM_TARGET(ta, iscsi_csap, tx_buffer, len);

        SEND_OUTGOING_PACKET_TO_NET(ta, acc_csap, tx_buffer, len);
        break; // CHECK_LOGIN_PHASE_END(tx_buffer);
    } while (1);


    TEST_SUCCESS;

cleanup:
    if (iscsi_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(ta, 0, iscsi_csap);

    if (acc_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(ta, 0, acc_csap);
    
    if (listen_csap != CSAP_INVALID_HANDLE)
        rcf_ta_csap_destroy(ta, 0, listen_csap);

    TEST_END;
}
