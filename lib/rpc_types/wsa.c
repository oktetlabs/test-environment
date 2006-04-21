/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from WinSock2.
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
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
 
#include "te_rpc_defs.h"
#include "te_rpc_wsa.h"



#ifdef FD_READ
#define HAVE_FD_READ    1
#else
#define HAVE_FD_READ    0
#define FD_READ         0
#endif

#ifdef FD_WRITE
#define HAVE_FD_WRITE   1
#else
#define HAVE_FD_WRITE   0
#define FD_WRITE        0
#endif

#ifdef FD_OOB
#define HAVE_FD_OOB     1
#else
#define HAVE_FD_OOB     0
#define FD_OOB          0
#endif

#ifdef FD_ACCEPT
#define HAVE_FD_ACCEPT  1
#else
#define HAVE_FD_ACCEPT  0
#define FD_ACCEPT       0
#endif

#ifdef FD_CONNECT
#define HAVE_FD_CONNECT 1
#else
#define HAVE_FD_CONNECT 0
#define FD_CONNECT      0
#endif

#ifdef FD_CLOSE
#define HAVE_FD_CLOSE   1
#else
#define HAVE_FD_CLOSE   0
#define FD_CLOSE        0
#endif

#ifdef FD_QOS
#define HAVE_FD_QOS     1
#else
#define HAVE_FD_QOS     0
#define FD_QOS          0
#endif

#ifdef FD_GROUP_QOS
#define HAVE_FD_GROUP_QOS    1
#else
#define HAVE_FD_GROUP_QOS    0
#define FD_GROUP_QOS         0
#endif

#ifdef FD_ROUTING_INTERFACE_CHANGE
#define HAVE_FD_ROUTING_INTERFACE_CHANGE   1
#else
#define HAVE_FD_ROUTING_INTERFACE_CHANGE   0
#define FD_ROUTING_INTERFACE_CHANGE        0
#endif

#ifdef FD_ADDRESS_LIST_CHANGE
#define HAVE_FD_ADDRESS_LIST_CHANGE        1
#else
#define HAVE_FD_ADDRESS_LIST_CHANGE        0
#define FD_ADDRESS_LIST_CHANGE             0
#endif

/** Convert RPC network evenet flags to native flags */
unsigned int
network_event_rpc2h(rpc_network_event flags)
{
    return 
           (!!(flags & RPC_FD_READ) * FD_READ) |
           (!!(flags & RPC_FD_WRITE) * FD_WRITE) |
           (!!(flags & RPC_FD_OOB) * FD_OOB) |
           (!!(flags & RPC_FD_ACCEPT) * FD_ACCEPT) |
           (!!(flags & RPC_FD_CONNECT) * FD_CONNECT) |
           (!!(flags & RPC_FD_CLOSE) * FD_CLOSE) |
           (!!(flags & RPC_FD_QOS) * FD_QOS) |
           (!!(flags & RPC_FD_GROUP_QOS) * FD_GROUP_QOS) |
           (!!(flags & RPC_FD_ROUTING_INTERFACE_CHANGE) *
                FD_ROUTING_INTERFACE_CHANGE) |
           (!!(flags & RPC_FD_ADDRESS_LIST_CHANGE) *
                FD_ADDRESS_LIST_CHANGE); 
}

/** Convert native network evenet flags to RPC flags */
rpc_network_event
network_event_h2rpc(unsigned int flags)
{
    return (!!(flags & FD_READ) * RPC_FD_READ) |
           (!!(flags & FD_WRITE) * RPC_FD_WRITE) |
           (!!(flags & FD_OOB) * RPC_FD_OOB) |
           (!!(flags & FD_ACCEPT) * RPC_FD_ACCEPT) |
           (!!(flags & FD_CONNECT) * RPC_FD_CONNECT) |
           (!!(flags & FD_CLOSE) * RPC_FD_CLOSE) |
           (!!(flags & FD_QOS) * RPC_FD_QOS) |
           (!!(flags & FD_GROUP_QOS) * RPC_FD_GROUP_QOS) |
           (!!(flags & FD_ROUTING_INTERFACE_CHANGE) *
                RPC_FD_ROUTING_INTERFACE_CHANGE) |
           (!!(flags & FD_ADDRESS_LIST_CHANGE) *
                RPC_FD_ADDRESS_LIST_CHANGE);
}

/** Convert RPC network event(s) to string */
const char *
network_event_rpc2str(rpc_network_event events)
{
    static char buf[128];
    char      * s = buf;
    
    buf[0] = 0;
    
#define APPEND(_event)                                  \
    do {                                                \
        if (events & RPC_##_event)                      \
        {                                               \
            if (s == buf)                               \
                s += sprintf(s, "%s",  #_event);        \
            else                                        \
                s += sprintf(s, " |  %s", #_event);     \
        }                                               \
    } while (0)
    
    APPEND(FD_READ);
    APPEND(FD_WRITE);
    APPEND(FD_OOB);
    APPEND(FD_ACCEPT);
    APPEND(FD_CONNECT);
    APPEND(FD_CLOSE);
    APPEND(FD_QOS);
    APPEND(FD_GROUP_QOS);
    APPEND(FD_ROUTING_INTERFACE_CHANGE);
    APPEND(FD_ADDRESS_LIST_CHANGE);

#undef APPEND

    return buf;
}


#ifndef TF_DISCONNECT
#define TF_DISCONNECT  0
#endif

#ifndef TF_REUSE_SOCKET
#define TF_REUSE_SOCKET  0
#endif

#ifndef TF_USE_DEFAULT_WORKER
#define TF_USE_DEFAULT_WORKER  0
#endif

#ifndef TF_USE_SYSTEM_THREAD
#define TF_USE_SYSTEM_THREAD  0
#endif

#ifndef TF_USE_KERNEL_APC
#define TF_USE_KERNEL_APC  0
#endif

#ifndef TF_WRITE_BEHIND
#define TF_WRITE_BEHIND  0
#endif

#define TRANSMIT_FILE_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(TF_DISCONNECT),          \
            RPC_BIT_MAP_ENTRY(TF_REUSE_SOCKET),        \
            RPC_BIT_MAP_ENTRY(TF_USE_DEFAULT_WORKER),  \
            RPC_BIT_MAP_ENTRY(TF_USE_SYSTEM_THREAD),   \
            RPC_BIT_MAP_ENTRY(TF_USE_KERNEL_APC),      \
            RPC_BIT_MAP_ENTRY(TF_WRITE_BEHIND)

/** Convert RPC transmit file flags to native flags */
unsigned int
transmit_file_flags_rpc2h(rpc_transmit_file_flags flags)
{
    return 
           (!!(flags & RPC_TF_DISCONNECT) * TF_DISCONNECT) |
           (!!(flags & RPC_TF_REUSE_SOCKET) * TF_REUSE_SOCKET) |
           (!!(flags & RPC_TF_USE_DEFAULT_WORKER) * TF_USE_DEFAULT_WORKER) |
           (!!(flags & RPC_TF_USE_SYSTEM_THREAD) * TF_USE_SYSTEM_THREAD) |
           (!!(flags & RPC_TF_USE_KERNEL_APC) * TF_USE_KERNEL_APC) |
           (!!(flags & RPC_TF_WRITE_BEHIND) * TF_WRITE_BEHIND);
}

unsigned int
servicetype_flags_rpc2h(rpc_servicetype_flags flags)
{
    return
        (!!(flags & RPC_SERVICETYPE_NOTRAFFIC)
            * SERVICETYPE_NOTRAFFIC) |
        (!!(flags & RPC_SERVICETYPE_BESTEFFORT)
            * SERVICETYPE_BESTEFFORT) |
        (!!(flags & RPC_SERVICETYPE_CONTROLLEDLOAD)
            * SERVICETYPE_CONTROLLEDLOAD) |
        (!!(flags & RPC_SERVICETYPE_GUARANTEED)
            * SERVICETYPE_GUARANTEED) |
        (!!(flags & RPC_SERVICETYPE_NETWORK_UNAVAILABLE)
            * SERVICETYPE_NETWORK_UNAVAILABLE) |
        (!!(flags & RPC_SERVICETYPE_GENERAL_INFORMATION)
            * SERVICETYPE_GENERAL_INFORMATION) |
        (!!(flags & RPC_SERVICETYPE_NOCHANGE)
            * SERVICETYPE_NOCHANGE) |
        (!!(flags & RPC_SERVICETYPE_NONCONFORMING)
            * SERVICETYPE_NONCONFORMING) |
        (!!(flags & RPC_SERVICETYPE_NETWORK_CONTROL)
            * SERVICETYPE_NETWORK_CONTROL) |
        (!!(flags & RPC_SERVICETYPE_QUALITATIVE)
            * SERVICETYPE_QUALITATIVE) |
        (!!(flags & RPC_SERVICE_NO_TRAFFIC_CONTROL)
            * SERVICE_NO_TRAFFIC_CONTROL) |
        (!!(flags & RPC_SERVICE_NO_QOS_SIGNALING)
            * SERVICE_NO_QOS_SIGNALING);
}

rpc_servicetype_flags
servicetype_flags_h2rpc(unsigned int flags)
{
    return
        (!!(flags & SERVICETYPE_NOTRAFFIC)
            * RPC_SERVICETYPE_NOTRAFFIC) |
        (!!(flags & SERVICETYPE_BESTEFFORT)
            * RPC_SERVICETYPE_BESTEFFORT) |
        (!!(flags & SERVICETYPE_CONTROLLEDLOAD)
            * RPC_SERVICETYPE_CONTROLLEDLOAD) |
        (!!(flags & SERVICETYPE_GUARANTEED)
            * RPC_SERVICETYPE_GUARANTEED) |
        (!!(flags & SERVICETYPE_NETWORK_UNAVAILABLE)
            * RPC_SERVICETYPE_NETWORK_UNAVAILABLE) |
        (!!(flags & SERVICETYPE_GENERAL_INFORMATION)
            * RPC_SERVICETYPE_GENERAL_INFORMATION) |
        (!!(flags & SERVICETYPE_NOCHANGE)
            * RPC_SERVICETYPE_NOCHANGE) |
        (!!(flags & SERVICETYPE_NONCONFORMING)
            * RPC_SERVICETYPE_NONCONFORMING) |
        (!!(flags & SERVICETYPE_NETWORK_CONTROL)
            * RPC_SERVICETYPE_NETWORK_CONTROL) |
        (!!(flags & SERVICETYPE_QUALITATIVE)
            * RPC_SERVICETYPE_QUALITATIVE) |
        (!!(flags & SERVICE_NO_TRAFFIC_CONTROL)
            * RPC_SERVICE_NO_TRAFFIC_CONTROL) |
        (!!(flags & SERVICE_NO_QOS_SIGNALING)
            * RPC_SERVICE_NO_QOS_SIGNALING);
}


#ifdef WSA_FLAG_OVERLAPPED
/** Convert rpc_open_sock_flags to the native ones */
unsigned int
open_sock_flags_rpc2h(unsigned int flags)
{
    return 
           (!!(flags & RPC_WSA_FLAG_OVERLAPPED) * WSA_FLAG_OVERLAPPED) |
           (!!(flags & RPC_WSA_FLAG_MULTIPOINT_C_ROOT) * 
            WSA_FLAG_MULTIPOINT_C_ROOT) |
           (!!(flags & RPC_WSA_FLAG_MULTIPOINT_C_LEAF) * 
            WSA_FLAG_MULTIPOINT_C_LEAF) |
           (!!(flags & RPC_WSA_FLAG_MULTIPOINT_D_ROOT) * 
            WSA_FLAG_MULTIPOINT_D_ROOT) |
           (!!(flags & RPC_WSA_FLAG_MULTIPOINT_D_LEAF) * 
            WSA_FLAG_MULTIPOINT_D_LEAF) ;

}
#endif


/** Convert rpc_join_leaf_flags to string */
const char *
join_leaf_rpc2str(rpc_join_leaf_flags open_code)
{
    switch (open_code)
    {
        RPC2STR(JL_SENDER_ONLY);
        RPC2STR(JL_RECEIVER_ONLY);
        RPC2STR(JL_BOTH);
        default: return "<JOIN_LEAF_FATAL_ERROR>";
    }
}


#ifdef JL_SENDER_ONLY
/** Convert rpc_join_leaf_flags to the native ones */
unsigned int
join_leaf_flags_rpc2h(unsigned int flags)
{
    return 
           (!!(flags & RPC_JL_SENDER_ONLY) * JL_SENDER_ONLY) |
           (!!(flags & RPC_JL_RECEIVER_ONLY) * JL_RECEIVER_ONLY) |
           (!!(flags & RPC_JL_BOTH) * JL_BOTH);
}

#endif
