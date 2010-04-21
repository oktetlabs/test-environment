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
#ifndef WINDOWS
#include "tarpc.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** Length of the common part of the "struct sockaddr" */
#define SA_COMMON_LEN \
    (sizeof(struct sockaddr) - sizeof(((struct sockaddr *)0)->sa_data))

/** Maximum length of buffer for sa_data_val in tarpc_sockaddr */
#define SA_DATA_MAX_LEN  (sizeof(struct sockaddr_storage) - SA_COMMON_LEN)


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

/** Convert RPC domain to string */
extern const char * domain_rpc2str(rpc_socket_domain domain);

/** Convert RPC domain to native domain */
extern int domain_rpc2h(rpc_socket_domain domain);

/** Convert native domain to RPC domain */
extern rpc_socket_domain domain_h2rpc(int domain);


/** Special family for sockaddr structures filled in with tarpc_sa */
#define TE_AF_TARPC_SA  254

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

/** Convert RPC address family to string */
extern const char * addr_family_rpc2str(rpc_socket_addr_family addr_family);

/** Convert RPC address family to native address family */
extern int addr_family_rpc2h(rpc_socket_addr_family addr_family);

/** Convert native address family to RPC address family */
extern rpc_socket_addr_family addr_family_h2rpc(int addr_family);


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

/** Convert RPC socket type to string */
extern const char * socktype_rpc2str(rpc_socket_type type);

/** Convert RPC socket type to native socket type */
extern int socktype_rpc2h(rpc_socket_type type);

/** Convert native socket type to RPC socket type */
extern rpc_socket_type socktype_h2rpc(int type);


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

/** Convert RPC protocol to string */
extern const char * proto_rpc2str(rpc_socket_proto proto);

/** Convert RPC IP protocol to native IP protocol constants */
extern int proto_rpc2h(rpc_socket_proto proto);

/** Convert native IP protocol to RPC IP protocol constants */
extern rpc_socket_proto proto_h2rpc(int proto);


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
extern const char * shut_how_rpc2str(rpc_shut_how how);


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
    RPC_MSG_PARTIAL   = 0x8000, /**< Don't fail if the message is trancated;
                                     indicates trancated message on output*/
    RPC_MSG_UNKNOWN   = 0x10000  /**< Incorrect flag */
} rpc_send_recv_flags;

/** Bitmask of all possible receive flags  */
#define RPC_MSG_ALL     (RPC_MSG_OOB | RPC_MSG_PEEK | RPC_MSG_DONTROUTE | \
                         RPC_MSG_DONTWAIT | RPC_MSG_WAITALL |             \
                         RPC_MSG_NOSIGNAL | RPC_MSG_TRUNC |               \
                         RPC_MSG_CTRUNC | RPC_MSG_ERRQUEUE |              \
                         RPC_MSG_MORE | RPC_MSG_CONFIRM | RPC_MSG_EOR |   \
                         RPC_MSG_PARTIAL | RPC_MSG_MCAST | RPC_MSG_BCAST)

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
            RPC_BIT_MAP_ENTRY(MSG_PARTIAL),     \
            RPC_BIT_MAP_ENTRY(MSG_UNKNOWN)

/**
 * send_recv_flags_rpc2str()
 */
RPCBITMAP2STR(send_recv_flags, SEND_RECV_FLAGS_MAPPING_LIST)

/** Convert RPC send/receive flags to native flags */
extern unsigned int send_recv_flags_rpc2h(unsigned int flags);

/** Convert native send/receive flags to RPC flags */
extern unsigned int send_recv_flags_h2rpc(unsigned int flags);


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
    RPC_SO_UPDATE_ACCEPT_CONTEXT, /**< Update the accepting socket
                                  with content of listening socket */    
    RPC_SO_UPDATE_CONNECT_CONTEXT, /** Update connected socket's state */
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
 
    RPC_SO_CONNECT_TIME,    /**< The number of seconds that the socket has
                                 been connected or 0xFFFFFFFF if it is not
                                 connected */

    RPC_SO_OPENTYPE,        /**< Once set, subsequent sockets created 
                                 will be non-overlapped. */

    RPC_SO_DONTLINGER,      /**< MS Windows specific: Indicates whether 
                                 a linger value was set on a socket. */
    RPC_SO_CONDITIONAL_ACCEPT,/**< MS Windows specific: Indicates incoming
                                 connections will be accepted or rejected
                                 by the application and not the stack. */
    RPC_SO_MAX_MSG_SIZE,    /**< MS Windows specific: Returns the maximum
                                 outbound message size for message-oriented
                                 sockets supported by the protocol.  Has no
                                 meaning for stream-oriented sockets. */
    RPC_SO_USELOOPBACK,     /**< MS Windows specific. */
    RPC_SO_EXCLUSIVEADDRUSE,/**< MS Windows specific. Prevents any other
                                 socket from binding to the same address
                                 and port. Option must be set before bind.
                                 */
    RPC_SO_GROUP_ID,        /**< MS Windows specific. */
    RPC_SO_GROUP_PRIORITY,  /**< MS Windows specific. */

    RPC_SO_PROTOCOL_INFOA,  /**< Returns the WSAPROTOCOL_INFOA structure */
    RPC_SO_PROTOCOL_INFOW,  /**< Returns the WSAPROTOCOL_INFOW structure */

    RPC_SO_DGRAM_ERRIND,    /**< Delayed ICMP errors on not connected
                                 datagram socket */
        
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

    RPC_IP_RECVDSTADDR,     /**< Whether to pass destination address
                                 with UDP datagram to the user in an
                                 IP_RECVDSTADDR control message */
    RPC_IP_RECVERR,         /**< Enable extended reliable error
                                 message passing */
    RPC_IP_RECVIF,          /**< Whether to pass interface index with
                                 UDP packet to the user in an 
                                 IP_RECVIF control message */
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

    RPC_IP_RECEIVE_BROADCAST,/**< Winsock2 specific option. 
                                 Allows or blocks broadcast reception. */
    RPC_IP_DONTFRAGMENT,    /**< Winsock2 specific option. 
                                 Indicates that data should not be
                                 fragmented regardless of the local MTU.
                                 Valid only for message oriented protocols.
                                 All Microsoft providers silently ignore
                                 this option. */

    RPC_IPV6_UNICAST_HOPS,  /**< Hop limit for unicast packets */

    RPC_IPV6_MULTICAST_HOPS,
                            /**< Hop limit for multicast packets */

    RPC_IPV6_MULTICAST_IF,  /**< Interface for outgoing multicast packets */
    
    RPC_IPV6_ADDRFORM,      /**< Turn AF_INET6 socket to AF_INET family */

    RPC_IPV6_RECVPKTINFO,   /**< Whether to receive control messages
                                 on incoming datagrams */
    
    RPC_IPV6_PKTOPTIONS,    /**< Specify packet options */
    
    RPC_IPV6_CHECKSUM,      /**< Offset of checksum for raw sockets */
    
    RPC_IPV6_NEXTHOP,       /**< Enable specifying next hop */
    
    RPC_IPV6_ROUTER_ALERT,  /**< Pass packets containing router
                                 alert option */
    
    RPC_IPV6_MULTICAST_LOOP,
                            /**< Whether to loopback outgoing
                                 multicast datagrams */
    
    RPC_IPV6_ADD_MEMBERSHIP,
                            /**< Join a multicast group */
    
    RPC_IPV6_DROP_MEMBERSHIP,
                            /**< Leave a multicast group */
    
    RPC_IPV6_MTU,           /**< MTU of current connected socket */
    
    RPC_IPV6_MTU_DISCOVER,  /**< Enable/disable Path MTU discover */

    RPC_IPV6_RECVERR,       /**< Whether to receive asyncronous
                                 error messages */
    
    RPC_IPV6_V6ONLY,        /**< Use socket for IPv6 communication only */
    
    RPC_IPV6_JOIN_ANYCAST,  /**< Join an anycast group */
    
    RPC_IPV6_LEAVE_ANYCAST, /**< Leave an anycast group */
    
    RPC_IPV6_IPSEC_POLICY,
    
    RPC_IPV6_XFRM_POLICY,
    
    RPC_IPV6_RTHDR,         /**< Deliver the routing header */
    
    RPC_IPV6_AUTHHDR,       /**< Deliver the authentification header */
    
    RPC_IPV6_DSTOPTS,       /**< Deliver the destination options */
    
    RPC_IPV6_HOPOPTS,       /**< Deliver the hop options */
    
    RPC_IPV6_FLOWINFO,      /**< Deliver the flow ID */
    
    RPC_IPV6_RECVHOPLIMIT,  /**< Deliver the hop count of the packet */
 
    RPC_TCP_MAXSEG,         /**< Set/get the maximum segment size for
                                 outgoing TCP packets */
    RPC_TCP_NODELAY,        /**< Enable/disable the Nagle algorithm */
    RPC_TCP_CORK,           /**< Enable/disable delay pushing to the net */
    RPC_TCP_KEEPIDLE,       /**< Start sending keepalive probes after 
                                 this period */
    RPC_TCP_KEEPINTVL,      /**< Interval between keepalive probes */
    RPC_TCP_KEEPCNT,        /**< Number of keepalive probess before death */
    
    RPC_TCP_KEEPALIVE_THRESHOLD,        /**< Start sending keepalive probes
                                             after this idle period in
                                             milliseconds */
    RPC_TCP_KEEPALIVE_ABORT_THRESHOLD,  /**< Abort TCP connection after
                                             this keep-alive failed period
                                             in milliseconds */

    RPC_TCP_INFO,
    RPC_TCP_DEFER_ACCEPT,   /**< Allows a listener to be awakened only when
                                 data arrives on the socket.*/
    RPC_UDP_NOCHECKSUM,     /**< MS Windows specific. 
                                 When TRUE, UDP datagrams are sent with
                                 the checksum of zero. Required for service
                                 providers. If a service provider does not
                                 have a mechanism to disable UDP checksum
                                 calculation, it may simply store this
                                 option without taking any action. */

    RPC_SOCKOPT_UNKNOWN     /**< Invalid socket option */

} rpc_sockopt;

/** Convert RPC socket option to string */
extern const char * sockopt_rpc2str(rpc_sockopt opt);

/** Convert RPC socket option constants to native ones */
extern int sockopt_rpc2h(rpc_sockopt opt);

/** Convert native socket options to RPC one */
extern rpc_sockopt sockopt_h2rpc(int opt_type, int opt);

/** Has socket option boolean semantic? */
extern te_bool sockopt_is_boolean(rpc_sockopt opt);

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

/** Convert RPC socket option constants to string */
extern const char * socklevel_rpc2str(rpc_socklevel level);

/** Convert RPC socket option constants to native ones */
extern int socklevel_rpc2h(rpc_socklevel level);

/** Convert native socket option constants to RPC ones */
extern rpc_socklevel socklevel_h2rpc(int level);

/** Convert RPC socket option constant to its level. */
extern rpc_socklevel rpc_sockopt2level(rpc_sockopt opt);


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
    RPC_SIOCSARP,           /**< Set ARP mapping */
    RPC_SIOCDARP,           /**< Delete ARP mapping */ 
    RPC_SIOCGARP,           /**< Get ARP mapping */
    RPC_SG_IO,
    RPC_SIOCETHTOOL,        /**< Linux-specific Ethtool */
    
    /* Winsock2-specific codes */
    RPC_SIO_ADDRESS_LIST_CHANGE,
    RPC_SIO_ADDRESS_LIST_QUERY,
    RPC_SIO_ADDRESS_LIST_SORT,
    RPC_SIO_ASSOCIATE_HANDLE,
    RPC_SIO_CHK_QOS,
    RPC_SIO_ENABLE_CIRCULAR_QUEUEING,
    RPC_SIO_FIND_ROUTE,
    RPC_SIO_FLUSH,
    RPC_SIO_GET_BROADCAST_ADDRESS,
    RPC_SIO_GET_EXTENSION_FUNCTION_POINTER,
    RPC_SIO_GET_GROUP_QOS,
    RPC_SIO_GET_QOS,
    RPC_SIO_KEEPALIVE_VALS,
    RPC_SIO_MULTIPOINT_LOOPBACK,
    RPC_SIO_MULTICAST_SCOPE,
    RPC_SIO_RCVALL,
    RPC_SIO_RCVALL_IGMPMCAST,
    RPC_SIO_RCVALL_MCAST,
    RPC_SIO_ROUTING_INTERFACE_CHANGE,
    RPC_SIO_ROUTING_INTERFACE_QUERY,
    RPC_SIO_SET_GROUP_QOS,
    RPC_SIO_SET_QOS,
    RPC_SIO_TRANSLATE_HANDLE,
    RPC_SIO_UDP_CONNRESET,
    RPC_SIO_INDEX_BIND,
    RPC_SIO_UCAST_IF,

    RPC_SIOUNKNOWN          /**< Invalid ioctl code */
    
} rpc_ioctl_code; 


/** Convert RPC ioctl requests to string */
extern const char * ioctl_rpc2str(rpc_ioctl_code code);

extern int ioctl_rpc2h(rpc_ioctl_code code);


/**
 * Convert host sockaddr to host sockaddr with @c TE_AF_TARPC_SA address
 * family.
 *
 * @param addr          Host sockaddr structure
 * @param rpc_sa        NULL or location for pointer to @e tarpc_sa
 *                      structure in the created host sockaddr structure
 *
 * @return Allocated memory or NULL.
 */
extern struct sockaddr * sockaddr_to_te_af(const struct sockaddr  *addr,
                                           tarpc_sa              **rpc_sa);

/**
 * Fill in 'tarpc_sa' structure to contain raw buffer of specified
 * length.
 *
 * @param buf           Buffer (may be NULL)
 * @param len           Length of the buffer (have to be @c 0, if @buf
 *                      is @c NULL)
 * @param rpc           Pointer to the structure to be filled in
 */
extern void sockaddr_raw2rpc(const void *buf, socklen_t len,
                             tarpc_sa *rpc);

/**
 * Convert sockaddr structure from host representation to RPC.
 * It should be either @c TE_AF_TARPC_SA or known address structure.
 *
 * @param sa            Sockaddr structure to be converted (may be NULL)
 * @param rpc           Pointer to the structure to be filled in
 */
extern void sockaddr_input_h2rpc(const struct sockaddr *sa, tarpc_sa *rpc);

/**
 * Convert sockaddr structure from host representation to RPC.
 * It does not recognize @c TE_AF_TARPC_SA address family.
 *
 * @param sa            Sockaddr structure to be converted (may be NULL)
 * @param rlen          Real length of the buffer under @a sa
 * @param len           Length returned by called function
 * @param rpc           Pointer to the structure to be filled in
 *                      (filled in by zeros or used to generate @a sa
 *                      from RPC to host representation)
 */
extern void sockaddr_output_h2rpc(const struct sockaddr *sa,
                                  socklen_t              rlen,
                                  socklen_t              len,
                                  tarpc_sa              *rpc);

extern te_errno sockaddr_rpc2h(const tarpc_sa *rpc,
                               struct sockaddr *sa, socklen_t salen,
                               struct sockaddr **sa_out,
                               socklen_t *salen_out);

/**
 * String representation of sockaddr structure including processing
 * of special case for @c TE_AF_TARPC_SA address family.
 *
 * @param addr          Host sockaddr structure
 *
 * @return Pointer to static buffer.
 */
extern const char * sockaddr_h2str(const struct sockaddr *addr);


/**
 * Convert RPC address family to corresponding structure name.
 *
 * @param addr_family   Address family
 *
 * @return Name of corresponding sockaddr structure.
 */
extern const char * addr_family_sockaddr_str(
                        rpc_socket_addr_family addr_family);


#if HAVE_LINUX_ETHTOOL_H

/** Convert ethtool command to TARPC_ETHTOOL_* types of its data. */
extern tarpc_ethtool_type ethtool_cmd2type(tarpc_ethtool_command cmd);
/** Returns a string with ethtool command name. */
extern const char * ethtool_cmd2str(tarpc_ethtool_command cmd);

/**
 * Copy ethtool data from RPC data structure to host. 
 *
 * @param rpc_edata RPC ethtool data structure
 * @param edata_p   pointer to return host ethtool structure;
 *                  this structure is allocated if the pointer is
 *                  initialised to NULL
 */
extern void ethtool_data_rpc2h(tarpc_ethtool *rpc_edata, caddr_t *edata_p);

/**
 * Copy ethtool data from the host data structure to RPC. 
 *
 * @param rpc_edata RPC ethtool data structure
 * @param edata     ethtool command structure
 */
extern void ethtool_data_h2rpc(tarpc_ethtool *rpc_edata, caddr_t edata);

#endif /* HAVE_LINUX_ETHTOOL_H */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_SOCKET_H__ */
