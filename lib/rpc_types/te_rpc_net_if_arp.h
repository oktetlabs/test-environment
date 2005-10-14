/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from net/if_arp.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 
#ifndef __TE_RPC_NET_IF_ARP_H__
#define __TE_RPC_NET_IF_ARP_H__

#include "te_rpc_defs.h"


/* arpreq flags */
typedef enum rpc_arp_flags {
    RPC_ATF_COM = 0x0001,         /**< Lookup complete */
    RPC_ATF_PERM = 0x0002,        /**< Permanent entry */
    RPC_ATF_PUBL = 0x0004,        /**< Publish entry */
    RPC_ATF_NETMASK = 0x0008,     /**< Use a netmask */
    RPC_ATF_DONTPUB = 0x0010      /**< Don't answer */
} rpc_arp_fl;    

#define RPC_ARP_FLAGS_ALL \
    (RPC_ATF_COM | RPC_ATF_PERM | RPC_ATF_PUBL |  \
     RPC_ATF_NETMASK | RPC_ATF_DONTPUB)

#define ARP_UNKNOWN 0xFFFF

/** List of mapping numerical value to string for 'rpc_io_fl' */
#define ARP_FL_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(ATF_COM), \
    RPC_BIT_MAP_ENTRY(ATF_PERM), \
    RPC_BIT_MAP_ENTRY(ATF_PUBL), \
    RPC_BIT_MAP_ENTRY(ATF_NETMASK), \
    RPC_BIT_MAP_ENTRY(ATF_DONTPUB)

#if HAVE_NET_IF_ARP_H

#ifdef ATF_NETMASK
#define HAVE_ATF_NETMASK 1
#else
#define HAVE_ATF_NETMASK 0
#define ATF_NETMASK      0
#endif
#ifdef ATF_DONTPUB
#define HAVE_ATF_DONTPUB 1
#else
#define HAVE_ATF_DONTPUB 0
#define ATF_DONTPUB      0
#endif

#define ARP_FLAGS_ALL \
    (ATF_COM | ATF_PERM | ATF_PUBL | ATF_NETMASK | ATF_DONTPUB)

static inline int
arp_fl_rpc2h(rpc_arp_fl flags)
{
    if ((flags & ~RPC_ARP_FLAGS_ALL) != 0)
        return ARP_UNKNOWN;
    
    return (!!(flags & RPC_ATF_COM) * ATF_COM) |
           (!!(flags & RPC_ATF_PERM) * ATF_PERM) |
           (!!(flags & RPC_ATF_PUBL) * ATF_PUBL)
#if HAVE_ATF_NETMASK
           | (!!(flags & RPC_ATF_NETMASK) * ATF_NETMASK)
#endif
#if HAVE_ATF_DONTPUB
           | (!!(flags & RPC_ATF_DONTPUB) * ATF_DONTPUB)
#endif
           ;
}

static inline int
arp_fl_h2rpc(int flags)
{
    return (!!(flags & ATF_COM) * RPC_ATF_COM) |
           (!!(flags & ATF_PERM) * RPC_ATF_PERM) |
           (!!(flags & ATF_PUBL) * RPC_ATF_PUBL)
#if HAVE_ATF_NETMASK
           | (!!(flags & ATF_NETMASK) * RPC_ATF_NETMASK)
#endif
#if HAVE_ATF_DONTPUB
           | (!!(flags & ATF_DONTPUB) * RPC_ATF_DONTPUB)
#endif
           ;
}

#endif /* HAVE_NET_IF_ARP_H */


#endif /* !__TE_RPC_NET_IF_ARP_H__ */
