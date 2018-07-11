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

#define TAPI_RTE_VERSION_NUM(a,b,c,d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

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

/**
 * Get DPDK version (4 components combined in a single number).
 *
 * @note WARNING: This RPC is a compelled elaboration to cope
 *                with drastic differences amongst DPDK versions.
 *                The engineer has to think meticulously before
 *                attempting to use this RPC for any new code.
 *
 * @return DPDK version.
 */
extern int rpc_dpdk_get_version(rcf_rpc_server *rpcs);

/**
 * rte_eal_hotplug_add() RPC
 *
 * @param busname Bus name for the device to be added to
 * @param devname Device name to undergo indentification and probing
 * @param devargs Device arguments to be passed to the driver
 *
 * @return @c 0 on success; jumps out on error (negative value).
 */
extern int rpc_rte_eal_hotplug_add(rcf_rpc_server *rpcs,
                                   const char     *busname,
                                   const char     *devname,
                                   const char     *devargs);

/**@} <!-- END te_lib_rpc_rte_eal --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_EAL_H__ */
