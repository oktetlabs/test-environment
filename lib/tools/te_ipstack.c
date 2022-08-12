/** @file
 * @brief Auxiliary tools to deal with IP stack headers and checksums
 *
 * Implementation of the auxiliary functions to operate the IP stack headers
 * and checksums
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#include "te_config.h"

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_ipstack.h"
#include "logger_api.h"
#include "tad_common.h"
#include "te_defs.h"

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
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

    pseudo_packet = calloc(1, pseudo_packet_len);
    if (pseudo_packet == NULL)
    {
        rc = TE_ENOMEM;
        ERROR("%s(): failed to allocate memory for a 'pseudo_packet'; rc = %d",
              __FUNCTION__, rc);
        goto out;
    }

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
                                    te_bool remove_vlan_hdr,
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

    /* Remove all VLAN headers if @p remove_vlan_hdr is @c TRUE */
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
