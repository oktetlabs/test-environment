/** @file
 * @brief TAD RTE mbuf Provider Interface
 *
 * Routines to convert packets from TAD to RTE mbuf representation and vice versa
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

#define TE_LGR_USER     "TAD RTE mbuf"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

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

    if (((m->ol_flags & PKT_RX_VLAN_PKT) == 0) &&
        ((m->ol_flags & PKT_TX_VLAN_PKT) == 0))
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
                         (tad_pkt_seg_free)free);

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

te_errno
tad_rte_mbuf_sap_write(tad_rte_mbuf_sap   *sap,
                       const               tad_pkt *pkt)
{
    struct rte_mbuf        *m;
    tad_pkt_seg            *tad_seg;
    int err = 0;

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
    }

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
