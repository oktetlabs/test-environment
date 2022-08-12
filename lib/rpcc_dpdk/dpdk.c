/** @file
 * @brief RPC client API for helper DPDK functions
 *
 * RPC client API for helper DPDK functions
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
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

static const char *
tarpc_rte_eth_representor_type2str(enum tarpc_rte_eth_representor_type type)
{
    switch (type)
    {
        case TARPC_RTE_ETH_REPRESENTOR_NONE:
            return "NONE";
        case TARPC_RTE_ETH_REPRESENTOR_VF:
            return "VF";
        case TARPC_RTE_ETH_REPRESENTOR_SF:
            return "SF";
        case TARPC_RTE_ETH_REPRESENTOR_PF:
            return "PF";
        default:
            return "<UNKNOWN>";
    }
}

static const char *
tarpc_rte_eth_representor_info2str(te_log_buf *tlbp,
                        const struct tarpc_rte_eth_representor_info *info)
{
    unsigned int i;

    te_log_buf_append(tlbp, "{ ");

    te_log_buf_append(tlbp, "controller=%d, pf=%d, n_ranges=%d",
                      info->controller, info->pf, info->ranges.ranges_len);

    te_log_buf_append(tlbp, ", ranges={ ");

    for (i = 0; i < info->ranges.ranges_len; i++)
    {
        struct tarpc_rte_eth_representor_range *range =
            &info->ranges.ranges_val[i];

        te_log_buf_append(tlbp, "{ type=%s, controller=%d, pf=%d, vfsf=%d, "
                          "id_base=%u, id_end=%u, name=%s }%s ",
                          tarpc_rte_eth_representor_type2str(range->type),
                          range->controller, range->pf, range->vfsf,
                          range->id_base, range->id_end, range->name,
                          (i == info->ranges.ranges_len - 1) ? "" : ",");
    }


    te_log_buf_append(tlbp, "} }");
    return te_log_buf_get(tlbp);
}

int
rpc_rte_eth_representor_info_get(rcf_rpc_server *rpcs, uint16_t port_id,
                                 struct tarpc_rte_eth_representor_info *info)
{
    struct tarpc_rte_eth_representor_info_get_in   in;
    struct tarpc_rte_eth_representor_info_get_out  out;
    te_log_buf                                    *tlbp = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.port_id = port_id;
    if (info != NULL)
    {
        in.info.info_len = 1;
        in.info.info_val = info;
    }

    rcf_rpc_call(rpcs, "rte_eth_representor_info_get", &in, &out);

    /*
     * No need to check return value since all values are allowed
     * (negative errno, 0 or more for the number of representor ranges)
     */

    if (RPC_IS_CALL_OK(rpcs) && out.retval >= 0 && info != NULL)
    {
        info->controller = out.info.controller;
        info->pf = out.info.pf;
        info->nb_ranges = out.info.nb_ranges;
        memcpy(info->ranges.ranges_val, out.info.ranges.ranges_val,
               info->nb_ranges * sizeof(*info->ranges.ranges_val));

        tlbp = te_log_buf_alloc();
    }

    TAPI_RPC_LOG(rpcs, rte_eth_representor_info_get,
                 "%hu %p",
                 NEG_ERRNO_FMT", info=%s",
                 port_id, info,
                 NEG_ERRNO_ARGS(out.retval), tlbp != NULL ?
                 tarpc_rte_eth_representor_info2str(tlbp, info) : "N/A");

    te_log_buf_free(tlbp);

    RETVAL_INT(rte_eth_representor_info_get, out.retval);
}
