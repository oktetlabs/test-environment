/** @file
 * @brief Test Environment
 *
 * Simple IPv4 CSAP test
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

#define TE_TEST_NAME    "ipstack/ip4_simple"

#define TE_LOG_LEVEL 0xff

#include "ipstack-ts.h" 


#include "ndn_eth.h"
#include "ndn_ipstack.h"


#define USE_RPC_CHECK 0



static void 
user_pkt_handler(const tapi_ip4_packet_t *pkt, void *userdata)
{
    struct in_addr src;
    struct in_addr dst;

    src.s_addr = pkt->src_addr;
    dst.s_addr = pkt->dst_addr;
    RING("%s(): pkt from %s to %s with pld %d bytes caugth",
         __FUNCTION__, inet_ntoa(src), inet_ntoa(dst),
         pkt->pld_len);
    UNUSED(userdata);
}

int
main(int argc, char *argv[])
{
#if USE_RPC_CHECK
    rcf_rpc_server *srv_src = NULL, *srv_dst = NULL;
    int sock_src = -1;
    int sock_dst = -1;
    struct sockaddr_in srv_addr;
#endif
    rcf_rpc_server *pco = NULL;

    int  sid;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    size_t  len = sizeof(ta);



    TEST_START; 
    TEST_GET_PCO(pco);
    
    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %r", rc);

    INFO("Found first TA: %s; len %d", ta, len);

    agt_a = ta;
    if (strlen(ta) + 1 >= len) 
        TEST_FAIL("There is no second Test Agent");

    agt_b = ta + strlen(ta) + 1;
    INFO("Found second TA: %s", agt_b, len);

    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            TEST_FAIL("rcf_ta_create_session failed");
            return 1;
        }
        INFO("Test: Created session: %d", sid); 
    }

 
    do {
        in_addr_t my_addr = inet_addr("192.168.37.18");

        int csap;
        int num;
        int rc_mod;

#if USE_RPC_CHECK
        if ((sock_src = rpc_socket(srv_src, RPC_AF_INET, RPC_SOCK_STREAM, 
                            RPC_IPPROTO_TCP)) < 0 || srv_src->_errno != 0)
        {
            TEST_FAIL("Calling of RPC socket() failed %r", srv_src->_errno);
        }

        if ((sock_dst = rpc_socket(srv_dst, RPC_AF_INET, RPC_SOCK_STREAM, 
                            RPC_IPPROTO_TCP)) < 0 || srv_dst->_errno != 0)
        {
            TEST_FAIL("Calling of RPC socket() failed %r", srv_dst->_errno);
        }
#endif

        rc = tapi_ip4_eth_csap_create(ta, sid, "eth0", NULL, NULL,
                                      htonl(INADDR_ANY), htonl(INADDR_ANY),
                                      &csap);
        if ((rc_mod = TE_RC_GET_MODULE(rc)) != 0)
        {
            TEST_FAIL("CSAP create failed, rc from module %d is %r\n", 
                        rc_mod, TE_RC_GET_ERROR(rc));

        } 

        
        rc = tapi_ip4_eth_recv_start(ta, sid, csap, NULL, NULL,
                                     htonl(INADDR_ANY), my_addr,
                                     5000, 4, RCF_TRRECV_PACKETS);

        if (rc) break;

#if 1
        sleep(2);
        INFO ("try to get\n");
        rc = tapi_tad_trrecv_get(ta, sid, csap,
                                 tapi_ip4_eth_trrecv_cb_data(
                                     user_pkt_handler, NULL), &num);
        INFO("trrecv_get: %r num: %d\n", rc, num);
        if (rc) break;

#endif
        num = 2;
        INFO ("sleep %d secs before wait\n", num);
        sleep (num);

        INFO ("try to wait\n");
        rc = tapi_tad_trrecv_wait(ta, sid, csap, 
                                  tapi_ip4_eth_trrecv_cb_data(
                                      user_pkt_handler, NULL), &num);
        INFO("trrecv_wait: %r num: %d\n", rc, num);
        if (rc)
        {
            if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT && 
                TE_RC_GET_MODULE(rc) == TE_TAD_CSAP)
            {
                RING("wait for packets timedout");
                rc = 0;
            }
            else 
                TEST_FAIL("Unexpected error for trrecv_wait: %r", rc);
        }

        rc = rcf_ta_csap_destroy(ta, sid, csap);
        INFO("csap %d destroy: %r ", csap, rc); 

    } while(0);

    if (rc)
        TEST_FAIL("Failed, rc %r", rc);

    TEST_SUCCESS;

cleanup:
#if USE_RPC_CHECK
    if (sock_dst >= 0 && rpc_close(srv_dst, sock_dst) < 0)
    {
        int err = RPC_ERRNO(srv_dst);
        ERROR("close DST socket failed %X", err);
    }
    if (sock_src >= 0 && rpc_close(srv_src, sock_src) < 0)
    {
        int err = RPC_ERRNO(srv_src);
        ERROR("close SRC socket failed %X", err);
    }

    if (srv_dst != NULL && (rcf_rpc_server_destroy(srv_dst) != 0))
    {
        WARN("Cannot delete dst RPC server\n");
    }

    if (srv_src  != NULL&& (rcf_rpc_server_destroy(srv_src) != 0))
    {
        WARN("Cannot delete src RPC server\n");
    }
#endif

    TEST_END;
}
