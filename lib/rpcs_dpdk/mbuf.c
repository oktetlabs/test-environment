/** @file
 * @brief RPC for DPDK MBUF
 *
 * RPC routines implementation to call DPDK (rte_mbuf_* and rte_pktmbuf_*)
 * functions
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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

#define TE_LGR_USER     "RPC DPDK MBUF"

#include "te_config.h"
#ifdef HAVE_PACKAGE_H
#include "package.h"
#endif

#include "rte_config.h"
#include "rte_mbuf.h"

#include "logger_api.h"

#include "unix_internal.h"
#include "tarpc_server.h"
#include "rpcs_dpdk.h"
#include "te_errno.h"

static uint64_t
tarpc_rte_pktmbuf_ol_flags2rpc(uint64_t rte)
{
    uint64_t    rpc = 0;

#define RTE_PKTMBUF_OL_FLAGS2RPC(_bit) \
    do {                                                            \
        uint64_t flag = _bit;                                       \
                                                                    \
        if (rte & flag)                                             \
        {                                                           \
            rte &= ~flag;                                           \
            rpc |= (1UL << TARPC_##_bit);                           \
        }                                                           \
    } while (0)

    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_VLAN_PKT);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_RSS_HASH);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_FDIR);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_L4_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_IP_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_EIP_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_OVERSIZE);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_HBUF_OVERFLOW);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_RECIP_ERR);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_MAC_ERR);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_IEEE1588_PTP);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_FDIR_ID);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_FDIR_FLX);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_RX_QINQ_PKT);

    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_QINQ_PKT);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_L4_NO_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_TCP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_SCTP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_UDP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_L4_MASK);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_IPV6);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_VLAN_PKT);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_OUTER_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_OUTER_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RPC(PKT_TX_OUTER_IPV6);

    RTE_PKTMBUF_OL_FLAGS2RPC(IND_ATTACHED_MBUF);

    RTE_PKTMBUF_OL_FLAGS2RPC(CTRL_MBUF_FLAG);
#undef RTE_PKTMBUF_OL_FLAGS2RPC

    if (rte != 0)
        rpc = (1UL << TARPC_PKT__UNKNOWN);

    return rpc;
}

static int
tarpc_rte_pktmbuf_ol_flags2rte(uint64_t rpc, uint64_t *rte)
{
    uint64_t    rte_tmp = 0;

#define RTE_PKTMBUF_OL_FLAGS2RTE(_bit) \
    do {                                                            \
        uint64_t flag = (1UL << TARPC_##_bit);                      \
                                                                    \
        if (rpc & flag)                                             \
        {                                                           \
            rpc &= ~flag;                                           \
            rte_tmp |= _bit;                                        \
        }                                                           \
    } while (0)

    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_VLAN_PKT);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_RSS_HASH);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_FDIR);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_L4_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_IP_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_EIP_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_OVERSIZE);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_HBUF_OVERFLOW);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_RECIP_ERR);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_MAC_ERR);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_IEEE1588_PTP);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_FDIR_ID);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_FDIR_FLX);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_RX_QINQ_PKT);

    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_QINQ_PKT);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_L4_NO_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_TCP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_SCTP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_UDP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_L4_MASK);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_IPV6);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_VLAN_PKT);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_OUTER_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_OUTER_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RTE(PKT_TX_OUTER_IPV6);

    RTE_PKTMBUF_OL_FLAGS2RTE(IND_ATTACHED_MBUF);

    RTE_PKTMBUF_OL_FLAGS2RTE(CTRL_MBUF_FLAG);
#undef RTE_PKTMBUF_OL_FLAGS2RTE

    if (rpc != 0)
        return (0);
    else
        *rte = rte_tmp;

    return (1);
}

TARPC_FUNC(rte_pktmbuf_pool_create, {},
{
    struct rte_mempool *mp;

    MAKE_CALL(mp = func(in->name, in->n, in->cache_size, in->priv_size,
                        in->data_room_size, in->socket_id));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(mp, ns);
    });
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_alloc, {},
{
    struct rte_mempool *mp = NULL;
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mp, ns);
    });

    MAKE_CALL(m = func(mp));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(m, ns);
    });
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_free, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(func(m));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        RCF_PCH_MEM_INDEX_FREE(in->m, ns);
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_append_data, {},
{
    struct rte_mbuf *m = NULL;
    uint8_t *dst;
    te_errno err = 0;

    if ((in->buf.buf_len != 0) && (in->buf.buf_val == NULL))
    {
        ERROR("Incorrect input data");
        err =  TE_RC(TE_RPCS, TE_EINVAL);
        goto finish;
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(dst = (uint8_t *)rte_pktmbuf_append(m, in->buf.buf_len));

    if (dst == NULL)
    {
        ERROR("Not enough tailroom space in the last segment of the mbuf");
        err = TE_RC(TE_RPCS, TE_ENOSPC);
        goto finish;
    }

    memcpy(dst, in->buf.buf_val, in->buf.buf_len);

finish:
    out->retval = -err;
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_read_data,
{
    COPY_ARG(buf);
},
{
    struct rte_mbuf *m = NULL;
    te_errno err = 0;
    ssize_t bytes_read = 0;
    size_t cur_offset = in->offset;

    if (in->buf.buf_val == NULL)
    {
        ERROR("Incorrect buffer");
        err =  TE_RC(TE_RPCS, TE_EINVAL);
        goto finish;
    }

    if (in->len > in->buf.buf_len)
    {
        ERROR("Not enough room in the specified buffer");
        err = TE_RC(TE_RPCS, TE_ENOSPC);
        goto finish;
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    if (m == NULL)
    {
        ERROR("NULL mbuf pointer isn't valid for 'read' operation");
        err = TE_RC(TE_RPCS, TE_EINVAL);
        goto finish;
    }

    do {
        if (cur_offset < m->data_len)
        {
            size_t bytes_to_copy = MIN(m->data_len - cur_offset,
                                       (in->len - bytes_read));

            memcpy(out->buf.buf_val + bytes_read,
                   rte_pktmbuf_mtod_offset(m, uint8_t *, cur_offset),
                   bytes_to_copy);

            bytes_read += bytes_to_copy;
            cur_offset = 0;
        }
        else
        {
            cur_offset -= m->data_len;
        }
    } while ((in->len - bytes_read) != 0 && (m = m->next) != NULL);

finish:
    out->retval = (err != 0) ? -err : bytes_read;
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_clone, {},
{
    struct rte_mempool *mp = NULL;
    struct rte_mbuf *m_orig = NULL;
    struct rte_mbuf *m_copy;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mp, ns);
    });

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m_orig = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(m_copy = func(m_orig, mp));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(m_copy, ns);
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_prepend_data, {},
{
    struct rte_mbuf *m = NULL;
    uint8_t *dst;
    te_errno err = 0;

    if ((in->buf.buf_len != 0) && (in->buf.buf_val == NULL))
    {
        ERROR("Incorrect input data");
        err =  TE_RC(TE_RPCS, TE_EINVAL);
        goto finish;
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(dst = (uint8_t *)rte_pktmbuf_prepend(m, in->buf.buf_len));

    if (dst == NULL)
    {
        ERROR("Not enough tailroom space in the last segment of the mbuf");
        err = TE_RC(TE_RPCS, TE_ENOSPC);
        goto finish;
    }

    memcpy(dst, in->buf.buf_val, in->buf.buf_len);

finish:
    out->retval = -err;
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_next, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(m->next, ns);
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_pkt_len, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = m->pkt_len;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_alloc_bulk, {},
{
    struct rte_mempool *mp = NULL;
    struct rte_mbuf **mbufs;
    tarpc_rte_mbuf *bulk;
    unsigned int i;
    te_errno err;

    mbufs = calloc(in->count, sizeof(*mbufs));
    if (mbufs == NULL)
    {
        ERROR("Failed to allocate an array for mbuf pointers storage");
        err = TE_RC(TE_RPCS, TE_ENOMEM);
        goto finish;
    }

    bulk = calloc(in->count, sizeof(*bulk));
    if (bulk == NULL)
    {
        ERROR("Failed to allocate an array of RPC mbuf pointers");
        err = TE_RC(TE_RPCS, TE_ENOMEM);
        goto finish;
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mp, ns);
    });

    MAKE_CALL(err = rte_pktmbuf_alloc_bulk(mp, mbufs, in->count));

    neg_errno_h2rpc(&err);
    if (err != 0)
    {
        free(bulk);
        goto finish;
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < in->count; ++i)
            bulk[i] = RCF_PCH_MEM_INDEX_ALLOC(mbufs[i], ns);
    });

    out->bulk.bulk_val = bulk;
    out->bulk.bulk_len = in->count;

finish:
    if (mbufs != NULL)
        free(mbufs);

    out->retval = -err;
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_chain, {},
{
    struct rte_mbuf *head = NULL;
    struct rte_mbuf *tail = NULL;
    te_errno err;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        head = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->head, ns);
        tail = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->tail, ns);
    });

    MAKE_CALL(err = func(head, tail));

    neg_errno_h2rpc(&err);

    out->retval = -err;
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_nb_segs, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = m->nb_segs;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_port, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = m->port;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_set_port, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        m->port = in->port;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_data_len, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = m->data_len;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_vlan_tci, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = m->vlan_tci;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_set_vlan_tci, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        m->vlan_tci = in->vlan_tci;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_vlan_tci_outer, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = m->vlan_tci_outer;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_set_vlan_tci_outer, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        m->vlan_tci_outer = in->vlan_tci_outer;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_flags, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = tarpc_rte_pktmbuf_ol_flags2rpc(m->ol_flags);
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_set_flags, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    if (tarpc_rte_pktmbuf_ol_flags2rte(in->ol_flags, &m->ol_flags))
        out->retval = 0;
    else
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_pool, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(m->pool, ns);
    });
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_headroom, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(out->retval = func(m));
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_tailroom, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(out->retval = func(m));
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_trim, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(out->retval = func(m, in->len));
}
)

TARPC_FUNC_STATIC(rte_pktmbuf_adj, {},
{
    struct rte_mbuf *m = NULL;
    char *new_start_ptr = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(new_start_ptr = func(m, in->len));

    if (new_start_ptr == NULL)
        out->retval = UINT16_MAX;
    else
        out->retval = m->data_off;
}
)
