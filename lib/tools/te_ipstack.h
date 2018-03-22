/** @file
 * @brief Auxiliary tools to deal with IP stack headers and checksums
 *
 * @defgroup te_tools_te_ipstack IP stack headers
 * @ingroup te_tools
 * @{
 *
 * Definition of the auxiliary data structures and functions to operate the IP
 * stack headers and checksums
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TOOLS_IPSTACK_H__
#define __TE_TOOLS_IPSTACK_H__

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

struct te_ipstack_pseudo_header_ip {
    in_addr_t src_addr;
    in_addr_t dst_addr;
    uint8_t _pad0;
    uint8_t next_hdr;
    uint16_t data_len;
};

struct te_ipstack_pseudo_header_ip6 {
    struct in6_addr src_addr;
    struct in6_addr dst_addr;
    uint32_t data_len;
    uint16_t _pad0;
    uint8_t _pad1;
    uint8_t next_hdr;
};

/**
 * Auxiliary function to calculate checksums for L4 datagrams (UDP, TCP)
 * using a so-called pseudo-header in accordance with principles of checksum
 * calculation defined by RFC 793 (in case of underlying IPv4 network)
 * and RFC 2460 (i.e., it takes into account pseudo-header composition
 * peculiarities brought by IPv6) but without taking into account any possible
 * IPv6 routing headers (i.e., it uses IPv6 DST address from the main header)
 *
 * @param ip_dst_addr     Destination IPv4/6 address
 * @param ip_src_addr     Source IPv4/6 address
 * @param next_hdr        L4 protocol ID (i.e., IPPROTO_UDP)
 * @param datagram        Buffer containing L4 header and L4 payload
 * @param datagram_len    Buffer length
 * @param cksum_out       Location for the resulting checksum
 *
 * @return Status code
 */
extern te_errno te_ipstack_calc_l4_cksum(const struct sockaddr  *ip_dst_addr,
                                         const struct sockaddr  *ip_src_addr,
                                         const uint8_t           next_hdr,
                                         const uint8_t          *datagram,
                                         const size_t            datagram_len,
                                         uint16_t               *cksum_out);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_IPSTACK__ */
/**@} <!-- END te_tools_te_ipstack --> */
