/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from WinSock2.
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
 
#ifndef __TE_RPC_WSA_H__
#define __TE_RPC_WSA_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * network event flag bits. 
 */
typedef enum rpc_network_event_bit {
    RPC_FD_READ_BIT      = 0,
    RPC_FD_WRITE_BIT     = 1,     
    RPC_FD_OOB_BIT       = 2,    
    RPC_FD_ACCEPT_BIT    = 3,   
    RPC_FD_CONNECT_BIT   = 4, 
    RPC_FD_CLOSE_BIT     = 5,  
    RPC_FD_QOS_BIT       = 6, 
    RPC_FD_GROUP_QOS_BIT = 7,
    RPC_FD_ROUTING_INTERFACE_CHANGE_BIT = 8,
    RPC_FD_ADDRESS_LIST_CHANGE_BIT      = 9
    
} rpc_network_event_bit;

#define RPC_FD_MAX_EVENTS 10

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

/** Convert RPC network event(s) to string */
extern const char * network_event_rpc2str(rpc_network_event events);

/** Convert RPC network evenet flags to native flags */
extern unsigned int network_event_rpc2h(rpc_network_event flags);

/** Convert native network evenet flags to RPC flags */
extern rpc_network_event network_event_h2rpc(unsigned int flags);


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

#define TRANSMIT_FILE_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(TF_DISCONNECT),          \
            RPC_BIT_MAP_ENTRY(TF_REUSE_SOCKET),        \
            RPC_BIT_MAP_ENTRY(TF_USE_DEFAULT_WORKER),  \
            RPC_BIT_MAP_ENTRY(TF_USE_SYSTEM_THREAD),   \
            RPC_BIT_MAP_ENTRY(TF_USE_KERNEL_APC),      \
            RPC_BIT_MAP_ENTRY(TF_WRITE_BEHIND)

/** Convert RPC transmit file flags to native flags */
extern unsigned int transmit_file_flags_rpc2h(
                        rpc_transmit_file_flags flags);


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

extern unsigned int servicetype_flags_rpc2h(rpc_servicetype_flags flags);

extern rpc_servicetype_flags servicetype_flags_h2rpc(unsigned int flags);


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
#ifndef SIO_UCAST_IF
#define SIO_UCAST_IF          _WSAIOW(IOC_VENDOR,6)
#endif
#ifndef SIO_LIMIT_BROADCASTS
#define SIO_LIMIT_BROADCASTS  _WSAIOW(IOC_VENDOR,7)
#endif
#ifndef SIO_INDEX_BIND
#define SIO_INDEX_BIND        _WSAIOW(IOC_VENDOR,8)
#endif
#ifndef SIO_ADDRESS_LIST_SORT
#define SIO_ADDRESS_LIST_SORT _WSAIORW(IOC_WS2,25)
#endif

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


/**
 * TA-independent flags for WSASocket()
 */ 
typedef enum rpc_open_sock_flags {
    RPC_WSA_FLAG_OVERLAPPED = 1,         /**< Causes an overlapped socket 
                                              to be created */
    RPC_WSA_FLAG_MULTIPOINT_C_ROOT = 2,  /**< Indicates that the socket 
                                              created will be a c_root in a 
                                              multipoint session */
    RPC_WSA_FLAG_MULTIPOINT_C_LEAF = 4,  /**< Indicates that the socket 
                                              created will be a c_leaf in a
                                              multipoint session */
    RPC_WSA_FLAG_MULTIPOINT_D_ROOT = 8,  /**< Indicates that the socket 
                                              created will be a d_root in a
                                              multipoint session */
    RPC_WSA_FLAG_MULTIPOINT_D_LEAF = 16  /**< Indicates that the socket 
                                              created will be a d_leaf in a 
                                              multipoint session */
} rpc_open_sock_flags;

#define OPEN_SOCK_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(WSA_FLAG_OVERLAPPED),               \
    RPC_BIT_MAP_ENTRY(WSA_FLAG_MULTIPOINT_C_ROOT),        \
    RPC_BIT_MAP_ENTRY(WSA_FLAG_MULTIPOINT_C_LEAF),        \
    RPC_BIT_MAP_ENTRY(WSA_FLAG_MULTIPOINT_D_ROOT),        \
    RPC_BIT_MAP_ENTRY(WSA_FLAG_MULTIPOINT_D_LEAF)         
          
/**
 * open_sock_flags_rpc2str()
 */
RPCBITMAP2STR(open_sock_flags, OPEN_SOCK_FLAGS_MAPPING_LIST)

/** Convert rpc_open_sock_flags to the native ones */
extern unsigned int open_sock_flags_rpc2h(unsigned int flags);


/**
 * TA-independent flags for WSAJoinLeaf()
 */
typedef enum rpc_join_leaf_flags {
    RPC_JL_SENDER_ONLY = 1,   /**< Indicates that the socket is acting as a 
                                   sender */
    RPC_JL_RECEIVER_ONLY = 2, /**< Indicates that the socket is acting as a 
                                   receiver */
    RPC_JL_BOTH = 4           /**< Indicates that the socket is acting both 
                                   as a sender and as a receiver */
} rpc_join_leaf_flags;

#define JOIN_LEAF_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(JL_SENDER_ONLY),          \
            RPC_BIT_MAP_ENTRY(JL_RECEIVER_ONLY),        \
            RPC_BIT_MAP_ENTRY(JL_BOTH)         
          
/**
 * join_leaf_flags_rpc2str()
 */
RPCBITMAP2STR(join_leaf_flags, JOIN_LEAF_FLAGS_MAPPING_LIST)

/** Convert rpc_join_leaf_flags to the native ones */
extern unsigned int join_leaf_flags_rpc2h(unsigned int flags);


/** Convert rpc_join_leaf_flags to string */
extern const char * join_leaf_rpc2str(rpc_join_leaf_flags open_code);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_WSA_H__ */
