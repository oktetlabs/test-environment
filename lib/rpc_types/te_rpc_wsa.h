/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from WinSock2.
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
 
#ifndef __TE_RPC_WSA_H__
#define __TE_RPC_WSA_H__

#include "te_rpc_defs.h"


/**
 * TA-independent network event flags. 
 */
typedef enum rpc_network_event {
    RPC_FD_READ      = 1,     /**< Readiness for reading */
    RPC_FD_WRITE     = 2,     /**< Readiness for writing */
    RPC_FD_OOB       = 4,     /**< Arrival of out-of-band data */
    RPC_FD_ACCEPT    = 8,     /**< Incoming connections */
    RPC_FD_CONNECT   = 0x10,  /**< Completed connection or
                                    multipoint join operation */
    RPC_FD_CLOSE     = 0x20,  /**< Socket closure */
    RPC_FD_QOS       = 0x40,  /**< Socket QOS changes */
    RPC_FD_GROUP_QOS = 0x80,  /**< Reserved. Socket group QOS changes */
    RPC_FD_ROUTING_INTERFACE_CHANGE = 0x100, /**< Routing interface change
                                                  for the specified
                                                  destination */
    RPC_FD_ADDRESS_LIST_CHANGE      = 0x200, /**< Local address list
                                                  changes for the address
                                                  family of the socket */
} rpc_network_event;

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

#define NETW_EVENT_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(FD_READ),                      \
            RPC_BIT_MAP_ENTRY(FD_WRITE),                     \
            RPC_BIT_MAP_ENTRY(FD_OOB),                       \
            RPC_BIT_MAP_ENTRY(FD_ACCEPT),                    \
            RPC_BIT_MAP_ENTRY(FD_CONNECT),                   \
            RPC_BIT_MAP_ENTRY(FD_CLOSE),                     \
            RPC_BIT_MAP_ENTRY(FD_QOS),                       \
            RPC_BIT_MAP_ENTRY(FD_GROUP_QOS),                 \
            RPC_BIT_MAP_ENTRY(FD_ROUTING_INTERFACE_CHANGE),  \
            RPC_BIT_MAP_ENTRY(FD_ADDRESS_LIST_CHANGE)

/** Convert RPC network evenet flags to native flags */
static inline unsigned int
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
static inline rpc_network_event
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
static inline const char *
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

/**
 * TA-independent TransmitFile() flags. 
 */
typedef enum rpc_transmit_file_flags {
    RPC_TF_DISCONNECT         = 1,     /**< Start a transport-level
                                            disconnect after all the file
                                            data has been queued for
                                            transmission */
    RPC_TF_REUSE_SOCKET       = 2,     /**< Prepare the socket handle
                                            to be reused */
    RPC_TF_USE_DEFAULT_WORKER = 4,     /**< Use the system's default
                                            thread */
    RPC_TF_USE_SYSTEM_THREAD  = 8,     /**< Use system threads */
    RPC_TF_USE_KERNEL_APC     = 0x10,  /**< Use kernel asynchronous
                                            procedure calls */
    RPC_TF_WRITE_BEHIND       = 0x20,  /**< Complete the TransmitFile
                                            request immediately, without
                                            pending */
} rpc_transmit_file_flags;

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
static inline unsigned int
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

/**
 * TA-independent Win32 SERVICETYPE flags. 
 */
typedef enum rpc_servicetype_flags {
    RPC_SERVICETYPE_NOTRAFFIC             = 0x00000000,
    RPC_SERVICETYPE_BESTEFFORT            = 0x00000001,
    RPC_SERVICETYPE_CONTROLLEDLOAD        = 0x00000002,
    RPC_SERVICETYPE_GUARANTEED            = 0x00000003,
    RPC_SERVICETYPE_NETWORK_UNAVAILABLE   = 0x00000004,
    RPC_SERVICETYPE_GENERAL_INFORMATION   = 0x00000005,
    RPC_SERVICETYPE_NOCHANGE              = 0x00000006,
    RPC_SERVICETYPE_NONCONFORMING         = 0x00000009,
    RPC_SERVICETYPE_NETWORK_CONTROL       = 0x0000000A,
    RPC_SERVICETYPE_QUALITATIVE           = 0x0000000D,
    RPC_SERVICE_NO_TRAFFIC_CONTROL        = 0x81000000,
    RPC_SERVICE_NO_QOS_SIGNALING          = 0x40000000
} rpc_servicetype_flags;

#ifndef SERVICETYPE_NOTRAFFIC
/* Cygwin 1.5.10-3 does not provide us with required definitions */
#define SERVICETYPE_NOTRAFFIC               0x00000000
#define SERVICETYPE_BESTEFFORT              0x00000001
#define SERVICETYPE_CONTROLLEDLOAD          0x00000002
#define SERVICETYPE_GUARANTEED              0x00000003
#define SERVICETYPE_NETWORK_UNAVAILABLE     0x00000004
#define SERVICETYPE_GENERAL_INFORMATION     0x00000005
#define SERVICETYPE_NOCHANGE                0x00000006
#define SERVICETYPE_NONCONFORMING           0x00000009
#define SERVICETYPE_NETWORK_CONTROL         0x0000000A
#define SERVICETYPE_QUALITATIVE             0x0000000D
#define SERVICE_NO_TRAFFIC_CONTROL          0x81000000
#define SERVICE_NO_QOS_SIGNALING            0x40000000
#endif

static inline unsigned int
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

static inline rpc_servicetype_flags
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


/**
 * TA-independent control codes for Windows WSAIoctl().
 */
typedef enum rpc_wsa_ioctl_code {
    RPC_WSA_FIONBIO = 1,
    RPC_WSA_FIONREAD,
    RPC_WSA_SIOCATMARK,
    RPC_WSA_SIO_ADDRESS_LIST_CHANGE,
    RPC_WSA_SIO_ADDRESS_LIST_QUERY,
    RPC_WSA_SIO_ASSOCIATE_HANDLE,
    RPC_WSA_SIO_CHK_QOS,
    RPC_WSA_SIO_ENABLE_CIRCULAR_QUEUEING,
    RPC_WSA_SIO_FIND_ROUTE,
    RPC_WSA_SIO_FLUSH,
    RPC_WSA_SIO_GET_BROADCAST_ADDRESS,
    RPC_WSA_SIO_GET_EXTENSION_FUNCTION_POINTER,
    RPC_WSA_SIO_GET_GROUP_QOS,
    RPC_WSA_SIO_GET_QOS,
    RPC_WSA_SIO_KEEPALIVE_VALS,
    RPC_WSA_SIO_MULTIPOINT_LOOPBACK,
    RPC_WSA_SIO_MULTICAST_SCOPE,
    RPC_WSA_SIO_RCVALL,
    RPC_WSA_SIO_RCVALL_IGMPMCAST,
    RPC_WSA_SIO_RCVALL_MCAST,
    RPC_WSA_SIO_ROUTING_INTERFACE_CHANGE,
    RPC_WSA_SIO_ROUTING_INTERFACE_QUERY,
    RPC_WSA_SIO_SET_QOS,
    RPC_WSA_SIO_TRANSLATE_HANDLE,
    RPC_WSA_SIO_UDP_CONNRESET
} rpc_wsa_ioctl_code;

#define RPCWSA2H(name_) \
    case RPC_WSA_##name_: return name_;

#ifdef SIO_ADDRESS_LIST_CHANGE
/* Cygwin 1.5.10-3 does not provide us with some required definitions */
#ifndef SIO_CHK_QOS
#define    mIOC_IN       0x80000000
#define    mIOC_OUT      0x40000000
#define    mIOC_VENDOR   0x04000000
#define    mCOMPANY      0x18000000
#define    ioctl_code    0x00000001
#define SIO_CHK_QOS mIOC_IN | mIOC_OUT | mIOC_VENDOR | mCOMPANY | ioctl_code
#endif
#ifndef SIO_RCVALL
#define SIO_RCVALL            _WSAIOW(IOC_VENDOR,1)
#endif
#ifndef SIO_RCVALL_MCAST
#define SIO_RCVALL_MCAST      _WSAIOW(IOC_VENDOR,2)
#endif
#ifndef SIO_RCVALL_IGMPMCAST
#define SIO_RCVALL_IGMPMCAST  _WSAIOW(IOC_VENDOR,3)
#endif
#ifndef SIO_KEEPALIVE_VALS
#define SIO_KEEPALIVE_VALS    _WSAIOW(IOC_VENDOR,4)
struct tcp_keepalive {
    u_long  onoff;
    u_long  keepalivetime;
    u_long  keepaliveinterval;
};
#endif
#ifndef  SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET     _WSAIOW(IOC_VENDOR,12)
#endif
#endif /* SIO_ADDRESS_LIST_CHANGE */

static inline int
wsa_ioctl_rpc2h(rpc_wsa_ioctl_code code)
{
    switch (code)
    {
#ifdef SIO_ADDRESS_LIST_CHANGE
        RPCWSA2H(FIONBIO);
        RPCWSA2H(FIONREAD);
        RPCWSA2H(SIOCATMARK);
        RPCWSA2H(SIO_ADDRESS_LIST_CHANGE);
        RPCWSA2H(SIO_ADDRESS_LIST_QUERY);
        RPCWSA2H(SIO_ASSOCIATE_HANDLE);
        RPCWSA2H(SIO_CHK_QOS);
        RPCWSA2H(SIO_ENABLE_CIRCULAR_QUEUEING);
        RPCWSA2H(SIO_FIND_ROUTE);
        RPCWSA2H(SIO_FLUSH);
        RPCWSA2H(SIO_GET_BROADCAST_ADDRESS);
        RPCWSA2H(SIO_GET_EXTENSION_FUNCTION_POINTER);
        RPCWSA2H(SIO_GET_GROUP_QOS);
        RPCWSA2H(SIO_GET_QOS);
        RPCWSA2H(SIO_KEEPALIVE_VALS);
        RPCWSA2H(SIO_MULTIPOINT_LOOPBACK);
        RPCWSA2H(SIO_MULTICAST_SCOPE);
        RPCWSA2H(SIO_RCVALL);
        RPCWSA2H(SIO_RCVALL_IGMPMCAST);
        RPCWSA2H(SIO_RCVALL_MCAST);
        RPCWSA2H(SIO_ROUTING_INTERFACE_CHANGE);
        RPCWSA2H(SIO_ROUTING_INTERFACE_QUERY);
        RPCWSA2H(SIO_SET_QOS);
        RPCWSA2H(SIO_TRANSLATE_HANDLE);
        RPCWSA2H(SIO_UDP_CONNRESET);
#endif /* SIO_ADDRESS_LIST_CHANGE */
        default:
            return 0;
    }
}

#undef RPCWSA2H

/**
 * TA-independent definitions for Windows CreateFile().
 * Attention: these flags are the most frequently used,
 * there are in Windows much more flags for CreateFile().
 */
typedef enum rpc_cf_access_right {
    RPC_CF_GENERIC_EXECUTE  = 0x01,
    RPC_CF_GENERIC_READ     = 0x02,
    RPC_CF_GENERIC_WRITE    = 0x04
} rpc_cf_access_right;

typedef enum rpc_cf_share_mode {
    RPC_CF_FILE_SHARE_DELETE  = 0x01,
    RPC_CF_FILE_SHARE_READ    = 0x02,
    RPC_CF_FILE_SHARE_WRITE   = 0x04
} rpc_cf_share_mode;

typedef enum rpc_cf_creation_disposition {
    RPC_CF_CREATE_ALWAYS      = 0x01,
    RPC_CF_CREATE_NEW         = 0x02,
    RPC_CF_OPEN_ALWAYS        = 0x04,
    RPC_CF_OPEN_EXISTING      = 0x08,
    RPC_CF_TRUNCATE_EXISTING  = 0x10
} rpc_cf_creation_disposition;

typedef enum rpc_cf_flags_attributes {
    RPC_CF_FILE_ATTRIBUTE_NORMAL = 0x01
} rpc_cf_flags_attributes;


/** TA-independent Winsock Error codes */
typedef enum rpc_win_error {
    RPC_WSAEACCES = 1,
    RPC_WSAEFAULT,
    RPC_WSAEINVAL,
    RPC_WSAEMFILE,
    RPC_WSAEWOULDBLOCK,
    RPC_WSAEINPROGRESS,
    RPC_WSAEALREADY,
    RPC_WSAENOTSOCK,
    RPC_WSAEDESTADDRREQ,
    RPC_WSAEMSGSIZE,
    RPC_WSAEPROTOTYPE,
    RPC_WSAENOPROTOOPT,
    RPC_WSAEPROTONOSUPPORT,
    RPC_WSAESOCKTNOSUPPORT,
    RPC_WSAEOPNOTSUPP,
    RPC_WSAEPFNOSUPPORT,
    RPC_WSAEAFNOSUPPORT,
    RPC_WSAEADDRINUSE,
    RPC_WSAEADDRNOTAVAIL,
    RPC_WSAENETDOWN,
    RPC_WSAENETUNREACH,
    RPC_WSAENETRESET,
    RPC_WSAECONNABORTED,
    RPC_WSAECONNRESET,
    RPC_WSAENOBUFS,
    RPC_WSAEISCONN,
    RPC_WSAENOTCONN,
    RPC_WSAESHUTDOWN,
    RPC_WSAETIMEDOUT,
    RPC_WSAECONNREFUSED,
    RPC_WSAEHOSTDOWN,
    RPC_WSAEHOSTUNREACH,
    RPC_WSAEPROCLIM,
    RPC_WSASYSNOTREADY,
    RPC_WSAVERNOTSUPPORTED,
    RPC_WSANOTINITIALISED,
    RPC_WSAEDISCON,
    RPC_WSATYPE_NOT_FOUND,
    RPC_WSAHOST_NOT_FOUND,
    RPC_WSATRY_AGAIN,
    RPC_WSANO_RECOVERY,
    RPC_WSANO_DATA,
    RPC_WSA_INVALID_HANDLE,
    RPC_WSA_INVALID_PARAMETER,
    RPC_WSA_IO_INCOMPLETE,
    RPC_WSA_IO_PENDING,
    RPC_WSA_NOT_ENOUGH_MEMORY,
    RPC_WSA_OPERATION_ABORTED,
    RPC_WSAEINVALIDPROCTABLE,
    RPC_WSAEINVALIDPROVIDER,
    RPC_WSAEPROVIDERFAILEDINIT,
    RPC_WAIT_TIMEOUT,
    RPC_WINERROR_UNKNOWN
} rpc_win_error;


#ifdef WSAEACCES
/** Convert native Winsock Error codes to RPC Winsock Error codes */
static inline int
win_error_h2rpc(int win_err)
{
    switch (win_err)
    {
        case 0: return 0;
        H2RPC(WSAEACCES);
        H2RPC(WSAEFAULT);
        H2RPC(WSAEINVAL);
        H2RPC(WSAEMFILE);
        H2RPC(WSAEWOULDBLOCK);
        H2RPC(WSAEINPROGRESS);
        H2RPC(WSAEALREADY);
        H2RPC(WSAENOTSOCK);
        H2RPC(WSAEDESTADDRREQ);
        H2RPC(WSAEMSGSIZE);
        H2RPC(WSAEPROTOTYPE);
        H2RPC(WSAENOPROTOOPT);
        H2RPC(WSAEPROTONOSUPPORT);
        H2RPC(WSAESOCKTNOSUPPORT);
        H2RPC(WSAEOPNOTSUPP);
        H2RPC(WSAEPFNOSUPPORT);
        H2RPC(WSAEAFNOSUPPORT);
        H2RPC(WSAEADDRINUSE);
        H2RPC(WSAEADDRNOTAVAIL);
        H2RPC(WSAENETDOWN);
        H2RPC(WSAENETUNREACH);
        H2RPC(WSAENETRESET);
        H2RPC(WSAECONNABORTED);
        H2RPC(WSAECONNRESET);
        H2RPC(WSAENOBUFS);
        H2RPC(WSAEISCONN);
        H2RPC(WSAENOTCONN);
        H2RPC(WSAESHUTDOWN);
        H2RPC(WSAETIMEDOUT);
        H2RPC(WSAECONNREFUSED);
        H2RPC(WSAEHOSTDOWN);
        H2RPC(WSAEHOSTUNREACH);
        H2RPC(WSAEPROCLIM);
        H2RPC(WSASYSNOTREADY);
        H2RPC(WSAVERNOTSUPPORTED);
        H2RPC(WSANOTINITIALISED);
        H2RPC(WSAEDISCON);
        H2RPC(WSATYPE_NOT_FOUND);
        H2RPC(WSAHOST_NOT_FOUND);
        H2RPC(WSATRY_AGAIN);
        H2RPC(WSANO_RECOVERY);
        H2RPC(WSANO_DATA);
        H2RPC(WSA_INVALID_HANDLE);
        H2RPC(WSA_INVALID_PARAMETER);
        H2RPC(WSA_IO_INCOMPLETE);
        H2RPC(WSA_IO_PENDING);
        H2RPC(WSA_NOT_ENOUGH_MEMORY);
        H2RPC(WSA_OPERATION_ABORTED);
        H2RPC(WSAEINVALIDPROCTABLE);
        H2RPC(WSAEINVALIDPROVIDER);
        H2RPC(WSAEPROVIDERFAILEDINIT);
        H2RPC(WAIT_TIMEOUT);
        default: return RPC_WINERROR_UNKNOWN;
    } 
}
#endif /* WSAEACCES */

static inline const char *
win_error_rpc2str(int win_err)
{
    switch (win_err)
    {
        RPC2STR(WSAEACCES);
        RPC2STR(WSAEFAULT);
        RPC2STR(WSAEINVAL);
        RPC2STR(WSAEMFILE);
        RPC2STR(WSAEWOULDBLOCK);
        RPC2STR(WSAEINPROGRESS);
        RPC2STR(WSAEALREADY);
        RPC2STR(WSAENOTSOCK);
        RPC2STR(WSAEDESTADDRREQ);
        RPC2STR(WSAEMSGSIZE);
        RPC2STR(WSAEPROTOTYPE);
        RPC2STR(WSAENOPROTOOPT);
        RPC2STR(WSAEPROTONOSUPPORT);
        RPC2STR(WSAESOCKTNOSUPPORT);
        RPC2STR(WSAEOPNOTSUPP);
        RPC2STR(WSAEPFNOSUPPORT);
        RPC2STR(WSAEAFNOSUPPORT);
        RPC2STR(WSAEADDRINUSE);
        RPC2STR(WSAEADDRNOTAVAIL);
        RPC2STR(WSAENETDOWN);
        RPC2STR(WSAENETUNREACH);
        RPC2STR(WSAENETRESET);
        RPC2STR(WSAECONNABORTED);
        RPC2STR(WSAECONNRESET);
        RPC2STR(WSAENOBUFS);
        RPC2STR(WSAEISCONN);
        RPC2STR(WSAENOTCONN);
        RPC2STR(WSAESHUTDOWN);
        RPC2STR(WSAETIMEDOUT);
        RPC2STR(WSAECONNREFUSED);
        RPC2STR(WSAEHOSTDOWN);
        RPC2STR(WSAEHOSTUNREACH);
        RPC2STR(WSAEPROCLIM);
        RPC2STR(WSASYSNOTREADY);
        RPC2STR(WSAVERNOTSUPPORTED);
        RPC2STR(WSANOTINITIALISED);
        RPC2STR(WSAEDISCON);
        RPC2STR(WSATYPE_NOT_FOUND);
        RPC2STR(WSAHOST_NOT_FOUND);
        RPC2STR(WSATRY_AGAIN);
        RPC2STR(WSANO_RECOVERY);
        RPC2STR(WSANO_DATA);
        RPC2STR(WSA_INVALID_HANDLE);
        RPC2STR(WSA_INVALID_PARAMETER);
        RPC2STR(WSA_IO_INCOMPLETE);
        RPC2STR(WSA_IO_PENDING);
        RPC2STR(WSA_NOT_ENOUGH_MEMORY);
        RPC2STR(WSA_OPERATION_ABORTED);
        RPC2STR(WSAEINVALIDPROCTABLE);
        RPC2STR(WSAEINVALIDPROVIDER);
        RPC2STR(WSAEPROVIDERFAILEDINIT);
        RPC2STR(WAIT_TIMEOUT);
        case 0: return "";
        default: return "WINERROR_UNKNOWN";
    } 
}

#endif /* !__TE_RPC_WSA_H__ */
