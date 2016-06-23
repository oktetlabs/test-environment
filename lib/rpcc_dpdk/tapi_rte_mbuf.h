/** @file
 * @brief Test API to work with RTE mbufs
 *
 * Definition of test API to work with RTE mbufs
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

#ifndef __TE_TAPI_RTE_MBUF_H__
#define __TE_TAPI_RTE_MBUF_H__

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"

#define TAPI_IPV6_VERSION 0x60
#define TAPI_IPV6_VERSION_MASK 0xf0

#ifdef __cplusplus
extern "C" {
#endif

struct tapi_pseudo_header_ip {
    in_addr_t src_addr;
    in_addr_t dst_addr;
    uint8_t _pad0;
    uint8_t next_hdr;
    uint16_t data_len;
};

struct tapi_pseudo_header_ip6 {
    struct in6_addr src_addr;
    struct in6_addr dst_addr;
    uint32_t data_len;
    uint16_t _pad0;
    uint8_t _pad1;
    uint8_t next_hdr;
};

/**
 * Auxiliary function to calculate checksums
 *
 * @param ptr             Pointer to a buffer containing data
 * @param nbytes          Buffer length
 *
 * @return Checksum value
 */
extern uint16_t tapi_calc_cksum(unsigned short *ptr, size_t nbytes);

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
 *
 * @return Checksum value; jumps out on error
 */
extern uint16_t tapi_calc_l4_cksum(const struct sockaddr *ip_dst_addr,
                                   const struct sockaddr *ip_src_addr,
                                   const uint8_t next_hdr,
                                   const uint8_t *datagram,
                                   const size_t datagram_len);

/**
 * Prepare an RTE mbuf with Ethernet frame containing particular data
 * (if buffer to contain the frame data is NULL, then random data will be put)
 *
 * @param mp              RTE mempool pointer
 * @param dst_addr        Destination Ethernet address (network byte order)
 * @param src_addr        Source Ethernet address (network byte order)
 * @param ether_type      Ethernet type value (host byte order)
 * @param payload         Data to be encapsulated into the frame or @c NULL
 * @param len             Data length
 *
 * @return RTE mbuf pointer on success; jumps out on failure
 */
extern rpc_rte_mbuf_p tapi_rte_mk_mbuf_eth(rcf_rpc_server *rpcs,
                                           rpc_rte_mempool_p mp,
                                           const uint8_t *dst_addr,
                                           const uint8_t *src_addr,
                                           const uint16_t ether_type,
                                           const uint8_t *payload, size_t len);

/**
 * Prepare an RTE mbuf with an Ethernet frame containing IP packet
 * (if buffer to contain IP payload is NULL, then random data will be put)
 *
 * @param mp              RTE mempool pointer
 * @param eth_dst_addr    Destination Ethernet address (network byte order)
 * @param eth_src_addr    Source Ethernet address (network byte order)
 * @param ip_dst_addr     Destination IPv4/6 address
 * @param ip_src_addr     Source IPv4/6 address
 * @param next_hdr        L4 protocol ID (i.e., IPPROTO_UDP)
 * @param payload         Data to be encapsulated into IP packet or @c NULL
 * @param len             Payload length
 *
 * @return RTE mbuf pointer on success; jumps out on failure
 */
extern rpc_rte_mbuf_p tapi_rte_mk_mbuf_ip(rcf_rpc_server *rpcs,
                                          rpc_rte_mempool_p mp,
                                          const uint8_t *eth_dst_addr,
                                          const uint8_t *eth_src_addr,
                                          const struct sockaddr *ip_dst_addr,
                                          const struct sockaddr *ip_src_addr,
                                          const uint8_t next_hdr,
                                          const uint8_t *payload,
                                          const size_t payload_len);

/**
 * Prepare an RTE mbuf with an Ethernet frame containing UDP packet
 *
 * @param mp              RTE mempool pointer
 * @param eth_dst_addr    Destination Ethernet address (network byte order)
 * @param eth_src_addr    Source Ethernet address (network byte order)
 * @param udp_dst_addr    Destination IPv4/6 address and port
 * @param udp_src_addr    Source IPv4/6 address and port
 * @param payload         Data to be encapsulated into UDP packet or @c NULL
 * @param len             Payload length
 *
 * @return RTE mbuf pointer on success; jumps out on failure
 */
extern rpc_rte_mbuf_p tapi_rte_mk_mbuf_udp(rcf_rpc_server *rpcs,
                                           rpc_rte_mempool_p mp,
                                           const uint8_t *eth_dst_addr,
                                           const uint8_t *eth_src_addr,
                                           const struct sockaddr *udp_dst_addr,
                                           const struct sockaddr *udp_src_addr,
                                           const uint8_t *payload,
                                           const size_t payload_len);

/**
 * Read the whole mbuf (chain) data (starting at a given offset)
 * into the buffer allocated internally and pass the number of bytes read
 * to the user-specified variable
 *
 * @param m               RTE mbuf pointer
 * @param offset          Offset into mbuf data
 * @param bytes_read      Amount of bytes read
 *
 * @return Pointer to a buffer containing data read from mbuf (chain)
 */
extern uint8_t *tapi_rte_get_mbuf_data(rcf_rpc_server *rpcs,
                                       rpc_rte_mbuf_p m, size_t offset,
                                       size_t *bytes_read);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RTE_MBUF_H__ */
