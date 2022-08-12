/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of power switch.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#include "te_config.h"
#include "tapi_rpc_power_sw.h"
#include "tapi_rpc_internal.h"

static power_sw_cmd
power_sw_str2cmd(const char *cmd_str)
{
    if (cmd_str == NULL)
        return CMD_UNSPEC;
    else if (strcmp(cmd_str, CMD_STR_TURN_ON) == 0)
        return CMD_TURN_ON;
    else if (strcmp(cmd_str, CMD_STR_TURN_OFF) == 0)
        return CMD_TURN_OFF;
    else if (strcmp(cmd_str, CMD_STR_RESTART) == 0)
        return CMD_RESTART;
    else
        return CMD_INVAL;
}

static power_sw_dev_type
power_sw_str2dev(const char *dev_str)
{
    if (dev_str == NULL)
        return DEV_TYPE_UNSPEC;
    else if (strcmp(dev_str, DEV_TYPE_STR_PARPORT) == 0)
        return DEV_TYPE_PARPORT;
    else if (strcmp(dev_str, DEV_TYPE_STR_TTY) == 0)
        return DEV_TYPE_TTY;
    else if (strcmp(dev_str, DEV_TYPE_STR_DIGISPARK) == 0)
        return DEV_TYPE_DIGISPARK;
    else
        return DEV_TYPE_INVAL;
}

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

    in.type = power_sw_str2dev(type);
    in.mask = mask;
    in.cmd = power_sw_str2cmd(cmd);

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

