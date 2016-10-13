/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler
 * Auxiliary tools for the advanced checksum matching mode (implementation)
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAD IP Stack Tools"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "tad_ipstack_impl.h"
#include "logger_ta_fast.h"

static te_errno
tad_ip4_prepare_addresses(tad_pkt          *ip_pdu,
                          struct sockaddr **ip_dst_addr_out,
                          struct sockaddr **ip_src_addr_out)
{
    struct sockaddr_in *dst_in_addr;
    struct sockaddr_in *src_in_addr;

    dst_in_addr = calloc(1, sizeof(*dst_in_addr));
    if (dst_in_addr == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    src_in_addr = calloc(1, sizeof(*src_in_addr));
    if (src_in_addr == NULL)
    {
        free(dst_in_addr);

        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    dst_in_addr->sin_family = src_in_addr->sin_family = AF_INET;
    tad_pkt_read_bits(ip_pdu, IP4_HDR_DST_OFFSET * WORD_32BIT,
                      WORD_32BIT, (uint8_t *)&dst_in_addr->sin_addr);
    tad_pkt_read_bits(ip_pdu, IP4_HDR_SRC_OFFSET * WORD_32BIT,
                      WORD_32BIT, (uint8_t *)&src_in_addr->sin_addr);

    *ip_dst_addr_out = (struct sockaddr *)dst_in_addr;
    *ip_src_addr_out = (struct sockaddr *)src_in_addr;

    return 0;
}

static te_errno
tad_ip6_prepare_addresses(tad_pkt          *ip_pdu,
                          struct sockaddr **ip_dst_addr_out,
                          struct sockaddr **ip_src_addr_out)
{
    struct sockaddr_in6 *dst_in6_addr;
    struct sockaddr_in6 *src_in6_addr;

    dst_in6_addr = calloc(1, sizeof(*dst_in6_addr));
    if (dst_in6_addr == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    src_in6_addr = calloc(1, sizeof(*src_in6_addr));
    if (src_in6_addr == NULL)
    {
        free(dst_in6_addr);

        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    dst_in6_addr->sin6_family = src_in6_addr->sin6_family = AF_INET6;
    tad_pkt_read_bits(ip_pdu, IP6_HDR_DST_OFFSET * WORD_32BIT,
                      IP6_HDR_SIN6_ADDR_LEN * WORD_32BIT,
                      (uint8_t *)&dst_in6_addr->sin6_addr);
    tad_pkt_read_bits(ip_pdu, IP6_HDR_SRC_OFFSET * WORD_32BIT,
                      IP6_HDR_SIN6_ADDR_LEN * WORD_32BIT,
                      (uint8_t *)&src_in6_addr->sin6_addr);

    *ip_dst_addr_out = (struct sockaddr *)dst_in6_addr;
    *ip_src_addr_out = (struct sockaddr *)src_in6_addr;

    return 0;
}

/* See description in 'tad_ipstack_impl.h' */
te_errno
tad_l4_match_cksum_advanced(csap_p          csap,
                            tad_pkt        *pdu,
                            tad_recv_pkt   *meta_pkt,
                            unsigned int    layer,
                            uint8_t         l4_proto,
                            const char     *cksum_script_val)
{
    te_errno            rc;
    size_t              l4_datagram_len;
    uint8_t            *l4_datagram_bin;
    tad_pkt            *ip_pdu;
    uint8_t             ip_version;
    struct sockaddr    *ip_dst_addr;
    struct sockaddr    *ip_src_addr;
    uint16_t            cksum;
    te_bool             does_cksum_match;

    /* Re-create the datagram */

    /* Copy L4 header + L4 payload */
    l4_datagram_len = pdu->segs_len;
    l4_datagram_bin = malloc(l4_datagram_len);
    if (l4_datagram_bin == NULL)
    {
        rc = TE_RC(TE_TAD_CSAP, TE_ENOMEM);
        ERROR(CSAP_LOG_FMT "Failed to allocate buffer for L4 datagram: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    tad_pkt_read_bits(pdu, 0, BITS_PER_BYTE * l4_datagram_len, l4_datagram_bin);

    /*
     * Extract information from the preceding IP header which
     * can be used to fill in the corresponding pseudo-header
     */
    ip_pdu = tad_pkts_first_pkt(&meta_pkt->layers[layer + 1].pkts);
    tad_pkt_read_bits(ip_pdu, 0, IP_HDR_VERSION_LEN, &ip_version);

    if (ip_version == IP4_VERSION)
        rc = tad_ip4_prepare_addresses(ip_pdu, &ip_dst_addr, &ip_src_addr);
    else
        rc = tad_ip6_prepare_addresses(ip_pdu, &ip_dst_addr, &ip_src_addr);

    if (rc != 0)
    {
        free(l4_datagram_bin);

        ERROR(CSAP_LOG_FMT "Failed to prepare IP addresses: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* Calculate the checksum */
    rc = TE_RC(TE_TAD_CSAP, te_ipstack_calc_l4_cksum(ip_dst_addr, ip_src_addr,
                                                     l4_proto, l4_datagram_bin,
                                                     l4_datagram_len, &cksum));

    free(ip_src_addr);
    free(ip_dst_addr);
    free(l4_datagram_bin);

    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to calculate L4 checksum: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    does_cksum_match = (strcmp(cksum_script_val, "correct") == 0) ?
                       (cksum == 0xffff) : (cksum != 0xffff);

    if (!does_cksum_match)
    {
        rc = TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
        F_VERB(CSAP_LOG_FMT "Match PDU vs L4 checksum failed: %r\n"
               "(the pattern unit expected the checksum to be %s)",
               CSAP_LOG_ARGS(csap), rc,
               (strcmp(cksum_script_val, "correct") == 0) ?
               "correct" : "incorrect");
        return rc;
    }

    return 0;
}
