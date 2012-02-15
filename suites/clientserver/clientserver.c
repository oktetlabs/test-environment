/** @file
 * @brief Test Environment
 *
 * A test client-server application
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
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 * 
 * $Id: clientserver.c
 */

/** @page example client-server application
 *
 *  @objective Simple client-server application.
 *
 */
#define TE_TEST_NAME "clientserver"

#define TEST_START_VARS         TEST_START_ENV_VARS
#define TEST_START_SPECIFIC     TEST_START_ENV
#define TEST_END_SPECIFIC       TEST_END_ENV

#include "te_config.h"
#include "tapi_test.h"
#include "tapi_env.h"

#include "tapi_sockaddr.h"
#include "tapi_rpc.h"
#include "tapi_cfg.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_rpc_client_server.h"
#include "tapi_sniffer.h"

int
main(int argc, char *argv[])
{
    char                ta[32];
    int                 i;
    int                 buffer_len;
    char               *send_buffer;
    char               *recv_buffer;
    struct sockaddr    *srv_addr;
    struct sockaddr    *clnt_addr;
    int                 srvr_s      = -1;
    int                 clnt_s      = -1;
    rcf_rpc_server     *pco_srv     = NULL;
    rcf_rpc_server     *pco_clnt    = NULL;
    const char         *str         = "Hello!";
    size_t              len         = sizeof(ta);
    tapi_sniffer_id    *snif;

    CHECK_RC(tapi_cfg_net_all_assign_ip(AF_INET));

    TEST_START;

    TEST_GET_PCO(pco_srv);
    TEST_GET_PCO(pco_clnt);
    TEST_GET_ADDR(pco_srv, srv_addr);
    TEST_GET_ADDR(pco_clnt, clnt_addr);

    buffer_len = strlen(str) + 2;
    send_buffer = malloc(buffer_len);
    memcpy(send_buffer + 1, str, strlen(str));
    send_buffer[0] = 0;

    recv_buffer = malloc(buffer_len);

    if (rcf_get_ta_list(ta, &len) != 0)
        TEST_FAIL("rcf_get_ta_list() failed");

    snif = tapi_sniffer_add(ta, "lo", "newsniffer", "ip", FALSE);
    sleep(1);
    tapi_sniffer_mark(ta, NULL, "My first marker packet for all snifs. ");
    CHECK_RC(tapi_sniffer_stop(snif));

    CHECK_RC(tapi_sniffer_start(snif));
    tapi_sniffer_mark(NULL, snif, "My second marker packet.");
    sleep(1);
    tapi_sniffer_mark(NULL, snif, "My third marker packet.");
    CHECK_RC(tapi_sniffer_del(snif));

    /* rpc_dgram_connection (pco_srv, pco_clnt, RPC_IPPROTO_UDP, */
    rpc_stream_connection (pco_srv, pco_clnt, RPC_IPPROTO_TCP,
                          srv_addr, clnt_addr, &srvr_s, &clnt_s);

    for (i = 0; i < 5; i++)
    {
        send_buffer[0]++;
        rc = rpc_send(pco_clnt, clnt_s, send_buffer, buffer_len, 0);
        if (rc > 0)
            RING("packet transmission completed, rc %d", rc);
        rc = rpc_recv(pco_srv, srvr_s, recv_buffer, buffer_len, 0);
        if (rc > 0)
            RING("recv finished, packet num %d > %s", recv_buffer[0],
                 recv_buffer + 1);
        sleep(1);
    }
    TEST_SUCCESS;

cleanup:
    CLEANUP_RPC_CLOSE(pco_srv, srvr_s);
    CLEANUP_RPC_CLOSE(pco_clnt, clnt_s);

    if(recv_buffer)
        free(recv_buffer);
    if(send_buffer)
        free(send_buffer);

    TEST_END;
}
