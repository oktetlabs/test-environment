/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from net/if.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_NET_IF_H__
#define __TE_RPC_NET_IF_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

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

extern unsigned int if_fl_rpc2h(unsigned int flags);

extern unsigned int if_fl_h2rpc(unsigned int flags);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_NET_IF_H__ */
