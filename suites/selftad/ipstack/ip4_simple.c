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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

#include "rcf_rpc.h"
#include "conf_api.h"
#include "tapi_rpcsock.h"

#include "tapi_test.h"
#include "tapi_ipstack.h"

#include "ndn_eth.h"
#include "ndn_ipstack.h"


#define USE_RPC_CHECK 0

#define USE_TAPI 1

int
main(int argc, char *argv[])
{
#if USE_RPC_CHECK
    rcf_rpc_server *srv_src, *srv_dst;
#endif
    int  sid;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    unsigned  len = sizeof(ta);

#if !(USE_TAPI)
    char path[1000];
#endif


    TEST_START; 
    
    if ((rc = rcf_get_ta_list(ta, &len)) != 0)
        TEST_FAIL("rcf_get_ta_list failed: %X", rc);

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

#if USE_RPC_CHECK
    if ((rc = rcf_rpc_server_create(agt_b, "FIRST", &srv_src)) != 0)
    {
        TEST_FAIL("Cannot create server %x", rc);
    }
    srv_src->def_timeout = 5000;
    
    rpc_setlibname(srv_src, NULL);


    if ((rc = rcf_rpc_server_create(agt_a, "SECOND", &srv_dst)) != 0)
    {
        TEST_FAIL("Cannot create server %x", rc);
    }
    srv_dst->def_timeout = 5000;
    
    rpc_setlibname(srv_dst, NULL);
#endif
 
    do {
#if !(USE_TAPI)
        struct timeval to;
        asn_value *csap_spec, *pattern;

        int rc_code;
#else
        in_addr_t my_addr = inet_addr("195.19.254.40");
#endif

        int csap;
        int num;
        int rc_mod;
#if USE_RPC_CHECK 
        int sock_src;
        int sock_dst;
        struct sockaddr_in srv_addr;
#endif

#if USE_RPC_CHECK
        if ((sock_src = rpc_socket(srv_src, RPC_AF_INET, RPC_SOCK_STREAM, 
                            RPC_IPPROTO_TCP)) < 0 || srv_src->_errno != 0)
        {
            TEST_FAIL("Calling of RPC socket() failed %x", srv_src->_errno);
        }

        if ((sock_dst = rpc_socket(srv_dst, RPC_AF_INET, RPC_SOCK_STREAM, 
                            RPC_IPPROTO_TCP)) < 0 || srv_dst->_errno != 0)
        {
            TEST_FAIL("Calling of RPC socket() failed %x", srv_dst->_errno);
        }
#endif

#if USE_TAPI
        rc = tapi_ip4_eth_csap_create(ta, sid, "eth0", NULL, NULL, &csap);
#else
        rc = asn_parse_value_text("{ ip4:{max-packet-size plain:100000},"
                                  " eth:{device-id plain:\"eth0\"}}", 
                                  ndn_csap_spec, &csap_spec, &num);
        VERB("CSAP spec parse rc %X, syms %d", rc, num);
        if (rc)
            TEST_FAIL("ASN error"); 

        strcpy(path, "/tmp/te_tcp_csap_create.XXXXXX"); 
        mkstemp(path); 
        VERB("file name for csap spec: '%s'", path);

        asn_save_to_file(csap_spec, path);


        rc = rcf_ta_csap_create(ta, sid, "ip4.eth", path, &csap); 
#endif
        if ((rc_mod = TE_RC_GET_MODULE(rc)) != 0)
        {
            TEST_FAIL("CSAP create failed, rc from module %d is 0x%x\n", 
                        rc_mod, TE_RC_GET_ERROR(rc));

        } 

#if USE_TAPI
        
        rc = tapi_ip4_eth_recv_start(ta, sid, csap, NULL, 
                                     (uint8_t*)&my_addr, 5000, 4);
#else
        strcpy(path, "/tmp/te_ip4_pattern.XXXXXX"); 
        mkstemp(path); 
        VERB("file name for tcp pattern: '%s'", path);

        num = 0; 
        rc = asn_parse_value_text(
                        "{{pdus { ip4:{dst-addr plain:'c3 13 fe 28'H}, "
                        "eth:{eth-type plain:2048}}}}",
                                  ndn_traffic_pattern, &pattern, &num);
        VERB("Pattern parse rc %X, syms %d", rc, num);
        if (rc)
            TEST_FAIL("ASN error"); 

        asn_save_to_file(pattern, path);
        rc = rcf_ta_trrecv_start(ta, sid, csap, path, 0, NULL, NULL, 0);
        INFO("trrecv_start: 0x%X \n", rc);
#endif /* USE_TAPI */
        if (rc) break;

#if 1
        sleep(2);
        INFO ("try to get\n");
        rc = rcf_ta_trrecv_get(ta, sid, csap, &num);
        INFO("trrecv_get: 0x%X num: %d\n", rc, num);
        if (rc) break;

#endif
        num = 2;
        INFO ("sleep %d secs before wait\n", num);
        sleep (num);

        INFO ("try to wait\n");
        rc = rcf_ta_trrecv_wait(ta, sid, csap, &num);
        INFO("trrecv_wait: 0x%X num: %d\n", rc, num);
        if (rc)
        {
            if (TE_RC_GET_ERROR(rc) == ETIMEDOUT && 
                TE_RC_GET_MODULE(rc) == TE_TAD_CSAP)
            {
                RING("wait for packets timedout");
                rc = 0;
            }
            else 
                TEST_FAIL("Unexpected error for trrecv_wait: %X", rc);
        }

        rc = rcf_ta_csap_destroy(ta, sid, csap);
        INFO("csap %d destroy: 0x%X ", csap, rc); 

    } while(0);

    if (rc)
        TEST_FAIL("Failed, rc %X", rc);

    TEST_SUCCESS;

cleanup:
#if USE_RPC_CHECK
    if (srv_dst && (rcf_rpc_server_destroy(srv_dst) != 0))
    {
        WARN("Cannot delete dst RPC server\n");
    }

    if (srv_src && (rcf_rpc_server_destroy(srv_src) != 0))
    {
        WARN("Cannot delete src RPC server\n");
    }
#endif

    TEST_END;
}
