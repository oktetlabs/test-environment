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

/* DPDK lcore mask */
typedef struct lcore_mask_t {
    /* The lcore mask, with enabled cores bits set.
     * The first element is the least signigicat byte (cores from 0 to 7).
     * Each element specifies 8 cores, least signigicant bit specifies
     * the core with the smallest index.
     */
    uint8_t bytes[32];
} lcore_mask_t;

/**
 * Set bit in a lcore mask
 *
 * @param[out] mask     lcore mask
 * @param[in]  bit      Bit number (0 is lsb)
 *
 * @return Status code.
 */
extern te_errno tapi_rte_lcore_mask_set_bit(lcore_mask_t *mask,
                                            unsigned int bit);

/**
 * @b rte_eal_init() RPC.
 *
 * If error is not expected using #RPC_AWAIT_IUT_ERROR(), the function
 * jumps out in the case of failure.
 */
extern int rpc_rte_eal_init(rcf_rpc_server *rpcs,
                            int argc, char **argv);

/**
 * Allocate EAL argument vector in accordance with environment binding.
 *
 * @param env       Environment binding
 * @param rpcs      RPC server handle
 * @param program_name  Name of a program (@c NULL means use RPC server name)
 * @param lcore_mask_override   Lcore mask to use (@c NULL means use the mask from
 *                              local DPDK configurator tree)
 * @param argc      Number of additional EAL arguments
 * @param argv      Additional EAL arguments
 *
 * @return Status code.
 */
extern te_errno tapi_rte_make_eal_args(tapi_env *env, rcf_rpc_server *rpcs,
                                       const char *program_name,
                                       const lcore_mask_t *lcore_mask_override,
                                       int argc, const char **argv,
                                       int *out_argc, char ***out_argv);

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

/**
 * Wrapper for rpc_rte_eal_hotplug_add() that also resets cached value
 * of EAL arguments in the configurator. The reset is almost always
 * required since hotplug changes the EAL configuration and it
 * interferes with dpdk_reuse_rpcs().
 */
extern te_errno tapi_rte_eal_hotplug_add(rcf_rpc_server *rpcs,
                                         const char *busname,
                                         const char *devname,
                                         const char *devargs);

/**
 * rte_eal_hotplug_remove() RPC
 *
 * @param busname Bus name for the device to be removed from
 * @param devname Device name
 *
 * @return @c 0 on success; jumps out on error (negative value).
 */
extern int rpc_rte_eal_hotplug_remove(rcf_rpc_server *rpcs,
                                      const char     *busname,
                                      const char     *devname);

/**
 * Wrapper for rpc_rte_eal_hotplug_remove() that also resets cached value
 * of EAL arguments in the configurator. The reset is almost always
 * required since hotplug changes the EAL configuration and it
 * interferes with dpdk_reuse_rpcs().
 */
extern te_errno tapi_rte_eal_hotplug_remove(rcf_rpc_server *rpcs,
                                            const char *busname,
                                            const char *devname);

/**
 * @b rte_epoll_wait() RPC
 *
 * If error is not expected using #RPC_AWAIT_IUT_ERROR(), the function
 * jumps out in the case of failure.
 */
extern int rpc_rte_epoll_wait(rcf_rpc_server *rpcs,
                              int epfd,
                              struct tarpc_rte_epoll_event *events,
                              int maxevents,
                              int timeout);

/**
 * Get device arguments of a PCI device.
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  vendor       PCI vendor identifier
 * @param[in]  device       PCI device identifier
 * @param[out] arg_list     Device arguments, must not be @c NULL,
 *                          on success points to comma-separated
 *                          string or to @c NULL.
 *
 * @return Status code
 */
extern te_errno tapi_rte_get_dev_args(const char *ta, const char *vendor,
                                      const char *device, char **arg_list);

/**
 * Get required number of service cores for a PCI device.
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  vendor       PCI vendor identifier
 * @param[in]  device       PCI device identifier
 * @param[out] nb_cores     Required number of service cores
 *
 * @return Status code
 */
extern te_errno tapi_rte_get_nb_required_service_cores(const char *ta,
                                                       const char *vendor,
                                                       const char *device,
                                                       unsigned int *nb_cores);

/**
 * Get required number of service cores for PCI devices specified in an
 * environment for an RPC server.
 *
 * @param[in]  env          Test environment
 * @param[in]  rpcs         RPC server
 * @param[out] nb_cores     Required number of service cores
 *
 * @return Status code
 */
extern te_errno tapi_eal_get_nb_required_service_cores_rpcs(tapi_env *env,
                                                       rcf_rpc_server *rpcs,
                                                       unsigned int *nb_cores);

/**
 * Wrapper for tapi_rte_get_dev_args() that accepts PCI address (BDF notation)
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  pci_addr     PCI address of a device
 * @param[out] arg_list     Device arguments, must not be @c NULL,
 *                          on success points to comma-separated
 *                          string or to @c NULL.
 *
 * @return Status code
 */
extern te_errno tapi_rte_get_dev_args_by_pci_addr(const char *ta,
                                                  const char *pci_addr,
                                                  char **arg_list);

/**@} <!-- END te_lib_rpc_rte_eal --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_EAL_H__ */
