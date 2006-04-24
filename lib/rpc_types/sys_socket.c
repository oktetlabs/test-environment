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

/* Required on Solaris2 (SunOS 5.11) to see IOCTLs */
#define BSD_COMP

#ifdef __CYGWIN__
#ifndef WINDOWS
#include "winsock2.h"
#include "mswsock.h"
#include "ws2tcpip.h"
#undef ERROR
#else
INCLUDE(te_win_defs.h)
#endif
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
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_NETINET_UDP_H
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

#endif

#include "logger_api.h"
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

#define MSG_MAX         0xFFFFFFFF

/** All flags supported on the host platform */
#define MSG_ALL         (MSG_OOB | MSG_PEEK | MSG_DONTROUTE |   \
                         MSG_DONTWAIT | MSG_WAITALL |           \
                         MSG_NOSIGNAL | MSG_TRUNC |             \
                         MSG_CTRUNC | MSG_ERRQUEUE |            \
                         MSG_MORE | MSG_CONFIRM | MSG_EOR |     \
                         MSG_MCAST | MSG_BCAST | MSG_PARTIAL)

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
           (!!(flags & MSG_ERRQUEUE) * RPC_MSG_ERRQUEUE) |
           (!!(flags & ~MSG_ALL) * RPC_MSG_UNKNOWN);
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
        RPC2STR(IPV6_PKTINFO);
        RPC2STR(IPV6_PKTOPTIONS);
        RPC2STR(IPV6_CHECKSUM);
        RPC2STR(IPV6_RTHDR);
        RPC2STR(IPV6_AUTHHDR);
        RPC2STR(IPV6_DSTOPTS);
        RPC2STR(IPV6_HOPOPTS);
        RPC2STR(IPV6_FLOWINFO);
        RPC2STR(IPV6_HOPLIMIT);
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
        RPC2STR(TCP_INFO);
        RPC2STR(TCP_DEFER_ACCEPT);

        RPC2STR(UDP_NOCHECKSUM);

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
        RPC2H_CHECK(IP_ADD_MEMBERSHIP);
        RPC2H_CHECK(IP_DROP_MEMBERSHIP);
        RPC2H_CHECK(IP_MULTICAST_IF);
        RPC2H_CHECK(IP_MULTICAST_LOOP);
        RPC2H_CHECK(IP_MULTICAST_TTL);
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
        RPC2H_CHECK(IPV6_PKTINFO);
        RPC2H_CHECK(IPV6_HOPOPTS);
        RPC2H_CHECK(IPV6_DSTOPTS);
        RPC2H_CHECK(IPV6_RTHDR);
        RPC2H_CHECK(IPV6_PKTOPTIONS);
        RPC2H_CHECK(IPV6_CHECKSUM);
        RPC2H_CHECK(IPV6_HOPLIMIT);
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
        RPC2H_CHECK(TCP_INFO);
        RPC2H_CHECK(TCP_DEFER_ACCEPT);
        RPC2H_CHECK(UDP_NOCHECKSUM);
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
                H2RPC_CHECK(IPV6_PKTINFO);
                H2RPC_CHECK(IPV6_PKTOPTIONS);
                H2RPC_CHECK(IPV6_CHECKSUM);
                H2RPC_CHECK(IPV6_RTHDR);
                H2RPC_CHECK(IPV6_AUTHHDR);
                H2RPC_CHECK(IPV6_DSTOPTS);
                H2RPC_CHECK(IPV6_HOPOPTS);
                H2RPC_CHECK(IPV6_FLOWINFO);
                H2RPC_CHECK(IPV6_HOPLIMIT);
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
#define IOCTL_MAX 0xFFFFFFFF
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
