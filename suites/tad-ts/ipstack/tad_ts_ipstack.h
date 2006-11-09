/** @file
 * @brief Test Environment
 *
 * Common definitions for tad-ts/ipstack test suite.
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Petrov <Konstantin.Perov@oktetlabs.ru>
 * 
 * $Id: $
 */

#ifndef __TAD_TS_IPSTACK_H__
#define __TAD_TS_IPSTACK_H__

#define IP_HEAD_LEN             20  /* Length of IP header */
#define ICMP_HEAD_LEN           4   /* Length of ICMP header */
#define UDP_HEAD_LEN            8   /* Length of UDP header */
#define UDP_PSEUDO_HEAD_LEN     12  /* Length of UDP pseudo header */
#define UDP_FULL_HEAD_LEN       20  /* Length of UDP full header */
#define TCP_HEAD_LEN            20  /* Length of TCP header */
#define MAX_OPTIONS_LEN         40  /* Maximal length of options field */

typedef struct ip_header {
    uint8_t     ver_len;
    uint8_t     tos;
    uint16_t    totlen;
    uint16_t    id;
    uint16_t    flags_offset;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    chksum;
    in_addr_t   srcaddr;
    in_addr_t   dstaddr;
} ip_header;

typedef struct icmp_header {
    uint16_t    message;
    uint16_t    chksum;
} icmp_header;

typedef struct udp_header {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint16_t    udp_length;
    uint16_t    chksum;
} udp_header;

typedef struct udp_pseudoheader {
    uint32_t    srcaddr;
    uint32_t    dstaddr;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    udp_length;
} udp_pseudoheader;

typedef struct udp_full_header {
    udp_pseudoheader    pseudoheader;
    udp_header          header;
} udp_full_header;

typedef struct tcp_header {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint32_t    seq_num;
    uint32_t    ack_num;
    uint8_t     len;
    uint8_t     flags;
    uint16_t    win_size;
    uint16_t    chksum;
    uint16_t    urg_ptr;
} tcp_header;

#endif /* __TAD_TS_IPSTACK_H__ */
