/** @file
 * @brief Test API for RPC
 *
 * TAPI for pipes function
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Yurij Plotnikov <Yurij.Plotnikov@oktetlabs.ru>
 *
 */

#include "te_config.h"

#include "tapi_rpc.h"
#include "te_bufs.h"

#define CHECK_BUF_SIZE 1024

int tapi_check_pipe(rcf_rpc_server *rpcs, int *pipefds)
{
    void    *tx_buf = NULL;
    void    *rx_buf = NULL;
    int      ret;

    tx_buf = te_make_buf_by_len(CHECK_BUF_SIZE);
    rx_buf = te_make_buf_by_len(CHECK_BUF_SIZE);
    te_fill_buf(tx_buf, CHECK_BUF_SIZE);

    if ((ret = rpc_write(rpcs, pipefds[1], tx_buf, CHECK_BUF_SIZE)) !=
        CHECK_BUF_SIZE)
    {
        ERROR("write() sent %d bytes instead of %d", ret, CHECK_BUF_SIZE);
        return -1;
    }
    ret = rpc_read(rpcs, pipefds[0], rx_buf, CHECK_BUF_SIZE);
    if (ret != CHECK_BUF_SIZE)
    {
        ERROR("Incorrect amount of data was received on the pipe.");
        return -1;
    }

    if (memcmp(tx_buf, rx_buf, CHECK_BUF_SIZE) != 0)
    {
        ERROR("Incorrect data were received on the pipe.");
        return -1;
    }
    return 0;
}
