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
#define TEST_START_VARS unsigned mtu; \
            char ta[32]; \
            char eth0_oid[128]; \
            int num_interfaces; \
            cfg_handle *interfaces; \
            struct sockaddr *addr; \
            char *dirname; \
            cfg_val_type type = CVT_ADDRESS; \
            int flag; \
            int len; \
            int shell_tid; \
            struct sockaddr_in host_addr; \
            uint8_t mac[ETHER_ADDR_LEN + 1];

#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_sockaddr.h"
#include "rcf_api.h"
#include "rcf_rpc.h"
#include "tapi_rpc.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpcsock_macros.h"

int
main(int argc, char *argv[])
{
    rcf_rpc_server *srv = NULL;
    int rc1;
    FILE *f;
    TEST_START;
    
    len = sizeof(ta);
    CHECK_RC(rcf_get_ta_list(ta, &len));
    INFO("Agent is %s", ta);

    CHECK_RC(rcf_rpc_server_create(ta, "test", &srv));
    CHECK_RC(rpc_setlibname(srv, strdup("/home/artem/src/testnut/build/libtestnut.so")));
    RING("kuku");
    RPC_DUP(rc1, srv, 1);
    RPC_DUP(rc1, srv, 2);
    RPC_DUP(rc1, srv, 3);

    TEST_SUCCESS;

cleanup:
    if (srv != NULL)
        CLEANUP_CHECK_RC(rcf_rpc_server_destroy(srv));
    TEST_END;
}
