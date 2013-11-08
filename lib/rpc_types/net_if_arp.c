/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from net/if_arp.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
 
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_net_if_arp.h"


#define ARP_UNKNOWN 0xFFFF


#if HAVE_NET_IF_ARP_H

#ifndef ATF_NETMASK
#define ATF_NETMASK      0
#endif
#ifndef ATF_DONTPUB
#define ATF_DONTPUB      0
#endif

#define ARP_FLAGS_ALL \
    (ATF_COM | ATF_PERM | ATF_PUBL | ATF_NETMASK | ATF_DONTPUB)

unsigned int
arp_fl_rpc2h(unsigned flags)
{
    if ((flags & ~RPC_ARP_FLAGS_ALL) != 0)
        return ARP_UNKNOWN;
    
    return (!!(flags & RPC_ATF_COM) * ATF_COM)
           | (!!(flags & RPC_ATF_PERM) * ATF_PERM)
           | (!!(flags & RPC_ATF_PUBL) * ATF_PUBL)
           | (!!(flags & RPC_ATF_NETMASK) * ATF_NETMASK)
           | (!!(flags & RPC_ATF_DONTPUB) * ATF_DONTPUB)
           ;
}

unsigned int
arp_fl_h2rpc(unsigned int flags)
{
    return (!!(flags & ATF_COM) * RPC_ATF_COM)
           | (!!(flags & ATF_PERM) * RPC_ATF_PERM)
           | (!!(flags & ATF_PUBL) * RPC_ATF_PUBL)
           | (!!(flags & ATF_NETMASK) * RPC_ATF_NETMASK)
           | (!!(flags & ATF_DONTPUB) * RPC_ATF_DONTPUB)
           ;
}

#endif /* HAVE_NET_IF_ARP_H */
