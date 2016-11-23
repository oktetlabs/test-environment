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

#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tapi_rpc_rte.h"
#include "tapi_ndn.h"

#define TAPI_IPV6_VERSION 0x60
#define TAPI_IPV6_VERSION_MASK 0xf0

#ifdef __cplusplus
extern "C" {
#endif

/** RTE mbuf SW checksum preparation modes */
enum tapi_rte_mbuf_cksum_mode {
    /**
     * Fill in checksum with invalid random non-zero value
     * (default behaviour, 0 may be used instead)
     */
    TAPI_RTE_MBUF_CKSUM_RAND = 0,
    /** Calculate and put correct checksum */
    TAPI_RTE_MBUF_CKSUM_GOOD,
    /** Fill in checksum with 0 */
    TAPI_RTE_MBUF_CKSUM_ZERO,

    /** The number of checksum modes */
    TAPI_RTE_MBUF_CKSUM_MODES
};

/** Bits space for checksum control */
#define TAPI_RTE_MBUF_CKSUM_BITS 2

/** Layer3 (IP4) checksum bits offset */
#define TAPI_RTE_MBUF_CKSUM_L3_OFF 0

/** Layer4 (TCP / UDP / ICMP) checksum bits offset */
#define TAPI_RTE_MBUF_CKSUM_L4_OFF (TAPI_RTE_MBUF_CKSUM_BITS)

/** RTE mbuf SW checksum preparation choices */
enum tapi_rte_mbuf_cksum {
    TAPI_RTE_MBUF_CKSUM_ZERO_L3 = (TAPI_RTE_MBUF_CKSUM_L3_OFF <<
                                   TAPI_RTE_MBUF_CKSUM_ZERO),
    TAPI_RTE_MBUF_CKSUM_ZERO_L4 = (TAPI_RTE_MBUF_CKSUM_L4_OFF <<
                                   TAPI_RTE_MBUF_CKSUM_ZERO),
    TAPI_RTE_MBUF_CKSUM_GOOD_L3 = (TAPI_RTE_MBUF_CKSUM_L3_OFF <<
                                   TAPI_RTE_MBUF_CKSUM_GOOD),
    TAPI_RTE_MBUF_CKSUM_GOOD_L4 = (TAPI_RTE_MBUF_CKSUM_L4_OFF <<
                                   TAPI_RTE_MBUF_CKSUM_GOOD),
    TAPI_RTE_MBUF_CKSUM_RAND_L3 = (TAPI_RTE_MBUF_CKSUM_L3_OFF <<
                                   TAPI_RTE_MBUF_CKSUM_RAND),
    TAPI_RTE_MBUF_CKSUM_RAND_L4 = (TAPI_RTE_MBUF_CKSUM_L4_OFF <<
                                   TAPI_RTE_MBUF_CKSUM_RAND),

    TAPI_RTE_MBUF_CKSUM_ZERO_ALL        = (TAPI_RTE_MBUF_CKSUM_ZERO_L3 |
                                           TAPI_RTE_MBUF_CKSUM_ZERO_L4),
    TAPI_RTE_MBUF_CKSUM_ZERO_L3_GOOD_L4 = (TAPI_RTE_MBUF_CKSUM_ZERO_L3 |
                                           TAPI_RTE_MBUF_CKSUM_GOOD_L4),
    TAPI_RTE_MBUF_CKSUM_GOOD_L3_ZERO_L4 = (TAPI_RTE_MBUF_CKSUM_GOOD_L3 |
                                           TAPI_RTE_MBUF_CKSUM_ZERO_L4),
    TAPI_RTE_MBUF_CKSUM_GOOD_ALL        = (TAPI_RTE_MBUF_CKSUM_GOOD_L3 |
                                           TAPI_RTE_MBUF_CKSUM_GOOD_L4),
    TAPI_RTE_MBUF_CKSUM_RAND_L3_GOOD_L4 = (TAPI_RTE_MBUF_CKSUM_RAND_L3 |
                                           TAPI_RTE_MBUF_CKSUM_GOOD_L4),
    TAPI_RTE_MBUF_CKSUM_GOOD_L3_RAND_L4 = (TAPI_RTE_MBUF_CKSUM_GOOD_L3 |
                                           TAPI_RTE_MBUF_CKSUM_RAND_L4),
    TAPI_RTE_MBUF_CKSUM_RAND_ALL        = (TAPI_RTE_MBUF_CKSUM_RAND_L3 |
                                           TAPI_RTE_MBUF_CKSUM_RAND_L4),
    TAPI_RTE_MBUF_CKSUM_ZERO_L3_RAND_L4 = (TAPI_RTE_MBUF_CKSUM_ZERO_L3 |
                                           TAPI_RTE_MBUF_CKSUM_RAND_L4),
    TAPI_RTE_MBUF_CKSUM_RAND_L3_ZERO_L4 = (TAPI_RTE_MBUF_CKSUM_RAND_L3 |
                                           TAPI_RTE_MBUF_CKSUM_ZERO_L4)
};

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
 * @param mp                  RTE mempool pointer
 * @param eth_dst_addr        Destination Ethernet address (network byte order)
 * @param eth_src_addr        Source Ethernet address (network byte order)
 * @param ip_dst_addr         Destination IPv4/6 address
 * @param ip_src_addr         Source IPv4/6 address
 * @param next_hdr            L4 protocol ID (i.e., IPPROTO_UDP)
 * @param payload             Data to be encapsulated into IP packet or @c NULL
 * @param payload_len         Payload length
 * @param cksum_opt           SW checksum preparation choice; possible values
 *                            are those from @b tapi_rte_mbuf_cksum enum
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
                                          const size_t payload_len,
                                          const int cksum_opt);

/**
 * Prepare an RTE mbuf with an Ethernet frame containing UDP packet
 *
 * @param mp                  RTE mempool pointer
 * @param eth_dst_addr        Destination Ethernet address (network byte order)
 * @param eth_src_addr        Source Ethernet address (network byte order)
 * @param udp_dst_addr        Destination IPv4/6 address and port
 * @param udp_src_addr        Source IPv4/6 address and port
 * @param payload             Data to be encapsulated into UDP packet or @c NULL
 * @param payload_len         Payload length
 * @param cksum_opt           SW checksum preparation choice; possible values
 *                            are those from @b tapi_rte_mbuf_cksum enum
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
                                           const size_t payload_len,
                                           const int cksum_opt);

/**
 * Prepare an RTE mbuf with an Ethernet frame containing TCP packet
 * (if TCP options are to be added, one should include them as a part of
 *  @p payload and set correct data offset [TCP header length +
 *  + options length] @p th_off)
 *
 * @param mp                  RTE mempool pointer
 * @param eth_dst_addr        Destination Ethernet address (network byte order)
 * @param eth_src_addr        Source Ethernet address (network byte order)
 * @param tcp_dst_addr        Destination IPv4/6 address and port
 * @param tcp_src_addr        Source IPv4/6 address and port
 * @param th_seq              TX data seq. number (network byte order) or @c 0
 * @param th_ack              RX data seq. number (network byte order) or @c 0
 * @param th_off              Data offset (includes options length) or @c 0
 *                            (measured in 32-bit words)
 * @param th_flags            TCP flags or @c 0
 * @param th_win              RX flow control window (network byte order) or @c 0
 * @param th_urp              Urgent pointer (network byte order) or @c 0
 * @param payload             Data to be encapsulated into TCP packet or @c NULL
 * @param payload_len         Payload length (should include options length)
 * @param cksum_opt           SW checksum preparation choice; possible values
 *                            are those from @b tapi_rte_mbuf_cksum enum
 *
 * @return RTE mbuf pointer on success; jumps out on failure
 */
extern rpc_rte_mbuf_p tapi_rte_mk_mbuf_tcp(rcf_rpc_server *rpcs,
                                           rpc_rte_mempool_p mp,
                                           const uint8_t *eth_dst_addr,
                                           const uint8_t *eth_src_addr,
                                           const struct sockaddr *tcp_dst_addr,
                                           const struct sockaddr *tcp_src_addr,
                                           const uint32_t th_seq,
                                           const uint32_t th_ack,
                                           const uint8_t th_off,
                                           const uint8_t th_flags,
                                           const uint16_t th_win,
                                           const uint16_t th_urp,
                                           const uint8_t *payload,
                                           const size_t payload_len,
                                           const int cksum_opt);

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


/**
 * Produce mbuf(s) from template, set offloads and provide
 * a pattern to capture resulting packets on the peer side
 *
 * @param mp        RTE mempool pointer
 * @param template  ASN.1 traffic template
 * @param transform A set of parameters describing certain trasformations
 *                  which are required to affect the outgoing packets
 * @param mbufs_out Location for RTE mbuf pointer(s)
 * @param n_mbufs   Location for the number of mbufs prepared
 * @param ptrn_out  Location for the pattern to be produced or @c NULL
 *
 * @note The function jumps out in case of error
 */
extern void tapi_rte_mk_mbuf_mk_ptrn_by_tmpl(rcf_rpc_server    *rpcs,
                                             asn_value         *template,
                                             rpc_rte_mempool_p  mp,
                                             send_transform    *transform,
                                             rpc_rte_mbuf_p   **mbufs_out,
                                             unsigned int      *n_mbufs_out,
                                             asn_value        **ptrn_out);

/**
 * Go through an array of packets (mbuf chains) and try to transform
 * each of them by means of a randomly selected segmentation pattern
 * in accordance with an up-to-date number of spare objects in @p mp
 *
 * @note @c rpc_rte_pktmbuf_redist() is used within the wrapper, and
 *       existing pointers in @p packets may be replaced with others
 *
 * @p mp            RTE mempool pointer
 * @p mp_size       RTE mempool size
 * @p packets       An array containing the original packets (RTE mbuf pointers)
 * @p nb_packets    The number of packets within the array
 */
extern void tapi_rte_pktmbuf_random_redist(rcf_rpc_server     *rpcs,
                                           rpc_rte_mempool_p   mp,
                                           unsigned int        mp_size,
                                           rpc_rte_mbuf_p     *packets,
                                           unsigned int        nb_packets);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RTE_MBUF_H__ */
