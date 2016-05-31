/** @file
 * @brief RPC client API for DPDK EAL
 *
 * RPC client API for DPDK EAL functions.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_EAL_H__
#define __TE_TAPI_RPC_RTE_EAL_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_eal TAPI for RTE EAL API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/**
 * @b rte_eal_init() RPC.
 *
 * If error is not expected using #RPC_AWAIT_IUT_ERROR(), the function
 * jumps out in the case of failure.
 */
extern int rpc_rte_eal_init(rcf_rpc_server *rpcs,
                            int argc, char **argv);

/**
 * Initialize EAL library in accordance with environment binding.
 *
 * @param env       Environment binding
 * @param rpcs      RPC server handle
 * @param argc      Number of additional EAL arguments
 * @param argv      Additional EAL arguments
 *
 * @return Status code.
 */
extern te_errno tapi_rte_eal_init(tapi_env *env, rcf_rpc_server *rpcs,
                                  int argc, char **argv);

/**@} <!-- END te_lib_rpc_rte_eal --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_EAL_H__ */
