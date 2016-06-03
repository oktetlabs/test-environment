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
#include "tapi_mem.h"
#include "log_bufs.h"
#include "rpcc_dpdk.h"

#include "tarpc.h"


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

    RETVAL_INT(rte_pktmbuf_chain, out.retval);
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

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_nb_segs, RPC_PTR_FMT, "%uhh",
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

    TAPI_RPC_LOG(rpcs, rte_pktmbuf_get_port, RPC_PTR_FMT, "%uhh",
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
