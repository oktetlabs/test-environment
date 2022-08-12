/** @file
 * @brief Test API to work with RTE mbufs
 *
 * Implementation of test API to work with RTE mbufs
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#include "te_config.h"

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#include <netinet/ip6.h>
#endif

#if HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "asn_impl.h"

#include "ndn_ipstack.h"

#include "tapi_rte_mbuf.h"
#include "tapi_rpc_rte_mbuf.h"
#include "tapi_rpc_rte_mempool.h"
#include "tapi_rpc_rte_mbuf_ndn.h"
#include "tapi_mem.h"
#include "te_bufs.h"
#include "te_defs.h"
#include "tapi_test_log.h"
#include "te_sockaddr.h"
#include "tad_common.h"
#include "te_ipstack.h"
#include "tapi_test.h"
#include "tapi_tcp.h"

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
                    const uint8_t *payload, const size_t payload_len,
                    const int cksum_opt)
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

        if ((cksum_opt & TAPI_RTE_MBUF_CKSUM_GOOD_L3) != 0)
            ih->check = calculate_checksum((void *)ih, sizeof(*ih));
        else if ((cksum_opt & TAPI_RTE_MBUF_CKSUM_RAND_L3) != 0)
            while (ih->check == 0)
                ih->check = rand();
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

rpc_rte_mbuf_p
tapi_rte_mk_mbuf_udp(rcf_rpc_server *rpcs,
                     rpc_rte_mempool_p mp,
                     const uint8_t *eth_dst_addr, const uint8_t *eth_src_addr,
                     const struct sockaddr *udp_dst_addr,
                     const struct sockaddr *udp_src_addr,
                     const uint8_t *payload, const size_t payload_len,
                     const int cksum_opt)
{
    uint8_t *datagram;
    struct udphdr *uh;
    rpc_rte_mbuf_p m;
    size_t header_len = sizeof(*uh);

    tapi_perform_sockaddr_sanity_checks(udp_dst_addr, udp_src_addr,
                                        payload_len + header_len);

    datagram = tapi_calloc(1, header_len + payload_len);
    uh = (struct udphdr *)datagram;

    uh->dest = te_sockaddr_get_port(udp_dst_addr);

    uh->source = te_sockaddr_get_port(udp_src_addr);

    uh->len = htons(header_len + payload_len);

    if (payload != NULL)
        memcpy(datagram + header_len, payload, payload_len);
    else
        te_fill_buf(datagram + header_len, payload_len);

    if ((cksum_opt & TAPI_RTE_MBUF_CKSUM_GOOD_L4) != 0)
    {
        CHECK_RC(te_ipstack_calc_l4_cksum(udp_dst_addr, udp_src_addr,
                                          IPPROTO_UDP, datagram,
                                          header_len + payload_len,
                                          &uh->check));
    }
    else if ((cksum_opt & TAPI_RTE_MBUF_CKSUM_RAND_L4) != 0)
    {
        while (uh->check == 0)
            uh->check = rand();
    }

    m = tapi_rte_mk_mbuf_ip(rpcs, mp, eth_dst_addr, eth_src_addr,
                            udp_dst_addr, udp_src_addr, IPPROTO_UDP,
                            datagram, header_len + payload_len, cksum_opt);

    free(datagram);

    return (m);
}

rpc_rte_mbuf_p
tapi_rte_mk_mbuf_tcp(rcf_rpc_server *rpcs,
                     rpc_rte_mempool_p mp,
                     const uint8_t *eth_dst_addr, const uint8_t *eth_src_addr,
                     const struct sockaddr *tcp_dst_addr,
                     const struct sockaddr *tcp_src_addr,
                     const uint32_t th_seq, const uint32_t th_ack,
                     const uint8_t th_off, const uint8_t th_flags,
                     const uint16_t th_win, const uint16_t th_urp,
                     const uint8_t *payload, const size_t payload_len,
                     const int cksum_opt)
{
    uint8_t *datagram;
    struct tcphdr *th;
    rpc_rte_mbuf_p m;
    size_t header_len = sizeof(*th);

    tapi_perform_sockaddr_sanity_checks(tcp_dst_addr, tcp_src_addr,
                                        payload_len + header_len);

    datagram = tapi_calloc(1, header_len + payload_len);

    th = (struct tcphdr *)datagram;

    th->dest = te_sockaddr_get_port(tcp_dst_addr);
    th->source = te_sockaddr_get_port(tcp_src_addr);
    th->seq = th_seq;
    th->ack_seq = th_ack;
    th->doff = ((th_off == 0) || (payload == NULL)) ?
               header_len / sizeof(uint32_t) : th_off;

    th->fin = (th_flags & TCP_FIN_FLAG) ? 1 : 0;
    th->syn = (th_flags & TCP_SYN_FLAG) ? 1 : 0;
    th->rst = (th_flags & TCP_RST_FLAG) ? 1 : 0;
    th->psh = (th_flags & TCP_PSH_FLAG) ? 1 : 0;
    th->ack = (th_flags & TCP_ACK_FLAG) ? 1 : 0;
    th->urg = (th_flags & TCP_URG_FLAG) ? 1 : 0;

    th->window = th_win;
    th->urg_ptr = th_urp;

    if (payload != NULL)
        memcpy(datagram + header_len, payload, payload_len);
    else
        te_fill_buf(datagram + header_len, payload_len);

    if ((cksum_opt & TAPI_RTE_MBUF_CKSUM_GOOD_L4) != 0)
    {
        CHECK_RC(te_ipstack_calc_l4_cksum(tcp_dst_addr, tcp_src_addr,
                                          IPPROTO_TCP, datagram,
                                          header_len + payload_len,
                                          &th->check));
    }
    else if ((cksum_opt & TAPI_RTE_MBUF_CKSUM_RAND_L4) != 0)
    {
        while (th->check == 0)
            th->check = rand();
    }

    m = tapi_rte_mk_mbuf_ip(rpcs, mp, eth_dst_addr, eth_src_addr,
                            tcp_dst_addr, tcp_src_addr, IPPROTO_TCP,
                            datagram, header_len + payload_len, cksum_opt);

    free(datagram);

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

/* See the description in 'tapi_rte_mbuf.h' */
void
tapi_rte_mk_mbuf_mk_ptrn_by_tmpl(rcf_rpc_server    *rpcs,
                                 asn_value         *template,
                                 rpc_rte_mempool_p  mp,
                                 send_transform    *transform,
                                 rpc_rte_mbuf_p   **mbufs_out,
                                 unsigned int      *n_mbufs_out,
                                 asn_value        **ptrn_out)
{
    te_errno           err = 0;
    rpc_rte_mbuf_p    *mbufs = NULL;
    unsigned int       n_mbufs;
    asn_value         *pattern_by_template;
    unsigned int       nb_pdus_o;
    asn_value        **pdus_o = NULL;
    unsigned int       nb_pdus_i;
    asn_value        **pdus_i = NULL;
    asn_value        **packets_prepared = NULL;
    unsigned int       n_packets_prepared = 0;
    asn_value         *pattern;
    unsigned int       i;

    pattern_by_template = tapi_tad_mk_pattern_from_template(template);
    err = (pattern_by_template == NULL) ? TE_ENOMEM : 0;
    if (err != 0)
        goto out;

    TAPI_ON_JMP(err = TE_EFAULT; goto out);

    (void)rpc_rte_mk_mbuf_from_template(rpcs, template, mp, &mbufs, &n_mbufs);

    if (transform != NULL)
    {
        unsigned int   nb_pdus;
        asn_value    **pdus;
        asn_value     *pdu_ip4_o;
        asn_value     *pdu_ip4_i;
        asn_value     *pdu_udp_i;
        asn_value     *pdu_tcp;

        err = tapi_tad_tmpl_relist_outer_inner_pdus(template,
                                                    &nb_pdus_o, &pdus_o,
                                                    &nb_pdus_i, &pdus_i);
        if (err != 0)
            goto out;

        if (pdus_i != NULL)
        {
            pdu_ip4_o = asn_choice_array_look_up_value(nb_pdus_o, pdus_o,
                                                       TE_PROTO_IP4);
            nb_pdus = nb_pdus_i;
            pdus = pdus_i;
        }
        else
        {
            pdu_ip4_o = NULL;
            nb_pdus = nb_pdus_o;
            pdus = pdus_o;
        }

        pdu_ip4_i = asn_choice_array_look_up_value(nb_pdus, pdus, TE_PROTO_IP4);
        pdu_udp_i = asn_choice_array_look_up_value(nb_pdus, pdus, TE_PROTO_UDP);
        pdu_tcp = asn_choice_array_look_up_value(nb_pdus, pdus, TE_PROTO_TCP);

        for (i = 0; i < n_mbufs; ++i)
        {
            uint64_t ol_flags = 0;

            if (transform->hw_flags != 0)
                ol_flags = rpc_rte_pktmbuf_get_flags(rpcs, mbufs[i]);

            if ((transform->hw_flags & SEND_COND_HW_OFFL_VLAN) ==
                SEND_COND_HW_OFFL_VLAN)
            {
                ol_flags |= (1UL << TARPC_RTE_MBUF_F_TX_VLAN);

                rpc_rte_pktmbuf_set_vlan_tci(rpcs, mbufs[i],
                                             transform->vlan_tci);
            }

            if (pdu_ip4_i != NULL &&
                (transform->hw_flags & SEND_COND_HW_OFFL_IP_CKSUM) ==
                SEND_COND_HW_OFFL_IP_CKSUM)
                    ol_flags |= (1UL << TARPC_RTE_MBUF_F_TX_IP_CKSUM);

            if (pdu_ip4_o != NULL &&
                (transform->hw_flags & SEND_COND_HW_OFFL_OUTER_IP_CKSUM) ==
                SEND_COND_HW_OFFL_OUTER_IP_CKSUM)
                    ol_flags |= (1UL << TARPC_RTE_MBUF_F_TX_OUTER_IP_CKSUM);

            if ((transform->hw_flags & SEND_COND_HW_OFFL_L4_CKSUM) ==
                SEND_COND_HW_OFFL_L4_CKSUM)
            {
                if (pdu_tcp != NULL)
                    ol_flags |= (1UL << TARPC_RTE_MBUF_F_TX_TCP_CKSUM);

                if (pdu_udp_i != NULL)
                    ol_flags |= (1UL << TARPC_RTE_MBUF_F_TX_UDP_CKSUM);
            }
            else
            {
                ol_flags |= (1UL << TARPC_RTE_MBUF_F_TX_L4_NO_CKSUM);
            }

            if (((transform->hw_flags & SEND_COND_HW_OFFL_TSO) ==
                 SEND_COND_HW_OFFL_TSO) && (pdu_tcp != NULL))
            {
                struct tarpc_rte_pktmbuf_tx_offload tx_offload;

                ol_flags |= (1UL << TARPC_RTE_MBUF_F_TX_TCP_SEG);

                memset(&tx_offload, 0, sizeof(tx_offload));

                rpc_rte_pktmbuf_get_tx_offload(rpcs, mbufs[i], &tx_offload);
                tx_offload.tso_segsz = transform->tso_segsz;
                rpc_rte_pktmbuf_set_tx_offload(rpcs, mbufs[i], &tx_offload);

                /*
                 * According to DPDK guide, among other requirements
                 * in case of TSO one should set IPv4 checksum to 0;
                 * here we simply rely on an assumption that the initial
                 * template was prepared in accordance with this principle
                 */
            }

            if (transform->hw_flags != 0)
                (void)rpc_rte_pktmbuf_set_flags(rpcs, mbufs[i], ol_flags);
        }
    }

    if (ptrn_out == NULL)
        goto skip_pattern;

    (void)rpc_rte_mbuf_match_pattern(rpcs, pattern_by_template, mbufs, n_mbufs,
                                     &packets_prepared, &n_packets_prepared);

    if (n_packets_prepared == 0)
        TEST_FAIL("Failed to convert the mbuf(s) to ASN.1 raw packets");

    err = tapi_tad_packets_to_pattern(packets_prepared, n_packets_prepared,
                                      transform, &pattern);
    if (err != 0)
        TEST_FAIL("Failed to produce a pattern from ASN.1 raw packets");

    *ptrn_out = pattern;

skip_pattern:
    *mbufs_out = mbufs;
    *n_mbufs_out = n_mbufs;

out:
    asn_free_value(pattern_by_template);

    free(pdus_i);
    free(pdus_o);

    if ((err != 0) && (mbufs != NULL))
    {
        for (i = 0; i < n_mbufs; ++i)
        {
            if (mbufs[i] != RPC_NULL)
                rpc_rte_pktmbuf_free(rpcs, mbufs[i]);
        }

        free(mbufs);
    }

    if (packets_prepared != NULL)
    {
        for (i = 0; i < n_packets_prepared; ++i)
            asn_free_value(packets_prepared[i]);

        free(packets_prepared);
    }

    if (err != 0)
        TEST_FAIL("%s() failed, rc = %r", __func__, TE_RC(TE_TAPI, err));

    TAPI_JMP_POP;
}

/* See the description in 'tapi_rte_mbuf.h' */
void
tapi_rte_pktmbuf_random_redist(rcf_rpc_server    *rpcs,
                               rpc_rte_mempool_p *mp_multi,
                               unsigned int       mp_multi_nb_items,
                               rpc_rte_mbuf_p    *packets,
                               unsigned int       nb_packets)
{
    te_bool      err_occurred;
    unsigned int i;

    TAPI_ON_JMP(err_occurred = TRUE; goto out);

    CHECK_NOT_NULL(rpcs);
    CHECK_NOT_NULL(packets);

    for (i = 0; i < nb_packets; ++i)
    {
        uint8_t                        nb_groups = rand_range(1, CHAR_BIT << 1);
        struct tarpc_pktmbuf_seg_group groups[nb_groups];
        unsigned int                   j;

        for (j = 0; j < nb_groups; ++j)
        {
            groups[j].num = 1;
            groups[j].len = rand_range(1, UINT8_MAX);
        }

        (void)rpc_rte_pktmbuf_redist_multi(rpcs, packets + i,
                                           mp_multi, mp_multi_nb_items,
                                           groups, nb_groups);
    }

    err_occurred = FALSE;
out:
    if (err_occurred)
        TEST_STOP;

    TAPI_JMP_POP;
}

/* See the description in 'tapi_rte_mbuf.h' */
void
tapi_rte_mk_mbufs_by_tmpl_get_pkts(rcf_rpc_server      *rpcs,
                                   const asn_value     *tmpl,
                                   rpc_rte_mempool_p    mp,
                                   rpc_rte_mbuf_p     **mbufs,
                                   unsigned int        *nb_mbufs,
                                   asn_value         ***pkts,
                                   unsigned int        *nb_pkts)
{
    assert(rpcs != NULL);
    assert(tmpl != NULL);
    assert(mbufs != NULL);
    assert(nb_mbufs != NULL);

    CHECK_RC(rpc_rte_mk_mbuf_from_template(rpcs, tmpl, mp, mbufs, nb_mbufs));

    if (pkts != NULL && nb_pkts != NULL)
    {
        asn_value *ptrn_by_tmpl;

        ptrn_by_tmpl = tapi_tad_mk_pattern_from_template((asn_value *)tmpl);
        CHECK_NOT_NULL(ptrn_by_tmpl);

        CHECK_RC(rpc_rte_mbuf_match_pattern(rpcs, ptrn_by_tmpl, *mbufs,
                                            *nb_mbufs, pkts, nb_pkts));

        asn_free_value(ptrn_by_tmpl);
    }
}

