/** @file
 * @brief RPC client API for DPDK EAL
 *
 * RPC client API for DPDK EAL functions.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_EAL_H__
#define __TE_TAPI_RPC_RTE_EAL_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"

#include "tapi_env.h"

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
                                  int argc, const char **argv);


/** Map RTE EAL process type to string. */
extern const char *tarpc_rte_proc_type_t2str(enum tarpc_rte_proc_type_t val);

/**
 * rte_eal_process_type() RPC.
 *
 * If error is not expected using #RPC_AWAIT_IUT_ERROR(), the function
 * jumps out in the case of unknown process type returned.
 */
extern enum tarpc_rte_proc_type_t
    rpc_rte_eal_process_type(rcf_rpc_server *rpcs);

/**@} <!-- END te_lib_rpc_rte_eal --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_EAL_H__ */
