/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of interface name/index
 * functions.
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

#ifndef __TE_TAPI_RPC_IFNAMEINDEX_H__
#define __TE_TAPI_RPC_IFNAMEINDEX_H__

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "rcf_rpc.h"


extern unsigned int rpc_if_nametoindex(rcf_rpc_server *handle,
                                       const char *ifname);

extern char *rpc_if_indextoname(rcf_rpc_server *handle,
                                unsigned int ifindex, char *ifname);

#if HAVE_NET_IF_H
extern struct if_nameindex *rpc_if_nameindex(rcf_rpc_server *handle);

extern void rpc_if_freenameindex(rcf_rpc_server *handle,
                                 struct if_nameindex *ptr);
#endif

#endif /* !__TE_TAPI_RPC_IFNAMEINDEX_H__ */
