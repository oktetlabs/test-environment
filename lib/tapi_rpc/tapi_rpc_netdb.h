/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for name/address resolution remote calls
 * Memory is allocated by these functions using malloc().
 * They are thread-safe.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_NETDB_H__
#define __TE_TAPI_RPC_NETDB_H__

#if HAVE_NETDB_H
#include <netdb.h>
#endif

#include "rcf_rpc.h"
#include "tapi_rpcsock_defs.h"

/**
 * Get network host entry by given name
 *
 * @param handle     RPC Server
 * @param name       given name   may be
 *                   - string hostname 
 *                   - ipv4 address in standart dot notation
 *                   - ipv6 address
 *
 * @return host entry on success otherwise NULL
 */
extern struct hostent *rpc_gethostbyname(rcf_rpc_server *handle,
                                         const char *name);

/**
 * Get network host entry by given address
 *
 * @param handle     RPC Server
 * @param addr       given address
 * @param len        length of given address
 * @param type       address type (IF_INET or IF_INET6)
 *
 * @return host entry on success otherwise NULL
 */
extern struct hostent *rpc_gethostbyaddr(rcf_rpc_server *handle,
                                         const char *addr,
                                         int len,
                                         rpc_socket_addr_family type);

#if HAVE_NETDB_H
extern int rpc_getaddrinfo(rcf_rpc_server *handle,
                           const char *node, const char *service,
                           const struct addrinfo *hints,
                           struct addrinfo **res);

extern void rpc_freeaddrinfo(rcf_rpc_server *handle,
                             struct addrinfo *res);
#endif

#endif /* !__TE_TAPI_RPC_NETDB_H__ */
