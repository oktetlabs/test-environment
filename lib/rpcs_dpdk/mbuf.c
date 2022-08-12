/** @file
 * @brief RPC for DPDK MBUF
 *
 * RPC routines implementation to call DPDK (rte_mbuf_* and rte_pktmbuf_*)
 * functions
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC DPDK MBUF"

#include "te_config.h"

#include "rte_config.h"
#include "rte_mbuf.h"
#include "rte_ether.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"
#include "rpc_dpdk_defs.h"
#include "te_errno.h"

static uint64_t
tarpc_rte_pktmbuf_ol_flags2rpc(uint64_t rte)
{
    uint64_t    rpc = 0;

#define RTE_PKTMBUF_MASK_OL_FLAGS2RPC(_rte_mask, _rte_value, _rpc_value) \
    do {                                                                 \
        if ((rte & _rte_mask) == _rte_value)                             \
        {                                                                \
            rte &= ~_rte_value;                                          \
            rpc |= _rpc_value;                                           \
        }                                                                \
    } while (0)

#define RTE_PKTMBUF_OL_FLAGS2RPC(_bit) \
    RTE_PKTMBUF_MASK_OL_FLAGS2RPC(_bit, _bit, (UINT64_C(1) << TARPC_##_bit))
#define RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RPC(_rte_value)                      \
    RTE_PKTMBUF_MASK_OL_FLAGS2RPC(RTE_MBUF_F_RX_IP_CKSUM_MASK, _rte_value, \
                                  TARPC_##_rte_value)
#define RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RPC(_rte_value)                      \
    RTE_PKTMBUF_MASK_OL_FLAGS2RPC(RTE_MBUF_F_RX_L4_CKSUM_MASK, _rte_value, \
                                  TARPC_##_rte_value)

    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_VLAN);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_VLAN_STRIPPED);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_RSS_HASH);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_FDIR);

    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_IP_CKSUM_UNKNOWN);
    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_IP_CKSUM_BAD);
    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_IP_CKSUM_GOOD);
    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_IP_CKSUM_NONE);

    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_L4_CKSUM_UNKNOWN);
    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_L4_CKSUM_BAD);
    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_L4_CKSUM_GOOD);
    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RPC(RTE_MBUF_F_RX_L4_CKSUM_NONE);

    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_OUTER_IP_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_IEEE1588_PTP);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_FDIR_ID);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_FDIR_FLX);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_QINQ);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_RX_QINQ_STRIPPED);

    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_QINQ);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_TCP_SEG);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_L4_NO_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_TCP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_SCTP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_UDP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_L4_MASK);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_IPV6);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_VLAN);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_OUTER_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_OUTER_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_OUTER_IPV6);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_OUTER_UDP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_TUNNEL_VXLAN);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_TUNNEL_GENEVE);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_TX_TUNNEL_GRE);

    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_INDIRECT);
    RTE_PKTMBUF_OL_FLAGS2RPC(RTE_MBUF_F_EXTERNAL);

#undef RTE_PKTMBUF_OL_FLAGS2RPC

    if (rte != 0)
        rpc |= (1UL << TARPC_RTE_MBUF_F__UNKNOWN);

    return rpc;
}

static int
tarpc_rte_pktmbuf_ol_flags2rte(uint64_t rpc, uint64_t *rte)
{
    uint64_t    rte_tmp = 0;

#define RTE_PKTMBUF_MASK_OL_FLAGS2RTE( _rpc_mask, _rpc_value, _rte_flag)  \
    do {                                                                  \
        if ((rpc & _rpc_mask) == _rpc_value)                              \
        {                                                                 \
            rpc &= ~_rpc_value;                                           \
            rte_tmp |= _rte_flag;                                         \
        }                                                                 \
    } while (0)

#define RTE_PKTMBUF_OL_FLAGS2RTE(_bit)                           \
    RTE_PKTMBUF_MASK_OL_FLAGS2RTE((UINT64_C(1) << TARPC_##_bit), \
                                  (UINT64_C(1) << TARPC_##_bit), _bit)
#define RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RTE(_rte_flag)                 \
    RTE_PKTMBUF_MASK_OL_FLAGS2RTE(TARPC_RTE_MBUF_F_RX_IP_CKSUM_MASK, \
                                  TARPC_##_rte_flag, _rte_flag)
#define RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RTE(_rte_flag)                 \
    RTE_PKTMBUF_MASK_OL_FLAGS2RTE(TARPC_RTE_MBUF_F_RX_L4_CKSUM_MASK, \
                                  TARPC_##_rte_flag, _rte_flag)

    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_VLAN);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_VLAN_STRIPPED);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_RSS_HASH);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_FDIR);

    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_IP_CKSUM_UNKNOWN);
    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_IP_CKSUM_NONE);
    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_IP_CKSUM_BAD);
    RTE_PKTMBUF_IP_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_IP_CKSUM_GOOD);

    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_L4_CKSUM_UNKNOWN);
    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_L4_CKSUM_NONE);
    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_L4_CKSUM_BAD);
    RTE_PKTMBUF_L4_CKSUM_OL_FLAGS2RTE(RTE_MBUF_F_RX_L4_CKSUM_GOOD);

    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_OUTER_IP_CKSUM_BAD);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_IEEE1588_PTP);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_FDIR_ID);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_FDIR_FLX);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_QINQ);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_RX_QINQ_STRIPPED);

    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_QINQ);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_TCP_SEG);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_IEEE1588_TMST);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_L4_NO_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_TCP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_SCTP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_UDP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_L4_MASK);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_IPV6);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_VLAN);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_OUTER_IP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_OUTER_IPV4);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_OUTER_IPV6);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_OUTER_UDP_CKSUM);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_TUNNEL_VXLAN);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_TUNNEL_GENEVE);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_TX_TUNNEL_GRE);

    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_INDIRECT);
    RTE_PKTMBUF_OL_FLAGS2RTE(RTE_MBUF_F_EXTERNAL);

#undef RTE_PKTMBUF_OL_FLAGS2RTE

    if (rpc != 0)
        return (0);
    else
        *rte = rte_tmp;

    return (1);
}

static void
tarpc_rte_pktmbuf_packet_type2rpc(struct rte_mbuf                      *m,
                                  struct tarpc_rte_pktmbuf_packet_type *p_type)
{
#define RTE_PKTMBUF_L2_PTYPE2RPC(_bit) \
    case RTE_PTYPE_L2_##_bit:                                    \
        p_type->l2_type = TARPC_RTE_PTYPE_L2_##_bit;             \
        break

#define RTE_PKTMBUF_L3_PTYPE2RPC(_bit) \
    case RTE_PTYPE_L3_##_bit:                                    \
        p_type->l3_type = TARPC_RTE_PTYPE_L3_##_bit;             \
        break

#define RTE_PKTMBUF_L4_PTYPE2RPC(_bit) \
    case RTE_PTYPE_L4_##_bit:                                    \
        p_type->l4_type = TARPC_RTE_PTYPE_L4_##_bit;             \
        break

#define RTE_PKTMBUF_TUNNEL_PTYPE2RPC(_bit) \
    case RTE_PTYPE_TUNNEL_##_bit:                                \
        p_type->tun_type = TARPC_RTE_PTYPE_TUNNEL_##_bit;        \
        break

#define RTE_PKTMBUF_INNER_L2_PTYPE2RPC(_bit) \
    case RTE_PTYPE_INNER_L2_##_bit:                              \
        p_type->inner_l2_type = TARPC_RTE_PTYPE_INNER_L2_##_bit; \
        break

#define RTE_PKTMBUF_INNER_L3_PTYPE2RPC(_bit) \
    case RTE_PTYPE_INNER_L3_##_bit:                              \
        p_type->inner_l3_type = TARPC_RTE_PTYPE_INNER_L3_##_bit; \
        break

#define RTE_PKTMBUF_INNER_L4_PTYPE2RPC(_bit) \
    case RTE_PTYPE_INNER_L4_##_bit:                              \
        p_type->inner_l4_type = TARPC_RTE_PTYPE_INNER_L4_##_bit; \
        break

    switch (m->packet_type & RTE_PTYPE_L2_MASK) {
        case 0:
            p_type->l2_type = TARPC_RTE_PTYPE_L2_UNKNOWN;
            break;
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_TIMESYNC);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_ARP);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_LLDP);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_NSH);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_VLAN);
        RTE_PKTMBUF_L2_PTYPE2RPC(ETHER_QINQ);
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
#ifdef RTE_PTYPE_TUNNEL_GTPC
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(GTPC);
#endif /* RTE_PTYPE_TUNNEL_GTPC */
#ifdef RTE_PTYPE_TUNNEL_GTPU
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(GTPU);
#endif /* RTE_PTYPE_TUNNEL_GTPU */
#ifdef RTE_PTYPE_TUNNEL_ESP
        RTE_PKTMBUF_TUNNEL_PTYPE2RPC(ESP);
#endif /* RTE_PTYPE_TUNNEL_ESP */
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
        RTE_PKTMBUF_INNER_L2_PTYPE2RPC(ETHER_QINQ);
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
                                  uint32_t                             *rte_out)
{
    uint32_t rte = 0;

#define RTE_PKTMBUF_L2_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_L2_##_bit:    \
        rte |= RTE_PTYPE_L2_##_bit;    \
        break

#define RTE_PKTMBUF_L3_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_L3_##_bit:    \
        rte |= RTE_PTYPE_L3_##_bit;    \
        break

#define RTE_PKTMBUF_L4_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_L4_##_bit:    \
        rte |= RTE_PTYPE_L4_##_bit;    \
        break

#define RTE_PKTMBUF_TUNNEL_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_TUNNEL_##_bit:    \
        rte |= RTE_PTYPE_TUNNEL_##_bit;    \
        break

#define RTE_PKTMBUF_INNER_L2_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_INNER_L2_##_bit:    \
        rte |= RTE_PTYPE_INNER_L2_##_bit;    \
        break

#define RTE_PKTMBUF_INNER_L3_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_INNER_L3_##_bit:    \
        rte |= RTE_PTYPE_INNER_L3_##_bit;    \
        break

#define RTE_PKTMBUF_INNER_L4_PTYPE2RTE(_bit) \
    case TARPC_RTE_PTYPE_INNER_L4_##_bit:    \
        rte |= RTE_PTYPE_INNER_L4_##_bit;    \
        break

    switch (p_type->l2_type) {
        case TARPC_RTE_PTYPE_L2_UNKNOWN:
            break;
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_TIMESYNC);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_ARP);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_LLDP);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_NSH);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_VLAN);
        RTE_PKTMBUF_L2_PTYPE2RTE(ETHER_QINQ);
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
#ifdef RTE_PTYPE_TUNNEL_GTPC
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(GTPC);
#endif /* RTE_PTYPE_TUNNEL_GTPC */
#ifdef RTE_PTYPE_TUNNEL_GTPU
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(GTPU);
#endif /* RTE_PTYPE_TUNNEL_GTPU */
#ifdef RTE_PTYPE_TUNNEL_ESP
        RTE_PKTMBUF_TUNNEL_PTYPE2RTE(ESP);
#endif /* RTE_PTYPE_TUNNEL_ESP */
        default:
            return (0);
    }

    switch (p_type->inner_l2_type) {
        case TARPC_RTE_PTYPE_INNER_L2_UNKNOWN:
            break;
        RTE_PKTMBUF_INNER_L2_PTYPE2RTE(ETHER);
        RTE_PKTMBUF_INNER_L2_PTYPE2RTE(ETHER_VLAN);
        RTE_PKTMBUF_INNER_L2_PTYPE2RTE(ETHER_QINQ);
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

    *rte_out = rte;

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

#ifdef HAVE_RTE_PKTMBUF_POOL_CREATE_BY_OPS
TARPC_FUNC(rte_pktmbuf_pool_create_by_ops, {},
{
    struct rte_mempool *mp;

    MAKE_CALL(mp = func(in->name, in->n, in->cache_size, in->priv_size,
                        in->data_room_size, in->socket_id, in->ops_name));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(mp, ns);
    });
}
)
#endif

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

TARPC_FUNC_STANDALONE(rte_pktmbuf_get_fdir_id, {},
{
    struct rte_mbuf *m;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
        if (m->ol_flags & RTE_MBUF_F_RX_FDIR_ID)
            out->retval = m->hash.fdir.hi;
        else
            out->retval = UINT32_MAX;
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

TARPC_FUNC_STATIC(rte_pktmbuf_refcnt_update, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(func(m, in->v));
}
)

static struct rte_mbuf *
redist_alloc_seg(struct rte_mempool *mp_def,
                 tarpc_rte_mempool  *mp_multi,
                 unsigned int        mp_multi_nb_items,
                 unsigned int       *mp_multi_next_idxp)
{
    unsigned int mp_multi_next_idx;
    unsigned int i;

    if ((mp_multi_nb_items == 0) || (mp_multi == NULL))
        goto out;

    mp_multi_next_idx = (mp_multi_next_idxp == NULL) ? 0 : *mp_multi_next_idxp;

    for (i = 0; i < mp_multi_nb_items; ++i)
    {
        struct rte_mempool *mp = NULL;
        unsigned int        idx = (mp_multi_next_idx++ % mp_multi_nb_items);

        RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
            mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(mp_multi[idx], ns);
        });

        if (mp != NULL)
        {
            struct rte_mbuf *m = rte_pktmbuf_alloc(mp);

            if (m != NULL)
            {
                if (mp_multi_next_idxp != NULL)
                    *mp_multi_next_idxp = mp_multi_next_idx;

                return m;
            }
        }
    }

out:
    return (mp_def == NULL) ? NULL : rte_pktmbuf_alloc(mp_def);
}

static int
rte_pktmbuf_redist(tarpc_rte_pktmbuf_redist_in  *in,
                   tarpc_rte_pktmbuf_redist_out *out)
{
    struct rte_mbuf                *mo = NULL;
    uint64_t                        nb_groups_avail;
    struct rte_mbuf                *mo_seg;
    uint16_t                        mo_seg_off;
    struct rte_mbuf                *mn = NULL;
    struct rte_mbuf                *mn_seg;
    unsigned int                    mp_multi_next_idx;
    uint32_t                        data_len_copied;
    struct tarpc_pktmbuf_seg_group *group;
    uint8_t                        *dst;
    uint16_t                        mn_seg_extent_len;
    te_errno                        err = 0;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        mo = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    if (mo == NULL)
    {
        err = TE_RC(TE_RPCS, TE_EINVAL);
        goto out;
    }

    out->m = in->m;

    nb_groups_avail = in->seg_groups.seg_groups_len;
    if (nb_groups_avail == 0)
        return (mo->nb_segs);

    mo_seg = mo;
    mo_seg_off = 0;
    mn_seg = NULL;

    mp_multi_next_idx = 0;
    data_len_copied = 0;

    for (group = in->seg_groups.seg_groups_val; nb_groups_avail-- > 0; ++group)
    {
        uint16_t group_nb_segs_avail = group->num;

        while (group_nb_segs_avail > 0)
        {
            uint16_t  mo_seg_dlen_avail = 0;
            uint16_t  mn_seg_dlen_avail;
            uint16_t  data_len_to_copy;

            for (; mo_seg != NULL; mo_seg = mo_seg->next, mo_seg_off = 0)
            {
                mo_seg_dlen_avail = mo_seg->data_len - mo_seg_off;
                if (mo_seg_dlen_avail != 0)
                     break;
            }

            if (mo_seg_dlen_avail == 0)
            {
                if ((group_nb_segs_avail > 1) || (nb_groups_avail > 1))
                    WARN("%s(): All the original data has been read out"
                         " although the pattern has not yet ended",
                         __FUNCTION__);
                goto pattern_done;
            }

            mn_seg_dlen_avail = (mn_seg != NULL) ?
                                group->len - mn_seg->data_len : 0;

            if (mn_seg_dlen_avail == 0)
            {
                mn_seg = redist_alloc_seg(mo->pool, in->mp_multi.mp_multi_val,
                                          in->mp_multi.mp_multi_len,
                                          &mp_multi_next_idx);
                if (mn_seg == NULL)
                {
                    WARN("%s(): All spare mempool objects have been spent",
                         __FUNCTION__);
                    goto pattern_done;
                }

                if (mn == NULL)
                {
                    mn = mn_seg;
                }
                else if (rte_pktmbuf_chain(mn, mn_seg) != 0)
                {
                    WARN("%s(): Reached the maximum allowed number of segments",
                         __FUNCTION__);
                    rte_pktmbuf_free(mn_seg);
                    mn_seg = NULL;
                    goto pattern_done;
                }

                mn_seg_dlen_avail = group->len;
            }

            data_len_to_copy = MIN(mo_seg_dlen_avail, mn_seg_dlen_avail);
            dst = (uint8_t *)rte_pktmbuf_append(mn, data_len_to_copy);
            if (dst == NULL)
            {
                ERROR("%s(): Failed to append data room of %" PRIu16 " bytes;"
                      " nb_segs = %" PRIu16 "; current data_len = %" PRIu16,
                      __FUNCTION__, data_len_to_copy, mn->nb_segs,
                      mn_seg->data_len);
                err = TE_RC(TE_RPCS, TE_ENOMEM);
                goto out;
            }

            rte_memcpy(dst,
                       rte_pktmbuf_mtod_offset(mo_seg, uint8_t *, mo_seg_off),
                       data_len_to_copy);

            mo_seg_off += data_len_to_copy;
            data_len_copied += data_len_to_copy;

            if (data_len_to_copy == mn_seg_dlen_avail)
            {
                --group_nb_segs_avail;
                mn_seg = NULL;
            }
        }
    }

pattern_done:
    if (mn == NULL)
    {
        WARN("%s(): The new mbuf chain has not emerged", __FUNCTION__);
        WARN("%s(): The original chain will be preserved", __FUNCTION__);
        return (mo->nb_segs);
    }

    mn_seg_extent_len = mo->pkt_len - data_len_copied;
    if (mn_seg_extent_len > 0)
    {
        WARN("%s(): %" PRIu16 " bytes of the original data has not been"
             " distributed after the pattern", __FUNCTION__, mn_seg_extent_len);
        WARN("%s(): The data will be added to the last segment", __FUNCTION__);

        dst = (uint8_t *)rte_pktmbuf_append(mn, mn_seg_extent_len);
        if (dst == NULL)
        {
            ERROR("%s(): Failed to append data room of %" PRIu16 " bytes",
                  __FUNCTION__, mn_seg_extent_len);
            err = TE_RC(TE_RPCS, TE_ENOMEM);
            goto out;
        }

        for (; mo_seg != NULL; mo_seg = mo_seg->next, mo_seg_off = 0)
        {
            rte_memcpy(dst,
                       rte_pktmbuf_mtod_offset(mo_seg, uint8_t *, mo_seg_off),
                       (mo_seg->data_len - mo_seg_off));
        }
    }

#define RTE_PKTMBUF_COPY_FIELD(_field) mn->_field = mo->_field
    RTE_PKTMBUF_COPY_FIELD(port);
    RTE_PKTMBUF_COPY_FIELD(ol_flags);
    RTE_PKTMBUF_COPY_FIELD(vlan_tci);
    RTE_PKTMBUF_COPY_FIELD(hash);
#ifdef HAVE_STRUCT_RTE_MBUF_SEQN
    RTE_PKTMBUF_COPY_FIELD(seqn);
#endif
    RTE_PKTMBUF_COPY_FIELD(vlan_tci_outer);
    RTE_PKTMBUF_COPY_FIELD(timesync);
    RTE_PKTMBUF_COPY_FIELD(packet_type);
    RTE_PKTMBUF_COPY_FIELD(tx_offload);
#undef RTE_PKTMBUF_COPY_FIELD

out:
    if (err == 0)
    {
        rte_pktmbuf_free(mo);
        RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
            RCF_PCH_MEM_INDEX_FREE(in->m, ns);
            out->m = RCF_PCH_MEM_INDEX_ALLOC(mn, ns);
        });
    }
    else
    {
        ERROR("%s(): Redistribution failed: %r", __FUNCTION__, err);
        rte_pktmbuf_free(mn);
    }

    return (err != 0) ? ((err > 0) ? -err : err) : mn->nb_segs;
}

TARPC_FUNC_STATIC(rte_pktmbuf_redist, {},
{
    MAKE_CALL(out->retval = func(in, out));
}
)

TARPC_FUNC_STATIC(rte_vlan_strip, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(out->retval = func(m));
}
)
