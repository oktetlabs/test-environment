/** @file
 * @brief Test Environment
 *
 * Simple RPC test
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <assert.h>
#include <netdb.h>

#include "te_config.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "rcf_rpc.h"
#include "conf_api.h"
#include "tapi_rpc.h"
#include "tapi_test.h"
#include "conf_api.h"
#include "tarpc.h"
#include "tapi_rpc.h"

#define TE_TEST_NAME    "rpctest"

int
main(int argc, char **argv)
{
    char ta[32];
    size_t len = sizeof(ta);
    int  sid;
    
    rcf_rpc_server *srv1 = NULL, *srv2 = NULL, *srv3 = NULL;
    
    rcf_rpc_server *dup;
    
    cfg_handle handle;
    
    int s1, s2, s3;

    tarpc_socket_in  in;
    tarpc_socket_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    TEST_START;
    
    if (rcf_get_ta_list(ta, &len) != 0)
        TEST_FAIL("rcf_get_ta_list() failed");

    if ((rc = rcf_rpc_server_create(ta, "Main", &srv1)) != 0)
        TEST_FAIL("Cannot create server 0x%X", rc);
    
    if ((rc = rcf_rpc_server_fork(srv1, "Forked", &srv2)) != 0)
        TEST_FAIL("Cannot fork server 0x%X", rc);
    
    if ((rc = rcf_rpc_server_thread_create(srv2, "Thread", &srv3)) != 0)
        TEST_FAIL("Cannot create threadserver 0x%X", rc);
        
    s1 = rpc_socket(srv1, RPC_AF_INET, RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);

    s2 = rpc_socket(srv2, RPC_AF_INET, RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);

    s3 = rpc_socket(srv3, RPC_AF_INET, RPC_SOCK_DGRAM, RPC_IPPROTO_UDP);
    
    rpc_close(srv1, s1);
    rpc_close(srv2, s2);
    rpc_close(srv3, s3);
    
    TEST_SUCCESS;
    
cleanup:    
    if (srv3 != NULL && rcf_rpc_server_destroy(srv3) != 0)
        ERROR("Cannot delete thread server");

    if (srv2 != NULL && rcf_rpc_server_destroy(srv2) != 0)
        ERROR("Cannot delete forked server");

    if (srv1 != NULL && rcf_rpc_server_destroy(srv1) != 0)
        ERROR("Cannot delete main server");

    TEST_END;
}
