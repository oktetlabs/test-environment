/** @file
 * @brief Auxiliary tools to deal with IP stack headers and checksums
 *
 * Definition of the tools and auxiliary data structures
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
