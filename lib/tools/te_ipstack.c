/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Auxiliary tools to deal with IP stack headers and checksums
 *
 * Implementation of the auxiliary functions to operate the IP stack headers
 * and checksums
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_ipstack.h"
#include "te_sockaddr.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "tad_common.h"
#include "te_defs.h"

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

te_errno
te_ipstack_calc_l4_cksum(const struct sockaddr  *ip_dst_addr,
                         const struct sockaddr  *ip_src_addr,
                         const uint8_t           next_hdr,
                         const uint8_t          *datagram,
                         const size_t            datagram_len,
                         uint16_t               *cksum_out)
{
    te_errno    rc = 0;
    uint8_t    *pseudo_packet;
    size_t      pseudo_packet_len;
    uint16_t    cksum;

    if (datagram == NULL)
    {
        rc = TE_EINVAL;
        ERROR("%s(): incorrect datagram buffer; rc = %d", __FUNCTION__, rc);
        goto out;
    }

    pseudo_packet_len = ((ip_dst_addr->sa_family == AF_INET) ?
                         sizeof(struct te_ipstack_pseudo_header_ip) :
                         sizeof(struct te_ipstack_pseudo_header_ip6)) +
                         datagram_len;

    pseudo_packet = TE_ALLOC(pseudo_packet_len);

    if (ip_dst_addr->sa_family == AF_INET)
    {
        struct te_ipstack_pseudo_header_ip *pseudo_ih;

        pseudo_ih = (struct te_ipstack_pseudo_header_ip *)pseudo_packet;

        pseudo_ih->src_addr = CONST_SIN(ip_src_addr)->sin_addr.s_addr;
        pseudo_ih->dst_addr = CONST_SIN(ip_dst_addr)->sin_addr.s_addr;
        pseudo_ih->next_hdr = next_hdr;
        pseudo_ih->data_len = htons(datagram_len);
    }
    else
    {
        struct te_ipstack_pseudo_header_ip6 *pseudo_i6h;

        pseudo_i6h = (struct te_ipstack_pseudo_header_ip6 *)pseudo_packet;

        memcpy(&pseudo_i6h->src_addr, &CONST_SIN6(ip_src_addr)->sin6_addr,
               sizeof(struct in6_addr));

        memcpy(&pseudo_i6h->dst_addr, &CONST_SIN6(ip_dst_addr)->sin6_addr,
               sizeof(struct in6_addr));

        pseudo_i6h->data_len = htonl(datagram_len);
        pseudo_i6h->next_hdr = next_hdr;
    }

    memcpy(pseudo_packet + (pseudo_packet_len - datagram_len),
           datagram, datagram_len);

    /*
     * We don't care about 16-bit word padding here and rely on
     * calculate_checksum() behaviour which should keep an eye on it
     */
    cksum = calculate_checksum((void *)pseudo_packet, pseudo_packet_len);
    cksum = ~cksum;

    free(pseudo_packet);

    /*
     * For UDP checksum=0 means no checksum, and zero checksum value
     * should be instead represented as 0xffff (see RFC 768).
     */
    if (next_hdr == IPPROTO_UDP)
        cksum = (cksum == 0) ? 0xffff : cksum;

    *cksum_out = cksum;

out:

    return rc;
}

/* See description in te_ipstack.h */
te_errno
te_ipstack_prepare_raw_tcpv4_packet(uint8_t *raw_packet, ssize_t *total_size,
                                    bool remove_vlan_hdr,
                                    struct sockaddr_ll *sadr_ll)
{
    struct ethhdr          *ethh;
    struct vlanhdr         *vlanh;
    struct iphdr           *iph;
    struct tcphdr          *tcph;

    struct sockaddr_in      dst_ip;
    struct sockaddr_in      src_ip;

    int                     i;
    int                     offset;
    int                     ip4_hdr_len;
    int                     eth_hdr_len;
    int                     vlan_hdr_len;
    te_errno                rc = 0;

    if (raw_packet == NULL || total_size == NULL)
        return TE_EINVAL;

    /* Get destination MAC address to send packet */
    ethh = (struct ethhdr *)(raw_packet);
    eth_hdr_len = sizeof(*ethh);

    /* Remove all VLAN headers if @p remove_vlan_hdr is @c true */
    if (remove_vlan_hdr)
    {
        while (ethh->h_proto == htons(ETH_P_8021Q))
        {
            vlanh = (struct vlanhdr *)(raw_packet + eth_hdr_len);
            vlan_hdr_len = sizeof(*vlanh);
            ethh->h_proto = vlanh->vlan_eth;

            memmove(raw_packet + eth_hdr_len,
                    raw_packet + eth_hdr_len + vlan_hdr_len,
                    *total_size - (eth_hdr_len + vlan_hdr_len));

            *total_size -= vlan_hdr_len;
        }
    }
    if (ethh->h_proto != htons(ETH_P_IP))
        return TE_EINVAL;

    offset = eth_hdr_len;

    /* Calculate IP4 checksum */
    iph = (struct iphdr *)(raw_packet + offset);

    if (iph->protocol != IPPROTO_TCP)
        return TE_EINVAL;

    ip4_hdr_len = iph->ihl * WORD_4BYTE;

    if (iph->check == 0)
        iph->check = ~calculate_checksum((void *)iph, ip4_hdr_len);

    offset += ip4_hdr_len;

    /* Calculate TCP checksum */
    tcph = (struct tcphdr *)(raw_packet + offset);

    if (tcph->check == 0)
    {
        dst_ip.sin_family = AF_INET;
        dst_ip.sin_addr.s_addr = iph->daddr;
        src_ip.sin_family = AF_INET;
        src_ip.sin_addr.s_addr = iph->saddr;

        rc = te_ipstack_calc_l4_cksum(CONST_SA(&dst_ip), CONST_SA(&src_ip),
                                      IPPROTO_TCP, (uint8_t *)tcph,
                                      (ntohs(iph->tot_len) - ip4_hdr_len),
                                      &tcph->check);
        if (rc != 0)
            return rc;
    }

    if (sadr_ll != NULL)
    {
        for (i = 0; i < ETH_ALEN; i++)
            sadr_ll->sll_addr[i] = ethh->h_dest[i];
        sadr_ll->sll_halen = ETH_ALEN;
    }

    return 0;
}

/* Get 16bit value from a packet, converting it to host byte order */
static uint16_t
get_16bit(const uint8_t *buf)
{
    return ntohs(*(uint16_t *)buf);
}

/* See description in te_ipstack.h */
te_errno
te_ipstack_mirror_udp_packet(uint8_t *pkt, size_t len)
{
    uint8_t saved_eth_addr[ETH_ALEN];
    size_t pos = 0;
    uint16_t etype;

    struct sockaddr_storage dst_addr_st;
    struct sockaddr_storage src_addr_st;
    struct sockaddr *dst_addr = SA(&dst_addr_st);
    struct sockaddr *src_addr = SA(&src_addr_st);

    struct udphdr *uh;
    uint16_t saved_port;
    uint16_t udp_len;

    memset(&dst_addr_st, 0, sizeof(dst_addr_st));
    memset(&src_addr_st, 0, sizeof(src_addr_st));

    memcpy(saved_eth_addr, pkt, ETH_ALEN);
    memcpy(pkt, pkt + ETH_ALEN, ETH_ALEN);
    memcpy(pkt + ETH_ALEN, saved_eth_addr, ETH_ALEN);

    /* Skip VLAN tags */
    pos = 2 * ETH_ALEN;
    if (get_16bit(pkt + pos) == ETH_P_8021AD)
        pos += 4;
    if (get_16bit(pkt + pos) == ETH_P_8021Q)
        pos += 4;

    etype = get_16bit(pkt + pos);
    if (etype != ETH_P_IP && etype != ETH_P_IPV6)
    {
        ERROR("%s(): received packet is neither IPv4 nor IPv6",
              __FUNCTION__);
        return TE_EPFNOSUPPORT;
    }

    pos += 2;
    if (etype == ETH_P_IP)
    {
        struct iphdr *ih = (struct iphdr *)(pkt + pos);
        uint32_t saved_ip_addr;

        saved_ip_addr = ih->saddr;
        ih->saddr = ih->daddr;
        ih->daddr = saved_ip_addr;

        dst_addr->sa_family = AF_INET;
        src_addr->sa_family = AF_INET;
        te_sockaddr_set_netaddr(dst_addr, &ih->daddr);
        te_sockaddr_set_netaddr(src_addr, &ih->saddr);

        ih->check = 0;
        ih->check = ~calculate_checksum(ih, ih->ihl * 4);

        pos += ih->ihl * 4;
    }
    else
    {
        struct ip6_hdr *ih = (struct ip6_hdr *)(pkt + pos);
        struct in6_addr saved_ip_addr;

        memcpy(&saved_ip_addr, &ih->ip6_src, sizeof(ih->ip6_src));
        memcpy(&ih->ip6_src, &ih->ip6_dst, sizeof(ih->ip6_src));
        memcpy(&ih->ip6_dst, &saved_ip_addr, sizeof(ih->ip6_dst));

        dst_addr->sa_family = AF_INET6;
        src_addr->sa_family = AF_INET6;
        te_sockaddr_set_netaddr(dst_addr, &ih->ip6_dst);
        te_sockaddr_set_netaddr(src_addr, &ih->ip6_src);

        pos += sizeof(*ih);

        if (ih->ip6_nxt != IPPROTO_UDP)
        {
            ERROR("%s(): received IPv6 packet is not UDP or contains "
                  "unexpected extension headers", __FUNCTION__);
            return TE_EPROTONOSUPPORT;
        }
    }

    uh = (struct udphdr *)&pkt[pos];

    saved_port = uh->source;
    uh->source = uh->dest;
    uh->dest = saved_port;

    te_sockaddr_set_port(dst_addr, uh->dest);
    te_sockaddr_set_port(src_addr, uh->source);

    /*
     * Note: uh->len may be less than len - pos for small frames
     * where some padding bytes are added at the end so that Ethernet
     * frame is not shorter than 64 bytes.
     */
    udp_len = ntohs(uh->len);
    if (udp_len > len - pos)
    {
        ERROR("%s(): UDP header has incorrect length field", __FUNCTION__);
        return TE_EBADMSG;
    }

    uh->check = 0;
    return te_ipstack_calc_l4_cksum(dst_addr, src_addr,
                                    IPPROTO_UDP,
                                    &pkt[pos], udp_len,
                                    &uh->check);
}
