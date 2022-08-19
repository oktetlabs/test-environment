/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC for helper DPDK functions
 *
 * RPC helper DPDK routines implementation.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "RPC dpdk"

#include "te_config.h"

#include "te_alloc.h"
#include "te_str.h"

#include "rte_config.h"
#include "rte_ethdev.h"
#include "rte_eth_ctrl.h"
#include "rte_version.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"

static int
dpdk_rte_eth_dev_info_get(uint16_t port, struct rte_eth_dev_info *dev_info)
{
    int rc = 0;

#if HAVE_RTE_DEV_INFO_GET_RETURN_VOID
    rte_eth_dev_info_get(port, dev_info);
#else
    rc = rte_eth_dev_info_get(port, dev_info);
#endif

    return rc;
}

TARPC_FUNC_STANDALONE(dpdk_find_representors, {},
{
#ifdef RTE_ETH_DEV_REPRESENTOR
    uint16_t port;
    struct rte_eth_dev_info info;
    unsigned int n_rep = 0;
    int rc = 0;

    RTE_ETH_FOREACH_DEV(port)
    {
        memset(&info, 0, sizeof(info));
        rc = dpdk_rte_eth_dev_info_get(port, &info);
        if (rc != 0)
            goto exit;

        if (info.dev_flags != NULL &&
            (*info.dev_flags & RTE_ETH_DEV_REPRESENTOR))
        {
            n_rep++;
        }
    }


    if (n_rep != 0)
    {
        unsigned int i;

        out->rep_port_ids.rep_port_ids_len = n_rep;
        out->rep_port_ids.rep_port_ids_val = TE_ALLOC(n_rep *
                sizeof(*out->rep_port_ids.rep_port_ids_val));
        if (out->rep_port_ids.rep_port_ids_val == NULL)
        {
            rc = -ENOMEM;
            goto exit;
        }

        i = 0;
        RTE_ETH_FOREACH_DEV(port)
        {
            memset(&info, 0, sizeof(info));
            rc = dpdk_rte_eth_dev_info_get(port, &info);
            if (rc != 0)
                goto exit;

            if (info.dev_flags != NULL &&
                (*info.dev_flags & RTE_ETH_DEV_REPRESENTOR))
            {
                out->rep_port_ids.rep_port_ids_val[i++] = port;
            }
        }
    }

    out->n_rep = n_rep;

exit:
    if (rc != 0)
        free(out->rep_port_ids.rep_port_ids_val);
    out->retval = rc;
    neg_errno_h2rpc(&out->retval);
#else
    out->n_rep = 0;
    out->rep_port_ids.rep_port_ids_len = 0;
    out->retval = 0;
#endif /* RTE_ETH_DEV_REPRESENTOR */
})

TARPC_FUNC(rte_eth_representor_info_get, {},
{
    int rc = 0;
    int ret;
    size_t copied;
    uint32_t i;
    struct rte_eth_representor_info *info = NULL;
    struct tarpc_rte_eth_representor_range *ranges = NULL;
    unsigned int n_ranges;

    if (in->info.info_len == 0)
    {
        MAKE_CALL(ret = func(in->port_id, NULL));
        rc = ret;
        goto exit;
    }

    n_ranges = in->info.info_val[0].ranges.ranges_len;
    info = TE_ALLOC(sizeof(*info) + n_ranges * sizeof(info->ranges[0]));
    if (info == NULL)
    {
        rc = -ENOMEM;
        goto exit;
    }
    info->nb_ranges_alloc = n_ranges;

    MAKE_CALL(ret = func(in->port_id, info));

    memset(&out->info, 0, sizeof(out->info));
    if (ret >= 0)
    {
        out->info.controller = info->controller;
        out->info.pf = info->pf;
        out->info.nb_ranges = info->nb_ranges;

        ranges = TE_ALLOC(info->nb_ranges * sizeof(*ranges));
        if (ranges == NULL)
        {
            rc = -ENOMEM;
            goto exit;
        }

        for (i = 0; i < info->nb_ranges; i++)
        {
            ranges[i].type = info->ranges[i].type;
            ranges[i].controller = info->ranges[i].controller;
            ranges[i].pf = info->ranges[i].pf;
            ranges[i].vfsf = info->ranges[i].vf;
            ranges[i].id_base = info->ranges[i].id_base;
            ranges[i].id_end = info->ranges[i].id_end;
            copied = te_strlcpy(ranges[i].name, info->ranges[i].name,
                                TARPC_RTE_DEV_NAME_MAX_LEN);
            if (copied >= TARPC_RTE_DEV_NAME_MAX_LEN)
            {
                rc = -ENAMETOOLONG;
                goto exit;
            }
        }

        out->info.ranges.ranges_len = info->nb_ranges;
        out->info.ranges.ranges_val = ranges;
    }
    rc = ret;

exit:
    free(info);
    out->retval = rc;
    neg_errno_h2rpc(&out->retval);
})

