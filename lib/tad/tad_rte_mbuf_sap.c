/** @file
 * @brief TAD RTE mbuf Provider Interface
 *
 * Routines to convert packets from TAD to RTE mbuf representation and vice versa
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD RTE mbuf"

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_csap_inst.h"
#include "tad_utils.h"
#include "tad_rte_mbuf_sap.h"
#include "tad_eth_sap.h"
#include "te_ethernet.h"

#include "rte_byteorder.h"


#define GRE_HDR_PROTOCOL_TYPE_OFFSET sizeof(uint16_t)
#define GRE_HDR_PROTOCOL_TYPE_NVGRE 0x6558

te_errno
tad_rte_mbuf_sap_read(tad_rte_mbuf_sap   *sap,
                      tad_pkt            *pkt,
                      size_t             *pkt_len,
                      unsigned           *pend)
{
    struct rte_mbuf        *m;
    struct rte_mbuf        *m_seg;
    tad_pkt_seg            *tad_seg;
    size_t                  tad_seg_offset;
    uint8_t                *new_seg_data;
    size_t                  bytes_remain;
    struct tad_vlan_tag     tag;
    int                     err = 0;

    assert(sap != NULL);
    assert(sap->pkt_ring != NULL);

    if (rte_ring_dequeue(sap->pkt_ring, (void **)&m) != 0)
    {
        err = TE_ENOENT;
        goto out;
    }

    *pend = rte_ring_count(sap->pkt_ring);

    if (tad_pkt_realloc_segs(pkt, m->pkt_len) != 0)
    {
        err = TE_ENOMEM;
        goto out;
    }

    tad_seg = CIRCLEQ_FIRST(&pkt->segs);
    tad_seg_offset = 0;

    for (m_seg = m; m_seg != NULL; m_seg = m_seg->next)
    {
        size_t bytes_copied = 0;

        while (bytes_copied < m_seg->data_len)
        {
            size_t to_copy;

            if (tad_seg->data_len == tad_seg_offset)
            {
                tad_seg = CIRCLEQ_NEXT(tad_seg, links);
                tad_seg_offset = 0;
            }

            to_copy = MIN(tad_seg->data_len - tad_seg_offset,
                          m_seg->data_len - bytes_copied);

            memcpy(tad_seg->data_ptr + tad_seg_offset,
                   rte_pktmbuf_mtod_offset(m_seg, void *, bytes_copied),
                   to_copy);
            bytes_copied += to_copy;
            tad_seg_offset += to_copy;
        }
    }

    *pkt_len = m->pkt_len;

    if (((m->ol_flags & RTE_MBUF_F_RX_VLAN_STRIPPED) == 0) &&
        ((m->ol_flags & RTE_MBUF_F_TX_VLAN) == 0))
        goto out;

    tad_seg = CIRCLEQ_FIRST(&pkt->segs);

    for (bytes_remain = 2 * ETHER_ADDR_LEN;
         bytes_remain >= tad_seg->data_len;
         bytes_remain -= tad_seg->data_len,
         tad_seg = CIRCLEQ_NEXT(tad_seg, links));

    new_seg_data = malloc(tad_seg->data_len + TAD_VLAN_TAG_LEN);
    if (new_seg_data == NULL)
    {
        err = errno;
        goto out;
    }

    memcpy(new_seg_data, tad_seg->data_ptr, bytes_remain);

    tag.vlan_tpid = htons(ETH_P_8021Q);
    tag.vlan_tci = htons(m->vlan_tci);
    memcpy(new_seg_data + bytes_remain, &tag, TAD_VLAN_TAG_LEN);

    memcpy(new_seg_data + bytes_remain + TAD_VLAN_TAG_LEN,
           tad_seg->data_ptr + bytes_remain,
           tad_seg->data_len - bytes_remain);

    tad_pkt_put_seg_data(pkt, tad_seg, new_seg_data,
                         tad_seg->data_len + TAD_VLAN_TAG_LEN,
                         tad_pkt_seg_data_free);

    *pkt_len += TAD_VLAN_TAG_LEN;

out:
    if (err != 0)
    {
        tad_pkt_free(pkt);
        /* FIXME: add switch() to show customized error messages */
        ERROR("%s(): Failed to convert RTE mbuf to TAD packet "
              "(rc = %d)", __FUNCTION__, err);
    }

    return TE_RC(TE_TAD_CSAP, err);
}

static void
handle_layer_info(const tad_pkt      *pkt,
                  te_tad_protocols_t  layer_tag,
                  size_t              layer_data_len,
                  struct rte_mbuf    *m,
                  uint64_t           *ol_flags_inner,
                  uint64_t           *ol_flags_outer)
{
    te_bool  encap_header_detected = FALSE;
    size_t   gre_hdr_offset;
    uint8_t  gre_hdr_first_word[WORD_4BYTE];
    uint16_t gre_hdr_protocol_type;

    switch (layer_tag)
    {
        case TE_PROTO_ETH:
            /*
             * m->l2_len may already count for outer L4 (if any)
             * and tunnel headers. Add Ethernet layer length.
             */
            m->l2_len += layer_data_len;
            break;

        case TE_PROTO_IP4:
            m->l3_len = layer_data_len;
            *ol_flags_inner |= RTE_MBUF_F_TX_IPV4;
            *ol_flags_outer |= RTE_MBUF_F_TX_OUTER_IPV4;
            break;

        case TE_PROTO_IP6:
            m->l3_len = layer_data_len;
            *ol_flags_inner |= RTE_MBUF_F_TX_IPV6;
            *ol_flags_outer |= RTE_MBUF_F_TX_OUTER_IPV6;
            break;

        case TE_PROTO_TCP:
            /*@fallthrough@*/
        case TE_PROTO_UDP:
            m->l4_len = layer_data_len;
            break;

        case TE_PROTO_VXLAN:
            encap_header_detected = TRUE;
            m->ol_flags |= RTE_MBUF_F_TX_TUNNEL_VXLAN;
            break;

        case TE_PROTO_GENEVE:
            encap_header_detected = TRUE;
            m->ol_flags |= RTE_MBUF_F_TX_TUNNEL_GENEVE;
            break;

        case TE_PROTO_GRE:
            encap_header_detected = TRUE;
            /*
             * At this point m->l2_len and m->l3_len describe outer header.
             * These will become m->outer_l2_len and m->outer_l3_len below.
             */
            gre_hdr_offset = m->l2_len + m->l3_len;
            /*
             * TE_PROTO_GRE may serve either GRE or NVGRE tunnel types.
             * RTE has no Tx tunnel offload flag for the latter.
             * Rule out NVGRE and set RTE_MBUF_F_TX_TUNNEL_GRE flag.
             */
            assert(gre_hdr_offset + WORD_4BYTE <= tad_pkt_len(pkt));
            tad_pkt_read_bits(pkt, gre_hdr_offset << 3, WORD_32BIT,
                              gre_hdr_first_word);
            memcpy(&gre_hdr_protocol_type,
                   gre_hdr_first_word + GRE_HDR_PROTOCOL_TYPE_OFFSET,
                   sizeof(gre_hdr_protocol_type));
            gre_hdr_protocol_type = rte_be_to_cpu_16(gre_hdr_protocol_type);
            if (gre_hdr_protocol_type != GRE_HDR_PROTOCOL_TYPE_NVGRE)
                m->ol_flags |= RTE_MBUF_F_TX_TUNNEL_GRE;
            break;

        default:
            break;
    }

    if (encap_header_detected)
    {
        m->outer_l2_len = m->l2_len;
        m->outer_l3_len = m->l3_len;

        /* Inner L2 length counts for outer L4 (if any) and tunnel headers. */
        m->l2_len = m->l4_len + layer_data_len;
        m->l3_len = 0;
        m->l4_len = 0;

        m->ol_flags |= *ol_flags_outer;
        *ol_flags_inner = 0;
    }
}

te_errno
tad_rte_mbuf_sap_write(tad_rte_mbuf_sap   *sap,
                       const               tad_pkt *pkt)
{
    struct rte_mbuf    *m;
    tad_pkt_seg        *tad_seg;
    te_tad_protocols_t  layer_tag_prev = TE_PROTO_INVALID;
    size_t              layer_data_len = 0;
    uint64_t            ol_flags_inner = 0;
    uint64_t            ol_flags_outer = 0;
    int                 err = 0;

    if ((m = rte_pktmbuf_alloc(sap->pkt_pool)) == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    CIRCLEQ_FOREACH(tad_seg, &pkt->segs, links)
    {
        size_t bytes_copied = 0;

        while (bytes_copied < tad_seg->data_len)
        {
            char *dst;
            size_t to_copy = MIN(rte_pktmbuf_tailroom(rte_pktmbuf_lastseg(m)),
                                 tad_seg->data_len - bytes_copied);

            if (to_copy == 0)
            {
                struct rte_mbuf *m_next;

                if ((m_next = rte_pktmbuf_alloc(sap->pkt_pool)) == NULL)
                {
                    err = TE_ENOMEM;
                    goto out;
                }

                if ((err = rte_pktmbuf_chain(m, m_next)) != 0)
                    goto out;

                continue;
            }

            if ((dst = rte_pktmbuf_append(m, to_copy)) == NULL)
            {
                err = TE_ENOBUFS;
                goto out;
            }

            memcpy(dst, tad_seg->data_ptr + bytes_copied, to_copy);
            bytes_copied += to_copy;
        }

        if (tad_seg->layer_tag != layer_tag_prev)
        {
            /*
             * The next layer starts here.
             * Handle the tag and length of the previous layer.
             */
            handle_layer_info(pkt, layer_tag_prev, layer_data_len, m,
                              &ol_flags_inner, &ol_flags_outer);
            layer_tag_prev = tad_seg->layer_tag;
            layer_data_len = tad_seg->data_len;
        }
        else
        {
            /* This segment belongs to the same layer as the previous one. */
            layer_data_len += tad_seg->data_len;
        }
    }

    /* If data in the last segment(s) is payload, this will do nothing. */
    handle_layer_info(pkt, layer_tag_prev, layer_data_len, m,
                      &ol_flags_inner, &ol_flags_outer);

    m->ol_flags |= ol_flags_inner;

    /*
     * In fact, rte_ring_enqueue() can return -EDQUOT which among other things
     * means that an object was queued, but we consider it to be an error anyway
     */
    err = rte_ring_enqueue(sap->pkt_ring, (void *)m);

out:
    if (err != 0)
    {
        rte_pktmbuf_free(m);
        ERROR("%s(): Failed to convert TAD packet to RTE mbuf "
              "(rc = %d)", __FUNCTION__, err);
    }

    return TE_RC(TE_TAD_CSAP, err);
}
