/** @file
 * @brief RPC client API for helper DPDK functions
 *
 * RPC client API for helper DPDK functions
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#include "te_config.h"

#include "log_bufs.h"
#include "tapi_mem.h"
#include "tapi_rpc_internal.h"
#include "tapi_test_log.h"

#include "tarpc.h"

#include "tapi_rpc_dpdk.h"
#include "rpcc_dpdk.h"
#include "te_str.h"

int
rpc_dpdk_find_representors(rcf_rpc_server *rpcs, unsigned int *n_rep,
                           uint16_t **rep_port_ids)
{
    unsigned int i;
    uint16_t *port_ids;
    tarpc_dpdk_find_representors_in  in;
    tarpc_dpdk_find_representors_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rcf_rpc_call(rpcs, "dpdk_find_representors", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(dpdk_find_representors, out.retval);

    TAPI_RPC_LOG(rpcs, dpdk_find_representors, "found: %u", NEG_ERRNO_FMT,
                 (out.retval == 0) ? out.n_rep : 0, NEG_ERRNO_ARGS(out.retval));

    if (out.retval == 0)
    {
        port_ids = tapi_calloc(out.n_rep, sizeof(*port_ids));
        for (i = 0; i < out.n_rep; i++)
            port_ids[i] = out.rep_port_ids.rep_port_ids_val[i];

        *n_rep = out.n_rep;
        *rep_port_ids = port_ids;
    }

    RETVAL_ZERO_INT(dpdk_find_representors, out.retval);
}
