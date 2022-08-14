/** @file
 * @brief API to configure some system options via /proc/sys
 *
 * Implementation of API for access to some system options via /proc/sys.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI RPC Proc"

#include "te_config.h"
#include "tapi_proc.h"
#include "tapi_rpc_unistd.h"

/* See description in tapi_proc.h */
int
tapi_cfg_net_route_flush(rcf_rpc_server *rpcs)
{
    int     fd;
    te_bool wait_err = FALSE;

    if (RPC_AWAITING_ERROR(rpcs))
        wait_err = TRUE;

    if ((fd = rpc_open(rpcs, "/proc/sys/net/ipv4/route/flush",
                       RPC_O_WRONLY, 0)) < 0)
        return -1;

    if (wait_err)
        RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_write(rpcs, fd, "1", 1) < 0)
    {
        rpc_close(rpcs, fd);
        return -1;
    }

    if (wait_err)
        RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_close(rpcs, fd) != 0)
        return -1;

    return 0;
}
