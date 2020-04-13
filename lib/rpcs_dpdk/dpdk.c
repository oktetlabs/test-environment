/** @file
 * @brief RPC for helper DPDK functions
 *
 * RPC helper DPDK routines implementation.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
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
