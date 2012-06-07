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

#define POWER_SW_CMD2_STR(_cmd)                                 \
(_cmd == CMD_TURN_ON) ?                                         \
    CMD_STR_TURN_ON :                                           \
        (_cmd == CMD_TURN_OFF) ?                                \
            CMD_STR_TURN_OFF :                                  \
                (_cmd == CMD_RESTART) ?                         \
                    CMD_STR_RESTART :                           \
                        (_cmd == CMD_UNSPEC) ?                  \
                            CMD_STR_UNSPEC :                    \
                                CMD_STR_INVAL

#define POWER_SW_DEV_TYPE2_STR(_type)                           \
(_type == DEV_TYPE_PARPORT) ?                                   \
    DEV_TYPE_STR_PARPORT :                                      \
        (_type == DEV_TYPE_TTY) ?                               \
            DEV_TYPE_STR_TTY :                                  \
                (_type == DEV_TYPE_UNSPEC) ?                    \
                    DEV_TYPE_STR_UNSPEC :                       \
                                DEV_TYPE_STR_INVAL

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
