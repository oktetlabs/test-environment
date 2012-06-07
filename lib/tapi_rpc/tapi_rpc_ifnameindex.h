/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of interface name/index
 * functions.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_IFNAMEINDEX_H__
#define __TE_TAPI_RPC_IFNAMEINDEX_H__

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_ifnameindex TAPI for interface name/index calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * Convert network interface name to index.
 *
 * @param handle    RPC server handle
 * @param ifname    interface name
 *
 * @return Interface index otherwise zero. 
 */
extern unsigned int rpc_if_nametoindex(rcf_rpc_server *handle,
                                       const char *ifname);
                                       
/**
 * Map network interface index to its corresponding name.
 *
 * @param handle    RPC server handle
 * @param ifindex   index of the interface
 * @param ifname    pointer to the network interface name with 
                    at least @c IF_NAMESIZE bytes size.
 *
 * @return Pointer to the interface name otherwise NULL
 *
 */
extern char *rpc_if_indextoname(rcf_rpc_server *handle,
                                unsigned int ifindex, char *ifname);

#if HAVE_NET_IF_H
/**
 * Get an array of all existing network interface structures. The end of 
 * the array is indicated by an structure with an if_index field equal to 
 * zero and an if_name field of NULL.
 * Applications should call @b rpc_freenameindex() to free memory used
 * by the returned array.
 *
 * @param  handle RPC server handle
 *
 * @return Array of all existing interfaces, upon successful completion,
 *         otherwise NULL is returned
 */
extern struct if_nameindex *rpc_if_nameindex(rcf_rpc_server *handle);

/**
 * Free the memory allocated by @b rpc_if_nameindex().
 *
 * @param handle RPC server handle
 * @param ptr    pointer to the array of interfaces
 */
extern void rpc_if_freenameindex(rcf_rpc_server *handle,
                                 struct if_nameindex *ptr);
#endif

/**@} <!-- END te_lib_rpc_ifnameindex --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_IFNAMEINDEX_H__ */
