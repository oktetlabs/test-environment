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

#ifndef __TE_TAPI_RPC_POWER_SW_H__
#define __TE_TAPI_RPC_POWER_SW_H__

#include "tapi_rpc_internal.h"
#include "te_power_sw.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_power_sw TAPI for remote calls of power switch
 * @ingroup te_lib_rpc_tapi
 * @{
 */

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

/**
 * Call power switch command on/off/rst for given
 * power switch lines
 *
 * @param rpcs      RPC server handle
 * @param type      Power switch device type tty/parport,
 *                  parport on default
 * @param dev       Power switch device name,
 *                  default parport device - /dev/parport0
 *                  default tty device - /dev/ttyS0
 * @param mask      Power lines bitmask, position of each
 *                  nonzero bit in mask denotes number of power
 *                  line to apply specified command
 * @param cmd       Power switch command on/off/rst to be applied
 *                  to power lines specified in bitmask
 *
 * @return          0 on success, otherwise -1
 */
extern int rpc_power_sw(rcf_rpc_server *rpcs, const char *type,
                        const char *dev,  int mask, const char *cmd);

/**@} <!-- END te_lib_rpc_power_sw --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_POWER_SW_H__ */
