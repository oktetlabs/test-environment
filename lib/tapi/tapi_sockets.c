
/** @file
 * @brief Functions to opearate with socket.
 *
 * Implementation of test API for working with socket.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id:
 */

#define TE_LGR_USER      "TAPI Socket"

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "te_errno.h"
#include "te_stdint.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_rpc_socket.h"

#include "tapi_sockets.h"

/* See description in tapi_sockets.h */
rpc_tcp_state
tapi_get_tcp_sock_state(struct rcf_rpc_server *pco,
                        int s)
{
    struct rpc_tcp_info tcpi;

    memset(&tcpi, 0, sizeof(struct tcp_info));
    
    rpc_getsockopt_gen(pco, s, rpc_sockopt2level(RPC_TCP_INFO),
                       RPC_TCP_INFO, &tcpi, NULL, NULL, 0);

    return tcpi.tcpi_state;
}

/* See description in tapi_sockets.h */
ssize_t
tapi_sock_read_data(rcf_rpc_server *rpcs,
                    int s,
                    te_dbuf *read_data)
{
#define READ_LEN  1024
    int   rc;
    char  data[READ_LEN];

    ssize_t  total_len = 0;

    while (TRUE)
    {
        RPC_AWAIT_ERROR(rpcs);
        rc = rpc_recv(rpcs, s, data, READ_LEN,
                      RPC_MSG_DONTWAIT);
        if (rc < 0)
        {
            if (RPC_ERRNO(rpcs) != RPC_EAGAIN)
            {
                ERROR("recv() failed with unexpected errno %r",
                      RPC_ERRNO(rpcs));
                return -1;
            }
            else
            {
                break;
            }
        }

        te_dbuf_append(read_data, data, rc);
        total_len += rc;
    }

    return total_len;
}
