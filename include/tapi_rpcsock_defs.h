/** @file
 * @brief Socket API RPC definitions
 *
 * Definition data types used in Socket API RPC.
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
 
#ifndef __TE_TAPI_RPCSOCK_DEFS_H__
#define __TE_TAPI_RPCSOCK_DEFS_H__

#include "logger_api.h"

/**
 * Coverts system native constant to its mirror in RPC namespace
 */
#define H2RPC(name_) \
    case name_: return RPC_ ## name_

/**
 * Coverts a constant from RPC namespace to system native constant
 */
#define RPC2H(name_) \
    case RPC_ ## name_: return name_

/**
 * Coverts a constant from RPC namespace to string representation 
 */
#define RPC2STR(name_) \
    case RPC_ ## name_: return #name_

/** Entry for mapping a bit of bitmask to its string value */
struct RPC_BIT_MAP_ENTRY {
    const char   *str_val; /**< String value */
    unsigned int  bit_val; /**< Numerical value */
};


/** Define one entry in the list of maping entries */
#define RPC_BIT_MAP_ENTRY(entry_val_) \
            { #entry_val_, (int)RPC_ ## entry_val_ }

#define RPCBITMAP2STR(bitmap_name_, mapping_list_) \
static inline const char *                         \
bitmap_name_ ## _rpc2str(int bitmap_name_)         \
{                                                  \
    struct RPC_BIT_MAP_ENTRY maps_[] = {           \
        mapping_list_,                             \
        { NULL, 0 }                                \
    };                                             \
    return bitmask2str(maps_, bitmap_name_);       \
}


/**
 * Convert an arbitrary bitmask to string according to 
 * the mapping passed
 *
 * @param maps  an array of mappings
 * @param val   bitmask value to be mapped
 *
 * @return String representation of bit mask
 */
static inline const char *
bitmask2str(struct RPC_BIT_MAP_ENTRY *maps, unsigned int val)
{
    /* Number of buffers used in the function */
#define N_BUFS 10
#define BUF_SIZE 1024
#define BIT_DELIMETER " | "

    static char buf[N_BUFS][BUF_SIZE];
    static char (*cur_buf)[BUF_SIZE] = (char (*)[BUF_SIZE])buf[0];

    char *ptr;
    int   i;

    /*
     * Firt time the function is called we start from the second buffer, but
     * then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[BUF_SIZE])buf[N_BUFS - 1])
        cur_buf = (char (*)[BUF_SIZE])buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;
    *ptr = '\0';

    for (i = 0; maps[i].str_val != NULL; i++)
    {
        if (val & maps[i].bit_val)
        {
            snprintf(ptr + strlen(ptr), BUF_SIZE - strlen(ptr),
                     "%s" BIT_DELIMETER, maps[i].str_val);
            /* clear processed bit */
            val &= (~maps[i].bit_val);
        }
    }
    if (val != 0)
    {
        /* There are some unprocessed bits */
        snprintf(ptr + strlen(ptr), BUF_SIZE - strlen(ptr),
                 "0x%x" BIT_DELIMETER, val);
    }

    if (strlen(ptr) == 0)
    {
        snprintf(ptr, BUF_SIZE, "0");
    }
    else
    {
        ptr[strlen(ptr) - strlen(BIT_DELIMETER)] = '\0';
    }

    return ptr;

#undef BIT_DELIMETER
#undef N_BUFS
#undef BUF_SIZE
}

#ifndef AF_LOCAL        
#define AF_LOCAL        AF_UNIX
#define PF_LOCAL        AF_LOCAL
#endif

/**
 * TA-independent protocol families.
 */
typedef enum rpc_socket_domain {
    RPC_PF_UNKNOWN, /**< Protocol family unknown to RPC server sockets */
    RPC_PF_INET,    /**< IPv4 */
    RPC_PF_INET6,   /**< IPv6 */
    RPC_PF_PACKET,  /**< Low level packet interface */
    RPC_PF_LOCAL,   /**< Local communication */
    RPC_PF_UNIX,    /**< Synonym of RPC_PF_LOCAL */
    RPC_PF_UNSPEC   /**< Unspecified */
} rpc_socket_domain;

/**
 * TA-independent address families.
 */
typedef enum rpc_socket_addr_family {
    RPC_AF_UNKNOWN, /**< Address family unknown to RPC server sockets */
    RPC_AF_INET,    /**< IPv4 */
    RPC_AF_INET6,   /**< IPv6 */
    RPC_AF_PACKET,  /**< Low level packet interface */
    RPC_AF_LOCAL,   /**< Local communication */
    RPC_AF_UNIX,    /**< Synonym of RPC_AF_LOCAL */
    RPC_AF_ETHER,   /**< Non-standard family for Ethernet addresses */
    RPC_AF_UNSPEC   /**< Unspecified */
} rpc_socket_addr_family;

/** Convert RPC domain to native domain */
static inline int
domain_rpc2h(rpc_socket_domain domain)
{
    switch (domain)
    {
        RPC2H(PF_INET);
        RPC2H(PF_INET6);
#ifdef PF_PACKET
        RPC2H(PF_PACKET);
#endif
        RPC2H(PF_LOCAL);
        RPC2H(PF_UNIX);
        RPC2H(PF_UNSPEC);
        default: return PF_MAX;
    }
}

/** Convert RPC domain to string */
static inline const char *
domain_rpc2str(rpc_socket_domain domain)
{
    switch (domain)
    {
        RPC2STR(PF_INET);
        RPC2STR(PF_INET6);
#ifdef PF_PACKET
        RPC2STR(PF_PACKET);
#endif
        RPC2STR(PF_LOCAL);
        RPC2STR(PF_UNIX);
        RPC2STR(PF_UNSPEC);
        RPC2STR(PF_UNKNOWN);

        /*
         * We should never reach the code below, because all value of the
         * enum have already checked.
         */
        default: return "<PF_FATAL_ERROR>";
    }
}

/** Convert RPC address family to native address family */
static inline int
addr_family_rpc2h(rpc_socket_addr_family addr_family)
{
    switch (addr_family)
    {
        RPC2H(AF_INET);
        RPC2H(AF_INET6);
#ifdef AF_PACKET
        RPC2H(AF_PACKET);
#endif
        RPC2H(AF_LOCAL);
        RPC2H(AF_UNIX);
        RPC2H(AF_UNSPEC);
        case RPC_AF_ETHER: return AF_LOCAL;
        case RPC_AF_UNKNOWN: return AF_MAX;
        default: return AF_MAX;
    }
}

/** Convert RPC address family to string */
static inline const char *
addr_family_rpc2str(rpc_socket_addr_family addr_family)
{
    switch (addr_family)
    {
        RPC2STR(AF_INET);
        RPC2STR(AF_INET6);
#ifdef AF_PACKET
        RPC2STR(AF_PACKET);
#endif
        RPC2STR(AF_LOCAL);
        RPC2STR(AF_UNIX);
        RPC2STR(AF_UNSPEC);
        RPC2STR(AF_UNKNOWN);
        RPC2STR(AF_ETHER);

        /*
         * We should never reach the code below, because all value of the
         * enum have already checked.
         */
        default: return "<AF_FATAL_ERROR>";
    }
}

/** Convert native domain to RPC domain */
static inline rpc_socket_domain
domain_h2rpc(int domain)
{
    switch (domain)
    {
        H2RPC(PF_INET);
        H2RPC(PF_INET6);
#ifdef PF_PACKET
        H2RPC(PF_PACKET);
#endif
        H2RPC(PF_UNIX);
        /* PF_UNIX is equal to PF_LOCAL */
        H2RPC(PF_UNSPEC);
        default: return RPC_PF_UNKNOWN;
    }
}

/** Convert native address family to RPC address family */
static inline rpc_socket_addr_family
addr_family_h2rpc(int addr_family)
{
    switch (addr_family)
    {
        H2RPC(AF_INET);
        H2RPC(AF_INET6);
#ifdef AF_PACKET
        H2RPC(AF_PACKET);
#endif
        H2RPC(AF_UNSPEC);
        /* AF_UNIX is equal to AF_LOCAL */
        case AF_LOCAL: return RPC_AF_ETHER;
        default: return RPC_AF_UNKNOWN;
    }
}

/**
 * TA-independent types of sockets (the communication semantics).
 */
typedef enum rpc_socket_type {
    RPC_SOCK_UNSPEC,    /**< Unspecified */
    RPC_SOCK_UNKNOWN,   /**< Socket type unknown to RPC server sockets */
    RPC_SOCK_DGRAM,     /**< SOCK_DGRAM in BSD */
    RPC_SOCK_STREAM,    /**< SOCK_STREAM in BSD */
    RPC_SOCK_RAW,       /**< SOCK_RAW in BSD */
    RPC_SOCK_SEQPACKET, /**< SOCK_SEQPACKET in BSD */
    RPC_SOCK_RDM,       /**< SOCK_RDM in BSD */
} rpc_socket_type;

/** Value corresponding to RPC_SOCK_UNKNOWN */ 
#define SOCK_MAX    0xFFFFFFFF  
#define SOCK_UNSPEC 0

/** Convert RPC socket type to native socket type */
static inline int
socktype_rpc2h(rpc_socket_type type)
{
    switch (type)
    {
        RPC2H(SOCK_DGRAM);
        RPC2H(SOCK_STREAM);
        RPC2H(SOCK_RAW);
        RPC2H(SOCK_SEQPACKET);
        RPC2H(SOCK_RDM);
        RPC2H(SOCK_UNSPEC);
        default: return SOCK_MAX;
    }
}

/** Convert RPC socket type to string */
static inline const char *
socktype_rpc2str(rpc_socket_type type)
{
    switch (type)
    {
        RPC2STR(SOCK_DGRAM);
        RPC2STR(SOCK_STREAM);
        RPC2STR(SOCK_RAW);
        RPC2STR(SOCK_SEQPACKET);
        RPC2STR(SOCK_RDM);
        RPC2STR(SOCK_UNSPEC);
        RPC2STR(SOCK_UNKNOWN);
        
        /*
         * We should never reach the code below, because all value of the
         * enum have already checked.
         */
        default: return "<SOCK_FATAL_ERROR>";
    }
}

/** Convert native socket type to RPC socket type */
static inline rpc_socket_type
socktype_h2rpc(int type)
{
    switch (type)
    {
        H2RPC(SOCK_DGRAM);
        H2RPC(SOCK_STREAM);
        H2RPC(SOCK_RAW);
        H2RPC(SOCK_SEQPACKET);
        H2RPC(SOCK_RDM);
        H2RPC(SOCK_UNSPEC);
        default: return RPC_SOCK_UNKNOWN;
    }
}

/**
 * TA-independent constants for IP protocols.
 */
typedef enum rpc_socket_proto {
    RPC_PROTO_UNKNOWN,  /**< IP protocol unknown to RPC server sockets */
    RPC_PROTO_DEF,      /**< Default protocol (0) */
    RPC_IPPROTO_IP,     /**< IPv4 protocol */
    RPC_IPPROTO_ICMP,   /**< Internet Control Message Protocol */
    RPC_IPPROTO_TCP,    /**< Transmission Control Protocol */
    RPC_IPPROTO_UDP,    /**< User Datagram Protocol */
} rpc_socket_proto;

/** Convert RPC IP protocol to native IP protocol constants */
static inline int
proto_rpc2h(int proto)
{
    switch (proto)
    {
        RPC2H(IPPROTO_IP);
        RPC2H(IPPROTO_ICMP);
        RPC2H(IPPROTO_UDP);
        RPC2H(IPPROTO_TCP);
        case RPC_PROTO_DEF: return 0;
        default:            return IPPROTO_MAX;
    }
}

/** Convert RPC protocol to string */
static inline const char *
proto_rpc2str(int proto)
{
    switch (proto)
    {
        RPC2STR(IPPROTO_IP);
        RPC2STR(IPPROTO_ICMP);
        RPC2STR(IPPROTO_UDP);
        RPC2STR(IPPROTO_TCP);
        RPC2STR(PROTO_UNKNOWN);
        case RPC_PROTO_DEF: return "0";
        default:            return "<PROTO_FATAL_ERROR>";
    }
}

/** Convert native IP protocol to RPC IP protocol constants */
static inline int
proto_h2rpc(int proto)
{
    switch (proto)
    {
        H2RPC(IPPROTO_IP);
        H2RPC(IPPROTO_ICMP);
        H2RPC(IPPROTO_UDP);
        H2RPC(IPPROTO_TCP);
        default: return RPC_PROTO_UNKNOWN;
    } 
}


/**
 * TA-independent receive flags. 
 */
typedef enum rpc_recv_flags {
    RPC_MSG_OOB       = 1,     /**< Receive out-of-band data */
    RPC_MSG_PEEK      = 2,     /**< Do not remove data from the queue */
    RPC_MSG_DONTROUTE = 4,     /**< Send to directly connected network */
    RPC_MSG_DONTWAIT  = 8,     /**< Do not block */
    RPC_MSG_WAITALL   = 0x10,  /**< Block until full request is specified */
    RPC_MSG_NOSIGNAL  = 0x20,  /**< Turn off raising of SIGPIPE */
    RPC_MSG_TRUNC     = 0x40,  /**< Return the real length of the packet,
                                    even when it was longer than the passed
                                    buffer */
    RPC_MSG_CTRUNC    = 0x80,  /**< Control data lost before delivery */
    RPC_MSG_ERRQUEUE  = 0x100, /**< Queued errors should be received from
                                    the socket error queue */
    RPC_MSG_MCAST     = 0x200, /**< Datagram was received as a link-layer 
                                    multicast */
    RPC_MSG_BCAST     = 0x400, /**< Datagram was received as a link-layer 
                                    broadcast */
    RPC_MSG_UNKNOWN   = 0x200  /**< Incorrect flag */
} rpc_send_recv_flags;

/** Bitmask of all possible receive flags  */
#define RPC_MSG_ALL     (RPC_MSG_OOB | RPC_MSG_PEEK | RPC_MSG_DONTROUTE | \
                         RPC_MSG_DONTWAIT | RPC_MSG_WAITALL |             \
                         RPC_MSG_NOSIGNAL | RPC_MSG_TRUNC |               \
                         RPC_MSG_CTRUNC | RPC_MSG_ERRQUEUE |              \
                         RPC_MSG_MCAST | RPC_MSG_BCAST)

#ifdef MSG_OOB
#define HAVE_MSG_OOB    1
#else
#define HAVE_MSG_OOB    0
#define MSG_OOB         0
#endif

#ifdef MSG_PEEK
#define HAVE_MSG_PEEK   1
#else
#define HAVE_MSG_PEEK   0
#define MSG_PEEK        0
#endif

#ifdef MSG_DONTROUTE
#define HAVE_MSG_DONTROUTE      1
#else
#define HAVE_MSG_DONTROUTE      0
#define MSG_DONTROUTE           0
#endif

#ifdef MSG_DONTWAIT
#define HAVE_MSG_DONTWAIT       1
#else
#define HAVE_MSG_DONTWAIT       0
#define MSG_DONTWAIT            0
#endif

#ifdef MSG_WAITALL
#define HAVE_MSG_WAITALL        1
#else
#define HAVE_MSG_WAITALL        0
#define MSG_WAITALL             0
#endif

#ifdef MSG_NOSIGNAL
#define HAVE_MSG_NOSIGNAL   1
#else
#define HAVE_MSG_NOSIGNAL   0
#define MSG_NOSIGNAL        0
#endif

#ifdef MSG_TRUNC
#define HAVE_MSG_TRUNC   1
#else
#ifdef MSG_PARTIAL
#define MSG_TRUNC MSG_PARTIAL
#define HAVE_MSG_TRUNC   1
#else
#define HAVE_MSG_TRUNC   0
#define MSG_TRUNC        0
#endif
#endif

#ifdef MSG_CTRUNC
#define HAVE_MSG_CTRUNC      1
#else
#define HAVE_MSG_CTRUNC      0
#define MSG_CTRUNC           0
#endif

#ifdef MSG_ERRQUEUE
#define HAVE_MSG_ERRQUEUE   1
#else
#define HAVE_MSG_ERRQUEUE   0
#define MSG_ERRQUEUE        0
#endif

#ifdef MSG_MCAST
#define HAVE_MSG_MCAST      1
#else
#define HAVE_MSG_MCAST      0
#define MSG_MCAST           0
#endif

#ifdef MSG_BCAST
#define HAVE_MSG_BCAST      1
#else
#define HAVE_MSG_BCAST      0
#define MSG_BCAST           0
#endif

#define SEND_RECV_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(MSG_OOB),         \
            RPC_BIT_MAP_ENTRY(MSG_PEEK),        \
            RPC_BIT_MAP_ENTRY(MSG_DONTROUTE),   \
            RPC_BIT_MAP_ENTRY(MSG_DONTWAIT),    \
            RPC_BIT_MAP_ENTRY(MSG_WAITALL),     \
            RPC_BIT_MAP_ENTRY(MSG_NOSIGNAL),    \
            RPC_BIT_MAP_ENTRY(MSG_TRUNC),       \
            RPC_BIT_MAP_ENTRY(MSG_CTRUNC),      \
            RPC_BIT_MAP_ENTRY(MSG_ERRQUEUE),    \
            RPC_BIT_MAP_ENTRY(MSG_MCAST),       \
            RPC_BIT_MAP_ENTRY(MSG_BCAST),       \
            RPC_BIT_MAP_ENTRY(MSG_UNKNOWN)

#define MSG_MAX         0xFFFFFFFF

/** All flags supported on the host platform */
#define MSG_ALL         (MSG_OOB | MSG_PEEK | MSG_DONTROUTE |   \
                         MSG_DONTWAIT | MSG_WAITALL |           \
                         MSG_NOSIGNAL | MSG_TRUNC |             \
                         MSG_CTRUNC | MSG_ERRQUEUE |            \
                         MSG_MCAST | MSG_BCAST)

/**
 * Generate warning in the log, if requested RPC mask contains flag
 * unsupported by the host platform.
 *
 * @param _flags    mask
 * @param _f        flag
 */
#define WARN_IF_UNSUPP(_flags, _f) \
    do {                                                                \
        if (!HAVE_##_f && ((_flags) & RPC_##_f))                        \
            WARN("Unsupported flag " #_f " is specified in the mask");  \
    } while (0)

/** Convert RPC send/receive flags to native flags */
static inline int
send_recv_flags_rpc2h(rpc_send_recv_flags flags)
{
#if 0
    WARN_IF_UNSUPP(flags, MSG_NOSIGNAL);
    WARN_IF_UNSUPP(flags, MSG_ERRQUEUE);
    WARN_IF_UNSUPP(flags, MSG_MCAST);
    WARN_IF_UNSUPP(flags, MSG_BCAST);
#endif

    return 
           (!!(flags & RPC_MSG_OOB) * MSG_OOB) |
           (!!(flags & RPC_MSG_PEEK) * MSG_PEEK) |
           (!!(flags & RPC_MSG_DONTROUTE) * MSG_DONTROUTE) |
           (!!(flags & RPC_MSG_DONTWAIT) * MSG_DONTWAIT) |
           (!!(flags & RPC_MSG_WAITALL) * MSG_WAITALL) |
           (!!(flags & RPC_MSG_NOSIGNAL) * MSG_NOSIGNAL) |
           (!!(flags & RPC_MSG_TRUNC) * MSG_TRUNC) |
           (!!(flags & RPC_MSG_CTRUNC) * MSG_CTRUNC) |
           (!!(flags & RPC_MSG_ERRQUEUE) * MSG_ERRQUEUE) |
           (!!(flags & RPC_MSG_MCAST) * MSG_MCAST) |
           (!!(flags & RPC_MSG_BCAST) * MSG_BCAST) |
           (!!(flags & RPC_MSG_UNKNOWN) * MSG_MAX) |
           (!!(flags & ~RPC_MSG_ALL) * MSG_MAX);
}

/** Convert native send/receive flags to RPC flags */
static inline rpc_send_recv_flags
send_recv_flags_h2rpc(int flags)
{
    return (!!(flags & MSG_OOB) * RPC_MSG_OOB) |
           (!!(flags & MSG_PEEK) * RPC_MSG_PEEK) |
           (!!(flags & MSG_DONTROUTE) * RPC_MSG_DONTROUTE) |
           (!!(flags & MSG_DONTWAIT) * RPC_MSG_DONTWAIT) |
           (!!(flags & MSG_WAITALL) * RPC_MSG_WAITALL) |
           (!!(flags & MSG_NOSIGNAL) * RPC_MSG_NOSIGNAL) |
           (!!(flags & MSG_TRUNC) * RPC_MSG_TRUNC) |
           (!!(flags & MSG_CTRUNC) * RPC_MSG_CTRUNC) |
           (!!(flags & MSG_MCAST) * RPC_MSG_MCAST) |
           (!!(flags & MSG_BCAST) * RPC_MSG_BCAST) |
           (!!(flags & MSG_ERRQUEUE) * RPC_MSG_ERRQUEUE) |
           (!!(flags & ~MSG_ALL) * RPC_MSG_UNKNOWN);
}


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
 * send_recv_flags_rpc2str()
 */
RPCBITMAP2STR(send_recv_flags, SEND_RECV_FLAGS_MAPPING_LIST)

#undef HAVE_MSG_NOSIGNAL
#undef HAVE_MSG_ERRQUEUE
#undef HAVE_MSG_MCAST
#undef HAVE_MSG_BCAST


/**
 * TA-independent types of socket shut down.
 */
typedef enum rpc_shut_how {
    RPC_SHUT_UNKNOWN,   /**< Shut down type unknown to RPC server sockets */
    RPC_SHUT_RD,        /**< Shut down for reading */
    RPC_SHUT_WR,        /**< Shut down for writing */
    RPC_SHUT_RDWR,      /**< Shut down for reading and writing */
    RPC_SHUT_NONE,      /**< This macros is used to pass to shutdown() 
                             function flag zero */
} rpc_shut_how;

/** Convert RPC protocol to string */
static inline const char *
shut_how_rpc2str(int how)
{
    switch (how)
    {
        RPC2STR(SHUT_UNKNOWN);
        RPC2STR(SHUT_RD);
        RPC2STR(SHUT_WR);
        RPC2STR(SHUT_RDWR);
        RPC2STR(SHUT_NONE);
        default: return "<SHUT_FATAL_ERROR>";
    }
}


typedef uint32_t rpc_fd_set;


typedef uint32_t rpc_sigset_t;

/* @todo Enum for poll events */

/**
 * TA-independent names of socket options.
 */
typedef enum rpc_sockopt {
    RPC_SO_ACCEPTCONN,      /**< Whether the socket is in listening
                                 state or not */
    RPC_SO_ACCEPTFILTER,    /**< Get/set accept filter of the listening
                                 socket */
    RPC_SO_BINDTODEVICE,    /**< Bind the socket to a particular 
                                 device */
    RPC_SO_BROADCAST,       /**< Permit/forbid sending of broadcast
                                 messages */
    RPC_SO_DEBUG,           /**< Turn on/off recording of debugging
                                 information */
    RPC_SO_DONTROUTE,       /**< When set, the socket is forbidden
                                 to sent via a gateway, but only to
                                 directly connected hosts */
    RPC_SO_ERROR,           /**< Reset the error status of the socket
                                 and returns the last error status */
    RPC_SO_KEEPALIVE,       /**< Enable sending of keep-alive messages
                                 on connection-oriented sockets. */
    RPC_SO_LINGER,          /**< Linger on close if data is present */
    RPC_SO_OOBINLINE,       /**< If set, out-of-band data received on
                                 the socket is placed in the normal
                                 input queue */
    RPC_SO_PRIORITY,        /**< Set the protocol-defined priority 
                                 for all packets to be sent on the
                                 socket */
    RPC_SO_RCVBUF,          /**< Get/set receive buffer size */
    RPC_SO_RCVLOWAT,        /**< Specify the minimum number of bytes
                                 in the buffer until the socket layer
                                 will pass the data to the user on
                                 receiving */
    RPC_SO_RCVTIMEO,        /**< Specify the receiving timeouts until
                                 reporting an error */
    RPC_SO_REUSEADDR,       /**< Indicate that the rules used in
                                 validating addresses supplied in a
                                 'bind' call should allow to reuse
                                 a local address */
    RPC_SO_SNDBUF,          /**< Get/set send buffer size */
    RPC_SO_SNDLOWAT,        /**< Specify the minimum number of bytes
                                 in the buffer until the socket layer
                                 will pass the data to the protocol
                                 layer on sending */
    RPC_SO_SNDTIMEO,        /**< Specify the sending timeouts until
                                 reporting an error */
    RPC_SO_TYPE,            /**< Return the socket's communication 
                                 type (the value of the second 
                                 argument used in 'socket' call) */

    RPC_IP_ADD_MEMBERSHIP,  /**< Join a multicast group */
    RPC_IP_DROP_MEMBERSHIP, /**< Leave a multicast group */
    RPC_IP_HDRINCL,         /**< If enabled, the user supplies an IP
                                 header in front of the user data */

    RPC_IP_MULTICAST_IF,    /**< Set the local device for a multicast
                                 socket */
    RPC_IP_MULTICAST_LOOP,  /**< Whether sent multicast packets
                                 should be looped back to the local
                                 sockets */
    RPC_IP_MULTICAST_TTL,   /**< Set/get TTL value used in outgoing
                                 multicast packets */
    RPC_IP_OPTIONS,         /**< Set/get the IP options to be sent 
                                 with every packet from the socket */
    RPC_IP_PKTINFO,         /**< Whether the IP_PKTINFO message
                                 should be passed or not */

#if 0                                 
    RPC_IP_RECVDSTADDR,     /**< Whether to pass destination address
                                 with UDP datagram to the user in an
                                 IP_RECVDSTADDR control message */
#endif                                 
    RPC_IP_RECVERR,         /**< Enable extended reliable error
                                 message passing */
#if 0                                 
    RPC_IP_RECVIF,          /**< Whether to pass interface index with
                                 UDP packet to the user in an 
                                 IP_RECVIF control message */
#endif                                 
    RPC_IP_RECVOPTS,        /**< Whether to pass all incoming IP
                                 header options to the user in an
                                 IP_OPTIONS control message */
    RPC_IP_RECVTOS,         /**< If enabled, the IP_TOS ancillary
                                 message is passed with incoming
                                 packets */
    RPC_IP_RECVTTL,         /**< When enabled, pass an IP_RECVTTL
                                 control message with the TTL field
                                 of the received packet as a byte */
    RPC_IP_RETOPTS,         /**< Identical to IP_RECVOPTS but returns
                                 raw unprocessed options with
                                 timestamp and route record options
                                 not filled in for this hop */
    RPC_IP_ROUTER_ALERT,    /**< Enable/disable router alerts */
    RPC_IP_TOS,             /**< Set/receive the Type-Of-Service (TOS)
                                 field that is sent with every IP
                                 packet originating from the socket */
    RPC_IP_TTL,             /**< Set/retrieve the current time to live
                                 (TTL) field that is sent in every
                                 packet originating from the socket */

    RPC_IP_MTU,             /**< Retrive the current known path MTU
                                 of the current connected socket */
    RPC_IP_MTU_DISCOVER,    /**< Enable/disable Path MTU discover
                                 on the socket */

    RPC_TCP_MAXSEG,         /**< Set/get the maximum segment size for
                                 outgoing TCP packets */
    RPC_TCP_NODELAY,        /**< Enable/disable the Nagle algorithm */
    RPC_TCP_CORK,           /**< Enable/disable delay pushing to the net */
    RPC_TCP_KEEPIDLE,       /**< Start sending keepalive probes after 
                                 this period */
    RPC_TCP_KEEPINTVL,      /**< Interval between keepalive probes */
    RPC_TCP_KEEPCNT,        /**< Number of keepalive probess before death */
    RPC_TCP_INFO,
    RPC_SOCKOPT_UNKNOWN     /**< Invalid socket option */

} rpc_sockopt;

#define RPC_SOCKOPT_MAX     0xFFFFFFFF

#if !defined(IP_MTU) && defined(MY_IP_MTU)
#define IP_MTU  MY_IP_MTU
#endif

/** Convert RPC socket option constants to native ones */
static inline int
sockopt_rpc2h(rpc_sockopt opt)
{
    switch (opt)
    {
#ifdef SO_ACCEPTCONN
        RPC2H(SO_ACCEPTCONN);
#endif
#ifdef SO_ACCEPTFILTER
        RPC2H(SO_ACCEPTFILTER);
#endif
#ifdef SO_BINDTODEVICE
        RPC2H(SO_BINDTODEVICE);
#endif
#ifdef SO_BROADCAST
        RPC2H(SO_BROADCAST);
#endif
#ifdef SO_DEBUG
        RPC2H(SO_DEBUG);
#endif
#ifdef SO_DONTROUTE
        RPC2H(SO_DONTROUTE);
#endif
#ifdef SO_ERROR
        RPC2H(SO_ERROR);
#endif
#ifdef SO_KEEPALIVE
        RPC2H(SO_KEEPALIVE);
#endif
#ifdef SO_LINGER
        RPC2H(SO_LINGER);
#endif
#ifdef SO_OOBINLINE
        RPC2H(SO_OOBINLINE);
#endif
#ifdef SO_PRIORITY
        RPC2H(SO_PRIORITY);
#endif
#ifdef SO_RCVBUF
        RPC2H(SO_RCVBUF);
#endif
#ifdef SO_RCVLOWAT
        RPC2H(SO_RCVLOWAT);
#endif
#ifdef SO_RCVTIMEO
        RPC2H(SO_RCVTIMEO);
#endif
#ifdef SO_REUSEADDR
        RPC2H(SO_REUSEADDR);
#endif
#ifdef SO_SNDBUF
        RPC2H(SO_SNDBUF);
#endif
#ifdef SO_SNDLOWAT
        RPC2H(SO_SNDLOWAT);
#endif
#ifdef SO_SNDTIMEO
        RPC2H(SO_SNDTIMEO);
#endif
#ifdef SO_TYPE
        RPC2H(SO_TYPE);
#endif
#ifdef IP_ADD_MEMBERSHIP
        RPC2H(IP_ADD_MEMBERSHIP);
#endif
#ifdef IP_DROP_MEMBERSHIP
        RPC2H(IP_DROP_MEMBERSHIP);
#endif
#ifdef IP_MULTICAST_IF
        RPC2H(IP_MULTICAST_IF);
#endif
#ifdef IP_MULTICAST_LOOP
        RPC2H(IP_MULTICAST_LOOP);
#endif
#ifdef IP_MULTICAST_TTL
        RPC2H(IP_MULTICAST_TTL);
#endif
#ifdef IP_OPTIONS
        RPC2H(IP_OPTIONS);
#endif
#ifdef IP_PKTINFO
        RPC2H(IP_PKTINFO);
#endif
#ifdef IP_RECVERR
        RPC2H(IP_RECVERR);
#endif
#ifdef IP_RECVOPTS
        RPC2H(IP_RECVOPTS);
#endif        
#ifdef IP_RECVTOS
        RPC2H(IP_RECVTOS);
#endif
#ifdef IP_RECVTTL
        RPC2H(IP_RECVTTL);
#endif
#ifdef IP_RETOPTS
        RPC2H(IP_RETOPTS);
#endif
#ifdef IP_TOS
        RPC2H(IP_TOS);
#endif
#ifdef IP_TTL
        RPC2H(IP_TTL);
#endif
#ifdef IP_MTU
        RPC2H(IP_MTU);
#endif
#ifdef IP_MTU_DISCOVER
        RPC2H(IP_MTU_DISCOVER);
#endif
#ifdef TCP_MAXSEG
        RPC2H(TCP_MAXSEG);
#endif
#ifdef TCP_NODELAY
        RPC2H(TCP_NODELAY);
#endif
#ifdef TCP_CORK
        RPC2H(TCP_CORK);
#endif
#ifdef TCP_KEEPIDLE
        RPC2H(TCP_KEEPIDLE);
#endif
#ifdef TCP_KEEPINTVL
        RPC2H(TCP_KEEPINTVL);
#endif
#ifdef TCP_KEEPCNT
        RPC2H(TCP_KEEPCNT);
#endif
#ifdef TCP_INFO
        RPC2H(TCP_INFO);
#endif
        default: return RPC_SOCKOPT_MAX;
    }
}

#if 0
/** Convert native socket options to RPC one */
static inline rpc_sockopt
sockopt_h2rpc(int opt_type, int opt)
{
    switch (opt_type)
    {
        case SOL_SOCKET:
            switch (opt)
            {
                H2RPC(SO_ACCEPTCONN);
#ifdef SO_ACCEPTFILTER
                H2RPC(SO_ACCEPTFILTER);
#endif
#ifdef SO_BINDTODEVICE
                H2RPC(SO_BINDTODEVICE);
#endif
                H2RPC(SO_BROADCAST);
                H2RPC(SO_DEBUG);
                H2RPC(SO_DONTROUTE);
                H2RPC(SO_ERROR);
                H2RPC(SO_KEEPALIVE);
                H2RPC(SO_LINGER);
                H2RPC(SO_OOBINLINE);
#ifdef SO_PRIORITY
                H2RPC(SO_PRIORITY);
#endif
                H2RPC(SO_RCVBUF);
                H2RPC(SO_RCVLOWAT);
                H2RPC(SO_RCVTIMEO);
                H2RPC(SO_REUSEADDR);
                H2RPC(SO_SNDBUF);
                H2RPC(SO_SNDLOWAT);
                H2RPC(SO_SNDTIMEO);
                H2RPC(SO_TYPE);
                default: return RPC_SOCKOPT_MAX;
            }
            break;

        case SOL_TCP:
            switch (opt)
            {
                H2RPC(TCP_MAXSEG);
                H2RPC(TCP_NODELAY);
#ifdef TCP_KEEPIDLE
                H2RPC(TCP_KEEPIDLE);
#endif
#ifdef TCP_KEEPINTVL
                H2RPC(TCP_KEEPINTVL);
#endif
#ifdef TCP_KEEPCNT
                H2RPC(TCP_KEEPCNT);
#endif
#ifdef TCP_INFO
                H2RPC(TCP_INFO);
#endif
                default: return RPC_SOCKOPT_MAX;
            }
            break;
        
        case SOL_IP:
            switch (opt)
            {
                H2RPC(IP_ADD_MEMBERSHIP);
                H2RPC(IP_DROP_MEMBERSHIP);
                H2RPC(IP_MULTICAST_IF);
                H2RPC(IP_MULTICAST_LOOP);
                H2RPC(IP_MULTICAST_TTL);
                H2RPC(IP_OPTIONS);
                H2RPC(IP_PKTINFO);
                H2RPC(IP_RECVERR);
                H2RPC(IP_RECVOPTS);
                H2RPC(IP_RECVTOS);
                H2RPC(IP_RECVTTL);
                H2RPC(IP_RETOPTS);
                H2RPC(IP_TOS);
                H2RPC(IP_TTL);
#ifdef IP_MTU
                H2RPC(IP_MTU);
#endif
#ifdef IP_MTU_DISCOVER
                H2RPC(IP_MTU_DISCOVER);
#endif
                default: return RPC_SOCKOPT_MAX;
            }
            break;

        default: return RPC_SOCKOPT_MAX;
    }
}
#endif

/** Convert RPC socket option to string */
static inline const char *
sockopt_rpc2str(rpc_sockopt opt)
{
    switch (opt)
    {
        RPC2STR(SO_ACCEPTCONN);
        RPC2STR(SO_ACCEPTFILTER);
        RPC2STR(SO_BINDTODEVICE);
        RPC2STR(SO_BROADCAST);
        RPC2STR(SO_DEBUG);
        RPC2STR(SO_DONTROUTE);
        RPC2STR(SO_ERROR);
        RPC2STR(SO_KEEPALIVE);
        RPC2STR(SO_LINGER);
        RPC2STR(SO_OOBINLINE);
        RPC2STR(SO_PRIORITY);
        RPC2STR(SO_RCVBUF);
        RPC2STR(SO_RCVLOWAT);
        RPC2STR(SO_RCVTIMEO);
        RPC2STR(SO_REUSEADDR);
        RPC2STR(SO_SNDBUF);
        RPC2STR(SO_SNDLOWAT);
        RPC2STR(SO_SNDTIMEO);
        RPC2STR(SO_TYPE);
        RPC2STR(IP_ADD_MEMBERSHIP);
        RPC2STR(IP_DROP_MEMBERSHIP);
        RPC2STR(IP_MULTICAST_IF);
        RPC2STR(IP_MULTICAST_LOOP);
        RPC2STR(IP_MULTICAST_TTL);
        RPC2STR(IP_OPTIONS);
        RPC2STR(IP_PKTINFO);
        RPC2STR(IP_RECVERR);
        RPC2STR(IP_RECVOPTS);
        RPC2STR(IP_RECVTOS);
        RPC2STR(IP_RECVTTL);
        RPC2STR(IP_RETOPTS);
        RPC2STR(IP_TOS);
        RPC2STR(IP_TTL);
        RPC2STR(IP_MTU);
        RPC2STR(IP_MTU_DISCOVER);
        RPC2STR(TCP_MAXSEG);
        RPC2STR(TCP_NODELAY);
        RPC2STR(TCP_CORK);
        RPC2STR(TCP_KEEPIDLE);
        RPC2STR(TCP_KEEPINTVL);
        RPC2STR(TCP_KEEPCNT);
        RPC2STR(TCP_INFO);

        RPC2STR(SOCKOPT_UNKNOWN);
        default: return "<SOCKOPT_FATAL_ERROR>";
    }
}

/* Define some value for unknown socket level */
#ifndef SOL_MAX
#define SOL_MAX 0xFFFFFFFF
#endif

/**
 * TA-independent socket options levels
 */
typedef enum rpc_socklevel {
    RPC_SOL_SOCKET,
    RPC_SOL_IP,
    RPC_SOL_TCP,
    RPC_SOL_UNKNOWN
} rpc_socklevel;

/** Convert RPC socket option constants to native ones */
static inline int
socklevel_rpc2h(rpc_socklevel level)
{
    switch (level)
    {
        RPC2H(SOL_SOCKET);
#ifndef SOL_IP
        case RPC_SOL_IP:  return IPPROTO_IP;
#else
        RPC2H(SOL_IP);
#endif
#ifndef SOL_TCP
        case RPC_SOL_TCP: return IPPROTO_TCP;
#else
        RPC2H(SOL_TCP);
#endif
        default: return SOL_MAX;
    }
}

/** Convert RPC socket option constants to string */
static inline const char *
socklevel_rpc2str(rpc_socklevel level)
{
    switch (level)
    {
        RPC2STR(SOL_SOCKET);
        RPC2STR(SOL_IP);
        RPC2STR(SOL_TCP);
        RPC2STR(SOL_UNKNOWN);
        default: return "<SOL_FATAL_ERROR>";
    }
}

/**
 * TA-independent IOCTL codes
 */
typedef enum rpc_ioctl_code {
    RPC_SIOCGSTAMP,         /**< Return timestamp of the last packet
                                 passed to the user */
    RPC_FIOASYNC,           /**< Enable/disable asynchronous I/O mode
                                 of the socket */
    RPC_FIONBIO,            /**< Enable/disable non-blocking 
                                 for socket I/O */
    RPC_FIONREAD,           /**< Get number of immediately readable
                                 bytes */
    RPC_SIOCATMARK,         /**< Determine if the current location
                                 in the input data is pointing to
                                 out-of-band data */
    RPC_SIOCINQ,            /**< (TCP) Returns the amount of queued
                                 unread data in the receive buffer.
                                 (UDP) Returns the size of the next
                                 pending datagram. */

    RPC_SIOCSPGRP,          /**< Set the process of process group to
                                 send SIGIO or SIGURG signals to when
                                 an asynchronous I/O operation has
                                 been finished or urgent data is
                                 available */
    RPC_SIOCGPGRP,          /**< Get the current process or process
                                 group that receives SIGIO or SIGURG
                                 signals. */

    RPC_SIOCGIFCONF,        /**< Get the list of network interfaces
                                 and their configuration */
    RPC_SIOCGIFFLAGS,       /**< Get interface flags */
    RPC_SIOCSIFFLAGS,       /**< Set interface flags */
    RPC_SIOCGIFADDR,        /**< Get the network interface address */
    RPC_SIOCSIFADDR,        /**< Set the network interface address */
    RPC_SIOCGIFNETMASK,     /**< Get the network interface mask */
    RPC_SIOCSIFNETMASK,     /**< Set the network interface mask */
    RPC_SIOCGIFBRDADDR,     /**< Get the network interface broadcast
                                 address */
    RPC_SIOCSIFBRDADDR,     /**< Set the network interface broadcast
                                 address */
    RPC_SIOCGIFDSTADDR,     /**< Get destination IP address of the
                                 interface in case */
    RPC_SIOCSIFDSTADDR,     /**< Set destination IP address of the
                                 interface in case */
    RPC_SIOCGIFHWADDR,      /**< Get hardware address of a network
                                 interface */
    RPC_SIOCGIFMTU,         /**< Get the value of MTU on a network
                                 interface */
    RPC_SIOCSIFMTU,         /**< Set the value of MTU on a network
                                 interface */
    RPC_SIO_FLUSH,          /**< Flush send queue */
    RPC_SIOCSARP,           /**< Set ARP mapping */
    RPC_SIOCDARP,           /**< Delete ARP mapping */ 
    RPC_SIOCGARP,           /**< Get ARP mapping */
    RPC_SIOUNKNOWN          /**< Invalid ioctl code */
} rpc_ioctl_code; 

/* Define some value for unknown IOCTL request */
#ifndef IOCTL_MAX
#define IOCTL_MAX 0xFFFFFFFF
#endif

#if !defined(SIOCINQ) && defined(FIONREAD)
#define SIOCINQ     FIONREAD
#endif

static inline int
ioctl_rpc2h(rpc_ioctl_code code)
{
    switch (code)
    {
#ifdef SIOCGSTAMP
        RPC2H(SIOCGSTAMP);
#endif
#ifdef FIOASYNC
        RPC2H(FIOASYNC);
#endif
#ifdef FIONBIO
        RPC2H(FIONBIO);
#endif
#ifdef FIONREAD
        RPC2H(FIONREAD);
#endif
#ifdef SIOCATMARK
        RPC2H(SIOCATMARK);
#endif
#ifdef SIOCINQ
        RPC2H(SIOCINQ);
#endif
#ifdef SIOCSPGRP
        RPC2H(SIOCSPGRP);
#endif
#ifdef SIOCGPGRP
        RPC2H(SIOCGPGRP);
#endif
#ifdef SIOCGIFCONF
        RPC2H(SIOCGIFCONF);
#endif
#ifdef SIOCGIFFLAGS
        RPC2H(SIOCGIFFLAGS);
#endif
#ifdef SIOCSIFFLAGS
        RPC2H(SIOCSIFFLAGS);
#endif
#ifdef SIOCGIFADDR
        RPC2H(SIOCGIFADDR);
#endif
#ifdef SIOCSIFADDR
        RPC2H(SIOCSIFADDR);
#endif
#ifdef SIOCGIFNETMASK
        RPC2H(SIOCGIFNETMASK);
#endif
#ifdef SIOCSIFNETMASK
        RPC2H(SIOCSIFNETMASK);
#endif
#ifdef SIOCGIFBRDADDR
        RPC2H(SIOCGIFBRDADDR);
#endif
#ifdef SIOCSIFBRDADDR
        RPC2H(SIOCSIFBRDADDR);
#endif
#ifdef SIOCGIFDSTADDR
        RPC2H(SIOCGIFDSTADDR);
#endif
#ifdef SIOCGIFHWADDR
        RPC2H(SIOCSIFDSTADDR);
#endif
#ifdef SIOCGIFHWADDR
        RPC2H(SIOCGIFHWADDR);
#endif
#ifdef SIOCGIFMTU
        RPC2H(SIOCGIFMTU);
#endif
#ifdef SIOCSIFMTU
        RPC2H(SIOCSIFMTU);
#endif
#ifdef SIO_FLUSH
        RPC2H(SIO_FLUSH);
#endif
#ifdef SIOCSARP
        RPC2H(SIOCSARP);
#endif
#ifdef SIOCDARP
        RPC2H(SIOCDARP);
#endif
#ifdef SIOCGARP
        RPC2H(SIOCGARP);
#endif
        default: return IOCTL_MAX;
    }
}

/** Convert RPC ioctl requests to string */
static inline const char *
ioctl_rpc2str(rpc_ioctl_code code)
{
    switch (code)
    {
        RPC2STR(SIOCGSTAMP);
        RPC2STR(FIOASYNC);
        RPC2STR(FIONBIO);
        RPC2STR(FIONREAD);
        RPC2STR(SIOCATMARK);
        RPC2STR(SIOCINQ);
        RPC2STR(SIOCSPGRP);
        RPC2STR(SIOCGPGRP);
        RPC2STR(SIOCGIFCONF);
        RPC2STR(SIOCGIFFLAGS);
        RPC2STR(SIOCSIFFLAGS);
        RPC2STR(SIOCGIFADDR);
        RPC2STR(SIOCSIFADDR);
        RPC2STR(SIOCGIFNETMASK);
        RPC2STR(SIOCSIFNETMASK);
        RPC2STR(SIOCGIFBRDADDR);
        RPC2STR(SIOCSIFBRDADDR);
        RPC2STR(SIOCGIFDSTADDR);
        RPC2STR(SIOCSIFDSTADDR);
        RPC2STR(SIOCGIFHWADDR);
        RPC2STR(SIOCGIFMTU);
        RPC2STR(SIOCSIFMTU);
        RPC2STR(SIO_FLUSH);
        RPC2STR(SIOUNKNOWN);
        RPC2STR(SIOCSARP);
        RPC2STR(SIOCDARP);
        RPC2STR(SIOCGARP);
        default: return "<IOCTL_FATAL_ERROR>";
    }
}

/** Length of the common part of the "struct sockaddr" */
#define SA_COMMON_LEN \
    (sizeof(struct sockaddr) - sizeof(((struct sockaddr *)0)->sa_data))

/**< Maximum length of buffer for sa_data_val in tarpc_sockaddr */
#define SA_DATA_MAX_LEN  (sizeof(struct sockaddr_storage) - SA_COMMON_LEN)

/** TA-independent signal constants */
typedef enum rpc_signum {
    RPC_SIGHUP,
    RPC_SIGINT, 
    RPC_SIGQUIT, 
    RPC_SIGILL,  
    RPC_SIGABRT, 
    RPC_SIGFPE,  
    RPC_SIGKILL, 
    RPC_SIGSEGV, 
    RPC_SIGPIPE, 
    RPC_SIGALRM,
    RPC_SIGTERM, 
    RPC_SIGUSR1, 
    RPC_SIGUSR2, 
    RPC_SIGCHLD, 
    RPC_SIGCONT, 
    RPC_SIGSTOP, 
    RPC_SIGTSTP, 
    RPC_SIGTTIN, 
    RPC_SIGTTOU,
    RPC_SIGIO,
} rpc_signum;
    
#if HAVE_SIGNAL_H
/** Convert RPC signal number to the native one */
static inline int
signum_rpc2h(rpc_signum s)
{
    switch (s)
    {
        RPC2H(SIGHUP);
        RPC2H(SIGINT);
        RPC2H(SIGQUIT);
        RPC2H(SIGILL);
        RPC2H(SIGABRT);
        RPC2H(SIGFPE);
        RPC2H(SIGKILL);
        RPC2H(SIGSEGV);
        RPC2H(SIGPIPE);
        RPC2H(SIGALRM);
        RPC2H(SIGTERM);
        RPC2H(SIGUSR1);
        RPC2H(SIGUSR2);
        RPC2H(SIGCHLD);
        RPC2H(SIGCONT);
        RPC2H(SIGSTOP);
        RPC2H(SIGTSTP);
        RPC2H(SIGTTIN);
        RPC2H(SIGTTOU);
        RPC2H(SIGIO);
#if defined(SIGUNUSED)
        default: return SIGUNUSED;
#elif defined(_SIG_MAXSIG)
        default: return _SIG_MAXSIG;
#elif defined(_NSIG)
        default: return _NSIG;
#elif defined(NSIG)
        default: return NSIG;
#else
#error There is no way to convert unknown/unsupported/unused signal number!
#endif
    }
}
#endif

/** Convert RPC signal number to string */
static inline const char *
signum_rpc2str(rpc_signum s)
{
    switch (s)
    {
        RPC2STR(SIGHUP);
        RPC2STR(SIGINT);
        RPC2STR(SIGQUIT);
        RPC2STR(SIGILL);
        RPC2STR(SIGABRT);
        RPC2STR(SIGFPE);
        RPC2STR(SIGKILL);
        RPC2STR(SIGSEGV);
        RPC2STR(SIGPIPE);
        RPC2STR(SIGALRM);
        RPC2STR(SIGTERM);
        RPC2STR(SIGUSR1);
        RPC2STR(SIGUSR2);
        RPC2STR(SIGCHLD);
        RPC2STR(SIGCONT);
        RPC2STR(SIGSTOP);
        RPC2STR(SIGTSTP);
        RPC2STR(SIGTTIN);
        RPC2STR(SIGTTOU);
        RPC2STR(SIGIO);
        default: return "<SIG_FATAL_ERROR>";
    }
}

typedef enum rpc_sighow {
    RPC_SIG_BLOCK,
    RPC_SIG_UNBLOCK,
    RPC_SIG_SETMASK
} rpc_sighow;

#define SIG_INVALID     0xFFFFFFFF

/**
 * In our RPC model rpc_signal() function always returns string, that 
 * equals to a function name that currently used as the signal handler,
 * and in case there is no signal handler registered rpc_signal()
 * returns "0x00000000", so in case of error function returns NULL.
 */
#define RPC_SIG_ERR     NULL

/** Convert RPC sigprocmask() how parameter to the native one */
static inline int
sighow_rpc2h(rpc_sighow how)
{
    switch (how)
    {
        RPC2H(SIG_BLOCK);
        RPC2H(SIG_UNBLOCK);
        RPC2H(SIG_SETMASK);
        default: return SIG_INVALID;
    }
}


typedef enum rpc_poll_event {
    RPC_POLLIN       = 0x0001, /**< There is data to read */
    RPC_POLLPRI      = 0x0002, /**< There is urgent data to read */
    RPC_POLLOUT      = 0x0004, /**< Writing now will not block */
    RPC_POLLERR      = 0x0008, /**< Error condition */
    RPC_POLLHUP      = 0x0010, /**< Hung up */
    RPC_POLLNVAL     = 0x0020, /**< Invalid request: fd not open */
    RPC_POLL_UNKNOWN = 0x0040  /**< Invalid poll event */
} rpc_poll_event;

/** Invalid poll evend */
#define POLL_UNKNOWN 0xFFFF

/** All known poll events */
#define RPC_POLL_ALL        (RPC_POLLIN | RPC_POLLPRI | RPC_POLLOUT | \
                             RPC_POLLERR | RPC_POLLHUP | RPC_POLLNVAL)

/** List of mapping numerical value to string for 'rpc_poll_event' */
#define POLL_EVENT_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(POLLIN), \
            RPC_BIT_MAP_ENTRY(POLLPRI), \
            RPC_BIT_MAP_ENTRY(POLLOUT), \
            RPC_BIT_MAP_ENTRY(POLLERR), \
            RPC_BIT_MAP_ENTRY(POLLHUP), \
            RPC_BIT_MAP_ENTRY(POLLNVAL), \
            RPC_BIT_MAP_ENTRY(POLL_UNKNOWN)

/**
 * poll_event_rpc2str()
 */
RPCBITMAP2STR(poll_event, POLL_EVENT_MAPPING_LIST)


#ifdef POLLIN

/** All known poll events */
#define POLL_ALL        (POLLIN | POLLPRI | POLLOUT | \
                         POLLERR | POLLHUP | POLLNVAL)

static inline short
poll_event_rpc2h(rpc_poll_event events)
{
    if ((events & ~RPC_POLL_ALL) != 0)
        return POLL_UNKNOWN;
        
    return (!!(events & RPC_POLLIN) * POLLIN) |
           (!!(events & RPC_POLLPRI) * POLLPRI) |
           (!!(events & RPC_POLLOUT) * POLLOUT) |
           (!!(events & RPC_POLLERR) * POLLERR) |
           (!!(events & RPC_POLLHUP) * POLLHUP) |
           (!!(events & RPC_POLLNVAL) * POLLNVAL);
}

static inline rpc_poll_event
poll_event_h2rpc(short events)
{
    return (!!(events & POLLIN) * RPC_POLLIN) |
           (!!(events & POLLPRI) * RPC_POLLPRI) |
           (!!(events & POLLOUT) * RPC_POLLOUT) |
           (!!(events & POLLERR) * RPC_POLLERR) |
           (!!(events & POLLHUP) * RPC_POLLHUP) |
           (!!(events & POLLNVAL) * RPC_POLLNVAL) |
           (!!(events & ~POLL_ALL) * RPC_POLL_UNKNOWN);
}

/** Maximum number of file descriptors passed to the poll */
#define RPC_POLL_NFDS_MAX       64

#endif /* POLLIN */


/** TA-independent addrinfo flags */
typedef enum rpc_ai_flags {
    RPC_AI_PASSIVE     = 1,    /**< Socket address is intended for `bind' */
    RPC_AI_CANONNAME   = 2,    /**< Request for canonical name */
    RPC_AI_NUMERICHOST = 4,    /**< Don't use name resolution */
    RPC_AI_UNKNOWN     = 8     /**< Invalid flags */
} rpc_ai_flags;

#ifdef AI_PASSIVE

#define AI_INVALID      0xFFFFFFFF
#define AI_ALL_FLAGS    (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)

/** Convert RPC AI flags to native ones */
static inline int
ai_flags_rpc2h(rpc_ai_flags flags)
{
    return (!!(flags & RPC_AI_PASSIVE) * AI_PASSIVE) |
           (!!(flags & RPC_AI_CANONNAME) * AI_CANONNAME) |
           (!!(flags & RPC_AI_NUMERICHOST) * AI_NUMERICHOST) |
           (!!(flags & RPC_AI_UNKNOWN) * AI_INVALID);
}

/** Convert native AI flags to RPC ones */
static inline rpc_ai_flags
ai_flags_h2rpc(int flags)
{
    if ((flags & ~AI_ALL_FLAGS) != 0)
        return RPC_AI_UNKNOWN;
        
    return (!!(flags & AI_PASSIVE) * RPC_AI_PASSIVE) |
           (!!(flags & AI_CANONNAME) * RPC_AI_CANONNAME) |
           (!!(flags & AI_NUMERICHOST) * RPC_AI_NUMERICHOST);
}

/* TA-independent getaddrinfo() return codes  */
typedef enum rpc_ai_rc {
    RPC_EAI_BADFLAGS,       /**< Invalid value for `ai_flags' field */
    RPC_EAI_NONAME,         /**< NAME or SERVICE is unknown */
    RPC_EAI_AGAIN,          /**< Temporary failure in name resolution */
    RPC_EAI_FAIL,           /**< Non-recoverable failure in name res */
    RPC_EAI_NODATA,         /**< No address associated with NAME */
    RPC_EAI_FAMILY,         /**< `ai_family' not supported */
    RPC_EAI_SOCKTYPE,       /**< `ai_socktype' not supported */
    RPC_EAI_SERVICE,        /**< SERVICE not supported for `ai_socktype' */
    RPC_EAI_ADDRFAMILY,     /**< Address family for NAME not supported */
    RPC_EAI_MEMORY,         /**< Memory allocation failure */
    RPC_EAI_SYSTEM,         /**< System error returned in `errno' */
    RPC_EAI_INPROGRESS,     /**< Processing request in progress */
    RPC_EAI_CANCELED,       /**< Request canceled */
    RPC_EAI_NOTCANCELED,    /**< Request not canceled */
    RPC_EAI_ALLDONE,        /**< All requests done */
    RPC_EAI_INTR,           /**< Interrupted by a signal */
    RPC_EAI_UNKNOWN
} rpc_ai_rc;

static inline int
ai_rc_h2rpc(rpc_ai_rc rc)
{
    switch (rc)
    {
        H2RPC(EAI_BADFLAGS);
        H2RPC(EAI_NONAME);
        H2RPC(EAI_AGAIN);
        H2RPC(EAI_FAIL);
#if (EAI_NODATA != EAI_NONAME)
        H2RPC(EAI_NODATA);
#endif
        H2RPC(EAI_FAMILY);
        H2RPC(EAI_SOCKTYPE);
        H2RPC(EAI_SERVICE);
#ifdef EAI_ADDRFAMILY
        H2RPC(EAI_ADDRFAMILY);
#endif
        H2RPC(EAI_MEMORY);
        H2RPC(EAI_SYSTEM);
#if 0
        H2RPC(EAI_INPROGRESS);
        H2RPC(EAI_CANCELED);
        H2RPC(EAI_NOTCANCELED);
        H2RPC(EAI_ALLDONE);
        H2RPC(EAI_INTR);
#endif        
        case 0:  return 0;
        default: return RPC_EAI_UNKNOWN;
    }
}

#endif /* AI_PASSIVE */


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

/* arpreq flags */
typedef enum rpc_arp_flags {
    RPC_ATF_COM = 0x0001,         /**< Lookup complete */
    RPC_ATF_PERM = 0x0002,        /**< Permanent entry */
    RPC_ATF_PUBL = 0x0004,        /**< Publish entry */
    RPC_ATF_NETMASK = 0x0008,     /**< Use a netmask */
    RPC_ATF_DONTPUB = 0x0010      /**< Don't answer */
} rpc_arp_fl;    

#define ARP_FLAGS_ALL (ATF_COM | ATF_PERM | ATF_PUBL | \
                       ATF_NETMASK | ATF_DONTPUB)

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
        case 0: return "";
        default: return "WINERROR_UNKNOWN";
    } 
}


typedef enum rpc_fcntl_command {
    RPC_F_DUPFD,
    RPC_F_GETFD,
    RPC_F_SETFD,
    RPC_F_GETFL,
    RPC_F_SETFL,
    RPC_F_GETLK,
    RPC_F_SETLK,
    RPC_F_SETLKW,
    RPC_F_SETOWN,
    RPC_F_GETOWN,
    RPC_F_SETSIG,
    RPC_F_GETSIG,
    RPC_F_SETLEASE,
    RPC_F_GETLEASE,
    RPC_F_NOTIFY,
    RPC_F_UNKNOWN
} rpc_fcntl_command;

typedef enum rpc_fcntl_arg
{
    RPC_O_ASYNC = 020000,
    RPC_O_APPEND = 02000,
    RPC_O_NONBLOCK = 04000,
    RPC_FASYNC = RPC_O_ASYNC,
    RPC_O_UNKNOWN = 0x8000
}rpc_fcntl_flag; 

#define O_UNKNOWN        0xFFFFFFFF
#define RPC_FCNTL_FLAGS_ALL \
    (RPC_O_ASYNC | RPC_O_APPEND | RPC_O_NONBLOCK | RPC_FASYNC)
        
/** Convert RPC fcntl args to native ones. */
static inline int
fcntl_flag_rpc2h(rpc_fcntl_flag flags)
{
    if ((flags & ~RPC_FCNTL_FLAGS_ALL) != 0)
        return O_UNKNOWN;
    if (flags == 0)
        return 0;
    return 0 
#ifdef O_ASYNC
        | (!!(flags & RPC_O_ASYNC) * O_ASYNC)
#endif
#ifdef O_APPEND
        | (!!(flags & RPC_O_APPEND) * O_APPEND) 
#endif
#ifdef O_NONBLOCK
        | (!!(flags & RPC_O_NONBLOCK) * O_NONBLOCK) 
#endif
#ifdef FASYNC
        | (!!(flags & RPC_FASYNC) * FASYNC)
#endif
        ;
}

/** Convert RPC fcntl args to native ones. */
static inline rpc_fcntl_flag
fcntl_flag_h2rpc(int flags)
{
    UNUSED(flags);
    return 0
#ifdef O_ASYNC
        | (!!(flags & O_ASYNC) * RPC_O_ASYNC) 
#endif        
#ifdef O_APPEND
        | (!!(flags & O_APPEND) * RPC_O_APPEND) 
#endif        
#ifdef O_NONBLOCK
        | (!!(flags & O_NONBLOCK) * RPC_O_NONBLOCK) 
#endif        
#ifdef FASYNC
        | (!!(flags & FASYNC) * RPC_FASYNC)
#endif
    ;        
}

#define F_UNKNOWN        0xFFFFFFFF

/** Convert RPC fcntl commands to native ones. */
static inline int
fcntl_rpc2h(rpc_fcntl_command cmd)
{
    switch(cmd)
    {
#ifdef F_DUPFD
        RPC2H(F_DUPFD);
#endif
#ifdef F_GETFD
        RPC2H(F_GETFD);
#endif
#ifdef F_SETFD
        RPC2H(F_SETFD);
#endif
#ifdef F_GETFL
        RPC2H(F_GETFL);
#endif
#ifdef F_SETFL
        RPC2H(F_SETFL);
#endif
#ifdef F_GETLK
        RPC2H(F_GETLK);
#endif
#ifdef F_SETLK
        RPC2H(F_SETLK);
#endif
#ifdef F_SETLKW
        RPC2H(F_SETLKW);
#endif
#ifdef F_GETOWN
        RPC2H(F_GETOWN);
#endif
#ifdef F_SETOWN
        RPC2H(F_SETOWN);
#endif
#ifdef F_GETSIG
        RPC2H(F_GETSIG);
#endif
#ifdef F_SETSIG
        RPC2H(F_SETSIG);
#endif
#ifdef F_GETLEASE
        RPC2H(F_GETLEASE);
#endif
#ifdef F_SETLEASE
        RPC2H(F_SETLEASE);
#endif
#ifdef F_NOTIFY
        RPC2H(F_NOTIFY);
#endif
        default: return F_UNKNOWN;
    }
}

static inline const char *
fcntl_rpc2str(rpc_fcntl_command cmd)
{
    switch (cmd)
    {
        RPC2STR(F_DUPFD);
        RPC2STR(F_GETFD);
        RPC2STR(F_SETFD);
        RPC2STR(F_GETFL);
        RPC2STR(F_SETFL);
        RPC2STR(F_GETLK);
        RPC2STR(F_SETLK);
        RPC2STR(F_SETLKW);
        RPC2STR(F_SETOWN);
        RPC2STR(F_GETOWN);
        RPC2STR(F_SETSIG);
        RPC2STR(F_GETSIG);
        RPC2STR(F_SETLEASE);
        RPC2STR(F_GETLEASE);
        RPC2STR(F_NOTIFY);
        default: return "F_UNKNOWN";
    }
}


/**
 * TA-independent sigaction() flags.
 */
typedef enum rpc_sa_flags {
    RPC_SA_NOCLDSTOP  = 1,     /**< Don't receive notification when
                                    child process stop */
    RPC_SA_ONESHOT    = 2,     /* with alias */
    RPC_SA_RESETHAND  = 2,     /**< Restore the signal action to the
                                    default state once the signal handler
                                    has been called */
    RPC_SA_ONSTACK    = 4,     /**< Call the signal handler on an alternate
                                    signal  stack */
    RPC_SA_RESTART    = 8,     /**< Make certain system calls restartable
                                    across signals */
    RPC_SA_NOMASK     = 0x10,  /* with alias */
    RPC_SA_NODEFER    = 0x10,  /**< Do not prevent the signal from being
                                    received from within its own signal
                                    handler */
    /* sa_sigaction() is not supported for now */
    RPC_SA_SIGINFO    = 0x20,  /**< In this case, sa_sigaction() should be
                                    set instead of sa_handler */
    RPC_SA_RESTORER   = 0x40,   /** < element is obsolete and
                                     should not be used
                                     (but Linux uses yet!) */
    RPC_SA_UNKNOWN    = 0x80   /**< Incorrect flag */
} rpc_sa_flags;

#if HAVE_STRUCT_SIGACTION_SA_RESTORER
#ifndef SA_RESTORER
#define SA_RESTORER       0x04000000
#define HAVE_SA_RESTORER  1
#endif
#else
#define HAVE_SA_RESTORER  0
#define SA_RESTORER       0
#endif

#ifdef SA_NOCLDSTOP
#define HAVE_SA_NOCLDSTOP 1
#else
#define HAVE_SA_NOCLDSTOP 0
#define SA_NOCLDSTOP      0
#endif

#ifdef SA_ONESHOT
#define HAVE_SA_ONESHOT   1
#else
#define HAVE_SA_ONESHOT   0
#define SA_ONESHOT        0
#endif

#ifdef SA_RESETHAND
#define HAVE_SA_RESETHAND 1
#else
#define HAVE_SA_RESETHAND 0
#define SA_RESETHAND      0
#endif

#ifdef SA_ONSTACK
#define HAVE_SA_ONSTACK   1
#else
#define HAVE_SA_ONSTACK   0
#define SA_ONSTACK        0
#endif

#ifdef SA_RESTART
#define HAVE_SA_RESTART   1
#else
#define HAVE_SA_RESTART   0
#define SA_RESTART        0
#endif

#ifdef SA_NOMASK
#define HAVE_SA_NOMASK    1
#else
#define HAVE_SA_NOMASK    0
#define SA_NOMASK         0
#endif

#ifdef SA_NODEFER
#define HAVE_SA_NODEFER   1
#else
#define HAVE_SA_NODEFER   0
#define SA_NODEFER        0
#endif

#ifdef SA_SIGINFO
#define HAVE_SA_SIGINFO   1
#else
#define HAVE_SA_SIGINFO   0
#define SA_SIGINFO        0
#endif

#define SA_FLAGS_UNKNOWN  0xFFFFFFFF

#define SA_FLAGS_ALL \
            (SA_NOCLDSTOP | SA_ONESHOT | SA_RESETHAND |  \
             SA_ONSTACK | SA_RESTART | SA_NOMASK |       \
             SA_NODEFER | SA_SIGINFO | SA_RESTORER)

#define RPC_SA_FLAGS_ALL \
            (RPC_SA_NOCLDSTOP | RPC_SA_ONESHOT | RPC_SA_RESETHAND |     \
             RPC_SA_ONSTACK | RPC_SA_RESTART | RPC_SA_NOMASK |          \
             RPC_SA_NODEFER | RPC_SA_SIGINFO | RPC_SA_RESTORER)

#define SA_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(SA_NOCLDSTOP), \
            RPC_BIT_MAP_ENTRY(SA_ONESHOT),   \
            RPC_BIT_MAP_ENTRY(SA_RESETHAND), \
            RPC_BIT_MAP_ENTRY(SA_ONSTACK),   \
            RPC_BIT_MAP_ENTRY(SA_RESTART),   \
            RPC_BIT_MAP_ENTRY(SA_NOMASK),    \
            RPC_BIT_MAP_ENTRY(SA_NODEFER),   \
            RPC_BIT_MAP_ENTRY(SA_SIGINFO),   \
            RPC_BIT_MAP_ENTRY(SA_RESTORER),  \
            RPC_BIT_MAP_ENTRY(SA_UNKNOWN)


/** Convert RPC sigaction flags to native flags */
static inline int
sigaction_flags_rpc2h(rpc_sa_flags flags)
{
    if ((flags & ~RPC_SA_FLAGS_ALL) != 0)
        return SA_FLAGS_UNKNOWN;
    return
#ifdef SA_NOCLDSTOP
        (!!(flags & RPC_SA_NOCLDSTOP) * SA_NOCLDSTOP) |
#endif
#ifdef SA_ONESHOT
        (!!(flags & RPC_SA_ONESHOT) * SA_ONESHOT) |
#endif
#ifdef SA_RESETHAND
        (!!(flags & RPC_SA_RESETHAND) * SA_RESETHAND) |
#endif
#ifdef SA_ONSTACK
        (!!(flags & RPC_SA_ONSTACK) * SA_ONSTACK) |
#endif
#ifdef SA_RESTART
        (!!(flags & RPC_SA_RESTART) * SA_RESTART) |
#endif
#ifdef SA_NOMASK
        (!!(flags & RPC_SA_NOMASK) * SA_NOMASK) |
#endif
#ifdef SA_NODEFER
        (!!(flags & RPC_SA_NODEFER) * SA_NODEFER) |
#endif
#ifdef SA_SIGINFO
        (!!(flags & RPC_SA_SIGINFO) * SA_SIGINFO) |
#endif
#ifdef SA_RESTORER
        (!!(flags & RPC_SA_RESTORER) * SA_RESTORER) |
#endif
        0;
}

/** Convert native sigaction flags to RPC flags */
static inline rpc_send_recv_flags
sigaction_flags_h2rpc(int flags)
{
    return
#ifdef SA_NOCLDSTOP
        (!!(flags & SA_NOCLDSTOP) * RPC_SA_NOCLDSTOP) |
#endif
#ifdef SA_ONESHOT
        (!!(flags & SA_ONESHOT) * RPC_SA_ONESHOT) |
#endif
#ifdef SA_RESETHAND
        (!!(flags & SA_RESETHAND) * RPC_SA_RESETHAND) |
#endif
#ifdef SA_ONSTACK
        (!!(flags & SA_ONSTACK) * RPC_SA_ONSTACK) |
#endif
#ifdef SA_RESTART
        (!!(flags & SA_RESTART) * RPC_SA_RESTART) |
#endif
#ifdef SA_NOMASK
        (!!(flags & SA_NOMASK) * RPC_SA_NOMASK) |
#endif
#ifdef SA_NODEFER
        (!!(flags & SA_NODEFER) * RPC_SA_NODEFER) |
#endif
#ifdef SA_SIGINFO
        (!!(flags & SA_SIGINFO) * RPC_SA_SIGINFO) |
#endif
#ifdef SA_RESTORER
        (!!(flags & SA_RESTORER) * RPC_SA_RESTORER) |
#endif
        (!!(flags & ~SA_FLAGS_ALL) * RPC_SA_UNKNOWN);
}

#endif /* !__TE_TAPI_RPCSOCK_DEFS_H__ */
