/** @file
 * @brief Auxiliary tools to deal with IP stack headers and checksums
 *
 * Implementation of the tools
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

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_ipstack.h"
#include "logger_api.h"
#include "tad_common.h"
#include "te_defs.h"

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

    free(pseudo_packet);

    *cksum_out = (cksum == 0) ? 0xffff : cksum;

out:

    return rc;
}
