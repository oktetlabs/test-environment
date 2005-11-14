/** @file
 * @brief Socket API RPC definitions
 *
 * RPC analogues of definitions from sys/socket.h.
 * Socket IOCTL requests are defined here as well.
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
 
#ifndef __TE_RPC_SYS_SOCKET_H__
#define __TE_RPC_SYS_SOCKET_H__

#include "te_rpc_defs.h"


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
        RPC2STR(PF_PACKET);
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



/**
 * TA-independent send/receive flags. 
 */
typedef enum rpc_send_recv_flags {
    RPC_MSG_OOB       = 1,      /**< Receive out-of-band data */
    RPC_MSG_PEEK      = 2,      /**< Do not remove data from the queue */
    RPC_MSG_DONTROUTE = 4,      /**< Send to directly connected network */
    RPC_MSG_DONTWAIT  = 8,      /**< Do not block */
    RPC_MSG_WAITALL   = 0x10,   /**< Block until full request is
                                     specified */
    RPC_MSG_NOSIGNAL  = 0x20,   /**< Turn off raising of SIGPIPE */
    RPC_MSG_TRUNC     = 0x40,   /**< Return the real length of the packet,
                                     even when it was longer than the passed
                                     buffer */
    RPC_MSG_CTRUNC    = 0x80,   /**< Control data lost before delivery */
    RPC_MSG_ERRQUEUE  = 0x100,  /**< Queued errors should be received from
                                     the socket error queue */
    RPC_MSG_MCAST     = 0x200,  /**< Datagram was received as a link-layer 
                                     multicast */
    RPC_MSG_BCAST     = 0x400,  /**< Datagram was received as a link-layer 
                                     broadcast */
    RPC_MSG_MORE      = 0x800,  /**< The caller has more data to send */
    RPC_MSG_CONFIRM   = 0x1000, /**< Tell the link layer that forward
                                     progress happened */
    RPC_MSG_EOR       = 0x2000, /**< Terminates a record */
    RPC_MSG_UNKNOWN   = 0x8000  /**< Incorrect flag */
} rpc_send_recv_flags;

/** Bitmask of all possible receive flags  */
#define RPC_MSG_ALL     (RPC_MSG_OOB | RPC_MSG_PEEK | RPC_MSG_DONTROUTE | \
                         RPC_MSG_DONTWAIT | RPC_MSG_WAITALL |             \
                         RPC_MSG_NOSIGNAL | RPC_MSG_TRUNC |               \
                         RPC_MSG_CTRUNC | RPC_MSG_ERRQUEUE |              \
                         RPC_MSG_MORE | RPC_MSG_CONFIRM | RPC_MSG_EOR |   \
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

#ifdef MSG_MORE
#define HAVE_MSG_MORE       1
#else
#define HAVE_MSG_MORE       0
#define MSG_MORE            0
#endif

#ifdef MSG_CONFIRM
#define HAVE_MSG_CONFIRM    1
#else
#define HAVE_MSG_CONFIRM    0
#define MSG_CONFIRM         0
#endif

#ifdef MSG_EOR
#define HAVE_MSG_EOR        1
#else
#define HAVE_MSG_EOR        0
#define MSG_EOR             0
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
            RPC_BIT_MAP_ENTRY(MSG_MORE),        \
            RPC_BIT_MAP_ENTRY(MSG_CONFIRM),     \
            RPC_BIT_MAP_ENTRY(MSG_EOR),         \
            RPC_BIT_MAP_ENTRY(MSG_UNKNOWN)

/**
 * send_recv_flags_rpc2str()
 */
RPCBITMAP2STR(send_recv_flags, SEND_RECV_FLAGS_MAPPING_LIST)


#define MSG_MAX         0xFFFFFFFF

/** All flags supported on the host platform */
#define MSG_ALL         (MSG_OOB | MSG_PEEK | MSG_DONTROUTE |   \
                         MSG_DONTWAIT | MSG_WAITALL |           \
                         MSG_NOSIGNAL | MSG_TRUNC |             \
                         MSG_CTRUNC | MSG_ERRQUEUE |            \
                         MSG_MORE | MSG_CONFIRM | MSG_EOR |     \
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
static inline unsigned int
send_recv_flags_rpc2h(unsigned int flags)
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
           (!!(flags & RPC_MSG_MORE) * MSG_MORE) |
           (!!(flags & RPC_MSG_CONFIRM) * MSG_CONFIRM) |
           (!!(flags & RPC_MSG_EOR) * MSG_EOR) |
           (!!(flags & RPC_MSG_UNKNOWN) * MSG_MAX) |
           (!!(flags & ~RPC_MSG_ALL) * MSG_MAX);
}

/** Convert native send/receive flags to RPC flags */
static inline unsigned int
send_recv_flags_h2rpc(unsigned int flags)
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
           (!!(flags & MSG_MORE) * RPC_MSG_MORE) |
           (!!(flags & MSG_CONFIRM) * RPC_MSG_CONFIRM) |
           (!!(flags & MSG_EOR) * RPC_MSG_EOR) |
           (!!(flags & MSG_ERRQUEUE) * RPC_MSG_ERRQUEUE) |
           (!!(flags & ~MSG_ALL) * RPC_MSG_UNKNOWN);
}

#undef HAVE_MSG_NOSIGNAL
#undef HAVE_MSG_ERRQUEUE
#undef HAVE_MSG_MCAST
#undef HAVE_MSG_BCAST
#undef HAVE_MSG_MORE
#undef HAVE_MSG_CONFIRM
#undef HAVE_MSG_EOR


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

    RPC_IPV6_UNICAST_HOPS,  /**< Hop limit for unicast packets */

    RPC_IPV6_MULTICAST_HOPS,
                            /**< Hop limit for multicast packets */

    RPC_IPV6_MULTICAST_IF,  /**< Interface for outgoing multicast packets */
    
    RPC_IPV6_ADDRFORM,      /**< Turn AF_INET6 socket to AF_INET family */

    RPC_IPV6_PKTINFO,       /**< Whether to receive control messages
                                 on incoming datagrams */
#if 0
    RPC_IPV6_RTHDR,
    RPC_IPV6_AUTHHDR,
    RPC_IPV6_DSTOPS,
    RPC_IPV6_HOPOPTS,
    RPC_IPV6_FLOWINFO,
    RPC_IPV6_HOPLIMIT,
#endif               
    
    RPC_IPV6_MULTICAST_LOOP,
                            /**< Whether to loopback outgoing
                                 multicast datagrams */
    
    RPC_IPV6_ADD_MEMBERSHIP,
                            /**< Join a multicast group */
    
    RPC_IPV6_DROP_MEMBERSHIP,
                            /**< Leave a multicast group */
    
    RPC_IPV6_MTU,           /**< MTU of current connected socket */
    
    RPC_IPV6_MTU_DISCOVER,  /**< enable/disable Path MTU discover */

    RPC_IPV6_RECVERR,       /**< Whether to receive asyncronous
                                 error messages */

#if 0    
    RPC_IPV6_ROUTER_ALERT,
#endif        

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

/** Convert native socket options to RPC one */
static inline rpc_sockopt
sockopt_h2rpc(int opt_type, int opt)
{
    switch (opt_type)
    {
#ifdef SOL_SOCKET    
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
#endif

#ifdef SOL_TCP
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
#endif

#ifdef SOL_IP        
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
#endif

#ifdef SOL_IPV6
        case SOL_IPV6:
            switch (opt)
            {
                H2RPC(IPV6_UNICAST_HOPS);
                H2RPC(IPV6_MULTICAST_HOPS);
                H2RPC(IPV6_MULTICAST_IF);
                H2RPC(IPV6_ADDRFORM);
                H2RPC(IPV6_PKTINFO);
#if 0                
                H2RPC(IPV6_RTHDR);
                H2RPC(IPV6_AUTHHDR);
                H2RPC(IPV6_DSTOPS);
                H2RPC(IPV6_HOPOPTS);
                H2RPC(IPV6_FLOWINFO);
                H2RPC(IPV6_HOPLIMIT);
#endif                
                H2RPC(IPV6_MULTICAST_LOOP);
                H2RPC(IPV6_ADD_MEMBERSHIP);
                H2RPC(IPV6_DROP_MEMBERSHIP);
#ifdef IPV6_MTU                
                H2RPC(IPV6_MTU);
#endif
#ifdef IPV6_MTU_DISCOVER                
                H2RPC(IPV6_MTU_DISCOVER);
#endif
                H2RPC(IPV6_RECVERR);
#if 0                
                H2RPC(IPV6_ROUTER_ALERT);
#endif                

                default: return RPC_SOCKOPT_MAX;
            }
#endif                
        default: return RPC_SOCKOPT_MAX;
    }
}

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

        RPC2STR(IPV6_UNICAST_HOPS);
        RPC2STR(IPV6_MULTICAST_HOPS);
        RPC2STR(IPV6_MULTICAST_IF);
        RPC2STR(IPV6_ADDRFORM);
        RPC2STR(IPV6_PKTINFO);
#if 0                
        RPC2STR(IPV6_RTHDR);
        RPC2STR(IPV6_AUTHHDR);
        RPC2STR(IPV6_DSTOPS);
        RPC2STR(IPV6_HOPOPTS);
        RPC2STR(IPV6_FLOWINFO);
        RPC2STR(IPV6_HOPLIMIT);
#endif                
        RPC2STR(IPV6_MULTICAST_LOOP);
        RPC2STR(IPV6_ADD_MEMBERSHIP);
        RPC2STR(IPV6_DROP_MEMBERSHIP);
        RPC2STR(IPV6_MTU);
        RPC2STR(IPV6_MTU_DISCOVER);
        RPC2STR(IPV6_RECVERR);
#if 0                
        RPC2STR(IPV6_ROUTER_ALERT);
#endif

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
    RPC_SOL_IPV6,
    RPC_SOL_TCP,
    RPC_SOL_UDP,
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
#ifndef SOL_IPV6
        case RPC_SOL_IPV6: return IPPROTO_IPV6;
#else
        RPC2H(SOL_IPV6);
#endif        
#ifndef SOL_TCP
        case RPC_SOL_TCP: return IPPROTO_TCP;
#else
        RPC2H(SOL_TCP);
#endif
#ifndef SOL_UDP
        case RPC_SOL_UDP: return IPPROTO_UDP;
#else
        RPC2H(SOL_UDP);
#endif
        default: return SOL_MAX;
    }
}

/** Convert native socket option constants to RPC ones */
static inline rpc_socklevel
socklevel_h2rpc(int level)
{
    switch (level)
    {
        H2RPC(SOL_SOCKET);
#ifndef SOL_IP
        case IPPROTO_IP: return RPC_SOL_IP;
#else
        H2RPC(SOL_IP);
#endif
#ifndef SOL_IPV6
        case IPPROTO_IPV6: return RPC_SOL_IPV6;
#else
        H2RPC(SOL_IPV6);
#endif
#ifndef SOL_TCP
        case IPPROTO_TCP: return RPC_SOL_TCP;
#else
        H2RPC(SOL_TCP);
#endif
#ifndef SOL_UDP
        case IPPROTO_UDP: return RPC_SOL_UDP;
#else
        H2RPC(SOL_UDP);
#endif
        default: return RPC_SOL_UNKNOWN;
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
        RPC2STR(SOL_IPV6);
        RPC2STR(SOL_TCP);
        RPC2STR(SOL_UDP);
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

#endif /* !__TE_RPC_SYS_SOCKET_H__ */
