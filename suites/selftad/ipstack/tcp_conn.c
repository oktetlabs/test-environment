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


int
main(int argc, char *argv[])
{
    rcf_rpc_server *rpc_srv;
    int  sid_a;
    char ta[32];
    char *agt_a = ta;
    char *agt_b;
    int  len = sizeof(ta);

    char path[1000];

    struct timeval to;

    int tcp_sock = -1;

    int timeout = 30;
    int rc_mod, rc_code;

    struct sockaddr_in srv_addr;


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
        if (rcf_ta_create_session(agt_a, &sid_a) != 0)
        {
            TEST_FAIL("rcf_ta_create_session failed");
            return 1;
        }
        INFO("Test: Created session: %d", sid_a); 
    }

    if ((rc = rcf_rpc_server_create(agt_b, "FIRST", &rpc_srv)) != 0)
    {
        TEST_FAIL("Cannot create server %x", rc);
    }
    rpc_srv->def_timeout = 5000;
    
    rpc_setlibname(rpc_srv, NULL); 
    
    if ((tcp_sock = rpc_socket(rpc_srv, RPC_AF_INET, RPC_SOCK_STREAM, 
                        RPC_IPPROTO_TCP)) < 0 || rpc_srv->_errno != 0)
        TEST_FAIL("Calling of RPC socket() failed %x", rpc_srv->_errno);


    TEST_SUCCESS;

cleanup:
    if (rpc_srv && (rcf_rpc_server_destroy(rpc_srv) != 0))
    {
        WARN("Cannot delete dst RPC server\n");
    }


    TEST_END;
}
