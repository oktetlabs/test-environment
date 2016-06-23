/** @file
 * @brief Test API to work with RTE mbufs
 *
 * Implementation of test API to work with RTE mbufs
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

#include "te_config.h"

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#include <netinet/ip6.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "tapi_rte_mbuf.h"
#include "tapi_rpc_rte_mbuf.h"
#include "tapi_mem.h"
#include "te_bufs.h"
#include "te_defs.h"
#include "tapi_test_log.h"

static void
tapi_perform_sockaddr_sanity_checks(const struct sockaddr *ip_dst_addr,
                                    const struct sockaddr *ip_src_addr,
                                    size_t payload_len)
{
    if (ip_dst_addr->sa_family != ip_src_addr->sa_family)
        TEST_FAIL("DST and SRC sockaddr families don't match");

    /* We assume that we are given either AF_INET sockaddr or AF_INET6 one */
    if (payload_len > (IP_MAXPACKET - ((ip_dst_addr->sa_family == AF_INET) ?
                                       sizeof(struct iphdr) : 0)))
        TEST_FAIL("The payload length of above permissible");
}

uint16_t
tapi_calc_cksum(unsigned short *ptr, size_t nbytes)
{
    register long sum = 0;
    uint16_t oddbyte;
    register short answer;

    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1)
    {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;

    return answer;
}

rpc_rte_mbuf_p
tapi_rte_mk_mbuf_eth(rcf_rpc_server *rpcs,
                     rpc_rte_mempool_p mp,
                     const uint8_t *dst_addr, const uint8_t *src_addr,
                     const uint16_t ether_type,
                     const uint8_t *payload, size_t len)
{
    rpc_rte_mbuf_p m;
    uint8_t *frame;
    struct ether_header *eh;

    m = rpc_rte_pktmbuf_alloc(rpcs, mp);

    frame = tapi_calloc(1, sizeof(*eh) + len);

    eh = (struct ether_header *)frame;

    memcpy(eh->ether_dhost, dst_addr, sizeof(eh->ether_dhost));
    memcpy(eh->ether_shost, src_addr, sizeof(eh->ether_shost));

    eh->ether_type = htons(ether_type);

    if (payload != NULL)
        memcpy(frame + sizeof(*eh), payload, len);
    else
        te_fill_buf(frame + sizeof(*eh), len);

    (void)rpc_rte_pktmbuf_append_data(rpcs, m, frame, sizeof(*eh) + len);

    free(frame);

    return (m);
}

rpc_rte_mbuf_p
tapi_rte_mk_mbuf_ip(rcf_rpc_server *rpcs,
                    rpc_rte_mempool_p mp,
                    const uint8_t *eth_dst_addr, const uint8_t *eth_src_addr,
                    const struct sockaddr *ip_dst_addr,
                    const struct sockaddr *ip_src_addr,
                    const uint8_t next_hdr,
                    const uint8_t *payload, const size_t payload_len)
{
    uint8_t *packet;
    size_t header_len;
    rpc_rte_mbuf_p m;

    tapi_perform_sockaddr_sanity_checks(ip_dst_addr, ip_src_addr, payload_len);

    header_len = (ip_dst_addr->sa_family == AF_INET) ?
                 sizeof(struct iphdr) : sizeof(struct ip6_hdr);

    packet = tapi_calloc(1, header_len + payload_len);

    if (ip_dst_addr->sa_family == AF_INET)
    {
        struct iphdr *ih = (struct iphdr *)packet;

        ih->version = IPVERSION;
        ih->ihl = sizeof(*ih) >> 2;
        ih->tos = IPTOS_CLASS_CS0;
        ih->id = htons(rand());
        ih->frag_off = (next_hdr == IPPROTO_TCP) ? htons(IP_DF) : 0;
        ih->ttl = MAXTTL;
        ih->protocol = next_hdr;
        ih->daddr = CONST_SIN(ip_dst_addr)->sin_addr.s_addr;
        ih->saddr = CONST_SIN(ip_src_addr)->sin_addr.s_addr;
        ih->tot_len = htons(sizeof(*ih) + payload_len);
        ih->check = tapi_calc_cksum((unsigned short *)ih, sizeof(*ih));
    }
    else
    {
        struct ip6_hdr *i6h = (struct ip6_hdr *)packet;

        i6h->ip6_vfc &= ~TAPI_IPV6_VERSION_MASK;
        i6h->ip6_vfc |= TAPI_IPV6_VERSION;
        i6h->ip6_plen = htons(payload_len);
        i6h->ip6_nxt = next_hdr;
        i6h->ip6_hlim = MAXTTL;

        memcpy(&i6h->ip6_dst, &CONST_SIN6(ip_dst_addr)->sin6_addr,
               sizeof(struct in6_addr));

        memcpy(&i6h->ip6_src, &CONST_SIN6(ip_src_addr)->sin6_addr,
               sizeof(struct in6_addr));
    }

    if (payload != NULL)
        memcpy(packet + header_len, payload, payload_len);
    else
        te_fill_buf(packet + header_len, payload_len);

    m = tapi_rte_mk_mbuf_eth(rpcs, mp, eth_dst_addr, eth_src_addr,
                             (ip_dst_addr->sa_family == AF_INET) ?
                             ETHERTYPE_IP : ETHERTYPE_IPV6,
                             packet, header_len + payload_len);

    free(packet);

    return (m);
}

uint8_t *
tapi_rte_get_mbuf_data(rcf_rpc_server *rpcs,
                       rpc_rte_mbuf_p m, size_t offset, size_t *bytes_read)
{
    uint32_t pkt_len;
    uint8_t *data_buf;

    pkt_len = rpc_rte_pktmbuf_get_pkt_len(rpcs, m);

    data_buf = tapi_calloc(1, pkt_len - offset);

    *bytes_read = (size_t)rpc_rte_pktmbuf_read_data(rpcs,
                                                    m, offset, pkt_len - offset,
                                                    data_buf, pkt_len - offset);

    return (data_buf);
}
