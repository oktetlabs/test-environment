/** @file
 * @brief RPC client API for DPDK mbuf library
 *
 * RPC client API for DPDK mbuf library functions
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte.h"
#include "tapi_rpc_rte_mbuf.h"
#include "tapi_mem.h"
#include "log_bufs.h"
#include "rpcc_dpdk.h"
#include "tapi_test_log.h"

#include "tarpc.h"


static const char *
tarpc_rte_pktmbuf_ol_flags2str(te_log_buf *tlbp, uint64_t ol_flags)
{
    const struct te_log_buf_bit2str ol_flags2str[] = {
#define TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(_bit) \
        { TARPC_PKT_##_bit, #_bit }
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_VLAN_PKT),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_RSS_HASH),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_FDIR),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_L4_CKSUM_BAD),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_IP_CKSUM_BAD),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_EIP_CKSUM_BAD),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_OVERSIZE),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_HBUF_OVERFLOW),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_RECIP_ERR),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_MAC_ERR),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_IEEE1588_PTP),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_IEEE1588_TMST),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_FDIR_ID),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_FDIR_FLX),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(RX_QINQ_PKT),

        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_QINQ_PKT),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_TCP_SEG),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_IEEE1588_TMST),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_L4_NO_CKSUM),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_TCP_CKSUM),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_SCTP_CKSUM),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_UDP_CKSUM),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_L4_MASK),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_IP_CKSUM),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_IPV4),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_IPV6),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_VLAN_PKT),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_OUTER_IP_CKSUM),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_OUTER_IPV4),
        TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR(TX_OUTER_IPV6),
#undef TARPC_RTE_PKTMBUF_PKT_OL_FLAGS2STR
        { TARPC_IND_ATTACHED_MBUF, "IND_ATTACHED_MBUF" },
        { TARPC_CTRL_MBUF_FLAG, "CTRL_MBUF_FLAG" },
        { 0, NULL }
    };

    return te_bit_mask2log_buf(tlbp, ol_flags, ol_flags2str);
}

static const char *
tarpc_rte_pktmbuf_packet_type2str(te_log_buf *tlbp,
                                  struct tarpc_rte_pktmbuf_packet_type *p_type)
{
    te_bool added = FALSE;

#define CASE_TARPC_RTE_PKTMBUF_L2_PTYPE2STR(_bit) \
    case TARPC_RTE_PTYPE_L2_##_bit:                                           \
        te_log_buf_append(tlbp, "L2_%s", #_bit);                              \
        added = TRUE;                                                         \
        break

#define CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR(_bit) \
    case TARPC_RTE_PTYPE_L3_##_bit:                                           \
        te_log_buf_append(tlbp, "%sL3_%s", added ? " | " : "", #_bit);        \
        added = TRUE;                                                         \
        break

#define CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR(_bit) \
    case TARPC_RTE_PTYPE_L4_##_bit:                                           \
        te_log_buf_append(tlbp, "%sL4_%s", added ? " | " : "", #_bit);        \
        added = TRUE;                                                         \
        break

#define CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR(_bit) \
    case TARPC_RTE_PTYPE_TUNNEL_##_bit:                                       \
        te_log_buf_append(tlbp, "%sTUNNEL_%s", added ? " | " : "", #_bit);    \
        added = TRUE;                                                         \
        break

#define CASE_TARPC_RTE_PKTMBUF_INNER_L2_PTYPE2STR(_bit) \
    case TARPC_RTE_PTYPE_INNER_L2_##_bit:                                     \
        te_log_buf_append(tlbp, "%sINNER_L2_%s", added ? " | " : "", #_bit);  \
        added = TRUE;                                                         \
        break

#define CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR(_bit) \
    case TARPC_RTE_PTYPE_INNER_L3_##_bit:                                     \
        te_log_buf_append(tlbp, "%sINNER_L3_%s", added ? " | " : "", #_bit);  \
        added = TRUE;                                                         \
        break

#define CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR(_bit) \
    case TARPC_RTE_PTYPE_INNER_L4_##_bit:                                     \
        te_log_buf_append(tlbp, "%sINNER_L4_%s", added ? " | " : "", #_bit);  \
        break

    switch (p_type->l2_type) {
        case TARPC_RTE_PTYPE_L2_UNKNOWN:
            break;
        CASE_TARPC_RTE_PKTMBUF_L2_PTYPE2STR(ETHER);
        CASE_TARPC_RTE_PKTMBUF_L2_PTYPE2STR(ETHER_TIMESYNC);
        CASE_TARPC_RTE_PKTMBUF_L2_PTYPE2STR(ETHER_ARP);
        CASE_TARPC_RTE_PKTMBUF_L2_PTYPE2STR(ETHER_LLDP);
        default:
            te_log_buf_append(tlbp, "L2_BAD_TYPE");
            added = TRUE;
            break;
    }

    switch (p_type->l3_type) {
        case TARPC_RTE_PTYPE_L3_UNKNOWN:
            break;
        CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR(IPV4);
        CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR(IPV4_EXT);
        CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR(IPV6);
        CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR(IPV4_EXT_UNKNOWN);
        CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR(IPV6_EXT);
        CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR(IPV6_EXT_UNKNOWN);
        default:
            te_log_buf_append(tlbp, "L3_BAD_TYPE");
            added = TRUE;
            break;
    }

    switch (p_type->l4_type) {
        case TARPC_RTE_PTYPE_L4_UNKNOWN:
            break;
        CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR(TCP);
        CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR(UDP);
        CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR(FRAG);
        CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR(SCTP);
        CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR(ICMP);
        CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR(NONFRAG);
        default:
            te_log_buf_append(tlbp, "L4_BAD_TYPE");
            added = TRUE;
            break;
    }

    switch (p_type->tun_type) {
        case TARPC_RTE_PTYPE_TUNNEL_UNKNOWN:
            break;
        CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR(IP);
        CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR(GRE);
        CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR(VXLAN);
        CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR(NVGRE);
        CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR(GENEVE);
        CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR(GRENAT);
        default:
            te_log_buf_append(tlbp, "TUNNEL_BAD_TYPE");
            added = TRUE;
            break;
    }

    switch (p_type->inner_l2_type) {
        case TARPC_RTE_PTYPE_INNER_L2_UNKNOWN:
            break;
        CASE_TARPC_RTE_PKTMBUF_INNER_L2_PTYPE2STR(ETHER);
        CASE_TARPC_RTE_PKTMBUF_INNER_L2_PTYPE2STR(ETHER_VLAN);
        default:
            te_log_buf_append(tlbp, "INNER_L2_BAD_TYPE");
            added = TRUE;
            break;
    }

    switch (p_type->inner_l3_type) {
        case TARPC_RTE_PTYPE_INNER_L3_UNKNOWN:
            break;
        CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR(IPV4);
        CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR(IPV4_EXT);
        CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR(IPV6);
        CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR(IPV4_EXT_UNKNOWN);
        CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR(IPV6_EXT);
        CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR(IPV6_EXT_UNKNOWN);
        default:
            te_log_buf_append(tlbp, "INNER_L3_BAD_TYPE");
            added = TRUE;
            break;
    }

    switch (p_type->inner_l4_type) {
        case TARPC_RTE_PTYPE_INNER_L4_UNKNOWN:
            break;
        CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR(TCP);
        CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR(UDP);
        CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR(FRAG);
        CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR(SCTP);
        CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR(ICMP);
        CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR(NONFRAG);
        default:
            te_log_buf_append(tlbp, "INNER_L4_BAD_TYPE");
            added = TRUE;
            break;
    }

#undef CASE_TARPC_RTE_PKTMBUF_L2_PTYPE2STR
#undef CASE_TARPC_RTE_PKTMBUF_L3_PTYPE2STR
#undef CASE_TARPC_RTE_PKTMBUF_L4_PTYPE2STR
#undef CASE_TARPC_RTE_PKTMBUF_TUNNEL_PTYPE2STR
#undef CASE_TARPC_RTE_PKTMBUF_INNER_L2_PTYPE2STR
#undef CASE_TARPC_RTE_PKTMBUF_INNER_L3_PTYPE2STR
#undef CASE_TARPC_RTE_PKTMBUF_INNER_L4_PTYPE2STR

    if (!added)
        te_log_buf_append(tlbp, "UNKNOWN");

    return te_log_buf_get(tlbp);
}

static const char *
tarpc_rte_pktmbuf_tx_offload2str(te_log_buf *tlbp,
                                 struct tarpc_rte_pktmbuf_tx_offload *tx_offload)
{
    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "l2_len = %hu, l3_len = %hu, l4_len = %hu, "
                      "tso_segsz = %hu, outer_l3_len = %hu, outer_l2_len = %hu",
                      tx_offload->l2_len, tx_offload->l3_len,
                      tx_offload->l4_len, tx_offload->tso_segsz,
                      tx_offload->outer_l3_len, tx_offload->outer_l2_len);

    te_log_buf_append(tlbp, " }");

    return te_log_buf_get(tlbp);
}

rpc_rte_mempool_p
rpc_rte_pktmbuf_pool_create(rcf_rpc_server *rpcs,
                            const char *name, uint32_t n, uint32_t cache_size,
                            uint16_t priv_size, uint16_t data_room_size,
                            int socket_id)
{
    tarpc_rte_pktmbuf_pool_create_in    in;
    tarpc_rte_pktmbuf_pool_create_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.name = tapi_strdup(name);
    in.n = n;
    in.cache_size = cache_size;
    in.priv_size = priv_size;
    in.data_room_size = data_room_size;
    in.socket_id = socket_id;

    rcf_rpc_call(rpcs, "rte_pktmbuf_pool_create", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_pktmbuf_pool_create, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_pool_create,
                 "%s, %u, %u, %hu, %hu, %d", RPC_PTR_FMT,
                 in.name, in.n, in.cache_size, in.priv_size, in.data_room_size,
                 in.socket_id, RPC_PTR_VAL(out.retval));

    free(in.name);

    RETVAL_RPC_PTR(rte_pktmbuf_pool_create, out.retval);
}

rpc_rte_mbuf_p
rpc_rte_pktmbuf_alloc(rcf_rpc_server *rpcs,
                      rpc_rte_mempool_p mp)
{
    tarpc_rte_pktmbuf_alloc_in  in;
    tarpc_rte_pktmbuf_alloc_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.mp = (tarpc_rte_mempool)mp;

    rcf_rpc_call(rpcs, "rte_pktmbuf_alloc", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_pktmbuf_alloc, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_alloc, RPC_PTR_FMT, RPC_PTR_FMT,
                 RPC_PTR_VAL(in.mp), RPC_PTR_VAL(out.retval));

    RETVAL_RPC_PTR(rte_pktmbuf_alloc, out.retval);
}

void
rpc_rte_pktmbuf_free(rcf_rpc_server *rpcs,
                     rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_free_in   in;
    tarpc_rte_pktmbuf_free_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_free", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_free, RPC_PTR_FMT, "", RPC_PTR_VAL(in.m));

    RETVAL_VOID(rte_pktmbuf_free);
}

int
rpc_rte_pktmbuf_append_data(rcf_rpc_server *rpcs,
                            rpc_rte_mbuf_p m, uint8_t *buf, size_t len)
{
    tarpc_rte_pktmbuf_append_data_in    in;
    tarpc_rte_pktmbuf_append_data_out   out;
    uint8_t *buf_copy;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if ((len != 0) && (buf == NULL))
    {
        ERROR("%s(): No buffer, but length is greater than 0", __FUNCTION__);
        RETVAL_ZERO_INT(rte_pktmbuf_append_data, -1);
    }

    buf_copy = tapi_memdup(buf, len);

    in.m = (tarpc_rte_mbuf)m;
    in.buf.buf_val = buf_copy;
    in.buf.buf_len = len;

    rcf_rpc_call(rpcs, "rte_pktmbuf_append_data", &in, &out);

    free(in.buf.buf_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_pktmbuf_append_data, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_append_data, RPC_PTR_FMT ", %u",
                 NEG_ERRNO_FMT, RPC_PTR_VAL(in.m), in.buf.buf_len,
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_pktmbuf_append_data, out.retval);
}

int
rpc_rte_pktmbuf_read_data(rcf_rpc_server *rpcs,
                          rpc_rte_mbuf_p m, size_t offset, size_t count,
                          uint8_t *buf, size_t rbuflen)
{
    tarpc_rte_pktmbuf_read_data_in  in;
    tarpc_rte_pktmbuf_read_data_out out;
    uint8_t *buf_copy;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if ((buf == NULL) || (count > rbuflen))
    {
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        RETVAL_INT(rte_pktmbuf_read_data, -rpcs->_errno);
    }

    buf_copy = tapi_memdup(buf, rbuflen);

    in.m = (tarpc_rte_mbuf)m;
    in.offset = offset;
    in.len = count;
    in.buf.buf_len = rbuflen;
    in.buf.buf_val = buf_copy;

    rcf_rpc_call(rpcs, "rte_pktmbuf_read_data", &in, &out);

    free(in.buf.buf_val);

    CHECK_RETVAL_VAR(rte_pktmbuf_read_data, out.retval,
                     out.retval < 0 || (size_t)out.retval > count,
                     -out.common._errno);

    if (RPC_IS_CALL_OK(rpcs) && out.buf.buf_val != NULL)
        memcpy(buf, out.buf.buf_val, out.buf.buf_len);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_read_data,
                 RPC_PTR_FMT ", %u, %u, %u", NEG_ERRNO_FMT, RPC_PTR_VAL(in.m),
                 in.offset, in.len, in.buf.buf_len, NEG_ERRNO_ARGS(out.retval));

    RETVAL_INT(rte_pktmbuf_read_data, out.retval);
}

rpc_rte_mbuf_p
rpc_rte_pktmbuf_clone(rcf_rpc_server *rpcs,
                      rpc_rte_mbuf_p m, rpc_rte_mempool_p mp)
{
    tarpc_rte_pktmbuf_clone_in  in;
    tarpc_rte_pktmbuf_clone_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;
    in.mp = (tarpc_rte_mempool)mp;

    rcf_rpc_call(rpcs, "rte_pktmbuf_clone", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_pktmbuf_clone, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_clone, RPC_PTR_FMT ", " RPC_PTR_FMT,
                 RPC_PTR_FMT, RPC_PTR_VAL(in.m), RPC_PTR_VAL(in.mp),
                 RPC_PTR_VAL(out.retval));

    RETVAL_RPC_PTR(rte_pktmbuf_clone, out.retval);
}

int
rpc_rte_pktmbuf_prepend_data(rcf_rpc_server *rpcs,
                             rpc_rte_mbuf_p m, uint8_t *buf, size_t len)
{
    tarpc_rte_pktmbuf_prepend_data_in   in;
    tarpc_rte_pktmbuf_prepend_data_out  out;
    uint8_t *buf_copy;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if ((len != 0) && (buf == NULL))
        RETVAL_ZERO_INT(rte_pktmbuf_prepend_data,
                        -TE_RC(TE_TAPI, TE_EINVAL));

    buf_copy = tapi_memdup(buf, len);

    in.m = (tarpc_rte_mbuf)m;
    in.buf.buf_val = buf_copy;
    in.buf.buf_len = len;

    rcf_rpc_call(rpcs, "rte_pktmbuf_prepend_data", &in, &out);

    free(in.buf.buf_val);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_pktmbuf_prepend_data, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_prepend_data, RPC_PTR_FMT ", %u",
                 NEG_ERRNO_FMT, RPC_PTR_VAL(in.m), in.buf.buf_len,
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_pktmbuf_prepend_data, out.retval);
}

rpc_rte_mbuf_p
rpc_rte_pktmbuf_get_next(rcf_rpc_server *rpcs,
                         rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_next_in   in;
    tarpc_rte_pktmbuf_get_next_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_next", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_get_next, out.retval,
                     out.retval == RPC_UNKNOWN_ADDR, RPC_UNKNOWN_ADDR);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_next, RPC_PTR_FMT, RPC_PTR_FMT,
                 RPC_PTR_VAL(in.m), RPC_PTR_VAL(out.retval));

    RETVAL_RPC_PTR_OR_NULL(rte_pktmbuf_get_next, out.retval);
}

uint32_t
rpc_rte_pktmbuf_get_pkt_len(rcf_rpc_server *rpcs,
                            rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_pkt_len_in    in;
    tarpc_rte_pktmbuf_get_pkt_len_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_pkt_len", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_get_pkt_len, out.retval, FALSE, UINT32_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_pkt_len, RPC_PTR_FMT, "%u",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_get_pkt_len, out.retval == UINT32_MAX);

    return (out.retval);
}

const char *
rpc_rte_mbufs2str(te_log_buf *tlbp,
                  const rpc_rte_mbuf_p *mbufs, unsigned int count,
                  rcf_rpc_server *rpcs)
{
    unsigned int i;

    te_log_buf_append(tlbp, "{ ");

    if (count == 0)
        te_log_buf_append(tlbp, "(empty)");
    else
        te_log_buf_append(tlbp, RPC_PTR_FMT, RPC_PTR_VAL(mbufs[0]));

    for (i = 1; i < count; ++i)
    {
        te_log_buf_append(tlbp, ", " RPC_PTR_FMT, RPC_PTR_VAL(mbufs[i]));
    }

    te_log_buf_append(tlbp, " }");

    return te_log_buf_get(tlbp);
}

int
rpc_rte_pktmbuf_alloc_bulk(rcf_rpc_server *rpcs,
                           rpc_rte_mempool_p mp, rpc_rte_mbuf_p *bulk,
                           unsigned int count)
{
    tarpc_rte_pktmbuf_alloc_bulk_in     in;
    tarpc_rte_pktmbuf_alloc_bulk_out    out;
    te_log_buf *tlbp;
    unsigned int i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.mp = (tarpc_rte_mempool)mp;
    in.count = count;

    rcf_rpc_call(rpcs, "rte_pktmbuf_alloc_bulk", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_pktmbuf_alloc_bulk, out.retval);

    if (out.retval == 0)
        for (i = 0; i < count; ++i)
            bulk[i] = out.bulk.bulk_val[i];

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_pktmbuf_alloc_bulk, RPC_PTR_FMT ", %u",
                 NEG_ERRNO_FMT ", %s", RPC_PTR_VAL(in.mp), in.count,
                 NEG_ERRNO_ARGS(out.retval),
                 rpc_rte_mbufs2str(tlbp, bulk, count, rpcs));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_pktmbuf_alloc_bulk, out.retval);
}

int
rpc_rte_pktmbuf_chain(rcf_rpc_server *rpcs,
                      rpc_rte_mbuf_p head, rpc_rte_mbuf_p tail)
{
    tarpc_rte_pktmbuf_chain_in  in;
    tarpc_rte_pktmbuf_chain_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.head = (tarpc_rte_mbuf)head;
    in.tail = (tarpc_rte_mbuf)tail;

    rcf_rpc_call(rpcs, "rte_pktmbuf_chain", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_pktmbuf_chain, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_chain, RPC_PTR_FMT ", " RPC_PTR_FMT,
                 NEG_ERRNO_FMT, RPC_PTR_VAL(in.head), RPC_PTR_VAL(in.tail),
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_pktmbuf_chain, out.retval);
}

uint8_t
rpc_rte_pktmbuf_get_nb_segs(rcf_rpc_server *rpcs,
                            rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_nb_segs_in    in;
    tarpc_rte_pktmbuf_get_nb_segs_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_nb_segs", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_get_nb_segs, out.retval, out.retval == 0, 0);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_nb_segs, RPC_PTR_FMT, "%hhu",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_get_nb_segs, out.retval == 0);

    return (out.retval);
}

uint8_t
rpc_rte_pktmbuf_get_port(rcf_rpc_server *rpcs,
                         rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_port_in   in;
    tarpc_rte_pktmbuf_get_port_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_port", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_port, RPC_PTR_FMT, "%hhu",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_get_port, FALSE);

    return (out.retval);
}

void
rpc_rte_pktmbuf_set_port(rcf_rpc_server *rpcs,
                         rpc_rte_mbuf_p m, uint8_t port)
{
    tarpc_rte_pktmbuf_set_port_in   in;
    tarpc_rte_pktmbuf_set_port_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;
    in.port = port;

    rcf_rpc_call(rpcs, "rte_pktmbuf_set_port", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_set_port, RPC_PTR_FMT ", %hhu", "",
                 RPC_PTR_VAL(in.m), in.port);

    RETVAL_VOID(rte_pktmbuf_set_port);
}

uint16_t
rpc_rte_pktmbuf_get_data_len(rcf_rpc_server *rpcs,
                             rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_data_len_in   in;
    tarpc_rte_pktmbuf_get_data_len_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_data_len", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_get_data_len, out.retval, FALSE, UINT16_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_data_len, RPC_PTR_FMT, "%hu",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_get_data_len, out.retval == UINT16_MAX);

    return (out.retval);
}

uint16_t
rpc_rte_pktmbuf_get_vlan_tci(rcf_rpc_server *rpcs,
                             rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_vlan_tci_in   in;
    tarpc_rte_pktmbuf_get_vlan_tci_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_vlan_tci", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_get_vlan_tci, out.retval, FALSE, UINT16_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_vlan_tci, RPC_PTR_FMT, "%hu",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_get_vlan_tci, out.retval == UINT16_MAX);

    return (out.retval);
}

void
rpc_rte_pktmbuf_set_vlan_tci(rcf_rpc_server *rpcs,
                             rpc_rte_mbuf_p m, uint16_t vlan_tci)
{
    tarpc_rte_pktmbuf_set_vlan_tci_in   in;
    tarpc_rte_pktmbuf_set_vlan_tci_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;
    in.vlan_tci = vlan_tci;

    rcf_rpc_call(rpcs, "rte_pktmbuf_set_vlan_tci", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_set_vlan_tci, RPC_PTR_FMT ", %hu", "",
                 RPC_PTR_VAL(in.m), in.vlan_tci);

    RETVAL_VOID(rte_pktmbuf_set_vlan_tci);
}

uint16_t
rpc_rte_pktmbuf_get_vlan_tci_outer(rcf_rpc_server *rpcs,
                                   rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_vlan_tci_outer_in     in;
    tarpc_rte_pktmbuf_get_vlan_tci_outer_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_vlan_tci_outer", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_get_vlan_tci_outer, out.retval, FALSE,
                     UINT16_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_vlan_tci_outer, RPC_PTR_FMT, "%hu",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_get_vlan_tci_outer, out.retval == UINT16_MAX);

    return (out.retval);
}

void
rpc_rte_pktmbuf_set_vlan_tci_outer(rcf_rpc_server *rpcs,
                                   rpc_rte_mbuf_p m, uint16_t vlan_tci_outer)
{
    tarpc_rte_pktmbuf_set_vlan_tci_outer_in     in;
    tarpc_rte_pktmbuf_set_vlan_tci_outer_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;
    in.vlan_tci_outer = vlan_tci_outer;

    rcf_rpc_call(rpcs, "rte_pktmbuf_set_vlan_tci_outer", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_set_vlan_tci_outer, RPC_PTR_FMT ", %hu", "",
                 RPC_PTR_VAL(in.m), in.vlan_tci_outer);

    RETVAL_VOID(rte_pktmbuf_set_vlan_tci_outer);
}

uint64_t
rpc_rte_pktmbuf_get_flags(rcf_rpc_server *rpcs,
                          rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_flags_in  in;
    tarpc_rte_pktmbuf_get_flags_out out;
    te_log_buf *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_flags", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_get_flags, out.retval, FALSE, UINT64_MAX);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_flags, RPC_PTR_FMT,
                 "%s", RPC_PTR_VAL(in.m),
                 tarpc_rte_pktmbuf_ol_flags2str(tlbp, out.retval));
    te_log_buf_free(tlbp);

    TAPI_RPC_OUT(rte_pktmbuf_get_flags, out.retval == UINT64_MAX);

    return (out.retval);
}

int
rpc_rte_pktmbuf_set_flags(rcf_rpc_server *rpcs,
                          rpc_rte_mbuf_p m, uint64_t ol_flags)
{
    tarpc_rte_pktmbuf_set_flags_in  in;
    tarpc_rte_pktmbuf_set_flags_out out;
    te_log_buf *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;
    in.ol_flags = ol_flags;

    rcf_rpc_call(rpcs, "rte_pktmbuf_set_flags", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_pktmbuf_set_flags, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_pktmbuf_set_flags,
                 RPC_PTR_FMT ", %s", NEG_ERRNO_FMT,
                 RPC_PTR_VAL(in.m),
                 tarpc_rte_pktmbuf_ol_flags2str(tlbp, in.ol_flags),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_pktmbuf_set_flags, out.retval);
}

rpc_rte_mempool_p
rpc_rte_pktmbuf_get_pool(rcf_rpc_server *rpcs,
                         rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_pool_in   in;
    tarpc_rte_pktmbuf_get_pool_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_pool", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_pktmbuf_get_pool, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_pool,
                 RPC_PTR_FMT, RPC_PTR_FMT,
                 RPC_PTR_VAL(in.m), RPC_PTR_VAL(out.retval));

    RETVAL_RPC_PTR(rte_pktmbuf_get_pool, out.retval);
}

uint16_t
rpc_rte_pktmbuf_headroom(rcf_rpc_server *rpcs,
                         rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_headroom_in   in;
    tarpc_rte_pktmbuf_headroom_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_headroom", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_headroom, out.retval, FALSE, UINT16_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_headroom, RPC_PTR_FMT, "%hu",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_headroom, out.retval == UINT16_MAX);

    return (out.retval);
}

uint16_t
rpc_rte_pktmbuf_tailroom(rcf_rpc_server *rpcs,
                         rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_tailroom_in   in;
    tarpc_rte_pktmbuf_tailroom_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_tailroom", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_tailroom, out.retval, FALSE, UINT16_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_tailroom, RPC_PTR_FMT, "%hu",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_tailroom, out.retval == UINT16_MAX);

    return (out.retval);
}

int
rpc_rte_pktmbuf_trim(rcf_rpc_server *rpcs,
                     rpc_rte_mbuf_p m,
                     uint16_t len)
{
    tarpc_rte_pktmbuf_trim_in   in;
    tarpc_rte_pktmbuf_trim_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;
    in.len = len;

    rcf_rpc_call(rpcs, "rte_pktmbuf_trim", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(rte_pktmbuf_trim, out.retval);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_trim, RPC_PTR_FMT ", %hu", "%d",
                 RPC_PTR_VAL(in.m), in.len, out.retval);

    RETVAL_ZERO_INT(rte_pktmbuf_trim, out.retval);
}

uint16_t
rpc_rte_pktmbuf_adj(rcf_rpc_server *rpcs,
                    rpc_rte_mbuf_p m, uint16_t len)
{
    tarpc_rte_pktmbuf_adj_in   in;
    tarpc_rte_pktmbuf_adj_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;
    in.len = len;

    rcf_rpc_call(rpcs, "rte_pktmbuf_adj", &in, &out);

    CHECK_RETVAL_VAR(rte_pktmbuf_adj, out.retval, FALSE, UINT16_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_adj, RPC_PTR_FMT ", %hu", "%hu",
                 RPC_PTR_VAL(in.m), in.len, out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_adj, out.retval == UINT16_MAX);

    return (out.retval);
}

void
rpc_rte_pktmbuf_get_packet_type(rcf_rpc_server *rpcs,
                                rpc_rte_mbuf_p m,
                                struct tarpc_rte_pktmbuf_packet_type *p_type)
{
    tarpc_rte_pktmbuf_get_packet_type_in    in;
    tarpc_rte_pktmbuf_get_packet_type_out   out;
    te_log_buf *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (p_type == NULL)
        TEST_FAIL("Invalid %s() p_type argument", __func__);

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_packet_type", &in, &out);

    *p_type = out.p_type;

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_packet_type, RPC_PTR_FMT,
                 "%s", RPC_PTR_VAL(in.m),
                 tarpc_rte_pktmbuf_packet_type2str(tlbp, p_type));
    te_log_buf_free(tlbp);

    RETVAL_VOID(rte_pktmbuf_get_packet_type);
}

int
rpc_rte_pktmbuf_set_packet_type(rcf_rpc_server *rpcs,
                                rpc_rte_mbuf_p m,
                             const struct tarpc_rte_pktmbuf_packet_type *p_type)
{
    tarpc_rte_pktmbuf_set_packet_type_in    in;
    tarpc_rte_pktmbuf_set_packet_type_out   out;
    te_log_buf *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (p_type == NULL)
        TEST_FAIL("Invalid %s() p_type argument", __func__);

    in.m = (tarpc_rte_mbuf)m;
    in.p_type = *p_type;

    rcf_rpc_call(rpcs, "rte_pktmbuf_set_packet_type", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_pktmbuf_set_packet_type,
                                          out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_pktmbuf_set_packet_type,
                 RPC_PTR_FMT ", %s", NEG_ERRNO_FMT, RPC_PTR_VAL(in.m),
                 tarpc_rte_pktmbuf_packet_type2str(tlbp, &in.p_type),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    RETVAL_ZERO_INT(rte_pktmbuf_set_packet_type, out.retval);
}

uint32_t
rpc_rte_pktmbuf_get_rss_hash(rcf_rpc_server *rpcs,
                             rpc_rte_mbuf_p m)
{
    tarpc_rte_pktmbuf_get_rss_hash_in   in;
    tarpc_rte_pktmbuf_get_rss_hash_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_rss_hash", &in, &out);

    /*
     * Strictly speaking, UINT32_MAX is a valid hash, but it will hardly occur
     */
    CHECK_RETVAL_VAR(rte_pktmbuf_get_rss_hash, out.retval, FALSE, UINT32_MAX);

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_rss_hash, RPC_PTR_FMT, "0x%08x",
                 RPC_PTR_VAL(in.m), out.retval);

    TAPI_RPC_OUT(rte_pktmbuf_get_rss_hash, out.retval == UINT32_MAX);

    return (out.retval);
}

void
rpc_rte_pktmbuf_get_tx_offload(rcf_rpc_server *rpcs,
                               rpc_rte_mbuf_p m,
                               struct tarpc_rte_pktmbuf_tx_offload *tx_offload)
{
    tarpc_rte_pktmbuf_get_tx_offload_in     in;
    tarpc_rte_pktmbuf_get_tx_offload_out    out;
    te_log_buf *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (tx_offload == NULL)
        TEST_FAIL("Invalid %s() tx_offload argument", __func__);

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_pktmbuf_get_tx_offload", &in, &out);

    *tx_offload = out.tx_offload;

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_tx_offload, RPC_PTR_FMT,
                 "tx_offload = %s", RPC_PTR_VAL(in.m),
                 tarpc_rte_pktmbuf_tx_offload2str(tlbp, tx_offload));
    te_log_buf_free(tlbp);

    RETVAL_VOID(rte_pktmbuf_get_tx_offload);
}

void
rpc_rte_pktmbuf_set_tx_offload(rcf_rpc_server *rpcs,
                               rpc_rte_mbuf_p m,
                          const struct tarpc_rte_pktmbuf_tx_offload *tx_offload)
{
    tarpc_rte_pktmbuf_set_tx_offload_in     in;
    tarpc_rte_pktmbuf_set_tx_offload_out    out;
    te_log_buf *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (tx_offload == NULL)
        TEST_FAIL("Invalid %s() tx_offload argument", __func__);

    in.m = (tarpc_rte_mbuf)m;
    in.tx_offload = *tx_offload;

    rcf_rpc_call(rpcs, "rte_pktmbuf_set_tx_offload", &in, &out);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_pktmbuf_set_tx_offload,
                 RPC_PTR_FMT ", tx_offload = %s", "", RPC_PTR_VAL(in.m),
                 tarpc_rte_pktmbuf_tx_offload2str(tlbp, &in.tx_offload));
    te_log_buf_free(tlbp);

    RETVAL_VOID(rte_pktmbuf_set_tx_offload);
}
