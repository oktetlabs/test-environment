/** @file
 * @brief TAD Packets Representation
 *
 * Implementation of Traffic Application Domain Packets Representation
 * routines.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD PKT"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_pkt.h"


#undef assert
#define assert(x) \
    ( { do {                    \
        if (!(x))           \
        {                   \
            *(int *)0 = 0;  \
        }                   \
    } while (0); } )


/* See description in tad_pkt.h */
void
tad_pkt_seg_data_free(void *ptr, size_t len)
{
    UNUSED(len);
    free(ptr);
}

/* See description in tad_pkt.h */
void
tad_pkt_init_seg_data(tad_pkt_seg *seg,
                      void *ptr, size_t len, tad_pkt_seg_free free)
{
    seg->data_ptr = ptr;
    seg->data_len = len;
    seg->data_free = free;
}

/* See description in tad_pkt.h */
void
tad_pkt_put_seg_data(tad_pkt *pkt, tad_pkt_seg *seg,
                     void *ptr, size_t len, tad_pkt_seg_free free)
{
    assert(pkt != NULL);
    assert(pkt->n_segs > 0);
    assert(seg != NULL);
    assert(pkt->segs_len >= seg->data_len);
    pkt->segs_len -= seg->data_len;

    tad_pkt_free_seg_data(seg);
    tad_pkt_init_seg_data(seg, ptr, len, free);

    pkt->segs_len += len;

    F_VERB("%s(): pkt=%p n_segs=%u segs_len=%u", __FUNCTION__,
           pkt, pkt->n_segs, (unsigned)pkt->segs_len);
}


/* See description in tad_pkt.h */
unsigned int
tad_pkt_seg_num(const tad_pkt *pkt)
{
    return pkt->n_segs;
}

/* See description in tad_pkt.h */
size_t
tad_pkt_len(const tad_pkt *pkt)
{
    return pkt->segs_len;
}


/* See description in tad_pkt.h */
void *
tad_pkt_opaque(const tad_pkt *pkt)
{
    return pkt->opaque;
}

/* See description in tad_pkt.h */
void
tad_pkt_set_opaque(tad_pkt *pkt, void *opaque,
                   tad_pkt_ctrl_free opaque_free)
{
    assert(pkt != NULL);

    if (pkt->opaque_free != NULL)
        pkt->opaque_free(pkt->opaque);

    pkt->opaque = opaque;
    pkt->opaque_free = opaque_free;
}


/* See description in tad_pkt.h */
void
tad_pkt_set_seg_data_len(tad_pkt *pkt, tad_pkt_seg *seg, size_t new_len)
{
    assert(pkt != NULL);
    assert(seg != NULL);
    pkt->segs_len -= seg->data_len;
    seg->data_len = new_len;
    pkt->segs_len += seg->data_len;
    F_VERB("%s(): pkt=%p n_segs=%u segs_len=%u", __FUNCTION__,
           pkt, pkt->n_segs, (unsigned)pkt->segs_len);
}

/* See description in tad_pkt.h */
void
tad_pkt_append_seg(tad_pkt *pkt, tad_pkt_seg *seg)
{
    CIRCLEQ_INSERT_TAIL(&pkt->segs, seg, links);
    pkt->n_segs++; 
    pkt->segs_len += seg->data_len;
    F_VERB("%s(): pkt=%p n_segs=%u segs_len=%u", __FUNCTION__,
           pkt, pkt->n_segs, (unsigned)pkt->segs_len);
}

/* See description in tad_pkt.h */
void
tad_pkt_prepend_seg(tad_pkt *pkt, tad_pkt_seg *seg)
{
    CIRCLEQ_INSERT_HEAD(&pkt->segs, seg, links);
    pkt->n_segs++; 
    pkt->segs_len += seg->data_len;
    F_VERB("%s(): pkt=%p n_segs=%u segs_len=%u", __FUNCTION__,
           pkt, pkt->n_segs, (unsigned)pkt->segs_len);
}

/* See description in tad_pkt.h */
void
tad_pkt_insert_after_seg(tad_pkt *pkt, tad_pkt_seg *seg,
                         tad_pkt_seg *new_seg)
{
    CIRCLEQ_INSERT_AFTER(&pkt->segs, seg, new_seg, links);
    pkt->n_segs++; 
    pkt->segs_len += new_seg->data_len;
    F_VERB("%s(): pkt=%p n_segs=%u segs_len=%u", __FUNCTION__,
           pkt, pkt->n_segs, (unsigned)pkt->segs_len);
}

/* See description in tad_pkt.h */
tad_pkt_seg *
tad_pkt_alloc_seg(void *data_ptr, size_t data_len,
                  tad_pkt_seg_free data_free)
{
    uint8_t        *mem;
    tad_pkt_seg    *seg;

    mem = malloc(sizeof(tad_pkt_seg) +
                 ((data_ptr == NULL) ?  data_len : 0));
    if (mem == NULL)
        return NULL;

    seg = (tad_pkt_seg *)mem;
    seg->my_free = free;
    if (data_ptr != NULL)
    {
        tad_pkt_init_seg_data(seg, data_ptr, data_len, data_free);
    }
    else if (data_len > 0)
    {
        tad_pkt_init_seg_data(seg, mem + sizeof(tad_pkt_seg),
                              data_len, NULL);
    }
    else
    {
        tad_pkt_init_seg_data(seg, NULL, 0, NULL);
    }

    return seg;
}

/* See description in tad_pkt.h */
void
tad_pkt_cleanup_seg_data(tad_pkt_seg *seg)
{
    F_ENTRY("seg=%p", seg);

    if (seg->data_free != NULL)
        seg->data_free(seg->data_ptr, seg->data_len);
    tad_pkt_init_seg_data(seg, NULL, 0, NULL);
}

/* See description in tad_pkt.h */
void
tad_pkt_free_seg_data(tad_pkt_seg *seg)
{
    if (seg->data_free != NULL)
        seg->data_free(seg->data_ptr, seg->data_len);
}

/* See description in tad_pkt.h */
void
tad_pkt_free_seg(tad_pkt_seg *seg)
{
    tad_pkt_free_seg_data(seg);
    if (seg->my_free != NULL)
        seg->my_free(seg);
}


/* See description in tad_pkt.h */
void
tad_pkt_init_segs(tad_pkt *pkt)
{
    CIRCLEQ_INIT(&pkt->segs);
    pkt->n_segs = 0;
    pkt->segs_len = 0;
}

/* See description in tad_pkt.h */
void
tad_pkt_cleanup_segs(tad_pkt *pkt)
{
    tad_pkt_seg    *p;

    F_ENTRY("pkt=%p", pkt);

    /* 
     * Clean up each segment of the list in reverse direction.
     * Don't care about integrity of the total packet data length.
     */
    for (p = pkt->segs.cqh_last;
         p != (void *)&(pkt->segs);
         p = p->links.cqe_prev)
    {
        tad_pkt_cleanup_seg_data(p);
    }

    /* Reset packet total data length */
    pkt->segs_len = 0;
}

/* See description in tad_pkt.h */
void
tad_pkt_free_segs(tad_pkt *pkt)
{
    tad_pkt_seg    *p, *q;

    /* 
     * Free each segment of the list in reverse direction.
     * Don't care about list integrity.
     */
    for (p = pkt->segs.cqh_last; p != (void *)&(pkt->segs); p = q)
    {
        q = p->links.cqe_prev;
        tad_pkt_free_seg(p);
    }

    /* Initialize packet from scratch */
    tad_pkt_init_segs(pkt);
}

/* See description in tad_pkt.h */
void
tad_pkt_init(tad_pkt *pkt, tad_pkt_ctrl_free my_free,
             void *opaque, tad_pkt_ctrl_free opaque_free)
{
    tad_pkt_init_segs(pkt);

    pkt->opaque = opaque;
    pkt->opaque_free = opaque_free;

    pkt->my_free = my_free;
}

/* See description in tad_pkt.h */
void
tad_pkt_cleanup(tad_pkt *pkt)
{
    F_ENTRY("pkt=%p", pkt);

    /* Free packet segments data */
#if 1
    tad_pkt_free_segs(pkt);
#else
    tad_pkt_cleanup_segs(pkt);
#endif

    /* TODO: Is it necessary to cleanup opaque data here? */
}

/* See description in tad_pkt.h */
void
tad_pkt_free(tad_pkt *pkt)
{
    /* Free packet segments */
    tad_pkt_free_segs(pkt);

    /* Free opaque data */
    if (pkt->opaque_free != NULL)
        pkt->opaque_free(pkt->opaque);

    /* Free packet itself */
    if (pkt->my_free != NULL)
        pkt->my_free(pkt);
}


/**
 * Callback to put packet segments data pointers with length to IO
 * vector.
 *
 * This function complies with tad_pkt_enumerate_seg_cb prototype.
 */
static te_errno
tad_pkt_segs_to_iov_cb(const tad_pkt *pkt, tad_pkt_seg *seg,
                       unsigned int seg_num, void *opaque)
{
    struct iovec   *iov = opaque;
    
    UNUSED(pkt);
    assert(iov != NULL);
    iov[seg_num].iov_base = seg->data_ptr;
    iov[seg_num].iov_len = seg->data_len;

    return 0;
}

/* See description in tad_pkt.h */
te_errno
tad_pkt_segs_to_iov(const tad_pkt *pkt, struct iovec *iov, size_t count)
{
    /* Current implementation does not use it */
    UNUSED(count);

    return tad_pkt_enumerate_seg(pkt, tad_pkt_segs_to_iov_cb, iov);
}


/* See description in tad_pkt.h */
unsigned int
tad_pkts_get_num(const tad_pkts *pkts)
{
    return pkts->n_pkts;
}

/* See description in tad_pkt.h */
void
tad_pkts_init(tad_pkts *pkts)
{
    CIRCLEQ_INIT(&pkts->pkts);
    pkts->n_pkts = 0;
}

/* See description in tad_pkt.h */
void
tad_cleanup_pkts(tad_pkts *pkts)
{
    tad_pkt    *p;

    F_ENTRY("pkts=%p", pkts);

    /* 
     * Clean up each packet of the list in reverse direction.
     */
    TAD_PKT_FOR_EACH_PKT_REV(&pkts->pkts, p)
    {
        tad_pkt_cleanup(p);
    }
}

/* See description in tad_pkt.h */
void
tad_free_pkts(tad_pkts *pkts)
{
    tad_pkt    *p, *q;

    /* 
     * Free each packet of the list in reverse direction.
     * Don't care about list integrity.
     */
    for (p = pkts->pkts.cqh_last; p != (void *)&(pkts->pkts); p = q)
    {
        q = p->links.cqe_prev;
        tad_pkt_free(p);
    }

    /* Initialize list of packets from scratch */
    tad_pkts_init(pkts);
}

/* See description in tad_pkt.h */
void
tad_pkts_add_one(tad_pkts *pkts, tad_pkt *pkt)
{
    CIRCLEQ_INSERT_TAIL(&pkts->pkts, pkt, links);
    pkts->n_pkts++;
}

/* See description in tad_pkt.h */
void
tad_pkts_del_one(tad_pkts *pkts, tad_pkt *pkt)
{
    CIRCLEQ_REMOVE(&pkts->pkts, pkt, links);
    pkts->n_pkts--;
}

/* See description in tad_pkt.h */
void
tad_pkts_move(tad_pkts *dst, tad_pkts *src)
{
    if (src->pkts.cqh_first != (void *)&src->pkts)
    {
        src->pkts.cqh_last->links.cqe_next = (void *)&dst->pkts;
        src->pkts.cqh_first->links.cqe_prev = dst->pkts.cqh_last;
        if (dst->pkts.cqh_first == (void *)&dst->pkts)
            dst->pkts.cqh_first = src->pkts.cqh_first;
        else
            dst->pkts.cqh_last->links.cqe_next = src->pkts.cqh_first;
        dst->pkts.cqh_last = src->pkts.cqh_last;

        dst->n_pkts += src->n_pkts;
        tad_pkts_init(src);
    }
}


/* See description in tad_pkt.h */
te_errno
tad_pkts_add_new_seg(tad_pkts *pkts, te_bool header,
                     void *data_ptr, size_t data_len,
                     tad_pkt_seg_free data_free)
{
    uint8_t        *mem;
    tad_pkt        *pkt;
    tad_pkt_seg    *seg;
    uint8_t        *data;

    assert(pkts != NULL);
    mem = malloc(tad_pkts_get_num(pkts) * (sizeof(tad_pkt_seg) +
                                           ((data_ptr == NULL) ?
                                                data_len : 0)));
    if (mem == NULL)
        return TE_OS_RC(TE_TAD_PKT, errno);

    seg = (tad_pkt_seg *)mem;
    data = (uint8_t *)(seg + tad_pkts_get_num(pkts));

    for (pkt = pkts->pkts.cqh_first;
         pkt != (void *)&(pkts->pkts);
         pkt = pkt->links.cqe_next, seg++)
    {
        /* Set non-NULL free function for the first packet segment only */
        seg->my_free = (pkt == pkts->pkts.cqh_first) ? free : NULL;

        if (data_ptr != 0)
        {
            tad_pkt_init_seg_data(seg, data_ptr, data_len,
                                  (pkt == pkts->pkts.cqh_first) ?
                                      data_free : NULL);
        }
        else if (data_len > 0)
        {
            tad_pkt_init_seg_data(seg, data, data_len, NULL);
            data += data_len;
        }
        else
        {
            tad_pkt_init_seg_data(seg, NULL, 0, NULL);
        }

        if (header)
            tad_pkt_prepend_seg(pkt, seg);
        else
            tad_pkt_append_seg(pkt, seg);
    }

    return 0;
}


/* See description in tad_pkt.h */
tad_pkt *
tad_pkt_alloc(unsigned int n_segs, size_t first_seg_len)
{
    uint8_t        *mem;
    tad_pkt        *pkt;
    tad_pkt_seg    *seg;
    uint8_t        *data;
    unsigned int    i;

    mem = malloc(sizeof(tad_pkt) + n_segs * (sizeof(tad_pkt_seg) +
                 first_seg_len));
    if (mem == NULL)
        return NULL;

    pkt = (tad_pkt *)mem;
    seg = (tad_pkt_seg *)(pkt + 1);
    data = (uint8_t *)(seg + n_segs);

    tad_pkt_init(pkt, free, NULL, NULL);

    for (i = 0; i < n_segs; ++i, ++seg)
    {
        seg->my_free = NULL;
        if (i == 0 && first_seg_len > 0)
        {
            tad_pkt_init_seg_data(seg, data, first_seg_len, NULL);
        }
        else
        {
            tad_pkt_init_seg_data(seg, NULL, 0, NULL);
        }
        tad_pkt_append_seg(pkt, seg);
    }

    F_EXIT("pkt=%p n_segs=%u len=%u", pkt, tad_pkt_seg_num(pkt),
           tad_pkt_len(pkt));

    return pkt;
}


/* See description in tad_pkt.h */
te_errno
tad_pkts_alloc(tad_pkts *pkts, unsigned int n_pkts, unsigned int n_segs,
               size_t first_seg_len)
{
    uint8_t        *mem;
    tad_pkt        *pkt;
    tad_pkt_seg    *seg;
    uint8_t        *data;
    unsigned int    i, j;

    assert(n_pkts > 0);
    mem = malloc(n_pkts * (sizeof(tad_pkt) + 
                           n_segs * (sizeof(tad_pkt_seg) +
                                     first_seg_len)));
    if (mem == NULL)
        return TE_OS_RC(TE_TAD_PKT, errno);

    pkt = (tad_pkt *)mem;
    seg = (tad_pkt_seg *)(pkt + n_pkts);
    data = (uint8_t *)(seg + n_pkts * n_segs);

    for (i = 0; i < n_pkts; ++i, ++pkt)
    {
        /* Set non-NULL free function for the first packet only */
        tad_pkt_init(pkt, (i == 0) ? free : NULL, NULL, NULL);

        for (j = 0; j < n_segs; ++j, ++seg)
        {
            seg->my_free = NULL;
            if (j == 0 && first_seg_len > 0)
            {
                tad_pkt_init_seg_data(seg, data, first_seg_len, NULL);
                data += first_seg_len;
            }
            else
            {
                tad_pkt_init_seg_data(seg, NULL, 0, NULL);
            }
            tad_pkt_append_seg(pkt, seg);
        }

        tad_pkts_add_one(pkts, pkt);
    }

    return 0;
}


/* See description in tad_pkt.h */
te_errno
tad_pkt_flatten_copy(tad_pkt *pkt, uint8_t **data, size_t *len)
{
    size_t          total_len;
    tad_pkt_seg    *seg;
    size_t          rest_len;
    uint8_t        *ptr;
    size_t          copy_len;

    if (pkt == NULL || data == NULL || (*data != NULL && len == NULL))
        return TE_RC(TE_TAD_PKT, TE_EINVAL);

    total_len = (len == NULL || *len == 0) ? tad_pkt_len(pkt) : *len;
    if (*data == NULL)
    {
        *data = malloc(total_len);
        if (*data == NULL)
            return TE_RC(TE_TAD_PKT, TE_ENOMEM);
    }

    ptr = *data;
    rest_len = total_len;
    TAD_PKT_FOR_EACH_SEG_FWD(&pkt->segs, seg)
    {
        copy_len = MIN(seg->data_len, rest_len);
        memcpy(ptr, seg->data_ptr, copy_len);
        ptr += copy_len;
        rest_len -= copy_len;
    }
    if (len != NULL)
    {
        if (*len > 0 && *len < tad_pkt_len(pkt))
            return TE_RC(TE_TAD_PKT, TE_ESMALLBUF);
        *len = tad_pkt_len(pkt);
    }
    return 0;
}


/* See description in tad_pkt.h */
te_errno
tad_pkt_enumerate_seg(const tad_pkt *pkt, tad_pkt_seg_enum_cb func,
                      void *opaque)
{
    tad_pkt_seg    *seg;
    unsigned int    seg_num = 0;
    te_errno        rc;

    TAD_PKT_FOR_EACH_SEG_FWD(&pkt->segs, seg)
    {
        if ((rc = func(pkt, seg, seg_num++, opaque)) != 0)
            return rc;
    }
    return 0;
}


/* See description in tad_pkt.h */
te_errno
tad_pkt_enumerate(tad_pkts *pkts, tad_pkt_enum_cb func, void *opaque)
{
    tad_pkt    *pkt;
    te_errno    rc;

    TAD_PKT_FOR_EACH_PKT_FWD(&pkts->pkts, pkt)
    {
        if ((rc = func(pkt, opaque)) != 0)
            return rc;
    }
    return 0;
}


/** 
 * Structure to pass the first segment enumeration data as one pointer.
 */
typedef struct tad_pkts_enumerate_first_segs_cb_data {
    tad_pkt_seg_enum_cb     func;   /**< User callback function */
    void                   *opaque; /**< User opaque data */
} tad_pkts_enumerate_first_segs_cb_data;

/**
 * Wrapper of user callback function to transparently interrupt segments
 * enumeration on the first one.
 * 
 * This function complies with tad_pkt_seg_enum_cb prototype.
 *
 * @return Status code.
 * @retval TE_EINTR     Interrupt enumeration
 */
static te_errno
tad_pkts_enumerate_first_segs_seg_cb(const tad_pkt *pkt, tad_pkt_seg *seg,
                                     unsigned int seg_num, void *opaque)
{
    tad_pkts_enumerate_first_segs_cb_data  *data = opaque;
    te_errno                                rc;

    assert(seg_num == 0);

    rc = data->func(pkt, seg, seg_num, data->opaque);
    if (rc == 0)
        rc = TE_RC(TE_TAD_PKT, TE_EINTR);
    return rc;
}

/**
 * Callback function to enumeration the first segment of each packet in
 * the list.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_pkts_enumerate_first_segs_pkt_cb(tad_pkt *pkt, void *opaque)
{
    te_errno    rc;

    rc = tad_pkt_enumerate_seg(pkt, tad_pkts_enumerate_first_segs_seg_cb,
                               opaque);
    if (TE_RC_GET_ERROR(rc) == TE_EINTR)
        rc = 0;
    return rc;
}

/* See description in tad_pkt.h */
te_errno 
tad_pkts_enumerate_first_segs(tad_pkts *pkts, tad_pkt_seg_enum_cb  func,
                              void *opaque)
{
    tad_pkts_enumerate_first_segs_cb_data   data = { func, opaque };

    return tad_pkt_enumerate(pkts, tad_pkts_enumerate_first_segs_pkt_cb,
                             &data);
}


/**
 * Get the next not empty segment of the packet.
 *
 * @param pkt       Packet
 * @Param seg       Current segment or NULL to get the first
 *
 * @return Pointer to a segment with non-zero length or NULL
 */
static tad_pkt_seg *
tad_pkt_next_not_empty_seg(tad_pkt *pkt, tad_pkt_seg *seg)
{
    do {
        if (seg == NULL)
            seg = pkt->segs.cqh_first;
        else
            seg = seg->links.cqe_next;
    } while (seg != NULL && seg->data_len == 0);

    return seg;
}



/** Fragmentation callback data */
typedef struct tad_pkt_fragment_cb_data {
    te_bool         skip_first_seg; /**< Skip the first segment */
    size_t          frag_data_len;  /**< Maximum fragment length */
    tad_pkt        *src_pkt;        /**< Source packet pointer */
    tad_pkt_seg    *src_seg;        /**< Current segment of the source
                                         packet */
    uint8_t        *src_data;       /**< Current data pointer in the
                                         segment */
    size_t          src_len;        /**< Length of the rest data in
                                         the current segment */
} tad_pkt_fragment_cb_data;

/**
 * Packet fragmentation callback. It is called for each fragment packet
 * to be created.
 *
 * @param pkt       Fragment packet
 * @param opaque    Pointer to fragmentation data
 *                  (tad_pkt_fragment_cb_data *)
 *
 * @return Status code.
 */
static te_errno
tad_pkt_fragment_cb(tad_pkt *pkt, void *opaque)
{
    tad_pkt_fragment_cb_data   *data = (tad_pkt_fragment_cb_data *)opaque;
    
    size_t          dst_rest = data->frag_data_len;
    tad_pkt_seg    *dst_seg = tad_pkt_first_seg(pkt);
    size_t          dst_seg_len;
    tad_pkt_seg    *new_seg;

    F_ENTRY("pkt=%p skip_first_seg=%d src_pkt=%p src_seg=%p src_data=%p "
            "src_len=%u", pkt, (int)data->skip_first_seg, data->src_pkt,
            data->src_seg, data->src_data, (unsigned)data->src_len);

    /* If we have allocated additional segment as header, skip it */
    if (data->skip_first_seg)
    {
        dst_seg = tad_pkt_next_seg(pkt, dst_seg);
        assert(dst_seg != NULL);
    }
    /* Destination segment have to be empty yet */
    assert(dst_seg->data_ptr == NULL && dst_seg->data_len == 0);

    F_VERB("%s(): Destination segment is %p", __FUNCTION__, dst_seg);

    do {
        if (data->src_len == 0)
        {
            data->src_seg = tad_pkt_next_not_empty_seg(data->src_pkt,
                                                       data->src_seg);
            if (data->src_seg == NULL)
            {
                F_VERB("%s(): No more non-empty source segments",
                       __FUNCTION__);
                return 0;
            }
            data->src_data = data->src_seg->data_ptr;
            data->src_len = data->src_seg->data_len;
            F_VERB("%s(): Next source segment ptr=%p len=%u",
                   __FUNCTION__, data->src_data, (unsigned)data->src_len);
        }

        /* 
         * Destination segment length can't be large than total
         * fragments length or source segment length
         */
        dst_seg_len = MIN(data->src_len, dst_rest);

        tad_pkt_put_seg_data(pkt, dst_seg, data->src_data, dst_seg_len,
                             NULL);
        F_VERB("%s(): destionation segment %p put ptr=%p len=%u",
               __FUNCTION__, dst_seg, data->src_data, dst_seg_len);

        /* Data remaining in the source segment */
        data->src_data += dst_seg_len;
        data->src_len -= dst_seg_len;
        /* Space remaining in the fragment */
        dst_rest -= dst_seg_len;

        if (dst_rest == 0)
        {
            F_VERB("%s(): No space rest in destination packet",
                   __FUNCTION__);
            break;
        }

        new_seg = tad_pkt_alloc_seg(NULL, 0, NULL);
        if (new_seg == NULL)
        {
            return TE_RC(TE_TAD_PKT, TE_ENOMEM);
        }
        tad_pkt_insert_after_seg(pkt, dst_seg, new_seg);
        dst_seg = new_seg;
        F_VERB("%s(): New segment %p allocated", __FUNCTION__, new_seg);

    } while (TRUE);

    return 0;
}

/* See description in tad_pkt.h */
te_errno
tad_pkt_fragment(tad_pkt *pkt, size_t frag_data_len,
                 ssize_t add_seg_len, te_bool header,
                 tad_pkts *pkts)
{
    struct tad_pkt_fragment_cb_data cb_data;

    te_bool         add_seg = (add_seg_len != -1);
    unsigned int    n_frags;
    tad_pkts        frags;
    te_errno        rc;

    if (pkt == NULL || pkts == NULL || frag_data_len == 0)
        return TE_RC(TE_TAD_PKT, TE_EINVAL);

    if (tad_pkt_len(pkt) == 0)
        return 0;
    assert(tad_pkt_seg_num(pkt) > 0);

    /* Calculate number of fragment packets */
    n_frags = (tad_pkt_len(pkt) + frag_data_len - 1) / frag_data_len;

    /* Initialize empty fragment packets list */
    tad_pkts_init(&frags);

    /* Allocate fragment packets */
    rc = tad_pkts_alloc(&frags, n_frags, add_seg ? 2 : 1,
                        add_seg ? add_seg_len : 0);
    if (rc != 0)
        return rc;

    /* Do fragmentation */
    cb_data.skip_first_seg = add_seg && header;
    cb_data.frag_data_len = frag_data_len;
    cb_data.src_pkt = pkt;
    cb_data.src_seg = NULL;
    cb_data.src_data = NULL;
    cb_data.src_len = 0;
    rc = tad_pkt_enumerate(&frags, tad_pkt_fragment_cb, &cb_data);
    if (rc != 0)
    {
        tad_free_pkts(&frags);
        return rc;
    }

    /* Move fragments from own list to provided by caller */
    tad_pkts_move(pkts, &frags);

    return 0;
}


/** 
 * Data to get segments which overlap with interested fragment
 * of the packet.
 */
typedef struct tad_pkt_get_frag_cb_data {
    size_t      frag_off;   /**< Interested fragment offset */
    size_t      frag_end;   /**< End of the interested fragment */
    size_t      seg_off;    /**< Current segment offset in the packet */
    tad_pkt    *dst;        /**< Packet to appent segments of the
                                 interested packet */
} tad_pkt_get_frag_cb_data;

/**
 * Callback function to get segments which overlap with interested
 * fragment of the source packet.
 *
 * This function complies with tad_pkt_seg_enum_cb prototype.
 */
static te_errno
tad_pkt_get_frag_cb(const tad_pkt *pkt, tad_pkt_seg *seg,
                    unsigned int seg_num, void *opaque)
{
    tad_pkt_get_frag_cb_data   *data = opaque;
    size_t                      next_seg_off;

    F_ENTRY("pkt=%p seg=%p seg_num=%u opaque=%p "
            "{frag_off=%u frag_end=%u seg_off=%u dst=%p}",
            pkt, seg, seg_num, opaque,
            data->frag_off, data->frag_end, data->seg_off, data->dst);

    next_seg_off = data->seg_off + seg->data_len;

    if (data->frag_end > data->seg_off &&
        data->frag_off < next_seg_off)
    {
        uint8_t        *ptr = seg->data_ptr;
        ssize_t         off = data->frag_off - data->seg_off;
        size_t          len = MIN(data->frag_end, next_seg_off) -
                              MAX(data->frag_off, data->seg_off);
        tad_pkt_seg    *dst_seg;

        if (off < 0)
            off = 0;
        if (len > seg->data_len)
            len = seg->data_len;
        
        dst_seg = tad_pkt_alloc_seg(ptr + off, len, NULL);
        if (dst_seg == NULL)
            return TE_RC(TE_TAD_PKT, TE_ENOMEM);

        tad_pkt_append_seg(data->dst, dst_seg);
        F_VERB("%s(): Segment off=%u len=%u appended", __FUNCTION__,
               off, len);
    }
    data->seg_off = next_seg_off;

    F_EXIT();    

    /* 
     * TODO: May be it is usefull to interrupt iteration here,
     * if we went after the interested fragment.
     */
    return 0;
}

/* See description in tad_pkt.h */
te_errno
tad_pkt_get_frag(tad_pkt *dst, tad_pkt *src,
                 size_t frag_off, size_t frag_len,
                 tad_pkt_get_frag_mode mode)
{
    tad_pkt_get_frag_cb_data  data =
        { frag_off, frag_off + frag_len, 0, dst };

    int         add_seg_len;
    te_errno    rc;

    F_ENTRY("off=%u len=%u mode=%u", (unsigned)frag_off,
            (unsigned)frag_len, mode);

    /* At first, check sizes */
    add_seg_len = frag_off + frag_len - tad_pkt_len(src);
    if (add_seg_len > 0)
    {
        if ((size_t)add_seg_len > frag_len)
            add_seg_len = frag_len;

        F_VERB("%s(): No enough data in source packet (len=%u) "
               "to get fragment %u+%u=%u", __FUNCTION__,
               (unsigned)tad_pkt_len(src), (unsigned)frag_off,
               (unsigned)frag_len, (unsigned)(frag_off + frag_len));
        switch (mode)
        {
            case TAD_PKT_GET_FRAG_ERROR:
                ERROR("Source packet is too small (%u bytes) to get "
                      "fragment %u+%u=%u", tad_pkt_len(src),
                      frag_off, frag_len, frag_off + frag_len);
                return TE_RC(TE_TAD_PKT, TE_E2BIG);

            case TAD_PKT_GET_FRAG_TRUNC:
                add_seg_len = 0;
                F_VERB("%s(): Truncating requested fragment",
                       __FUNCTION__);
                break;

            case TAD_PKT_GET_FRAG_RAND:
            case TAD_PKT_GET_FRAG_ZERO:
                /* Do nothing special */
                F_VERB("%s(): One more segment with %s data will be "
                       "added", __FUNCTION__,
                       (mode == TAD_PKT_GET_FRAG_ZERO) ? "zero" : "random");
                break;

            default:
                ERROR("%s(): Unknown get fragment mode", __FUNCTION__);
                return TE_RC(TE_TAD_PKT, TE_EINVAL);
        }
    }

    rc = tad_pkt_enumerate_seg(src, tad_pkt_get_frag_cb, &data);
    if (rc != 0)
        return rc;

    if (add_seg_len > 0)
    {
        tad_pkt_seg    *seg;

        seg = tad_pkt_alloc_seg(NULL, add_seg_len, NULL);
        if (seg == NULL)
            return TE_RC(TE_TAD_PKT, TE_ENOMEM);

        if (mode == TAD_PKT_GET_FRAG_ZERO)
        {
            memset(seg->data_ptr, 0, seg->data_len);
        }
        else if (mode == TAD_PKT_GET_FRAG_RAND)
        {
            uint8_t        *ptr = seg->data_ptr;
            unsigned int    i;

            for (i = 0; i < seg->data_len; ++i)
                ptr[i] = rand();
        }
        else
            assert(FALSE);

        tad_pkt_append_seg(dst, seg);
        F_VERB("%s(): Additional segment appended", __FUNCTION__);
    }

    F_EXIT("OK");

    return 0;
}


/* See description in tad_pkt.h */
void
tad_pkt_read(const tad_pkt *pkt, const tad_pkt_seg *seg,
             size_t off, size_t len, uint8_t *dst)
{
    size_t  r;

    assert(pkt != NULL);
    assert(seg != NULL);
    assert(off < seg->data_len);
    assert(dst != NULL);

    F_ENTRY("pkt=%p seg=%p off=%u len=%u dst=%p",
            pkt, seg, (unsigned)off, (unsigned)len, dst);

    r = MIN(len, seg->data_len - off);
    memcpy(dst, (uint8_t *)(seg->data_ptr) + off, r);
    len -= r;
    if (len > 0)
    {
        do {
            dst += r;
            seg = tad_pkt_next_seg(pkt, seg);
            assert(seg != NULL);
            r = MIN(len, seg->data_len);
            memcpy(dst, seg->data_ptr, r);
            len -= r;
        } while (len > 0);
    }
}

/* See description in tad_pkt.h */
void
tad_pkt_read_bits(const tad_pkt *pkt, size_t bitoff, size_t bitlen,
                  uint8_t *dst)
{
    const tad_pkt_seg  *seg;
    size_t              off;

    F_ENTRY("pkt=%p bitoff=%u bitlen=%u dst=%p",
            pkt, (unsigned)bitoff, (unsigned)bitlen, dst);

    assert((tad_pkt_len(pkt) << 3) >= (bitoff + bitlen));

    /* Find the first segment with data to read */
    seg = tad_pkt_first_seg(pkt);
    assert(seg != NULL);
    for (off = bitoff >> 3; off >= seg->data_len; )
    {
        off -= seg->data_len;
        seg = tad_pkt_next_seg(pkt, seg);
        assert(seg != NULL);
    }

    if (((bitoff & 7) == 0) && ((bitlen & 7) == 0))
    {
        /* Everything it byte-aligned */
        tad_pkt_read(pkt, seg, off, bitlen >> 3, dst);
    }
    else if (((bitoff + bitlen) & 7) == 0)
    {
        /* End of the data to read is byte aligned */

        /* Read required part of the first byte */
        *dst = ((uint8_t *)(seg->data_ptr))[off] &
               (0xff >> (bitoff & 7));

        if ((bitlen >> 3) > 0)
        {
            /* Possibly we run out from the current segment */
            for (++off; off >= seg->data_len; off -= seg->data_len)
            {
                seg = tad_pkt_next_seg(pkt, seg);
                assert(seg != NULL);
            }
            tad_pkt_read(pkt, seg, off, bitlen >> 3, dst + 1);
        }
    }
    else if ((bitoff + bitlen) > 8)
    {
        /* No support yet */
        assert(FALSE);
    }
    else
    {
        unsigned int shift = 8 - bitoff - bitlen;

        *dst = (((uint8_t *)(seg->data_ptr))[off] >> shift) &
               (0xff >> (8 - bitlen));
    }
}


/* See description in tad_pkt.h */
/**
 * Match packet content by mask.
 *
 * @param pkt           Packet
 * @param len           Length of the mask/value
 * @param mask          Mask bytes
 * @param value         Value bytes
 * @param exact_len     Is packet length should be equal to length of
 *                      the mask or may be greater?
 *
 * @return Status code.
 */
te_errno
tad_pkt_match_mask(const tad_pkt *pkt, size_t len, const uint8_t *mask,
                   const uint8_t *value, te_bool exact_len)
{
    const tad_pkt_seg  *seg;
    size_t              slen;
    const uint8_t      *d, *m, *v;

    if (exact_len && (tad_pkt_len(pkt) != len))
    {
        VERB("%s(): mask_len %u not equal packet len %u",
             __FUNCTION__, (unsigned)len, (unsigned)tad_pkt_len(pkt));
        return TE_ETADNOTMATCH;
    }

    if (len > tad_pkt_len(pkt))
        len = tad_pkt_len(pkt);

    F_VERB("%s(): length to be matched is %u", __FUNCTION__,
           (unsigned)len);

    m = mask; v = value;
    TAD_PKT_FOR_EACH_SEG_FWD(&pkt->segs, seg)
    {
        for (slen = seg->data_len, d = seg->data_ptr;
             (len > 0) && (slen > 0);
             d++, m++, v++, len--, slen--)
        {
            if ((*d & *m) != (*v & *m))
            { 
                return TE_ETADNOTMATCH;
            }
        }
        if (len == 0)
            break;
    }
    assert(len == 0);

    return 0;
}
