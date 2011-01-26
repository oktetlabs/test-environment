/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of ioctl() function
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: ioctl.c 65691 2010-08-24 12:59:21Z rast $
 */

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
