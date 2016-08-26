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

#include "rpc_server.h"
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

static void
tarpc_rte_pktmbuf_packet_type2rpc(struct rte_mbuf *m,
                                  struct tarpc_rte_pktmbuf_packet_type *p_type)
{
#define RTE_PKTMBUF_L2_PTYPE2RPC(_bit) \
    case RTE_PTYPE_L2_##_bit:                                          \
        p_type->l2_type = TARPC_RTE_PTYPE_L2_##_bit;                   \
        break

#define RTE_PKTMBUF_L3_PTYPE2RPC(_bit) \
    case RTE_PTYPE_L3_##_bit:                                          \
        p_type->l3_type = TARPC_RTE_PTYPE_L3_##_bit;                   \
        break

#define RTE_PKTMBUF_L4_PTYPE2RPC(_bit) \
    case RTE_PTYPE_L4_##_bit:                                          \
        p_type->l4_type = TARPC_RTE_PTYPE_L4_##_bit;                   \
        break

#define RTE_PKTMBUF_TUNNEL_PTYPE2RPC(_bit) \
    case RTE_PTYPE_TUNNEL_##_bit:                                      \
        p_type->tun_type = TARPC_RTE_PTYPE_TUNNEL_##_bit;              \
        break

#define RTE_PKTMBUF_INNER_L2_PTYPE2RPC(_bit) \
    case RTE_PTYPE_INNER_L2_##_bit:                                    \
        p_type->inner_l2_type = TARPC_RTE_PTYPE_INNER_L2_##_bit;       \
        break

#define RTE_PKTMBUF_INNER_L3_PTYPE2RPC(_bit) \
    case RTE_PTYPE_INNER_L3_##_bit:                                    \
        p_type->inner_l3_type = TARPC_RTE_PTYPE_INNER_L3_##_bit;       \
        break

#define RTE_PKTMBUF_INNER_L4_PTYPE2RPC(_bit) \
    case RTE_PTYPE_INNER_L4_##_bit:                                    \
        p_type->inner_l4_type = TARPC_RTE_PTYPE_INNER_L4_##_bit;       \
        break

    switch (m->packet_type & RTE_PTYPE_L2_MASK) {
        case 0:
            p_type->l2_type = TARPC_RTE_PTYPE_L2_UNKNOWN;
            break;
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_TIMESYNC);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_ARP);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_LLDP);
        default:
            p_type->l2_type = TARPC_RTE_PTYPE_L2__UNKNOWN;
            break;
    }

    switch (m->packet_type & RTE_PTYPE_L3_MASK) {
        case 0:
            p_type->l3_type = TARPC_RTE_PTYPE_L3_UNKNOWN;
            break;
        RTE_PKTMBUF_L3_PTYPE2RPC(IPV4);
        RTE_PKTMBUF_L3_PTYPE2RPC(IPV4_EXT);
        RTE_PKTMBUF_L3_PTYPE2RPC(IPV6);
        RTE_PKTMBUF_L3_PTYPE2RPC(IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_L3_PTYPE2RPC(IPV6_EXT);
        RTE_PKTMBUF_L3_PTYPE2RPC(IPV6_EXT_UNKNOWN);
        default:
            p_type->l3_type = TARPC_RTE_PTYPE_L3__UNKNOWN;
            break;
    }

    switch (m->packet_type & RTE_PTYPE_L4_MASK) {
        case 0:
            p_type->l4_type = TARPC_RTE_PTYPE_L4_UNKNOWN;
            break;
        RTE_PKTMBUF_L4_PTYPE2RPC(TCP);
        RTE_PKTMBUF_L4_PTYPE2RPC(UDP);
        RTE_PKTMBUF_L4_PTYPE2RPC(FRAG);
        RTE_PKTMBUF_L4_PTYPE2RPC(SCTP);
        RTE_PKTMBUF_L4_PTYPE2RPC(ICMP);
        RTE_PKTMBUF_L4_PTYPE2RPC(NONFRAG);
        default:
            p_type->l4_type = TARPC_RTE_PTYPE_L4__UNKNOWN;
            break;
    }

    switch (m->packet_type & RTE_PTYPE_TUNNEL_MASK) {
        case 0:
            p_type->tun_type = TARPC_RTE_PTYPE_TUNNEL_UNKNOWN;
            break;
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(IP);
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(GRE);
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(VXLAN);
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(NVGRE);
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(GENEVE);
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(GRENAT);
        default:
            p_type->tun_type = TARPC_RTE_PTYPE_TUNNEL__UNKNOWN;
            break;
    }

    switch (m->packet_type & RTE_PTYPE_INNER_L2_MASK) {
        case 0:
            p_type->inner_l2_type = TARPC_RTE_PTYPE_INNER_L2_UNKNOWN;
            break;
        RTE_PKTMBUF_INNER_L2_PTYPE2RPC(ETHER);
        RTE_PKTMBUF_INNER_L2_PTYPE2RPC(ETHER_VLAN);
        default:
            p_type->inner_l2_type = TARPC_RTE_PTYPE_INNER_L2__UNKNOWN;
            break;
    }

    switch (m->packet_type & RTE_PTYPE_INNER_L3_MASK) {
        case 0:
            p_type->inner_l3_type = TARPC_RTE_PTYPE_INNER_L3_UNKNOWN;
            break;
        RTE_PKTMBUF_INNER_L3_PTYPE2RPC(IPV4);
        RTE_PKTMBUF_INNER_L3_PTYPE2RPC(IPV4_EXT);
        RTE_PKTMBUF_INNER_L3_PTYPE2RPC(IPV6);
        RTE_PKTMBUF_INNER_L3_PTYPE2RPC(IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_INNER_L3_PTYPE2RPC(IPV6_EXT);
        RTE_PKTMBUF_INNER_L3_PTYPE2RPC(IPV6_EXT_UNKNOWN);
        default:
            p_type->inner_l3_type = TARPC_RTE_PTYPE_INNER_L3__UNKNOWN;
            break;
    }

    switch (m->packet_type & RTE_PTYPE_INNER_L4_MASK) {
        case 0:
            p_type->inner_l4_type = TARPC_RTE_PTYPE_INNER_L4_UNKNOWN;
            break;
        RTE_PKTMBUF_INNER_L4_PTYPE2RPC(TCP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RPC(UDP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RPC(FRAG);
        RTE_PKTMBUF_INNER_L4_PTYPE2RPC(SCTP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RPC(ICMP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RPC(NONFRAG);
        default:
            p_type->inner_l4_type = TARPC_RTE_PTYPE_INNER_L4__UNKNOWN;
            break;
    }

#undef RTE_PKTMBUF_L2_PTYPE2RPC
#undef RTE_PKTMBUF_L3_PTYPE2RPC
#undef RTE_PKTMBUF_L4_PTYPE2RPC
#undef RTE_PKTMBUF_TUNNEL_PTYPE2RPC
#undef RTE_PKTMBUF_INNER_L2_PTYPE2RPC
#undef RTE_PKTMBUF_INNER_L3_PTYPE2RPC
#undef RTE_PKTMBUF_INNER_L4_PTYPE2RPC
}

static int
tarpc_rte_pktmbuf_packet_type2rte(struct tarpc_rte_pktmbuf_packet_type *p_type,
                                  uint32_t *packet_type)
{
    uint32_t rte_tmp = 0;

#define RTE_PKTMBUF_L2_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_L2_##_bit:                                    \
        rte_tmp |= RTE_PTYPE_L2_##_bit;                                \
        break

#define RTE_PKTMBUF_L3_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_L3_##_bit:                                    \
        rte_tmp |= RTE_PTYPE_L3_##_bit;                                \
        break

#define RTE_PKTMBUF_L4_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_L4_##_bit:                                    \
        rte_tmp |= RTE_PTYPE_L4_##_bit;                                \
        break

#define RTE_PKTMBUF_TUNNEL_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_TUNNEL_##_bit:                                \
        rte_tmp |= RTE_PTYPE_TUNNEL_##_bit;                            \
        break

#define RTE_PKTMBUF_INNER_L2_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_INNER_L2_##_bit:                              \
        rte_tmp |= RTE_PTYPE_INNER_L2_##_bit;                          \
        break

#define RTE_PKTMBUF_INNER_L3_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_INNER_L3_##_bit:                              \
        rte_tmp |= RTE_PTYPE_INNER_L3_##_bit;                          \
        break

#define RTE_PKTMBUF_INNER_L4_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_INNER_L4_##_bit:                              \
        rte_tmp |= RTE_PTYPE_INNER_L4_##_bit;                          \
        break

    switch (p_type->l2_type) {
        case TARPC_RTE_PTYPE_L2_UNKNOWN:
            break;
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_TIMESYNC);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_ARP);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_LLDP);
        default:
            return (0);
    }

    switch (p_type->l3_type) {
        case TARPC_RTE_PTYPE_L3_UNKNOWN:
            break;
        RTE_PKTMBUF_L3_PTYPE2RTE(IPV4);
        RTE_PKTMBUF_L3_PTYPE2RTE(IPV4_EXT);
        RTE_PKTMBUF_L3_PTYPE2RTE(IPV6);
        RTE_PKTMBUF_L3_PTYPE2RTE(IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_L3_PTYPE2RTE(IPV6_EXT);
        RTE_PKTMBUF_L3_PTYPE2RTE(IPV6_EXT_UNKNOWN);
        default:
            return (0);
    }

    switch (p_type->l4_type) {
        case TARPC_RTE_PTYPE_L4_UNKNOWN:
            break;
        RTE_PKTMBUF_L4_PTYPE2RTE(TCP);
        RTE_PKTMBUF_L4_PTYPE2RTE(UDP);
        RTE_PKTMBUF_L4_PTYPE2RTE(FRAG);
        RTE_PKTMBUF_L4_PTYPE2RTE(SCTP);
        RTE_PKTMBUF_L4_PTYPE2RTE(ICMP);
        RTE_PKTMBUF_L4_PTYPE2RTE(NONFRAG);
        default:
            return (0);
    }

    switch (p_type->tun_type) {
        case TARPC_RTE_PTYPE_TUNNEL_UNKNOWN:
            break;
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(IP);
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(GRE);
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(VXLAN);
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(NVGRE);
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(GENEVE);
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(GRENAT);
        default:
            return (0);
    }

    switch (p_type->inner_l2_type) {
        case TARPC_RTE_PTYPE_INNER_L2_UNKNOWN:
            break;
        RTE_PKTMBUF_INNER_L2_PTYPE2RTE(ETHER);
        RTE_PKTMBUF_INNER_L2_PTYPE2RTE(ETHER_VLAN);
        default:
            return (0);
    }

    switch (p_type->inner_l3_type) {
        case TARPC_RTE_PTYPE_INNER_L3_UNKNOWN:
            break;
        RTE_PKTMBUF_INNER_L3_PTYPE2RTE(IPV4);
        RTE_PKTMBUF_INNER_L3_PTYPE2RTE(IPV4_EXT);
        RTE_PKTMBUF_INNER_L3_PTYPE2RTE(IPV6);
        RTE_PKTMBUF_INNER_L3_PTYPE2RTE(IPV4_EXT_UNKNOWN);
        RTE_PKTMBUF_INNER_L3_PTYPE2RTE(IPV6_EXT);
        RTE_PKTMBUF_INNER_L3_PTYPE2RTE(IPV6_EXT_UNKNOWN);
        default:
            return (0);
    }

    switch (p_type->inner_l4_type) {
        case TARPC_RTE_PTYPE_INNER_L4_UNKNOWN:
            break;
        RTE_PKTMBUF_INNER_L4_PTYPE2RTE(TCP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RTE(UDP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RTE(FRAG);
        RTE_PKTMBUF_INNER_L4_PTYPE2RTE(SCTP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RTE(ICMP);
        RTE_PKTMBUF_INNER_L4_PTYPE2RTE(NONFRAG);
        default:
            return (0);
    }

#undef RTE_PKTMBUF_L2_PTYPE2RTE
#undef RTE_PKTMBUF_L3_PTYPE2RTE
#undef RTE_PKTMBUF_L4_PTYPE2RTE
#undef RTE_PKTMBUF_TUNNEL_PTYPE2RTE
#undef RTE_PKTMBUF_INNER_L2_PTYPE2RTE
#undef RTE_PKTMBUF_INNER_L3_PTYPE2RTE
#undef RTE_PKTMBUF_INNER_L4_PTYPE2RTE

    *packet_type = rte_tmp;

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
    COPY_ARG_NOTNULL(buf);
},
{
    struct rte_mbuf *m = NULL;
    te_errno err = 0;
    ssize_t bytes_read = 0;
    size_t cur_offset = in->offset;

    if (out->buf.buf_val == NULL)
    {
        ERROR("Incorrect buffer");
        err =  TE_RC(TE_RPCS, TE_EINVAL);
        goto finish;
    }

    if (in->len > out->buf.buf_len)
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

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_packet_type, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        tarpc_rte_pktmbuf_packet_type2rpc(m, &out->p_type);
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_set_packet_type, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    if (tarpc_rte_pktmbuf_packet_type2rte(&in->p_type, &m->packet_type))
        out->retval = 0;
    else
        out->retval = -TE_RC(TE_RPCS, TE_EINVAL);
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_rss_hash, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->retval = m->hash.rss;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_tx_offload, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        out->tx_offload.l2_len = m->l2_len;
        out->tx_offload.l3_len = m->l3_len;
        out->tx_offload.l4_len = m->l4_len;
        out->tx_offload.tso_segsz = m->tso_segsz;
        out->tx_offload.outer_l3_len = m->outer_l3_len;
        out->tx_offload.outer_l2_len = m->outer_l2_len;
    });
}
)

TARPC_FUNC_STANDALONE(rte_pktmbuf_set_tx_offload, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        m->l2_len = in->tx_offload.l2_len;
        m->l3_len = in->tx_offload.l3_len;
        m->l4_len = in->tx_offload.l4_len;
        m->tso_segsz = in->tx_offload.tso_segsz;
        m->outer_l3_len = in->tx_offload.outer_l3_len;
        m->outer_l2_len = in->tx_offload.outer_l2_len;
    });
}
)

static unsigned int
ta_count_max_segs_within_pattern(tarpc_rte_pktmbuf_redist_in *in, size_t len)
{
    unsigned int i;
    unsigned int j;
    unsigned int nb_segs = 0;

    for (i = 0; i < in->seg_groups.seg_groups_len; ++i)
    {
        for (j = 0;
             j < in->seg_groups.seg_groups_val[i].num;
             ++j)
        {
            nb_segs++;

            if (len <= in->seg_groups.seg_groups_val[i].len)
                goto out;
            else
                len -= in->seg_groups.seg_groups_val[i].len;
        }
    }

out:
    return (nb_segs);
}

static int
rte_pktmbuf_redist(tarpc_rte_pktmbuf_redist_in *in,
                   tarpc_rte_pktmbuf_redist_out *out)
{
    size_t data_count;
    unsigned int i, j, k;
    unsigned int pattern_nb_segs = 0;
    size_t pattern_sum_len = 0;
    uint8_t nb_segs_new = 0;
    struct rte_mbuf *m = NULL;
    struct rte_mbuf *m_cur = NULL;
    uint8_t *m_contig_data = NULL;
    struct rte_mbuf **new_segs = NULL;
    te_errno err = 0;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    /* Initialize out->m to the old pointer (for a possible roll-back) */
    out->m = in->m;

    /* Sanity checks */
    if (m == NULL || m->pkt_len == 0 ||
        in->seg_groups.seg_groups_len == 0 ||
        in->seg_groups.seg_groups_val == NULL)
    {
        err = TE_RC(TE_RPCS, TE_EINVAL);
        goto out;
    }

    /* Calculate the total figures for the pattern */
    for (i = 0; i < in->seg_groups.seg_groups_len; ++i)
    {
        if (in->seg_groups.seg_groups_val[i].num == 0 ||
            in->seg_groups.seg_groups_val[i].len == 0)
        {
            err = TE_RC(TE_RPCS, TE_EINVAL);
            goto out;
        }
        pattern_nb_segs += in->seg_groups.seg_groups_val[i].num;
        pattern_sum_len += in->seg_groups.seg_groups_val[i].num *
                           in->seg_groups.seg_groups_val[i].len;
    }

    /* Determine the total number of segments in the new chain to be built */
    data_count = 0;
    while (data_count < m->pkt_len)
    {
        unsigned int nb_segs_to_add;
        size_t len_to_add = (m->pkt_len - data_count);

        if (pattern_sum_len > len_to_add)
        {
            nb_segs_to_add = ta_count_max_segs_within_pattern(in, len_to_add);
            data_count += len_to_add;
        }
        else
        {
            nb_segs_to_add = pattern_nb_segs;
            data_count += pattern_sum_len;
        }

        if ((nb_segs_new + nb_segs_to_add) >= (1 << (sizeof(m->nb_segs) * 8)))
        {
            err = TE_RC(TE_RPCS, TE_EOVERFLOW);
            goto out;
        }

        nb_segs_new += nb_segs_to_add;
    }

    /* Allocate an array of mbuf pointers for the new chain to be built */
    new_segs = malloc(nb_segs_new * sizeof(*new_segs));
    if (new_segs == NULL)
    {
        err = TE_RC(TE_RPCS, TE_ENOMEM);
        goto out;
    }

    /* Allocate mbuf segments for the new chain to be built */
    err = rte_pktmbuf_alloc_bulk(m->pool, new_segs, nb_segs_new);

    neg_errno_h2rpc(&err);
    if (err != 0)
        goto out;

    /* Allocate a contiguous buffer to store the entire chain data */
    m_contig_data = malloc(m->pkt_len);
    if (m_contig_data == NULL)
    {
        err = TE_RC(TE_RPCS, TE_ENOMEM);
        goto out;
    }

    /* Copy entire chain data into the contiguous buffer */
    for (data_count = 0, m_cur = m; m_cur != NULL;
         data_count += m_cur->data_len, m_cur = m_cur->next)
        memcpy(m_contig_data + data_count,
               rte_pktmbuf_mtod_offset(m_cur, uint8_t *, 0),
               m_cur->data_len);

    /* Distribute the packet data across the new mbuf segments */
    k = 0;
    data_count = 0;
    while (data_count < m->pkt_len)
    {
        for (i = 0; (i < in->seg_groups.seg_groups_len) &&
             (data_count < m->pkt_len); ++i)
        {
                for (j = 0;
                     (j < in->seg_groups.seg_groups_val[i].num) &&
                     (data_count < m->pkt_len);
                     ++j)
                {
                    uint8_t *dst;
                    uint16_t to_copy = MIN((m->pkt_len - data_count),
                             in->seg_groups.seg_groups_val[i].len);

                    m_cur = new_segs[k++];

                    dst = (uint8_t *)rte_pktmbuf_append(m_cur, to_copy);
                    if (dst == NULL)
                    {
                        err = TE_RC(TE_RPCS, TE_ENOSPC);
                        goto out;
                    }

                    memcpy(dst, m_contig_data + data_count, to_copy);

                    data_count += to_copy;
                }
        }
    }

    /* Produce a chain from the segments */
    for (i = 1; i < nb_segs_new; ++i)
    {
        err = rte_pktmbuf_chain(new_segs[0], new_segs[i]);

        neg_errno_h2rpc(&err);
        if (err != 0)
            goto out;
    }

#define RTE_PKTMBUF_COPY_FIELD(_field) new_segs[0]->_field = m->_field
    RTE_PKTMBUF_COPY_FIELD(port);
    RTE_PKTMBUF_COPY_FIELD(ol_flags);
    RTE_PKTMBUF_COPY_FIELD(vlan_tci);
    RTE_PKTMBUF_COPY_FIELD(hash);
    RTE_PKTMBUF_COPY_FIELD(seqn);
    RTE_PKTMBUF_COPY_FIELD(vlan_tci_outer);
    RTE_PKTMBUF_COPY_FIELD(timesync);
    RTE_PKTMBUF_COPY_FIELD(packet_type);
    RTE_PKTMBUF_COPY_FIELD(tx_offload);
#undef RTE_PKTMBUF_COPY_FIELD

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        out->m = RCF_PCH_MEM_INDEX_ALLOC(new_segs[0], ns);
    });

out:
    /* If no error occurred, get rid of the original chain */
    if (err == 0)
    {
        rte_pktmbuf_free(m);
        RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
            RCF_PCH_MEM_INDEX_FREE(in->m, ns);
        });
    }

    free(m_contig_data);

    if (new_segs != NULL)
    {
        if (err != 0)
            for (i = 0; i < nb_segs_new; ++i)
                rte_pktmbuf_free(new_segs[i]);

        free(new_segs);
    }

    return (err != 0) ? (-err) : (nb_segs_new);
}

TARPC_FUNC_STATIC(rte_pktmbuf_redist, {},
{
    MAKE_CALL(out->retval = func(in, out));
}
)
