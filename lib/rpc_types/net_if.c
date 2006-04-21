/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from net/if.h.
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

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
 
#include "te_rpc_defs.h"
#include "te_rpc_net_if.h"


#ifndef IFF_MASTER
#define IFF_MASTER          0
#endif
#ifndef IFF_SLAVE
#define IFF_SLAVE           0
#endif
#ifndef IFF_PORTSEL
#define IFF_PORTSEL         0
#endif
#ifndef IFF_NOTRAILERS
#define IFF_NOTRAILERS      0
#endif
#ifndef IFF_NOARP
#define IFF_NOARP           0
#endif
#ifndef IFF_DEBUG
#define IFF_DEBUG           0
#endif
#ifndef IFF_POINTOPOINT
#define IFF_POINTOPOINT     0
#endif
#ifndef IFF_RUNNING     
#define IFF_RUNNING         0
#endif
#ifndef IFF_PROMISC
#define IFF_PROMISC         0
#endif
#ifndef IFF_ALLMULTI      
#define IFF_ALLMULTI        0
#endif

#define IF_FLAGS_ALL (IFF_UP | IFF_BROADCAST | IFF_DEBUG |       \
                         IFF_POINTOPOINT | IFF_NOTRAILERS |      \
                         IFF_RUNNING | IFF_NOARP | IFF_PROMISC | \
                         IFF_ALLMULTI | IFF_MASTER | IFF_SLAVE | \
                         IFF_MULTICAST | IFF_PORTSEL |           \
                         IFF_AUTOMEDIA)

#define IFF_UNKNOWN 0xFFFF

unsigned int
if_fl_rpc2h(unsigned int flags)
{
    if ((flags & ~RPC_IF_FLAGS_ALL) != 0)
        return IFF_UNKNOWN;
    
    return (!!(flags & RPC_IFF_UP) * IFF_UP) |
           (!!(flags & RPC_IFF_BROADCAST) * IFF_BROADCAST) |
           (!!(flags & RPC_IFF_DEBUG) * IFF_DEBUG) |
           (!!(flags & RPC_IFF_POINTOPOINT) * IFF_POINTOPOINT) |
           (!!(flags & RPC_IFF_NOTRAILERS) * IFF_NOTRAILERS) |
           (!!(flags & RPC_IFF_RUNNING) * IFF_RUNNING) |
           (!!(flags & RPC_IFF_NOARP) * IFF_NOARP) |
           (!!(flags & RPC_IFF_PROMISC) * IFF_PROMISC) |
           (!!(flags & RPC_IFF_ALLMULTI) * IFF_ALLMULTI) |
           (!!(flags & RPC_IFF_MASTER) * IFF_MASTER) |
           (!!(flags & RPC_IFF_SLAVE) * IFF_SLAVE) |
           (!!(flags & RPC_IFF_MULTICAST) * IFF_MULTICAST) |
           (!!(flags & RPC_IFF_PORTSEL) * IFF_PORTSEL) |
           (!!(flags & RPC_IFF_AUTOMEDIA));
}

unsigned int
if_fl_h2rpc(unsigned int flags)
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
