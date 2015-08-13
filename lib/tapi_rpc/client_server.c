/** @file
 * @brief Test API - RPC to create client-server connections
 *
 * Routines to create client-server connections.
 *
 * Actually we can't say that in @c SOCK_DGRAM socket type there are
 * clients and servers, but we will call them so in case each socket
 * is connected to its peer, so that send() and recv() operations lead
 * to sending and receiving data to/from the particular remote peer.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id: tapi_rpc.h 22478 2006-01-06 11:58:27Z arybchik $
 */

/** User name of the library to be used for logging */
#define TE_LGR_USER "Generic Connection LIB"

#include "te_config.h"

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

#include "logger_api.h"
#include "te_sockaddr.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_client_server.h"
#include "tapi_rpcsock_macros.h"

#include "tapi_test.h"


/** Default backlog of the TCP server */
#define TAPI_RPC_CLIENT_SERVER_BACKLOG_DEF  1


/* See the description in tapi_rpc_client_server.h */
int
rpc_create_and_bind_socket(rcf_rpc_server    *rpc,
                           rpc_socket_type    sock_type,
                           rpc_socket_proto   proto,
                           te_bool            wild,
                           te_bool            set_reuse_addr,
                           const struct sockaddr   *addr)
{
    struct sockaddr_storage bind_addr;
    int                     sockd;
    int                     result = EXIT_SUCCESS;

    /**
     * Pointer to the place where the system returns the port socket
     * is bound to
     */
    in_port_t *port_sys = NULL;
    /** Pointer to the location of port value in user buffer */
    in_port_t *port_usr = NULL;


    if (addr == NULL)
    {
        ERROR("%s(): it is not allowed to pass NULL server address",
               __FUNCTION__);
        return -1;
    }
    if (sizeof(bind_addr) < te_sockaddr_get_size(addr))
    {
        ERROR("%s(): it is not support address structures with "
              "length more than %d", __FUNCTION__, sizeof(bind_addr));
        return -1;
    }
    memcpy(&bind_addr, addr, te_sockaddr_get_size(addr));

    switch (bind_addr.ss_family)
    {
        case AF_INET:
            if (SIN(addr)->sin_addr.s_addr == htonl(INADDR_ANY))
            {
                ERROR("%s(): it is not allowed to pass wildcard IPv4 address",
                      __FUNCTION__);
                return -1;
            }
            if (wild)
            {
                SIN(&bind_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
            }
            if (SIN(&bind_addr)->sin_port == 0)
            {
                port_sys = &(SIN(&bind_addr)->sin_port);
                port_usr = &(SIN(addr)->sin_port);
            }
            break;

        case AF_INET6:
            if (memcmp(&(SIN6(addr)->sin6_addr),
                       &in6addr_any, sizeof(in6addr_any)) == 0)
            {
                ERROR("%s(): it is not allowed to pass wildcard IPv6 address",
                      __FUNCTION__);
                return -1;
            }
            if (wild)
            {
                memcpy(&(SIN6(&bind_addr)->sin6_addr),
                       &in6addr_any, sizeof(in6addr_any));
            }
            if (SIN6(&bind_addr)->sin6_port == 0)
            {
                port_sys = &(SIN6(&bind_addr)->sin6_port);
                port_usr = &(SIN6(addr)->sin6_port);
            }
            break;

        default:
            ERROR("%s(): Address family %d is not supported yet",
                  __FUNCTION__, bind_addr.ss_family);
            return -1;
    }

    sockd = rpc_socket(rpc, rpc_socket_domain_by_addr(addr), 
                       sock_type, proto);
    if (set_reuse_addr)
    {
        int on = TRUE;

        rpc_setsockopt(rpc, sockd, RPC_SO_REUSEADDR, &on);
    }
    
    rpc_bind(rpc, sockd, SA(&bind_addr));

    if (port_usr != NULL)
    {
        socklen_t addrlen = sizeof(bind_addr);

        /* Determine assigned port */
        rpc_getsockname(rpc, sockd, SA(&bind_addr), &addrlen);
        memcpy(port_usr, port_sys, sizeof(*port_usr));
    }

    if (result != EXIT_SUCCESS)
    {
        CLEANUP_RPC_CLOSE(rpc, sockd);
        sockd = -1;
    }
    
    return sockd;
}

/* See the description in tapi_rpc_client_server.h */
int 
rpc_stream_server(rcf_rpc_server *srvr,
                  rpc_socket_proto proto, te_bool srvr_wild, 
                  const struct sockaddr *srvr_addr)
{
    int sockd = -1;
    
    sockd = rpc_create_and_bind_socket(srvr,
                                       RPC_SOCK_STREAM, proto, srvr_wild,
                                       FALSE /* Do not set SO_REUSEADDR */,
                                       srvr_addr);
    if (sockd < 0)
    {
        ERROR("Cannot create server socket of type SOCK_STREAM");
    }
    else
    {
        rpc_listen(srvr, sockd, TAPI_RPC_CLIENT_SERVER_BACKLOG_DEF);
    }

    return sockd;
}

/* See the description in tapi_rpc_client_server.h */
int
rpc_stream_client_fake(rcf_rpc_server *clnt,
                       rpc_socket_domain domain, rpc_socket_proto proto,
                       const struct sockaddr *clnt_addr,
                       te_bool fake,
                       rcf_rpc_server *srvr,
                       const struct sockaddr *srvr_addr,
                       const struct sockaddr *gw)
{
    int sockd = -1;

    sockd = rpc_socket(clnt, domain, RPC_SOCK_STREAM, proto);

    if (clnt_addr != NULL)
    {
        if (fake)
            SET_TRANSPARENT(clnt, clnt_addr, sockd, srvr, srvr_addr,
                            gw);

         rpc_bind(clnt, sockd, clnt_addr);
    }

    return sockd;
}

/* See the description in tapi_rpc_client_server.h */
int
rpc_stream_connection_fake(rcf_rpc_server *srvr, rcf_rpc_server *clnt,
                           rpc_socket_proto proto,
                           const struct sockaddr *srvr_addr,
                           const struct sockaddr *clnt_addr,
                           const struct sockaddr *gw_addr,
                           te_bool fake,
                           int *srvr_s, int *clnt_s)
{
    int result        = EXIT_SUCCESS;
    int srvr_sock     = -1;
    int clnt_sock     = -1;
    int accepted_sock = -1;
    
    if ((srvr_sock = rpc_stream_server(srvr, 
                                       proto, FALSE,
                                       srvr_addr)) < 0)
    {
        ERROR("%s(): Cannot create server socket of type SOCK_STREAM",
              __FUNCTION__);
        return -1;
    }

    if ((clnt_sock = rpc_stream_client_fake(clnt,
                         rpc_socket_domain_by_addr(clnt_addr),
                         proto, clnt_addr, fake, srvr, srvr_addr,
                         gw_addr)) < 0)
    {
        ERROR("%s(): Cannot create clent socket of type SOCK_STREAM",
              __FUNCTION__);
        result = EXIT_FAILURE;
        goto cleanup;
    }
    
    rpc_connect(clnt, clnt_sock, srvr_addr);
    accepted_sock = rpc_accept(srvr, srvr_sock, NULL, NULL);
    
cleanup:
    /*
     * We should close server socket anyway because we've already had a
     * connection "clnt_sock" <-> "accepted_sock"
     */
    CLEANUP_RPC_CLOSE(srvr, srvr_sock);

    if (result == EXIT_FAILURE)
    {
        CLEANUP_RPC_CLOSE(srvr, accepted_sock);
        CLEANUP_RPC_CLOSE(clnt, clnt_sock);
    }
    else
    {
        *srvr_s = accepted_sock;
        *clnt_s = clnt_sock;
    }

    return ((result == EXIT_SUCCESS) ? 0 : -1);
}


/* See the description in tapi_rpc_client_server.h */
int
rpc_dgram_connection_gen_wild(rcf_rpc_server *srvr, rcf_rpc_server *clnt,
                              rpc_socket_proto proto,
                              const struct sockaddr *srvr_addr,
                              const struct sockaddr *clnt_addr,
                              int *srvr_s, int *clnt_s,
                              te_bool srvr_connect,
                              te_bool clnt_connect,
                              te_bool bind_wildcard)
{
    int srvr_sock = -1;
    int clnt_sock = -1;
    int result = EXIT_SUCCESS;

    if (srvr_addr == NULL || clnt_addr == NULL)
    {
        ERROR("%s(): srvr_addr and clnt_addr parameters must be not NULL");
        return -1;
    }

    if ((srvr_sock = rpc_create_and_bind_socket(srvr,
                         RPC_SOCK_DGRAM, proto,
                         bind_wildcard,
                         FALSE, /* Do not set
                                   SO_REUSEADDR */
                         SA(srvr_addr) /* FIXME */)) < 0)
    {
        ERROR("Cannot create socket of type SOCK_DGRAM");
        result = EXIT_FAILURE;
        goto cleanup;
    }
    if ((clnt_sock = rpc_create_and_bind_socket(clnt,
                         RPC_SOCK_DGRAM, proto,
                         FALSE, /* Do not bind to
                                   wildcard address */
                         FALSE, /* Do not set
                                   SO_REUSEADDR */
                         SA(clnt_addr) /* FIXME */)) < 0)
    {
        ERROR("Cannot create socket of type SOCK_DGRAM");
        result = EXIT_FAILURE;
        goto cleanup;
    }

    if (clnt_connect)
        rpc_connect(clnt, clnt_sock, srvr_addr);
    if (srvr_connect)
        rpc_connect(srvr, srvr_sock, clnt_addr);

cleanup:

    if (result != EXIT_SUCCESS)
    {
        CLEANUP_RPC_CLOSE(clnt, clnt_sock);
        CLEANUP_RPC_CLOSE(srvr, srvr_sock);
        return -1;
    }

    *srvr_s = srvr_sock;
    *clnt_s = clnt_sock;

    return 0;
}

/* See the description in tapi_rpc_client_server.h */
int
rpc_dgram_connection(rcf_rpc_server *srvr, rcf_rpc_server *clnt,
                     rpc_socket_proto proto,
                     const struct sockaddr *srvr_addr,
                     const struct sockaddr *clnt_addr,
                     int *srvr_s, int *clnt_s)
{
    return rpc_dgram_connection_gen(srvr, clnt, proto,
                                    srvr_addr, clnt_addr,
                                    srvr_s, clnt_s,
                                    TRUE, TRUE);
}

/* See the description in tapi_rpc_client_server.h */
int
rpc_gen_connection_wild(rcf_rpc_server *srvr, rcf_rpc_server *clnt,
                        rpc_socket_type sock_type,
                        rpc_socket_proto proto,
                        const struct sockaddr *srvr_addr,
                        const struct sockaddr *clnt_addr,
                        const struct sockaddr *gw_addr,
                        int *srvr_s, int *clnt_s,
                        te_bool srvr_connect,
                        te_bool bind_wildcard,
                        te_bool fake)
{
    switch (sock_type)
    {
        case RPC_SOCK_STREAM:
            return rpc_stream_connection_fake(srvr, clnt, proto,
                                              srvr_addr, clnt_addr,
                                              gw_addr, fake,
                                              srvr_s, clnt_s);
            break;

        case RPC_SOCK_DGRAM:
            return rpc_dgram_connection_gen_wild(srvr, clnt, proto,
                                                 srvr_addr, clnt_addr,
                                                 srvr_s, clnt_s,
                                                 srvr_connect, TRUE,
                                                 bind_wildcard);
            break;

        default:
            ERROR("%s(): Socket type %d is not supported",
                  __FUNCTION__, sock_type);
            return -1;
    }

    return -1;
}
