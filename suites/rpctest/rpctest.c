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
#include <netdb.h>
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "rcf_rpc.h"
#include "conf_api.h"
#include "tapi_rpcsock.h"

const char *te_lgr_entity = "rpctest";

static int
handler(int s)
{
    exit(1);
}

int
main()
{
    char ta[32];
    int  len = sizeof(ta);
    
    rcf_rpc_server *srv, *srv1;
    
    int  res;
    char msg[32] = { 0, };
    int  s;
    int  rc;
    
    unsigned int        num;
    cfg_handle         *bebe;
    
    struct sockaddr_in addr;
    
    char buf[16];
    int  fromlen = sizeof(struct sockaddr_in);
    
    signal(SIGINT, handler);
    
    cfg_find_pattern("/bebe:*", &num, &bebe);
    
    if (rcf_get_ta_list(ta, &len) != 0)
    {
        printf("rcf_get_ta_list failed\n");
        return 1;
    }
    printf("Agent: %s\n", ta);
    rpc_setlibname(ta, "/lib/libc.so.6");
    
    if ((rc = rcf_rpc_server_create(ta, "FIRST", &srv)) != 0)
    {
        printf("Cannot create server %x\n", rc);
        return 1;
    }
    srv->def_timeout = 5000;
    
    if ((rc = rcf_rpc_server_thread_create(srv, "SECOND", &srv1)) != 0)
    {
        printf("Cannot create server %x\n", rc);
        return 1;
    }
    srv1->def_timeout = 5000;

    if ((s = rpc_socket(srv, RPC_AF_INET, RPC_SOCK_DGRAM, 
                        RPC_IPPROTO_UDP)) < 0 || srv->_errno != 0)
    {
        printf("Calling of RPC socket() failed %x\n", srv->_errno);
        return 1;
    }

    if ((s = rpc_socket(srv1, RPC_AF_INET, RPC_SOCK_DGRAM, 
                        RPC_IPPROTO_UDP)) < 0 || srv1->_errno != 0)
    {
        printf("Calling of RPC socket() failed %x\n", srv1->_errno);
        return 1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9002);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (rpc_bind(srv, s, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        srv->_errno != 0)
    {
        printf("Calling of RPC bind() failed %x %d\n", srv->_errno,
               srv->stat);
        return 1;
    }
    
    memset(&addr, 0, sizeof(addr));
    srv->op = RCF_RPC_CALL;
    if (rpc_recvfrom(srv, s, buf, 16, 0, &addr, &fromlen) < 0 ||
        srv->_errno != 0)
    {
        printf("Calling of RPC recvfrom() failed %x\n", srv->_errno);
        return 1;
    }
    printf("Calling wait\n");
    srv->op = RCF_RPC_WAIT;
    if (rpc_recvfrom(srv, s, buf, 16, 0, &addr, &fromlen) < 0 ||
        srv->_errno != 0)
    {
        printf("Calling of RPC recvfrom() failed %x\n", srv->_errno);
        return 1;
    } 

    printf("Received: %s from %s\n", buf, inet_ntoa(addr.sin_addr));  
    
    if (rpc_close(srv, s) < 0 || srv->_errno != 0)
    {
        printf("Calling of RPC close() failed %x\n", srv->_errno);
        return 1;
    }
    
    if (rcf_rpc_server_destroy(srv1) != 0)
    {
        printf("Cannot delete server\n");
        return 1;
    }

    if (rcf_rpc_server_destroy(srv) != 0)
    {
        printf("Cannot delete server\n");
        return 1;
    }
    return 0;
}
