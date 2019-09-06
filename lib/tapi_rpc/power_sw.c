/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of power switch.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#include "tapi_rpc_power_sw.h"
#include "tapi_rpc_internal.h"

#define POWER_SW_STR2_CMD(_cmd_str)                                 \
(_cmd_str == NULL) ?                                                \
    CMD_UNSPEC :                                                    \
        (strcmp(_cmd_str, CMD_STR_TURN_ON) == 0) ?                  \
            CMD_TURN_ON :                                           \
                (strcmp(_cmd_str, CMD_STR_TURN_OFF) == 0) ?         \
                    CMD_TURN_OFF :                                  \
                        (strcmp(_cmd_str, CMD_STR_RESTART) == 0) ?  \
                            CMD_RESTART :\
                                CMD_INVAL

#define POWER_SW_STR2_DEV_TYPE(_type_str)                           \
(_type_str == NULL) ?                                               \
    DEV_TYPE_UNSPEC :                                               \
        (strcmp(_type_str, DEV_TYPE_STR_PARPORT) == 0) ?            \
            DEV_TYPE_PARPORT :                                      \
                (strcmp(_type_str, DEV_TYPE_STR_TTY) == 0) ?        \
                    DEV_TYPE_TTY :                                  \
                        DEV_TYPE_INVAL

int
rpc_power_sw(rcf_rpc_server *rpcs, const char *type,
             const char *dev,  int mask, const char *cmd)
{
    tarpc_power_sw_in   in;
    tarpc_power_sw_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(power_sw, -1);
    }

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

    in.dev = (dev == NULL) ? strdup("unspec") : strdup(dev);

    rcf_rpc_call(rpcs, "power_sw", &in, &out);

    TAPI_RPC_LOG(rpcs, power_sw, "%s, %s, %X, %s", "%d",
                 (type != NULL) ? type : "unspec",
                 (dev != NULL) ? dev : "unspec",
                 mask, cmd, out.retval);

    if (in.dev != NULL)
        free(in.dev);

    RETVAL_INT(power_sw, out.retval);
}

