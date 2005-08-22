/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from net/if.h.
 * 
 * 
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 
#ifndef __TE_RPC_NET_IF_H__
#define __TE_RPC_NET_IF_H__

#include "te_rpc_defs.h"


/* ifreq flags */
typedef enum rpc_if_fl {
    RPC_IFF_UP          = 0x0001,   /**< Interface is up */
    RPC_IFF_BROADCAST   = 0x0002,   /**< Broadcast adress valid */
    RPC_IFF_DEBUG       = 0x0004,   /**< Is a loopback net */
    RPC_IFF_POINTOPOINT = 0x0008,   /**< Interface is point-to-point link */
    RPC_IFF_NOTRAILERS  = 0x0010,   /**< Avoid use of trailers */
    RPC_IFF_RUNNING     = 0x0020,   /**< Resources allocated */
    RPC_IFF_NOARP       = 0x0040,   /**< No address resolution protocol */
    RPC_IFF_PROMISC     = 0x0080,   /**< Receive all packets */
    RPC_IFF_ALLMULTI    = 0x0100,   /**< Receive all multicast packets */
    RPC_IFF_MASTER      = 0x0200,   /**< Master of a load balancer */
    RPC_IFF_SLAVE       = 0x0400,   /**< Slave of a load balancer */
    RPC_IFF_MULTICAST   = 0x0800,   /**< Supports multicast */
    RPC_IFF_PORTSEL     = 0x1000,   /**< Can set media type */
    RPC_IFF_AUTOMEDIA   = 0x2000,   /**< Auto media select active */
    RPC_IFF_UNKNOWN     = 0x8000,   /**< Unknown flag */
} rpc_if_fl;

#define RPC_IF_FLAGS_ALL \
    (RPC_IFF_UP | RPC_IFF_BROADCAST | RPC_IFF_DEBUG |    \
     RPC_IFF_POINTOPOINT | RPC_IFF_NOTRAILERS |          \
     RPC_IFF_RUNNING | RPC_IFF_NOARP | RPC_IFF_PROMISC | \
     RPC_IFF_ALLMULTI | RPC_IFF_MASTER | RPC_IFF_SLAVE | \
     RPC_IFF_MULTICAST | RPC_IFF_PORTSEL |               \
     RPC_IFF_AUTOMEDIA)

/** List of mapping numerical value to string for 'rpc_io_fl' */
#define IF_FL_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(IFF_UP), \
    RPC_BIT_MAP_ENTRY(IFF_BROADCAST), \
    RPC_BIT_MAP_ENTRY(IFF_DEBUG), \
    RPC_BIT_MAP_ENTRY(IFF_POINTOPOINT), \
    RPC_BIT_MAP_ENTRY(IFF_NOTRAILERS), \
    RPC_BIT_MAP_ENTRY(IFF_RUNNING), \
    RPC_BIT_MAP_ENTRY(IFF_NOARP), \
    RPC_BIT_MAP_ENTRY(IFF_PROMISC), \
    RPC_BIT_MAP_ENTRY(IFF_ALLMULTI), \
    RPC_BIT_MAP_ENTRY(IFF_MASTER), \
    RPC_BIT_MAP_ENTRY(IFF_SLAVE), \
    RPC_BIT_MAP_ENTRY(IFF_MULTICAST), \
    RPC_BIT_MAP_ENTRY(IFF_PORTSEL), \
    RPC_BIT_MAP_ENTRY(IFF_AUTOMEDIA), \
    RPC_BIT_MAP_ENTRY(IFF_UNKNOWN)

/**
 * if_fl_rpc2str()
 */
RPCBITMAP2STR(if_fl, IF_FL_MAPPING_LIST)

#ifdef IFF_UP

#ifdef IFF_MASTER
#define HAVE_IFF_MASTER     1
#else
#define HAVE_IFF_MASTER     0
#define IFF_MASTER          0
#endif
#ifdef IFF_SLAVE
#define HAVE_IFF_SLAVE      1
#else
#define HAVE_IFF_SLAVE      0
#define IFF_SLAVE           0
#endif
#ifdef IFF_PORTSEL
#define HAVE_IFF_PORTSEL    1
#else
#define HAVE_IFF_PORTSEL    0
#define IFF_PORTSEL         0
#endif
#ifdef IFF_NOTRAILERS
#define HAVE_IFF_NOTRAILERS 1
#else
#define HAVE_IFF_NOTRAILERS 0
#define IFF_NOTRAILERS      0
#endif

#define IF_FLAGS_ALL (IFF_UP | IFF_BROADCAST | IFF_DEBUG |       \
                         IFF_POINTOPOINT | IFF_NOTRAILERS |      \
                         IFF_RUNNING | IFF_NOARP | IFF_PROMISC | \
                         IFF_ALLMULTI | IFF_MASTER | IFF_SLAVE | \
                         IFF_MULTICAST | IFF_PORTSEL |           \
                         IFF_AUTOMEDIA)

#define IFF_UNKNOWN 0xFFFF

static inline int
if_fl_rpc2h(rpc_if_fl flags)
{
    if ((flags & ~RPC_IF_FLAGS_ALL) != 0)
        return IFF_UNKNOWN;
    
    return (!!(flags & RPC_IFF_UP) * IFF_UP) |
           (!!(flags & RPC_IFF_BROADCAST) * IFF_BROADCAST) |
           (!!(flags & RPC_IFF_DEBUG) * IFF_DEBUG) |
           (!!(flags & RPC_IFF_POINTOPOINT) * IFF_POINTOPOINT) |
#if HAVE_IFF_NOTRAILERS
           (!!(flags & RPC_IFF_NOTRAILERS) * IFF_NOTRAILERS) |
#endif
           (!!(flags & RPC_IFF_RUNNING) * IFF_RUNNING) |
           (!!(flags & RPC_IFF_NOARP) * IFF_NOARP) |
           (!!(flags & RPC_IFF_PROMISC) * IFF_PROMISC) |
           (!!(flags & RPC_IFF_ALLMULTI) * IFF_ALLMULTI) |
#if HAVE_IFF_MASTER
           (!!(flags & RPC_IFF_MASTER) * IFF_MASTER) |
#endif
#if HAVE_IFF_SLAVE
           (!!(flags & RPC_IFF_SLAVE) * IFF_SLAVE) |
#endif
           (!!(flags & RPC_IFF_MULTICAST) * IFF_MULTICAST) |
#if HAVE_IFF_PORTSEL
           (!!(flags & RPC_IFF_PORTSEL) * IFF_PORTSEL) |
#endif
           (!!(flags & RPC_IFF_AUTOMEDIA));
}

static inline int
if_fl_h2rpc(int flags)
{
    return (!!(flags & IFF_UP) * RPC_IFF_UP) |
           (!!(flags & IFF_BROADCAST) * RPC_IFF_BROADCAST) |
           (!!(flags & IFF_DEBUG) * RPC_IFF_DEBUG) |
           (!!(flags & IFF_POINTOPOINT) * RPC_IFF_POINTOPOINT) |
           (!!(flags & IFF_NOTRAILERS) * RPC_IFF_NOTRAILERS) |
           (!!(flags & IFF_RUNNING) * RPC_IFF_RUNNING) |
           (!!(flags & IFF_NOARP) * RPC_IFF_NOARP) |
           (!!(flags & IFF_PROMISC) * RPC_IFF_PROMISC) |
           (!!(flags & IFF_ALLMULTI) * RPC_IFF_ALLMULTI) |
           (!!(flags & IFF_MASTER) * RPC_IFF_MASTER) |
           (!!(flags & IFF_SLAVE) * RPC_IFF_SLAVE) |
           (!!(flags & IFF_MULTICAST) * RPC_IFF_MULTICAST) |
           (!!(flags & IFF_PORTSEL) * RPC_IFF_PORTSEL) |
           (!!(flags & (~IFF_UNKNOWN)) * RPC_IFF_UNKNOWN);
}

#undef HAVE_IFF_NOTRAILERS
#undef HAVE_IFF_MASTER
#undef HAVE_IFF_SLAVE
#undef HAVE_IFF_PORTSEL

#endif /* IFF_UP */


#endif /* !__TE_RPC_NET_IF_H__ */
