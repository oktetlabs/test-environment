/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for name/address resolution remote calls
 * Memory is allocated by these functions using malloc().
 * They are thread-safe.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TAPI_RPC_NETDB_H__
#define __TE_TAPI_RPC_NETDB_H__

#if HAVE_NETDB_H
#include <netdb.h>
#endif

#include "rcf_rpc.h"
#include "te_rpc_netdb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_netdb TAPI for name/address resolution remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Get network host entry by given name.
 *
 * @param handle     RPC Server
 * @param name       given name   may be
 *                   - string hostname
 *                   - ipv4 address in standart dot notation
 *                   - ipv6 address
 *
 * @return Host entry on success otherwise NULL
 */
extern struct hostent *rpc_gethostbyname(rcf_rpc_server *handle,
                                         const char *name);

/**
 * Get network host entry by given address.
 *
 * @param handle     RPC Server
 * @param addr       given address
 * @param len        length of given address
 * @param type       address type (IF_INET or IF_INET6)
 *
 * @return Host entry on success otherwise NULL
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

/**@} <!-- END te_lib_rpc_netdb --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_NETDB_H__ */
