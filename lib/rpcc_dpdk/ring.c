/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC client API for DPDK ring library
 *
 * RPC client API for DPDK ring library functions (implementation)
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte.h"
#include "tapi_rpc_rte_ring.h"
#include "tapi_mem.h"
#include "log_bufs.h"
#include "rpcc_dpdk.h"
#include "tapi_test_log.h"

#include "tarpc.h"

rpc_rte_ring_p
rpc_rte_ring_create(rcf_rpc_server *rpcs,
                    const char     *name,
                    unsigned        count,
                    int             socket_id,
                    unsigned        flags)
{
    tarpc_rte_ring_create_in    in;
    tarpc_rte_ring_create_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.name = tapi_strdup(name);
    in.count = count;
    in.socket_id = socket_id;
    in.flags = flags;

    rcf_rpc_call(rpcs, "rte_ring_create", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_ring_create, out.retval);

    TAPI_RPC_LOG(rpcs, rte_ring_create, "%s, %u, %d", RPC_PTR_FMT,
                 in.name, in.count, in.socket_id, RPC_PTR_VAL(out.retval));

    free(in.name);

    RETVAL_RPC_PTR(rte_ring_create, out.retval);
}

void
rpc_rte_ring_free(rcf_rpc_server *rpcs,
                       rpc_rte_ring_p  ring)
{
    tarpc_rte_ring_free_in  in;
    tarpc_rte_ring_free_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.ring = (tarpc_rte_ring)ring;

    rcf_rpc_call(rpcs, "rte_ring_free", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_ring_free, RPC_PTR_FMT, "", RPC_PTR_VAL(in.ring));

    RETVAL_VOID(rte_ring_free);
}

int
rpc_rte_ring_enqueue_mbuf(rcf_rpc_server *rpcs,
                          rpc_rte_ring_p  ring,
                          rpc_rte_mbuf_p  m)
{
    tarpc_rte_ring_enqueue_mbuf_in  in;
    tarpc_rte_ring_enqueue_mbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.ring = (tarpc_rte_ring)ring;
    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_ring_enqueue_mbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_ring_enqueue_mbuf, out.retval);

    TAPI_RPC_LOG(rpcs, rte_ring_enqueue_mbuf, RPC_PTR_FMT ", " RPC_PTR_FMT,
                 NEG_ERRNO_FMT, RPC_PTR_VAL(in.ring), RPC_PTR_VAL(in.m),
                 NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_ring_enqueue_mbuf, out.retval);
}

rpc_rte_mbuf_p
rpc_rte_ring_dequeue_mbuf(rcf_rpc_server *rpcs,
                          rpc_rte_ring_p  ring)
{
    tarpc_rte_ring_dequeue_mbuf_in  in;
    tarpc_rte_ring_dequeue_mbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.ring = (tarpc_rte_ring)ring;

    rcf_rpc_call(rpcs, "rte_ring_dequeue_mbuf", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(rte_ring_dequeue_mbuf, out.retval);

    TAPI_RPC_LOG(rpcs, rte_ring_dequeue_mbuf, RPC_PTR_FMT, RPC_PTR_FMT,
                 RPC_PTR_VAL(in.ring), RPC_PTR_VAL(out.retval));

    RETVAL_RPC_PTR(rte_ring_dequeue_mbuf, out.retval);
}
