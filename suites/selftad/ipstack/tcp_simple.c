/** @file
 * @brief Test Environment
 *
 * Simple UDP CSAP test
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

#define TE_TEST_NAME    "ipstack/tcp_simple"

#define TE_LOG_LEVEL 0xff

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
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


int
main(int argc, char *argv[])
{
    rcf_rpc_server *srv_src, *srv_dst;
    int  sid;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    int  len = sizeof(ta);

    char path[1000];


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
 
    do {
        struct timeval to;
        asn_value *csap_spec, *pattern;

        int sock_src;
        int sock_dst;

        int csap;
        int num;
        int timeout = 30;
        int rc_mod, rc_code;

        struct sockaddr_in srv_addr;

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

        rc = asn_parse_value_text("{tcp:{local-port plain:0}, "
                                  " ip4:{max-packet-size plain:100000},"
                                  " eth:{device-id plain:\"eth0\"}}", 
                                  ndn_csap_spec, &csap_spec, &num);
        VERB("CSAP spec parse rc %X, syms %d", rc, num);
        if (rc)
            TEST_FAIL("ASN error"); 

        strcpy(path, "/tmp/te_tcp_csap_create.XXXXXX"); 
        mkstemp(path); 
        VERB("file name for csap spec: '%s'", path);

        asn_save_to_file(csap_spec, path);


        rc = rcf_ta_csap_create(ta, sid, "tcp.ip4.eth", path, &csap); 
        INFO("csap_create rc: %d, csap id %d\n", rc, csap); 
        if ((rc_mod = TE_RC_GET_MODULE(rc)) != 0)
        {
            INFO ("rc from module %d is 0x%x\n", 
                        rc_mod, TE_RC_GET_ERROR(rc));
        } 

        strcpy(path, "/tmp/te_tcp_pattern.XXXXXX"); 
        mkstemp(path); 
        VERB("file name for tcp pattern: '%s'", path);

        num = 0; 
        rc = asn_parse_value_text(
"{{ action function:\"tadf_forw_packet:a\","
" pdus {tcp:{dst-port plain:6100},"
" ip4:{protocol plain:6}, eth:{eth-type plain:2048}}}}",
                                  ndn_traffic_pattern, &pattern, &num);
        VERB("Pattern parse rc %X, syms %d", rc, num);
        if (rc)
            TEST_FAIL("ASN error"); 

        asn_save_to_file(pattern, path);
#if 1
        rc = rcf_ta_trrecv_start(ta, sid, csap, path, 0, NULL, NULL, 0);
        INFO("trrecv_start: 0x%X \n", rc);
        if (rc) break;

#if 1
        sleep(5);
        INFO ("try to get\n");
        rc = rcf_ta_trrecv_get(ta, sid, csap, &num);
        INFO("trrecv_get: 0x%X num: %d\n", rc, num);
        if (rc) break;

#endif
        num = 10;
        INFO ("sleep %d secs before stop\n", num);
        sleep (num);

        INFO ("try to stop\n");
        rc = rcf_ta_trrecv_stop(ta, sid, csap, &num);
        INFO("trrecv_stop: 0x%X num: %d\n", rc, num);

#endif
        rc = rcf_ta_csap_destroy(ta, sid, csap);
        INFO("csap %d destroy: 0x%X ", csap, rc); 

    } while(0);

    if (rc)
        TEST_FAIL("Failed, rc %X", rc);

    TEST_SUCCESS;

cleanup:
    if (srv_dst && (rcf_rpc_server_destroy(srv_dst) != 0))
    {
        WARN("Cannot delete dst RPC server\n");
    }

    if (srv_src && (rcf_rpc_server_destroy(srv_src) != 0))
    {
        WARN("Cannot delete src RPC server\n");
    }

    TEST_END;
}
