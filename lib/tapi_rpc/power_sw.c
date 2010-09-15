/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of power switch.
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in
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
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#include "tapi_rpc_power_sw.h"
#include "tapi_rpc_internal.h"

int
rpc_power_sw(rcf_rpc_server *rpcs, const char *type,
             const char *dev,  int mask, const char *cmd)
{
    rcf_rpc_op          op;
    tarpc_power_sw_in   in;
    tarpc_power_sw_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(power_sw, -1);
    }

    op = rpcs->op;
    in.type = POWER_SW_STR2_DEV_TYPE(type);
    in.mask = mask;
    in.cmd = POWER_SW_STR2_CMD(cmd);

    if (in.type == DEV_TYPE_INVAL)
    {
        ERROR("%s(): Invalid power switch device type specificetion %s",
              __FUNCTION__, type);
        return -1;
    }

    if (in.cmd == CMD_INVAL)
    {
        ERROR("%s(): Invalid power switch command specificetion %s",
              __FUNCTION__, cmd);
        return -1;
    }

    if (in.cmd == CMD_UNSPEC)
    {
        ERROR("%s(): Power switch command is not specified",
              __FUNCTION__);
        return -1;
    }

    in.dev = (dev == NULL) ? NULL : strdup(dev);

    rcf_rpc_call(rpcs, "power_sw", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: power_sw(%s, %s, %X, %s) "
                 "-> %d (%s)", rpcs->ta, rpcs->name, rpcop2str(op),
                 type, (dev != NULL) ? dev : "unspec",
                 mask, cmd, out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    if (in.dev != NULL)
        free(in.dev);

    RETVAL_INT(power_sw, out.retval);
}

