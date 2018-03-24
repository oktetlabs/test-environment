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
