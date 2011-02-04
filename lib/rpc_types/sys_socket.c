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

#define TE_LGR_USER     "RPC types"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __CYGWIN__
#ifndef WINDOWS
#include "winsock2.h"
#include "mswsock.h"
#include "ws2tcpip.h"
#undef ERROR
#else
INCLUDE(te_win_defs.h)
#endif
#define _NETINET_IN_H 1
extern const char *inet_ntop(int af, const void *src, char *dst,
                             socklen_t cnt);
#else

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#if HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_LINUX_UDP_H
#include <linux/udp.h>
#elif HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#if HAVE_SCSI_SG_H
#include <scsi/sg.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif
#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "te_alloc.h"
#include "te_sockaddr.h"

#ifndef WINDOWS
#include "tarpc.h"
#else
typedef int32_t tarpc_int;
typedef uint32_t tarpc_socklen_t;

struct tarpc_sin {
    uint16_t port;
    uint8_t addr[4];
};
typedef struct tarpc_sin tarpc_sin;

struct tarpc_sin6 {
    uint16_t port;
    uint32_t flowinfo;
    uint8_t addr[16];
    uint32_t scope_id;
    uint32_t src_id;
};
typedef struct tarpc_sin6 tarpc_sin6;

struct tarpc_local {
    uint8_t data[6];
};
typedef struct tarpc_local tarpc_local;

enum tarpc_socket_addr_family {
    TARPC_AF_INET = 1,
    TARPC_AF_INET6 = 2,
    TARPC_AF_LOCAL = 4,
    TARPC_AF_UNSPEC = 7,
};
typedef enum tarpc_socket_addr_family tarpc_socket_addr_family;

enum tarpc_flags {
    TARPC_SA_NOT_NULL = 0x1,
    TARPC_SA_RAW = 0x2,
    TARPC_SA_LEN_AUTO = 0x4,
};
typedef enum tarpc_flags tarpc_flags;

struct tarpc_sa_data {
    tarpc_socket_addr_family type;
    union {
        struct tarpc_sin in;
        struct tarpc_sin6 in6;
        struct tarpc_local local;
    } tarpc_sa_data_u;
};
typedef struct tarpc_sa_data tarpc_sa_data;

struct tarpc_sa {
    tarpc_flags flags;
    tarpc_socklen_t len;
    tarpc_int sa_family;
    tarpc_sa_data data;
    struct {
        u_int raw_len;
        uint8_t *raw_val;
    } raw;
};
typedef struct tarpc_sa tarpc_sa;
#endif

#include "te_rpc_defs.h"
#include "te_rpc_sys_socket.h"


/** Convert RPC domain to string */
const char *
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

/** Convert RPC domain to native domain */
int
domain_rpc2h(rpc_socket_domain domain)
{
    switch (domain)
    {
        RPC2H_CHECK(PF_UNSPEC);
        RPC2H_CHECK(PF_INET);
        RPC2H_CHECK(PF_INET6);
        RPC2H_CHECK(PF_PACKET);
        RPC2H_CHECK(PF_LOCAL);
        RPC2H_CHECK(PF_UNIX);
        default:
            WARN("%s is converted to PF_MAX(%u)",
                 domain_rpc2str(domain), PF_MAX);
            return PF_MAX;
    }
}

/** Convert native domain to RPC domain */
rpc_socket_domain
domain_h2rpc(int domain)
{
    switch (domain)
    {
        H2RPC_CHECK(PF_UNSPEC);
        H2RPC_CHECK(PF_INET);
        H2RPC_CHECK(PF_INET6);
        H2RPC_CHECK(PF_PACKET);
        H2RPC_CHECK(PF_UNIX);
        default: return RPC_PF_UNKNOWN;
    }
}

/** Convert RPC address family to string */
const char *
addr_family_rpc2str(rpc_socket_addr_family addr_family)
{
    switch (addr_family)
    {
        RPC2STR(AF_INET);
        RPC2STR(AF_INET6);
        RPC2STR(AF_PACKET);
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

/** Convert RPC address family to native address family */
int
addr_family_rpc2h(rpc_socket_addr_family addr_family)
{
    switch (addr_family)
    {
        RPC2H_CHECK(AF_INET);
        RPC2H_CHECK(AF_INET6);
        RPC2H_CHECK(AF_PACKET);
        RPC2H_CHECK(AF_LOCAL);
        RPC2H_CHECK(AF_UNIX);
        RPC2H_CHECK(AF_UNSPEC);
#ifdef AF_LOCAL        
        case RPC_AF_ETHER: return AF_LOCAL;
#endif        
        case RPC_AF_UNKNOWN: return AF_MAX;

        default:
            WARN("%s is converted to AF_MAX(%u)",
                 addr_family_rpc2str(addr_family), AF_MAX);
            return AF_MAX;
    }
}

/** Convert native address family to RPC address family */
rpc_socket_addr_family
addr_family_h2rpc(int addr_family)
{
    switch (addr_family)
    {
        H2RPC_CHECK(AF_INET);
        H2RPC_CHECK(AF_INET6);
        H2RPC_CHECK(AF_PACKET);
        H2RPC_CHECK(AF_UNSPEC);
        /* AF_UNIX is equal to AF_LOCAL */
#ifdef AF_LOCAL        
        case AF_LOCAL: return RPC_AF_ETHER;
#endif        
        default: return RPC_AF_UNKNOWN;
    }
}



/** Convert RPC socket type to string */
const char *
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


/** Value corresponding to RPC_SOCK_UNKNOWN */ 
#define SOCK_MAX    0xFFFFFFFF  
#define SOCK_UNSPEC 0

/** Convert RPC socket type to native socket type */
int
socktype_rpc2h(rpc_socket_type type)
{
    switch (type)
    {
        case RPC_SOCK_UNSPEC: return SOCK_UNSPEC;
        RPC2H_CHECK(SOCK_DGRAM);
        RPC2H_CHECK(SOCK_STREAM);
        RPC2H_CHECK(SOCK_RAW);
        RPC2H_CHECK(SOCK_SEQPACKET);
        RPC2H_CHECK(SOCK_RDM);
        default:
            WARN("%s is converted to SOCK_MAX(%u)",
                 socktype_rpc2str(type), SOCK_MAX);
            return SOCK_MAX;
    }
}

/** Convert native socket type to RPC socket type */
rpc_socket_type
socktype_h2rpc(int type)
{
    switch (type)
    {
        case SOCK_UNSPEC: return RPC_SOCK_UNSPEC;
        H2RPC_CHECK(SOCK_DGRAM);
        H2RPC_CHECK(SOCK_STREAM);
        H2RPC_CHECK(SOCK_RAW);
        H2RPC_CHECK(SOCK_SEQPACKET);
        H2RPC_CHECK(SOCK_RDM);
        default: return RPC_SOCK_UNKNOWN;
    }
}



/** Convert RPC protocol to string */
const char *
proto_rpc2str(rpc_socket_proto proto)
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

/** Convert RPC IP protocol to native IP protocol constants */
int
proto_rpc2h(rpc_socket_proto proto)
{
    switch (proto)
    {
        RPC2H_CHECK(IPPROTO_IP);
        RPC2H_CHECK(IPPROTO_ICMP);
        RPC2H_CHECK(IPPROTO_UDP);
        RPC2H_CHECK(IPPROTO_TCP);
        case RPC_PROTO_DEF: return 0;
        default:
            WARN("%s is converted to IPPROTO_MAX(%u)",
                 proto_rpc2str(proto), IPPROTO_MAX);
            return IPPROTO_MAX;
    }
}

/** Convert native IP protocol to RPC IP protocol constants */
rpc_socket_proto
proto_h2rpc(int proto)
{
    switch (proto)
    {
        H2RPC_CHECK(IPPROTO_IP);
        H2RPC_CHECK(IPPROTO_ICMP);
        H2RPC_CHECK(IPPROTO_UDP);
        H2RPC_CHECK(IPPROTO_TCP);
        default: return RPC_PROTO_UNKNOWN;
    } 
}



/** Convert RPC protocol to string */
const char *
shut_how_rpc2str(rpc_shut_how how)
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

#ifdef WINDOWS

#define MSG_DONTWAIT    0
#define MSG_WAITALL     0
#define MSG_NOSIGNAL    0
#define MSG_CTRUNC      0
#define MSG_MCAST       0
#define MSG_BCAST       0
#define MSG_MORE        0
#define MSG_ERRQUEUE    0
#define MSG_CONFIRM     0
#define MSG_EOR         0
#define MSG_WAITFORONE  0

#else

#ifndef MSG_OOB
#define MSG_OOB         0
#endif
#ifndef MSG_PEEK
#define MSG_PEEK        0
#endif
#ifndef MSG_DONTROUTE
#define MSG_DONTROUTE   0
#endif
#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT    0
#endif
#ifndef MSG_WAITALL
#define MSG_WAITALL     0
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL    0
#endif
#ifndef MSG_PARTIAL
#define MSG_PARTIAL     0
#endif
#ifndef MSG_TRUNC
#define MSG_TRUNC       0
#endif
#ifndef MSG_CTRUNC
#define MSG_CTRUNC      0
#endif
#ifndef MSG_ERRQUEUE
#define MSG_ERRQUEUE    0
#endif
#ifndef MSG_MCAST
#define MSG_MCAST       0
#endif
#ifndef MSG_BCAST
#define MSG_BCAST       0
#endif
#ifndef MSG_MORE
#define MSG_MORE        0
#endif
#ifndef MSG_CONFIRM
#define MSG_CONFIRM     0
#endif
#ifndef MSG_EOR
#define MSG_EOR         0
#endif
#ifndef MSG_WAITFORONE
#define MSG_WAITFORONE  0x10000
#endif

#endif

#define MSG_MAX         0xFFFFFFFF

/** All flags supported on the host platform */
#define MSG_ALL         (MSG_OOB | MSG_PEEK | MSG_DONTROUTE |   \
                         MSG_DONTWAIT | MSG_WAITALL |           \
                         MSG_NOSIGNAL | MSG_TRUNC |             \
                         MSG_CTRUNC | MSG_ERRQUEUE |            \
                         MSG_MORE | MSG_CONFIRM | MSG_EOR |     \
                         MSG_MCAST | MSG_BCAST | MSG_PARTIAL |  \
                         MSG_WAITFORONE)

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
unsigned int
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
           (!!(flags & RPC_MSG_PARTIAL) * MSG_PARTIAL) |
           (!!(flags & RPC_MSG_WAITFORONE) * MSG_WAITFORONE) |
           (!!(flags & RPC_MSG_UNKNOWN) * MSG_MAX) |
           (!!(flags & ~RPC_MSG_ALL) * MSG_MAX);
}

/** Convert native send/receive flags to RPC flags */
unsigned int
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
           (!!(flags & MSG_PARTIAL) * RPC_MSG_PARTIAL) |
           (!!(flags & MSG_WAITFORONE) * RPC_MSG_WAITFORONE) |
           (!!(flags & MSG_ERRQUEUE) * RPC_MSG_ERRQUEUE) |
           (!!(flags & ~MSG_ALL) * RPC_MSG_UNKNOWN);
}


/** Convert RPC socket option constant to its level. */
rpc_socklevel
rpc_sockopt2level(rpc_sockopt opt)
{
    switch (opt)
    {
        case RPC_SO_ACCEPTCONN:
        case RPC_SO_ACCEPTFILTER:
        case RPC_SO_BINDTODEVICE:
        case RPC_SO_BROADCAST:
        case RPC_SO_DEBUG:
        case RPC_SO_DONTROUTE:
        case RPC_SO_ERROR:
        case RPC_SO_KEEPALIVE:
        case RPC_SO_LINGER:
        case RPC_SO_OOBINLINE:
        case RPC_SO_PRIORITY:
        case RPC_SO_RCVBUF:
        case RPC_SO_RCVLOWAT:
        case RPC_SO_RCVTIMEO:
        case RPC_SO_REUSEADDR:
        case RPC_SO_SNDBUF:
        case RPC_SO_SNDLOWAT:
        case RPC_SO_UPDATE_ACCEPT_CONTEXT:
        case RPC_SO_UPDATE_CONNECT_CONTEXT:
        case RPC_SO_SNDTIMEO:
        case RPC_SO_TYPE:
        case RPC_SO_CONNECT_TIME:
        case RPC_SO_OPENTYPE:
        case RPC_SO_DONTLINGER:
        case RPC_SO_CONDITIONAL_ACCEPT:
        case RPC_SO_MAX_MSG_SIZE:
        case RPC_SO_USELOOPBACK:
        case RPC_SO_EXCLUSIVEADDRUSE:
        case RPC_SO_GROUP_ID:
        case RPC_SO_GROUP_PRIORITY:
        case RPC_SO_PROTOCOL_INFOA:
        case RPC_SO_PROTOCOL_INFOW:
        case RPC_SO_DGRAM_ERRIND:
        case RPC_SO_TIMESTAMP:
            return RPC_SOL_SOCKET;

        case RPC_IP_ADD_MEMBERSHIP:
        case RPC_IP_DROP_MEMBERSHIP:
        case RPC_IP_MULTICAST_IF:
        case RPC_IP_MULTICAST_LOOP:
        case RPC_IP_MULTICAST_TTL:
        case RPC_MCAST_JOIN_GROUP:
        case RPC_MCAST_LEAVE_GROUP:
        case RPC_IP_OPTIONS:
        case RPC_IP_PKTINFO:
        case RPC_IP_RECVERR:
        case RPC_IP_RECVOPTS:
        case RPC_IP_RECVTOS:
        case RPC_IP_RECVTTL:
        case RPC_IP_RETOPTS:
        case RPC_IP_ROUTER_ALERT:
        case RPC_IP_TOS:
        case RPC_IP_TTL:
        case RPC_IP_MTU:
        case RPC_IP_MTU_DISCOVER:
        case RPC_IP_RECEIVE_BROADCAST:
        case RPC_IP_DONTFRAGMENT:
            return RPC_SOL_IP;

        case RPC_IPV6_UNICAST_HOPS:
        case RPC_IPV6_MULTICAST_HOPS:
        case RPC_IPV6_MULTICAST_IF:
        case RPC_IPV6_ADDRFORM:
        case RPC_IPV6_RECVPKTINFO:
        case RPC_IPV6_PKTOPTIONS:
        case RPC_IPV6_CHECKSUM:
        case RPC_IPV6_RTHDR:
        case RPC_IPV6_AUTHHDR:
        case RPC_IPV6_DSTOPTS:
        case RPC_IPV6_HOPOPTS:
        case RPC_IPV6_FLOWINFO:
        case RPC_IPV6_RECVHOPLIMIT:
        case RPC_IPV6_NEXTHOP:
        case RPC_IPV6_MULTICAST_LOOP:
        case RPC_IPV6_ADD_MEMBERSHIP:
        case RPC_IPV6_DROP_MEMBERSHIP:
        case RPC_IPV6_MTU:
        case RPC_IPV6_MTU_DISCOVER:
        case RPC_IPV6_RECVERR:
        case RPC_IPV6_ROUTER_ALERT:
        case RPC_IPV6_V6ONLY:
        case RPC_IPV6_JOIN_ANYCAST:
        case RPC_IPV6_LEAVE_ANYCAST:
        case RPC_IPV6_IPSEC_POLICY:
        case RPC_IPV6_XFRM_POLICY:
            return RPC_SOL_IPV6;

        case RPC_TCP_MAXSEG:
        case RPC_TCP_NODELAY:
        case RPC_TCP_CORK:
        case RPC_TCP_KEEPIDLE:
        case RPC_TCP_KEEPINTVL:
        case RPC_TCP_KEEPCNT:
        case RPC_TCP_KEEPALIVE_THRESHOLD:
        case RPC_TCP_KEEPALIVE_ABORT_THRESHOLD:
        case RPC_TCP_INFO:
        case RPC_TCP_DEFER_ACCEPT:
            return RPC_SOL_TCP;

        case RPC_UDP_NOCHECKSUM:
        case RPC_UDP_CORK:
            return RPC_SOL_UDP;

        default:
            ERROR("Conversion of unknown socket option %u to level",
                  opt);
            return RPC_SOL_UNKNOWN;
    }
}

/** Convert RPC socket option to string */
const char *
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
        RPC2STR(SO_UPDATE_ACCEPT_CONTEXT);
        RPC2STR(SO_UPDATE_CONNECT_CONTEXT);
        RPC2STR(SO_SNDTIMEO);
        RPC2STR(SO_TYPE);
        RPC2STR(SO_CONNECT_TIME);
        RPC2STR(SO_OPENTYPE);
        RPC2STR(SO_DONTLINGER);
        RPC2STR(SO_CONDITIONAL_ACCEPT);
        RPC2STR(SO_MAX_MSG_SIZE);
        RPC2STR(SO_USELOOPBACK);
        RPC2STR(SO_EXCLUSIVEADDRUSE);
        RPC2STR(SO_GROUP_ID);
        RPC2STR(SO_GROUP_PRIORITY);
        RPC2STR(SO_PROTOCOL_INFOA);
        RPC2STR(SO_PROTOCOL_INFOW);
        RPC2STR(SO_DGRAM_ERRIND);
        RPC2STR(SO_TIMESTAMP);
        RPC2STR(IP_ADD_MEMBERSHIP);
        RPC2STR(IP_DROP_MEMBERSHIP);
        RPC2STR(IP_MULTICAST_IF);
        RPC2STR(IP_MULTICAST_LOOP);
        RPC2STR(IP_MULTICAST_TTL);
        RPC2STR(MCAST_JOIN_GROUP);
        RPC2STR(MCAST_LEAVE_GROUP);
        RPC2STR(IP_OPTIONS);
        RPC2STR(IP_PKTINFO);
        RPC2STR(IP_RECVERR);
        RPC2STR(IP_RECVOPTS);
        RPC2STR(IP_RECVTOS);
        RPC2STR(IP_RECVTTL);
        RPC2STR(IP_RETOPTS);
        RPC2STR(IP_ROUTER_ALERT);
        RPC2STR(IP_TOS);
        RPC2STR(IP_TTL);
        RPC2STR(IP_MTU);
        RPC2STR(IP_MTU_DISCOVER);
        RPC2STR(IP_RECEIVE_BROADCAST);
        RPC2STR(IP_DONTFRAGMENT);

        RPC2STR(IPV6_UNICAST_HOPS);
        RPC2STR(IPV6_MULTICAST_HOPS);
        RPC2STR(IPV6_MULTICAST_IF);
        RPC2STR(IPV6_ADDRFORM);
        RPC2STR(IPV6_RECVPKTINFO);
        RPC2STR(IPV6_PKTOPTIONS);
        RPC2STR(IPV6_CHECKSUM);
        RPC2STR(IPV6_RTHDR);
        RPC2STR(IPV6_AUTHHDR);
        RPC2STR(IPV6_DSTOPTS);
        RPC2STR(IPV6_HOPOPTS);
        RPC2STR(IPV6_FLOWINFO);
        RPC2STR(IPV6_RECVHOPLIMIT);
        RPC2STR(IPV6_NEXTHOP);
        RPC2STR(IPV6_MULTICAST_LOOP);
        RPC2STR(IPV6_ADD_MEMBERSHIP);
        RPC2STR(IPV6_DROP_MEMBERSHIP);
        RPC2STR(IPV6_MTU);
        RPC2STR(IPV6_MTU_DISCOVER);
        RPC2STR(IPV6_RECVERR);
        RPC2STR(IPV6_ROUTER_ALERT);
        RPC2STR(IPV6_V6ONLY);
        RPC2STR(IPV6_JOIN_ANYCAST);
        RPC2STR(IPV6_LEAVE_ANYCAST);
        RPC2STR(IPV6_IPSEC_POLICY);
        RPC2STR(IPV6_XFRM_POLICY);

        RPC2STR(TCP_MAXSEG);
        RPC2STR(TCP_NODELAY);
        RPC2STR(TCP_CORK);
        RPC2STR(TCP_KEEPIDLE);
        RPC2STR(TCP_KEEPINTVL);
        RPC2STR(TCP_KEEPCNT);
        RPC2STR(TCP_KEEPALIVE_THRESHOLD);
        RPC2STR(TCP_KEEPALIVE_ABORT_THRESHOLD);
        RPC2STR(TCP_INFO);
        RPC2STR(TCP_DEFER_ACCEPT);

        RPC2STR(UDP_NOCHECKSUM);
        RPC2STR(UDP_CORK);

        RPC2STR(SOCKOPT_UNKNOWN);
        default: return "<SOCKOPT_FATAL_ERROR>";
    }
}

#define RPC_SOCKOPT_MAX     0xFFFFFFFF

#if !defined(IP_MTU) && defined(MY_IP_MTU)
#define IP_MTU  MY_IP_MTU
#endif

/** Convert RPC socket option constants to native ones */
int
sockopt_rpc2h(rpc_sockopt opt)
{
    switch (opt)
    {
        RPC2H_CHECK(SO_ACCEPTCONN);
        RPC2H_CHECK(SO_ACCEPTFILTER);
        RPC2H_CHECK(SO_BINDTODEVICE);
        RPC2H_CHECK(SO_BROADCAST);
        RPC2H_CHECK(SO_DEBUG);
        RPC2H_CHECK(SO_DONTROUTE);
        RPC2H_CHECK(SO_ERROR);
        RPC2H_CHECK(SO_KEEPALIVE);
        RPC2H_CHECK(SO_LINGER);
        RPC2H_CHECK(SO_OOBINLINE);
        RPC2H_CHECK(SO_PRIORITY);
        RPC2H_CHECK(SO_RCVBUF);
        RPC2H_CHECK(SO_RCVLOWAT);
        RPC2H_CHECK(SO_RCVTIMEO);
        RPC2H_CHECK(SO_REUSEADDR);
        RPC2H_CHECK(SO_SNDBUF);
        RPC2H_CHECK(SO_SNDLOWAT);
        RPC2H_CHECK(SO_UPDATE_ACCEPT_CONTEXT);
        RPC2H_CHECK(SO_UPDATE_CONNECT_CONTEXT);
        RPC2H_CHECK(SO_SNDTIMEO);
        RPC2H_CHECK(SO_TYPE);
        RPC2H_CHECK(SO_CONNECT_TIME);
        RPC2H_CHECK(SO_OPENTYPE);
        RPC2H_CHECK(SO_DONTLINGER);
        RPC2H_CHECK(SO_CONDITIONAL_ACCEPT);
        RPC2H_CHECK(SO_MAX_MSG_SIZE);
        RPC2H_CHECK(SO_USELOOPBACK);
        RPC2H_CHECK(SO_EXCLUSIVEADDRUSE);
        RPC2H_CHECK(SO_GROUP_ID);
        RPC2H_CHECK(SO_GROUP_PRIORITY);
        RPC2H_CHECK(SO_PROTOCOL_INFOA);
        RPC2H_CHECK(SO_PROTOCOL_INFOW);
        RPC2H_CHECK(SO_DGRAM_ERRIND);
        RPC2H_CHECK(SO_TIMESTAMP);
        RPC2H_CHECK(IP_ADD_MEMBERSHIP);
        RPC2H_CHECK(IP_DROP_MEMBERSHIP);
        RPC2H_CHECK(IP_MULTICAST_IF);
        RPC2H_CHECK(IP_MULTICAST_LOOP);
        RPC2H_CHECK(IP_MULTICAST_TTL);
        RPC2H_CHECK(MCAST_JOIN_GROUP);
        RPC2H_CHECK(MCAST_LEAVE_GROUP);
        RPC2H_CHECK(IP_OPTIONS);
        RPC2H_CHECK(IP_PKTINFO);
        RPC2H_CHECK(IP_RECVERR);
        RPC2H_CHECK(IP_RECVOPTS);
        RPC2H_CHECK(IP_RECVTOS);
        RPC2H_CHECK(IP_RECVTTL);
        RPC2H_CHECK(IP_RETOPTS);
        RPC2H_CHECK(IP_TOS);
        RPC2H_CHECK(IP_TTL);
        RPC2H_CHECK(IP_MTU);
        RPC2H_CHECK(IP_MTU_DISCOVER);
        RPC2H_CHECK(IP_RECEIVE_BROADCAST);
        RPC2H_CHECK(IP_DONTFRAGMENT);
        RPC2H_CHECK(IPV6_ADDRFORM);
        RPC2H_CHECK(IPV6_RECVPKTINFO);
        RPC2H_CHECK(IPV6_HOPOPTS);
        RPC2H_CHECK(IPV6_DSTOPTS);
        RPC2H_CHECK(IPV6_RTHDR);
        RPC2H_CHECK(IPV6_PKTOPTIONS);
        RPC2H_CHECK(IPV6_CHECKSUM);
        RPC2H_CHECK(IPV6_RECVHOPLIMIT);
        RPC2H_CHECK(IPV6_NEXTHOP);
        RPC2H_CHECK(IPV6_AUTHHDR);
        RPC2H_CHECK(IPV6_UNICAST_HOPS);
        RPC2H_CHECK(IPV6_MULTICAST_IF);
        RPC2H_CHECK(IPV6_MULTICAST_HOPS);
        RPC2H_CHECK(IPV6_MULTICAST_LOOP);
        RPC2H_CHECK(IPV6_ADD_MEMBERSHIP);
        RPC2H_CHECK(IPV6_DROP_MEMBERSHIP);
        RPC2H_CHECK(IPV6_ROUTER_ALERT);
        RPC2H_CHECK(IPV6_MTU_DISCOVER);
        RPC2H_CHECK(IPV6_MTU);
        RPC2H_CHECK(IPV6_RECVERR);
        RPC2H_CHECK(IPV6_V6ONLY);
        RPC2H_CHECK(IPV6_JOIN_ANYCAST);
        RPC2H_CHECK(IPV6_LEAVE_ANYCAST);
        RPC2H_CHECK(IPV6_IPSEC_POLICY);
        RPC2H_CHECK(IPV6_XFRM_POLICY);
        RPC2H_CHECK(TCP_MAXSEG);
        RPC2H_CHECK(TCP_NODELAY);
        RPC2H_CHECK(TCP_CORK);
        RPC2H_CHECK(TCP_KEEPIDLE);
        RPC2H_CHECK(TCP_KEEPINTVL);
        RPC2H_CHECK(TCP_KEEPCNT);
        RPC2H_CHECK(TCP_KEEPALIVE_THRESHOLD);
        RPC2H_CHECK(TCP_KEEPALIVE_ABORT_THRESHOLD);
        RPC2H_CHECK(TCP_INFO);
        RPC2H_CHECK(TCP_DEFER_ACCEPT);
        RPC2H_CHECK(UDP_NOCHECKSUM);
        RPC2H_CHECK(UDP_CORK);
        default:
            WARN("%s is converted to RPC_SOCKOPT_MAX(%u)",
                 sockopt_rpc2str(opt), RPC_SOCKOPT_MAX);
            return RPC_SOCKOPT_MAX;
    }
}

#if !defined(SOL_IP) && defined(IPPROTO_IP)
#define SOL_IP          IPPROTO_IP
#endif

#if !defined(SOL_IPV6) && defined(IPPROTO_IPV6)
#define SOL_IPV6        IPPROTO_IPV6
#endif

#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
#define SOL_TCP         IPPROTO_TCP
#endif

#if !defined(SOL_UDP) && defined(IPPROTO_UDP)
#define SOL_UDP         IPPROTO_UDP
#endif


/** Convert native socket options to RPC one */
rpc_sockopt
sockopt_h2rpc(int opt_type, int opt)
{
    switch (opt_type)
    {
        case SOL_SOCKET:
            switch (opt)
            {
                H2RPC_CHECK(SO_ACCEPTCONN);
                H2RPC_CHECK(SO_ACCEPTFILTER);
                H2RPC_CHECK(SO_BINDTODEVICE);
                H2RPC_CHECK(SO_BROADCAST);
                H2RPC_CHECK(SO_DEBUG);
                H2RPC_CHECK(SO_DONTROUTE);
                H2RPC_CHECK(SO_ERROR);
                H2RPC_CHECK(SO_KEEPALIVE);
                H2RPC_CHECK(SO_LINGER);
                H2RPC_CHECK(SO_OOBINLINE);
                H2RPC_CHECK(SO_PRIORITY);
                H2RPC_CHECK(SO_RCVBUF);
                H2RPC_CHECK(SO_RCVLOWAT);
                H2RPC_CHECK(SO_RCVTIMEO);
                H2RPC_CHECK(SO_REUSEADDR);
                H2RPC_CHECK(SO_SNDBUF);
                H2RPC_CHECK(SO_SNDLOWAT);
                H2RPC_CHECK(SO_UPDATE_CONNECT_CONTEXT);
                H2RPC_CHECK(SO_UPDATE_ACCEPT_CONTEXT);
                H2RPC_CHECK(SO_SNDTIMEO);
                H2RPC_CHECK(SO_TYPE);
                H2RPC_CHECK(SO_CONNECT_TIME);
                H2RPC_CHECK(SO_OPENTYPE);
                H2RPC_CHECK(SO_DONTLINGER);
                H2RPC_CHECK(SO_CONDITIONAL_ACCEPT);
                H2RPC_CHECK(SO_MAX_MSG_SIZE);
                H2RPC_CHECK(SO_USELOOPBACK);
                H2RPC_CHECK(SO_EXCLUSIVEADDRUSE);
                H2RPC_CHECK(SO_GROUP_ID);
                H2RPC_CHECK(SO_GROUP_PRIORITY);
                H2RPC_CHECK(SO_PROTOCOL_INFOA);
                H2RPC_CHECK(SO_PROTOCOL_INFOW);
                H2RPC_CHECK(SO_DGRAM_ERRIND);
                H2RPC_CHECK(SO_TIMESTAMP);
                default: return RPC_SOCKOPT_MAX;
            }
            break;

        case SOL_TCP:
            switch (opt)
            {
                H2RPC_CHECK(TCP_MAXSEG);
                H2RPC_CHECK(TCP_NODELAY);
                H2RPC_CHECK(TCP_KEEPIDLE);
                H2RPC_CHECK(TCP_KEEPINTVL);
                H2RPC_CHECK(TCP_KEEPCNT);
                H2RPC_CHECK(TCP_KEEPALIVE_THRESHOLD);
                H2RPC_CHECK(TCP_KEEPALIVE_ABORT_THRESHOLD);
                H2RPC_CHECK(TCP_INFO);
                default: return RPC_SOCKOPT_MAX;
            }
            break;

        case SOL_IP:
            switch (opt)
            {
                H2RPC_CHECK(IP_ADD_MEMBERSHIP);
                H2RPC_CHECK(IP_DROP_MEMBERSHIP);
                H2RPC_CHECK(IP_MULTICAST_IF);
                H2RPC_CHECK(IP_MULTICAST_LOOP);
                H2RPC_CHECK(IP_MULTICAST_TTL);
                H2RPC_CHECK(MCAST_JOIN_GROUP);
                H2RPC_CHECK(MCAST_LEAVE_GROUP);
                H2RPC_CHECK(IP_OPTIONS);
                H2RPC_CHECK(IP_PKTINFO);
                H2RPC_CHECK(IP_RECVERR);
                H2RPC_CHECK(IP_RECVOPTS);
                H2RPC_CHECK(IP_RECVTOS);
                H2RPC_CHECK(IP_RECVTTL);
                H2RPC_CHECK(IP_RETOPTS);
                H2RPC_CHECK(IP_TOS);
                H2RPC_CHECK(IP_TTL);
                H2RPC_CHECK(IP_MTU);
                H2RPC_CHECK(IP_MTU_DISCOVER);
                H2RPC_CHECK(IP_RECEIVE_BROADCAST);
                H2RPC_CHECK(IP_DONTFRAGMENT);
                default: return RPC_SOCKOPT_MAX;
            }
            break;

        case SOL_IPV6:
            switch (opt)
            {
                H2RPC_CHECK(IPV6_UNICAST_HOPS);
                H2RPC_CHECK(IPV6_MULTICAST_HOPS);
                H2RPC_CHECK(IPV6_MULTICAST_IF);
                H2RPC_CHECK(IPV6_ADDRFORM);
                H2RPC_CHECK(IPV6_RECVPKTINFO);
                H2RPC_CHECK(IPV6_PKTOPTIONS);
                H2RPC_CHECK(IPV6_CHECKSUM);
                H2RPC_CHECK(IPV6_RTHDR);
                H2RPC_CHECK(IPV6_AUTHHDR);
                H2RPC_CHECK(IPV6_DSTOPTS);
                H2RPC_CHECK(IPV6_HOPOPTS);
                H2RPC_CHECK(IPV6_FLOWINFO);
                H2RPC_CHECK(IPV6_RECVHOPLIMIT);
                H2RPC_CHECK(IPV6_NEXTHOP);
                H2RPC_CHECK(IPV6_MULTICAST_LOOP);
                H2RPC_CHECK(IPV6_ADD_MEMBERSHIP);
                H2RPC_CHECK(IPV6_DROP_MEMBERSHIP);
                H2RPC_CHECK(IPV6_MTU);
                H2RPC_CHECK(IPV6_MTU_DISCOVER);
                H2RPC_CHECK(IPV6_RECVERR);
                H2RPC_CHECK(IPV6_V6ONLY);
                H2RPC_CHECK(IPV6_JOIN_ANYCAST);
                H2RPC_CHECK(IPV6_LEAVE_ANYCAST);
                H2RPC_CHECK(IPV6_IPSEC_POLICY);
                H2RPC_CHECK(IPV6_XFRM_POLICY);
                H2RPC_CHECK(IPV6_ROUTER_ALERT);
                default: return RPC_SOCKOPT_MAX;
            }
            break;

        case SOL_UDP:
            switch (opt)
            {
                H2RPC_CHECK(UDP_NOCHECKSUM);
                default: return RPC_SOCKOPT_MAX;
            }
            break;

        default: return RPC_SOCKOPT_MAX;
    }
}

/** Has socket option boolean semantic? */
te_bool
sockopt_is_boolean(rpc_sockopt opt)
{
    switch (opt)
    {
        case RPC_SO_ACCEPTCONN:
        case RPC_SO_ACCEPTFILTER:
        case RPC_SO_BROADCAST:
        case RPC_SO_DEBUG:
        case RPC_SO_DONTROUTE:
        case RPC_SO_KEEPALIVE:
        case RPC_SO_OOBINLINE:
        case RPC_SO_REUSEADDR:
        case RPC_SO_DONTLINGER:
        case RPC_SO_USELOOPBACK:
        case RPC_SO_EXCLUSIVEADDRUSE:
        case RPC_SO_DGRAM_ERRIND:

        case RPC_IP_MULTICAST_LOOP:
        case RPC_IP_PKTINFO:
        case RPC_IP_RECVERR:
        case RPC_IP_RECVOPTS:
        case RPC_IP_RECVTOS:
        case RPC_IP_RECVTTL:
        case RPC_IP_ROUTER_ALERT:
        case RPC_IP_MTU_DISCOVER:
        case RPC_IP_RECEIVE_BROADCAST:
        case RPC_IP_DONTFRAGMENT:

        case RPC_IPV6_RECVPKTINFO:
        case RPC_IPV6_PKTOPTIONS:
        case RPC_IPV6_CHECKSUM:
        case RPC_IPV6_MULTICAST_LOOP:
        case RPC_IPV6_MTU_DISCOVER:
        case RPC_IPV6_RECVERR:
        case RPC_IPV6_ROUTER_ALERT:
        case RPC_IPV6_V6ONLY:

        case RPC_TCP_NODELAY:
        case RPC_TCP_CORK:

        case RPC_UDP_NOCHECKSUM:
        case RPC_UDP_CORK:
            return TRUE;

        default:
            return FALSE;
    }
}


/** Convert RPC socket option constants to string */
const char *
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

/* Define some value for unknown socket level */
#ifndef SOL_MAX
#define SOL_MAX 0xFFFFFFFF
#endif

/** Convert RPC socket option constants to native ones */
int
socklevel_rpc2h(rpc_socklevel level)
{
    switch (level)
    {
        RPC2H_CHECK(SOL_SOCKET);
        RPC2H_CHECK(SOL_IP);
        RPC2H_CHECK(SOL_IPV6);
        RPC2H_CHECK(SOL_TCP);
        RPC2H_CHECK(SOL_UDP);
        default:
            WARN("%s is converted to SOL_MAX(%u)",
                 socklevel_rpc2str(level), SOL_MAX);
            return SOL_MAX;
    }
}

/** Convert native socket option constants to RPC ones */
rpc_socklevel
socklevel_h2rpc(int level)
{
    switch (level)
    {
        H2RPC_CHECK(SOL_SOCKET);
        H2RPC_CHECK(SOL_IP);
        H2RPC_CHECK(SOL_IPV6);
        H2RPC_CHECK(SOL_TCP);
        H2RPC_CHECK(SOL_UDP);
        default: return RPC_SOL_UNKNOWN;
    }
}



/** Convert RPC ioctl requests to string */
const char *
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
        RPC2STR(SIOCGIFNAME);
        RPC2STR(SIOCGIFINDEX);
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
        RPC2STR(SIOUNKNOWN);
        RPC2STR(SIOCSARP);
        RPC2STR(SIOCDARP);
        RPC2STR(SIOCGARP);
        
        RPC2STR(SG_IO);
        RPC2STR(SIOCETHTOOL);

        RPC2STR(SIO_ADDRESS_LIST_CHANGE);
        RPC2STR(SIO_ADDRESS_LIST_QUERY);
        RPC2STR(SIO_ADDRESS_LIST_SORT);
        RPC2STR(SIO_ASSOCIATE_HANDLE);
        RPC2STR(SIO_CHK_QOS);
        RPC2STR(SIO_ENABLE_CIRCULAR_QUEUEING);
        RPC2STR(SIO_FIND_ROUTE);
        RPC2STR(SIO_FLUSH);
        RPC2STR(SIO_GET_BROADCAST_ADDRESS);
        RPC2STR(SIO_GET_EXTENSION_FUNCTION_POINTER);
        RPC2STR(SIO_GET_GROUP_QOS);
        RPC2STR(SIO_GET_QOS);
        RPC2STR(SIO_KEEPALIVE_VALS);
        RPC2STR(SIO_MULTIPOINT_LOOPBACK);
        RPC2STR(SIO_MULTICAST_SCOPE);
        RPC2STR(SIO_RCVALL);
        RPC2STR(SIO_RCVALL_IGMPMCAST);
        RPC2STR(SIO_RCVALL_MCAST);
        RPC2STR(SIO_ROUTING_INTERFACE_CHANGE);
        RPC2STR(SIO_ROUTING_INTERFACE_QUERY);
        RPC2STR(SIO_SET_GROUP_QOS);
        RPC2STR(SIO_SET_QOS);
        RPC2STR(SIO_TRANSLATE_HANDLE);
        RPC2STR(SIO_UDP_CONNRESET);
        RPC2STR(SIO_INDEX_BIND);
        RPC2STR(SIO_UCAST_IF);
        
        default: return "<IOCTL_FATAL_ERROR>";
    }
}


/* Define some value for unknown IOCTL request */
#ifndef IOCTL_MAX
#define IOCTL_MAX 0x7FFFFFFF
#endif

int
ioctl_rpc2h(rpc_ioctl_code code)
{
    switch (code)
    {
        RPC2H_CHECK(SIOCGSTAMP);
        RPC2H_CHECK(FIOASYNC);
        RPC2H_CHECK(FIONBIO);
        RPC2H_CHECK(FIONREAD);
        RPC2H_CHECK(SIOCATMARK);
        RPC2H_CHECK(SIOCINQ);
        RPC2H_CHECK(SIOCSPGRP);
        RPC2H_CHECK(SIOCGPGRP);
        RPC2H_CHECK(SIOCGIFCONF);
        RPC2H_CHECK(SIOCGIFNAME);
        RPC2H_CHECK(SIOCGIFINDEX);
        RPC2H_CHECK(SIOCGIFFLAGS);
        RPC2H_CHECK(SIOCSIFFLAGS);
        RPC2H_CHECK(SIOCGIFADDR);
        RPC2H_CHECK(SIOCSIFADDR);
        RPC2H_CHECK(SIOCGIFNETMASK);
        RPC2H_CHECK(SIOCSIFNETMASK);
        RPC2H_CHECK(SIOCGIFBRDADDR);
        RPC2H_CHECK(SIOCSIFBRDADDR);
        RPC2H_CHECK(SIOCGIFDSTADDR);
        RPC2H_CHECK(SIOCSIFDSTADDR);
        RPC2H_CHECK(SIOCGIFHWADDR);
        RPC2H_CHECK(SIOCGIFMTU);
        RPC2H_CHECK(SIOCSIFMTU);
        RPC2H_CHECK(SIOCSARP);
        RPC2H_CHECK(SIOCDARP);
        RPC2H_CHECK(SIOCGARP);
        RPC2H_CHECK(SG_IO);
        RPC2H_CHECK(SIOCETHTOOL);

        RPC2H_CHECK(SIO_ADDRESS_LIST_CHANGE);
        RPC2H_CHECK(SIO_ADDRESS_LIST_QUERY);
        RPC2H_CHECK(SIO_ASSOCIATE_HANDLE);
        RPC2H_CHECK(SIO_ENABLE_CIRCULAR_QUEUEING);
        RPC2H_CHECK(SIO_FIND_ROUTE);
        RPC2H_CHECK(SIO_FLUSH);
        RPC2H_CHECK(SIO_GET_BROADCAST_ADDRESS);
        RPC2H_CHECK(SIO_GET_EXTENSION_FUNCTION_POINTER);
        RPC2H_CHECK(SIO_GET_GROUP_QOS);
        RPC2H_CHECK(SIO_GET_QOS);
        RPC2H_CHECK(SIO_MULTIPOINT_LOOPBACK);
        RPC2H_CHECK(SIO_MULTICAST_SCOPE);
        RPC2H_CHECK(SIO_ROUTING_INTERFACE_CHANGE);
        RPC2H_CHECK(SIO_ROUTING_INTERFACE_QUERY);
        RPC2H_CHECK(SIO_SET_GROUP_QOS);
        RPC2H_CHECK(SIO_SET_QOS);
        RPC2H_CHECK(SIO_TRANSLATE_HANDLE);

        RPC2H_CHECK(SIO_ADDRESS_LIST_SORT);
        RPC2H_CHECK(SIO_CHK_QOS);
        RPC2H_CHECK(SIO_KEEPALIVE_VALS);
        RPC2H_CHECK(SIO_RCVALL);
        RPC2H_CHECK(SIO_RCVALL_IGMPMCAST);
        RPC2H_CHECK(SIO_RCVALL_MCAST);
        RPC2H_CHECK(SIO_UDP_CONNRESET);
        RPC2H_CHECK(SIO_INDEX_BIND);
        RPC2H_CHECK(SIO_UCAST_IF);
        
        default:
            WARN("%s is converted to IOCTL_MAX(%u)",
                 ioctl_rpc2str(code), IOCTL_MAX);
            return IOCTL_MAX;
    }
}


/** See the description in ta_rpc_sys_socket.h */
struct sockaddr *
sockaddr_to_te_af(const struct sockaddr *addr, tarpc_sa **rpc_sa)
{
    struct sockaddr *res;

    res = TE_ALLOC(TE_OFFSET_OF(struct sockaddr, sa_data) +
                   sizeof(tarpc_sa));
    if (res == NULL)
        return NULL;

    res->sa_family = TE_AF_TARPC_SA;
    sockaddr_input_h2rpc(addr, (tarpc_sa *)res->sa_data);

    if (rpc_sa != NULL)
        *rpc_sa = (tarpc_sa *)res->sa_data;

    return res;
}

/** See the description in ta_rpc_sys_socket.h */
void
sockaddr_raw2rpc(const void *buf, socklen_t len, tarpc_sa *rpc)
{
    assert(rpc != NULL);
    memset(rpc, 0, sizeof(*rpc));

    if (buf == NULL)
    {
        assert(len == 0);
        /* Flag TARPC_SA_NOT_NULL is clear */
    }
    else
    {
        rpc->flags = TARPC_SA_RAW | TARPC_SA_NOT_NULL;
        rpc->raw.raw_len = len;
        rpc->raw.raw_val = (uint8_t *)buf;
    }
}

/** See the description in ta_rpc_sys_socket.h */
void
sockaddr_input_h2rpc(const struct sockaddr *sa, tarpc_sa *rpc)
{
    assert(rpc != NULL);
    memset(rpc, 0, sizeof(*rpc));

    if (sa == NULL)
    {
        /* Flag TARPC_SA_NOT_NULL is clear */
        return;
    }

    rpc->flags |= TARPC_SA_NOT_NULL;

    if (sa->sa_family == TE_AF_TARPC_SA)
    {
        memcpy(rpc, sa->sa_data, sizeof(*rpc));
        return;
    }

    rpc->flags |= TARPC_SA_LEN_AUTO;

    switch (sa->sa_family)
    {
        case AF_UNSPEC:
            rpc->sa_family = rpc->data.type = RPC_AF_UNSPEC;
            break;

        case AF_INET:
        {
            const struct sockaddr_in   *sin = CONST_SIN(sa);

            rpc->sa_family = rpc->data.type = RPC_AF_INET;
            rpc->data.tarpc_sa_data_u.in.port = ntohs(sin->sin_port);
            assert(sizeof(rpc->data.tarpc_sa_data_u.in.addr) ==
                   sizeof(sin->sin_addr));
            memcpy(rpc->data.tarpc_sa_data_u.in.addr, &sin->sin_addr,
                   sizeof(rpc->data.tarpc_sa_data_u.in.addr));
            break;
        }

        case AF_INET6:
        {
            const struct sockaddr_in6  *sin6 = CONST_SIN6(sa);

            rpc->sa_family = rpc->data.type = RPC_AF_INET6;
            rpc->data.tarpc_sa_data_u.in6.port = ntohs(sin6->sin6_port);
            rpc->data.tarpc_sa_data_u.in6.flowinfo = sin6->sin6_flowinfo;
            assert(sizeof(rpc->data.tarpc_sa_data_u.in6.addr) ==
                   sizeof(sin6->sin6_addr));
            memcpy(rpc->data.tarpc_sa_data_u.in6.addr, &sin6->sin6_addr,
                   sizeof(rpc->data.tarpc_sa_data_u.in6.addr));
            rpc->data.tarpc_sa_data_u.in6.scope_id = sin6->sin6_scope_id;
#if HAVE_STRUCT_SOCKADDR_IN6___SIN6_SRC_ID
            rpc->data.tarpc_sa_data_u.in6.src_id = sin6->__sin6_src_id;
#endif
            break;
        }

#ifdef AF_LOCAL
        case AF_LOCAL:
            rpc->sa_family = rpc->data.type = RPC_AF_LOCAL;
            assert(sizeof(sa->sa_data) >=
                   sizeof(rpc->data.tarpc_sa_data_u.local.data));
            memcpy(rpc->data.tarpc_sa_data_u.local.data, sa->sa_data,
                   sizeof(rpc->data.tarpc_sa_data_u.local.data));
            break;
#endif

        default:
            assert(FALSE);
            break;
    }
}

/** See the description in ta_rpc_sys_socket.h */
void
sockaddr_output_h2rpc(const struct sockaddr *sa, socklen_t rlen,
                      socklen_t len, tarpc_sa *rpc)
{
    assert(rpc != NULL);

    if (sa == NULL)
    {
        /* 
         * Assume that NULL on output may be only in the case of NULL
         * on input
         */
        assert(~rpc->flags & TARPC_SA_NOT_NULL);
        return;
    }

    /* We can't assert here, since it's may be pure output */
    rpc->flags |= TARPC_SA_NOT_NULL;

    if (rpc->flags & TARPC_SA_RAW)
    {
        assert((socklen_t)rpc->raw.raw_len == rlen);
        if (memcmp(rpc->raw.raw_val, sa, rlen) == 0)
        {
            /* Raw data specified by caller has not been modified */
            return;
        }
        /* Raw data was specified on input, but it has been modified */
        rpc->flags &= ~TARPC_SA_RAW;
        free(rpc->raw.raw_val);
        rpc->raw.raw_val = NULL;
        rpc->raw.raw_len = 0;
    }
    else
    {
        assert(rpc->raw.raw_val == NULL);
        assert(rpc->raw.raw_len == 0);
    }

    if (len < (socklen_t)(TE_OFFSET_OF(struct sockaddr, sa_family) +
                          sizeof(sa->sa_family)))
    {
        ERROR("%s(): Address is too short (%u), it does not contain "
              "even 'sa_family' - assertion failure", __FUNCTION__,
              (unsigned)len);
        assert(FALSE); /* Not supported yet */
        return;
    }

    switch (sa->sa_family)
    {
        case AF_INET:
        {
            const struct sockaddr_in   *sin = CONST_SIN(sa);

            if (len < (socklen_t)sizeof(struct sockaddr_in))
            {
                ERROR("%s(): Address is to short (%u) to be 'struct "
                      "sockaddr_in' (%u) - assertion failure",
                      __FUNCTION__, (unsigned)len,
                      (unsigned)sizeof(struct sockaddr_in));
                assert(FALSE); /* Not supported yet */
                return;
            }
            rpc->sa_family = rpc->data.type = RPC_AF_INET;
            rpc->data.tarpc_sa_data_u.in.port = ntohs(sin->sin_port);
            assert(sizeof(rpc->data.tarpc_sa_data_u.in.addr) ==
                   sizeof(sin->sin_addr));
            memcpy(rpc->data.tarpc_sa_data_u.in.addr, &sin->sin_addr,
                   sizeof(rpc->data.tarpc_sa_data_u.in.addr));
            break;
        }

        case AF_INET6:
        {
            const struct sockaddr_in6  *sin6 = CONST_SIN6(sa);

            if (len < (socklen_t)sizeof(struct sockaddr_in6))
            {
                ERROR("%s(): Address is to short (%u) to be 'struct "
                      "sockaddr_in6' (%u) - assertion failure",
                      __FUNCTION__, (unsigned)len,
                      (unsigned)sizeof(struct sockaddr_in6));
                assert(FALSE); /* Not supported yet */
                return;
            }
            rpc->sa_family = rpc->data.type = RPC_AF_INET6;
            rpc->data.tarpc_sa_data_u.in6.port = ntohs(sin6->sin6_port);
            rpc->data.tarpc_sa_data_u.in6.flowinfo = sin6->sin6_flowinfo;
            assert(sizeof(rpc->data.tarpc_sa_data_u.in6.addr) ==
                   sizeof(sin6->sin6_addr));
            memcpy(rpc->data.tarpc_sa_data_u.in6.addr, &sin6->sin6_addr,
                   sizeof(rpc->data.tarpc_sa_data_u.in6.addr));
            rpc->data.tarpc_sa_data_u.in6.scope_id = sin6->sin6_scope_id;
#if HAVE_STRUCT_SOCKADDR_IN6___SIN6_SRC_ID
            rpc->data.tarpc_sa_data_u.in6.src_id = sin6->__sin6_src_id;
#endif
            break;
        }

#ifdef AF_LOCAL
        case AF_LOCAL:
            if (len < (socklen_t)sizeof(struct sockaddr))
            {
                ERROR("%s(): Address is to short (%u) to be 'struct "
                      "sockaddr' (%u) - assertion failure",
                      __FUNCTION__, (unsigned)len,
                      (unsigned)sizeof(struct sockaddr));
                assert(FALSE); /* Not supported yet */
                return;
            }
            rpc->sa_family = rpc->data.type = RPC_AF_LOCAL;
            assert(sizeof(sa->sa_data) >=
                   sizeof(rpc->data.tarpc_sa_data_u.local.data));
            memcpy(rpc->data.tarpc_sa_data_u.local.data, sa->sa_data,
                   sizeof(rpc->data.tarpc_sa_data_u.local.data));
            break;
#endif

        default:
            WARN("%s(): Address family %u is not supported - use raw "
                 "representation", __FUNCTION__, sa->sa_family);
            /* Raw representation */
            rpc->flags |= TARPC_SA_RAW;
            len = 0;
            break;
    }

    if (rlen > len)
    {
        /* Add trailer raw bytes */
        rpc->raw.raw_val = malloc(rlen - len);
        assert(rpc->raw.raw_val != NULL);
        rpc->raw.raw_len = rlen - len;
        memcpy(rpc->raw.raw_val, (uint8_t *)sa + len, rlen - len);
    }
}

/** See the description in ta_rpc_sys_socket.h */
te_errno
sockaddr_rpc2h(const tarpc_sa *rpc,
               struct sockaddr *sa, socklen_t salen,
               struct sockaddr **sa_out, socklen_t *salen_out)
{
    struct sockaddr    *res_sa;
    socklen_t           len_auto;

    assert(rpc != NULL);

    if (rpc->flags & TARPC_SA_NOT_NULL)
        res_sa = sa;
    else
        res_sa = NULL;

    if (sa_out != NULL)
    {
        *sa_out = res_sa;
    }
    else if ((res_sa == NULL) && (sa != NULL))
    {
        ERROR("Unable to indicate that NULL address is returned");
        return TE_EFAULT;
    }

    if (res_sa != NULL)
    {
        if (rpc->flags & TARPC_SA_RAW)
        {
            assert(rpc->raw.raw_val != NULL);
            assert((socklen_t)rpc->raw.raw_len <= salen);
            memcpy(res_sa, rpc->raw.raw_val, rpc->raw.raw_len);
            if (salen_out != NULL)
                *salen_out = rpc->raw.raw_len;
            return 0;
        }
        memset(res_sa, 0, salen);
        res_sa->sa_family = addr_family_rpc2h(rpc->sa_family);
    }

    switch (rpc->data.type)
    {
        case RPC_AF_INET:
        {
            struct sockaddr_in *sin = SIN(res_sa);

            if (sin != NULL)
            {
                sin->sin_port =
                    htons(rpc->data.tarpc_sa_data_u.in.port);
                assert(sizeof(rpc->data.tarpc_sa_data_u.in.addr) ==
                       sizeof(sin->sin_addr));
                memcpy(&sin->sin_addr,
                       rpc->data.tarpc_sa_data_u.in.addr,
                       sizeof(sin->sin_addr));
            }
            len_auto = sizeof(struct sockaddr_in);
            break;
        }

        case RPC_AF_INET6:
        {
            struct sockaddr_in6 *sin6 = SIN6(res_sa);

            if (sin6 != NULL)
            {
                sin6->sin6_port=
                    htons(rpc->data.tarpc_sa_data_u.in6.port);
                sin6->sin6_flowinfo =
                    rpc->data.tarpc_sa_data_u.in6.flowinfo;
                assert(sizeof(rpc->data.tarpc_sa_data_u.in6.addr) ==
                       sizeof(sin6->sin6_addr));
                memcpy(&sin6->sin6_addr,
                       rpc->data.tarpc_sa_data_u.in6.addr,
                       sizeof(sin6->sin6_addr));
                sin6->sin6_scope_id =
                    rpc->data.tarpc_sa_data_u.in6.scope_id;
#if HAVE_STRUCT_SOCKADDR_IN6___SIN6_SRC_ID
                sin6->__sin6_src_id =
                    rpc->data.tarpc_sa_data_u.in6.src_id;
#endif
            }
            len_auto = sizeof(struct sockaddr_in6);
            break;
        }

        case RPC_AF_LOCAL:
            if (res_sa != NULL)
            {
                assert(sizeof(res_sa->sa_data) >=
                       sizeof(rpc->data.tarpc_sa_data_u.local.data));
                memcpy(res_sa->sa_data,
                       rpc->data.tarpc_sa_data_u.local.data,
                       sizeof(rpc->data.tarpc_sa_data_u.local.data));
            }
            /* FALLTHROUGH */

        case RPC_AF_UNSPEC:
            len_auto = sizeof(struct sockaddr);
            break;

        default:
            if (res_sa != NULL)
            {
                assert(FALSE);
                len_auto = 0;
            }
            else
            {
                len_auto = 0;
            }
            break;
    }

    if (res_sa != NULL && rpc->raw.raw_val != NULL)
    {
        assert(salen >= len_auto + (socklen_t)rpc->raw.raw_len);
        memcpy((uint8_t *)res_sa + len_auto, rpc->raw.raw_val,
               rpc->raw.raw_len);
        len_auto += rpc->raw.raw_len;
    }

    if (salen_out != NULL)
    {
        if (rpc->flags & TARPC_SA_LEN_AUTO)
            *salen_out = len_auto;
        else
            *salen_out = rpc->len;
    }

    return 0;
}

/** See the description in ta_rpc_sys_socket.h */
const char *
sockaddr_h2str(const struct sockaddr *addr)
{
    static char  buf[1000];

    const tarpc_sa *rpc_sa;

    if (addr == NULL)
        return "(nil)";

    if (addr->sa_family != TE_AF_TARPC_SA)
        return te_sockaddr2str(addr);

    rpc_sa = (const tarpc_sa *)addr->sa_data;
    if (rpc_sa->flags & TARPC_SA_NOT_NULL)
    {
        snprintf(buf, sizeof(buf), "family=%s",
                 addr_family_rpc2str(rpc_sa->sa_family));

        switch (rpc_sa->data.type)
        {
            case RPC_AF_INET:
            {
                char addr_buf[INET_ADDRSTRLEN];

                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                         " %s:%u",
                         inet_ntop(AF_INET,
                                   rpc_sa->data.tarpc_sa_data_u.in.addr,
                                   addr_buf, sizeof(addr_buf)),
                         (unsigned)rpc_sa->data.tarpc_sa_data_u.in.port);
                break;
            }

            case RPC_AF_INET6:
            {
                char addr_buf[INET6_ADDRSTRLEN];

                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                    " %s:%u flowinfo=0x%x scope_id=%u src_id=%u",
                    inet_ntop(AF_INET6,
                              rpc_sa->data.tarpc_sa_data_u.in6.addr,
                              addr_buf, sizeof(addr_buf)),
                    (unsigned)rpc_sa->data.tarpc_sa_data_u.in6.port,
                    (unsigned)rpc_sa->data.tarpc_sa_data_u.in6.flowinfo,
                    (unsigned)rpc_sa->data.tarpc_sa_data_u.in6.scope_id,
                    (unsigned)rpc_sa->data.tarpc_sa_data_u.in6.src_id);
                break;
            }

            default:
                break;
        }
    }
    else
    {
        snprintf(buf, sizeof(buf), "NULL");
    }

    if (rpc_sa->flags & TARPC_SA_LEN_AUTO)
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 " len=AUTO");
    else
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 " len=%u", (unsigned)rpc_sa->len);

    return buf;
}

/** Convert RPC address family to corresponding structrue name */
const char *
addr_family_sockaddr_str(rpc_socket_addr_family addr_family)
{
    switch (addr_family)
    {
        case RPC_AF_INET:   return "struct sockaddr_in";
        case RPC_AF_INET6:  return "struct sockaddr_in6";
        default:            return NULL;
    }
}

#if HAVE_LINUX_ETHTOOL_H
/** Convert ethtool command to TARPC_ETHTOOL_* types of its data */
tarpc_ethtool_type
ethtool_cmd2type(tarpc_ethtool_command cmd)
{
    switch (cmd)
    {
        case ETHTOOL_GSET:
        case ETHTOOL_SSET:
            return TARPC_ETHTOOL_CMD;

        case ETHTOOL_GMSGLVL:
        case ETHTOOL_SMSGLVL:
        case ETHTOOL_NWAY_RST:
        case ETHTOOL_GLINK:
#ifdef ETHTOOL_GRXCSUM
        case ETHTOOL_GRXCSUM:
#endif
#ifdef ETHTOOL_SRXCSUM
        case ETHTOOL_SRXCSUM:
#endif
#ifdef ETHTOOL_GTXCSUM
        case ETHTOOL_GTXCSUM:
#endif
#ifdef ETHTOOL_STXCSUM
        case ETHTOOL_STXCSUM:
#endif
#ifdef ETHTOOL_GSG
        case ETHTOOL_GSG:
#endif
#ifdef ETHTOOL_SSG
        case ETHTOOL_SSG:
#endif
#ifdef ETHTOOL_GTSO
        case ETHTOOL_GTSO:
#endif
#ifdef ETHTOOL_STSO
        case ETHTOOL_STSO:
#endif
#ifdef ETHTOOL_PHYS_ID
        case ETHTOOL_PHYS_ID:
#endif
#ifdef ETHTOOL_GUFO
        case ETHTOOL_GUFO:
#endif
#ifdef ETHTOOL_SUFO
        case ETHTOOL_SUFO:
#endif
            return TARPC_ETHTOOL_VALUE;

        default:
            return 0;
    }
    return 0;
}

/** Returns a string with ethtool command name. */
const char *
ethtool_cmd2str(tarpc_ethtool_command cmd)
{
    switch (cmd)
    {
#define MACRO2STR(_macro) \
    case ETHTOOL_ ## _macro:            \
        return #_macro;

        MACRO2STR(GSET);
        MACRO2STR(SSET);
        MACRO2STR(GMSGLVL);
        MACRO2STR(SMSGLVL);
        MACRO2STR(NWAY_RST);
        MACRO2STR(GLINK);
#ifdef ETHTOOL_GRXCSUM
        MACRO2STR(GRXCSUM);
#endif
#ifdef ETHTOOL_SRXCSUM
        MACRO2STR(SRXCSUM);
#endif
#ifdef ETHTOOL_GTXCSUM
        MACRO2STR(GTXCSUM);
#endif
#ifdef ETHTOOL_STXCSUM
        MACRO2STR(STXCSUM);
#endif
#ifdef ETHTOOL_GSG
        MACRO2STR(GSG);
#endif
#ifdef ETHTOOL_SSG
        MACRO2STR(SSG);
#endif
#ifdef ETHTOOL_GTSO
        MACRO2STR(GTSO);
#endif
#ifdef ETHTOOL_STSO
        MACRO2STR(STSO);
#endif
#ifdef ETHTOOL_PHYS_ID
        MACRO2STR(PHYS_ID);
#endif
#ifdef ETHTOOL_GUFO
        MACRO2STR(GUFO);
#endif
#ifdef ETHTOOL_SUFO
        MACRO2STR(SUFO);
#endif
#undef MACRO2STR
        default:
            return "(unknown)";
    }
    return NULL; /* unreachable */
}

#define COPY_FIELD(to, from, fname) \
    (to)->fname = (from)->fname;
/**
 * Copy ethtool data from RPC data structure to host. 
 *
 * @param rpc_edata RPC ethtool data structure
 * @param edata_p   pointer to return host ethtool structure;
 *                  this structure is allocated if the pointer is
 *                  initialised to NULL
 */
void 
ethtool_data_rpc2h(tarpc_ethtool *rpc_edata, caddr_t *edata_p)
{
    switch (ethtool_cmd2type(rpc_edata->command))
    {
        case TARPC_ETHTOOL_CMD:
            {
                struct ethtool_cmd *ecmd = 
                        *(struct ethtool_cmd **)edata_p;
                tarpc_ethtool_cmd  *rpc_ecmd = 
                        &rpc_edata->data.tarpc_ethtool_data_u.cmd;

                if (ecmd == NULL)
                {
                    ecmd = calloc(sizeof(struct ethtool_cmd), 1);
                    if (ecmd == NULL)
                        break;
                    *edata_p = (caddr_t)ecmd;
                }

                COPY_FIELD(ecmd, rpc_ecmd, supported);
                COPY_FIELD(ecmd, rpc_ecmd, advertising);
                COPY_FIELD(ecmd, rpc_ecmd, speed);
                COPY_FIELD(ecmd, rpc_ecmd, duplex);
                COPY_FIELD(ecmd, rpc_ecmd, port);
                COPY_FIELD(ecmd, rpc_ecmd, phy_address);
                COPY_FIELD(ecmd, rpc_ecmd, transceiver);
                COPY_FIELD(ecmd, rpc_ecmd, autoneg);
                COPY_FIELD(ecmd, rpc_ecmd, maxtxpkt);
                COPY_FIELD(ecmd, rpc_ecmd, maxrxpkt);

                break;
            }

        case TARPC_ETHTOOL_VALUE:
            {
                struct ethtool_value *evalue = 
                        *(struct ethtool_value **)edata_p;
                tarpc_ethtool_value  *rpc_evalue = 
                        &rpc_edata->data.tarpc_ethtool_data_u.value;

                if (evalue == NULL)
                {
                    evalue = calloc(sizeof(struct ethtool_value), 1);
                    if (evalue == NULL)
                        break;
                    *edata_p = (caddr_t)evalue;
                }

                COPY_FIELD(evalue, rpc_evalue, data);

                break;
            }

        default:
            ERROR("%s: Unknown ethtool command.", __FUNCTION__);
            break;
    }
    *((tarpc_ethtool_command *)(*edata_p)) = rpc_edata->command;
}

/**
 * Copy ethtool data from the host data structure to RPC. 
 *
 * @param edata     ethtool command structure
 * @param rpc_edata RPC ethtool data structure
 */
void 
ethtool_data_h2rpc(tarpc_ethtool *rpc_edata, caddr_t edata)
{
    rpc_edata->command = *((tarpc_ethtool_command *)edata);
    rpc_edata->data.type = ethtool_cmd2type(rpc_edata->command);
    switch (rpc_edata->data.type)
    {
        case TARPC_ETHTOOL_CMD:
            {
                struct ethtool_cmd *ecmd = 
                        (struct ethtool_cmd *)edata;
                tarpc_ethtool_cmd  *rpc_ecmd = 
                        &rpc_edata->data.tarpc_ethtool_data_u.cmd;

                COPY_FIELD(rpc_ecmd, ecmd, supported);
                COPY_FIELD(rpc_ecmd, ecmd, advertising);
                COPY_FIELD(rpc_ecmd, ecmd, speed);
                COPY_FIELD(rpc_ecmd, ecmd, duplex);
                COPY_FIELD(rpc_ecmd, ecmd, port);
                COPY_FIELD(rpc_ecmd, ecmd, phy_address);
                COPY_FIELD(rpc_ecmd, ecmd, transceiver);
                COPY_FIELD(rpc_ecmd, ecmd, autoneg);
                COPY_FIELD(rpc_ecmd, ecmd, maxtxpkt);
                COPY_FIELD(rpc_ecmd, ecmd, maxrxpkt);

                break;
            }

        case TARPC_ETHTOOL_VALUE:
            {
                struct ethtool_value *evalue = 
                        (struct ethtool_value *)edata;
                tarpc_ethtool_value  *rpc_evalue = 
                        &rpc_edata->data.tarpc_ethtool_data_u.value;

                COPY_FIELD(rpc_evalue, evalue, data);

                break;
            }

        default:
            ERROR("%s: Unknown ethtool command type.", __FUNCTION__);
            break;
    }

}
#undef COPY_FIELD
#endif /* HAVE_LINUX_ETHTOOL_H */
