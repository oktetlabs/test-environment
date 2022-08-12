/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler
 * Auxiliary tools for the advanced checksum matching mode (implementation)
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAD IP Stack Tools"

#include "te_config.h"

#include "te_alloc.h"
#include "tad_ipstack_impl.h"
#include "logger_ta_fast.h"

static void
tad_ip4_prepare_addresses(tad_pkt                  *ip_pdu,
                          struct sockaddr_storage  *ip_dst_addr,
                          struct sockaddr_storage  *ip_src_addr)
{
    struct sockaddr_in *dst_in_addr = SIN(ip_dst_addr);
    struct sockaddr_in *src_in_addr = SIN(ip_src_addr);

    dst_in_addr->sin_family = src_in_addr->sin_family = AF_INET;
    tad_pkt_read_bits(ip_pdu, IP4_HDR_DST_OFFSET * WORD_32BIT,
                      WORD_32BIT, (uint8_t *)&dst_in_addr->sin_addr);
    tad_pkt_read_bits(ip_pdu, IP4_HDR_SRC_OFFSET * WORD_32BIT,
                      WORD_32BIT, (uint8_t *)&src_in_addr->sin_addr);
}

static void
tad_ip6_prepare_addresses(tad_pkt                  *ip_pdu,
                          struct sockaddr_storage  *ip_dst_addr,
                          struct sockaddr_storage  *ip_src_addr)
{
    struct sockaddr_in6 *dst_in6_addr = SIN6(ip_dst_addr);
    struct sockaddr_in6 *src_in6_addr = SIN6(ip_src_addr);

    dst_in6_addr->sin6_family = src_in6_addr->sin6_family = AF_INET6;
    tad_pkt_read_bits(ip_pdu, IP6_HDR_DST_OFFSET * WORD_32BIT,
                      IP6_HDR_SIN6_ADDR_LEN * WORD_32BIT,
                      (uint8_t *)&dst_in6_addr->sin6_addr);
    tad_pkt_read_bits(ip_pdu, IP6_HDR_SRC_OFFSET * WORD_32BIT,
                      IP6_HDR_SIN6_ADDR_LEN * WORD_32BIT,
                      (uint8_t *)&src_in6_addr->sin6_addr);
}

/* See description in 'tad_ipstack_impl.h' */
te_errno
tad_does_cksum_match(csap_p              csap,
                     tad_cksum_str_code  cksum_str_code,
                     uint16_t            cksum,
                     unsigned int        layer)
{
    uint16_t cksum_cmp;
    te_errno rc;

    switch (csap->layers[layer].proto_tag) {
        case TE_PROTO_UDP:
            cksum_cmp = 0xffff;
            break;
        default:
            cksum_cmp = 0;
            break;
    }

    rc = ((cksum_str_code == TAD_CKSUM_STR_CODE_CORRECT ||
           cksum_str_code == TAD_CKSUM_STR_CODE_CORRECT_OR_ZERO) ==
          (cksum == cksum_cmp)) ?
         0 : TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
    if (rc != 0)
        F_VERB(CSAP_LOG_FMT "Match PDU vs layer %u checksum failed: %r\n"
               "(the pattern unit expected the checksum to be %s)",
               CSAP_LOG_ARGS(csap), layer, rc,
               (cksum_str_code == TAD_CKSUM_STR_CODE_CORRECT) ?
               TAD_CKSUM_STR_VAL_CORRECT :
               (cksum_str_code == TAD_CKSUM_STR_CODE_CORRECT_OR_ZERO) ?
               TAD_CKSUM_STR_VAL_CORRECT_OR_ZERO :
               TAD_CKSUM_STR_VAL_INCORRECT);

    return rc;
}

/* See description in 'tad_ipstack_impl.h' */
te_errno
tad_l4_match_cksum_advanced(csap_p              csap,
                            tad_pkt            *pdu,
                            tad_recv_pkt       *meta_pkt,
                            unsigned int        layer,
                            uint8_t             l4_proto,
                            tad_cksum_str_code  cksum_str_code)
{
    te_errno                rc;
    size_t                  l4_datagram_len;
    uint8_t                *l4_datagram_bin;
    tad_pkt                *ip_pdu;
    uint8_t                 ip_version;
    struct sockaddr_storage ip_dst_addr;
    struct sockaddr_storage ip_src_addr;
    uint16_t                cksum;

    if (cksum_str_code == TAD_CKSUM_STR_CODE_CORRECT_OR_ZERO)
    {
        size_t cksum_off = 6;

        if (l4_proto != IPPROTO_UDP)
            return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);

        tad_pkt_read_bits(pdu, BITS_PER_BYTE * cksum_off,
                          BITS_PER_BYTE * sizeof(cksum), (uint8_t *)&cksum);
        if (cksum == 0)
            return 0;
    }

    /* Re-create the datagram */

    /* Copy L4 header + L4 payload */
    l4_datagram_len = pdu->segs_len;
    l4_datagram_bin = TE_ALLOC(l4_datagram_len);
    if (l4_datagram_bin == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    tad_pkt_read_bits(pdu, 0, BITS_PER_BYTE * l4_datagram_len, l4_datagram_bin);

    /*
     * Extract information from the preceding IP header which
     * can be used to fill in the corresponding pseudo-header
     */
    ip_pdu = tad_pkts_first_pkt(&meta_pkt->layers[layer + 1].pkts);
    tad_pkt_read_bits(ip_pdu, 0, IP_HDR_VERSION_LEN, &ip_version);

    if (ip_version == IP4_VERSION)
    {
        uint16_t ip4_tot_len;

        /*
         * To avoid calculating wrong TCP checksum in a frame of a minimum
         * length (eg. 64 bytes) which includes trailing zero bytes at the
         * end, but does not count for them in IPv4 total length field, we
         * count the extra bytes ab initio and fix 'l4_datagram_len' value
         *
         * (We suppose that 'ip_pdu->segs_len' is the whole IP packet length)
         */
        tad_pkt_read_bits(ip_pdu, IP4_HDR_TOTAL_LEN_OFFSET * BITS_PER_BYTE,
                          sizeof(ip4_tot_len) * BITS_PER_BYTE,
                          (uint8_t *)&ip4_tot_len);

        ip4_tot_len = ntohs(ip4_tot_len);

        if (ip_pdu->segs_len > ip4_tot_len)
        {
            size_t trailer_len = ip_pdu->segs_len - ip4_tot_len;
            if (l4_datagram_len > trailer_len)
            {
                l4_datagram_len -= trailer_len;
            }
            else
            {
                free(l4_datagram_bin);
                return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
            }
        }

        tad_ip4_prepare_addresses(ip_pdu, &ip_dst_addr, &ip_src_addr);
    }
    else
    {
        /*
         * In a typical case of an Ethernet frame we have (at least)
         * 14 bytes (Ethernet header length) + 40 bytes (IPv6 header
         * length) = 54 bytes of L2-L3 header space, and addition of
         * L4 header size covers the minimum frame length, hence, we
         * don't consider zero trailer prior to checksum calculation
         */

        tad_ip6_prepare_addresses(ip_pdu, &ip_dst_addr, &ip_src_addr);
    }

    /* Calculate the checksum */
    rc = TE_RC(TE_TAD_CSAP, te_ipstack_calc_l4_cksum(SA(&ip_dst_addr),
                                                     SA(&ip_src_addr),
                                                     l4_proto, l4_datagram_bin,
                                                     l4_datagram_len, &cksum));

    free(l4_datagram_bin);

    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to calculate L4 checksum: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return tad_does_cksum_match(csap, cksum_str_code, cksum, layer);
}
