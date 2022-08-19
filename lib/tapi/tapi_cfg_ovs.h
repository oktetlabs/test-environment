/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief OVS Configuration Model TAPI
 *
 * Definition of test API for OVS configuration model
 * (doc/cm/cm_ovs.yml).
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_OVS_H__
#define __TE_TAPI_CFG_OVS_H__

#include "conf_api.h"
#include "te_kvpair.h"

#define TAPI_OVS_OTHER_CFG "other_config"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_ovs PCI devices configuration of Test Agents
 * @ingroup tapi_conf
 * @{
 */

/** Configuration entry type, see tapi_cfg_ovs_cfg_name for details */
typedef enum tapi_cfg_ovs_cfg_type {
    TAPI_CFG_OVS_CFG_DPDK_ALLOC_MEM = 0,
    TAPI_CFG_OVS_CFG_DPDK_SOCKET_MEM,
    TAPI_CFG_OVS_CFG_DPDK_LCORE_MASK,
    TAPI_CFG_OVS_CFG_DPDK_HUGEPAGE_DIR,
    TAPI_CFG_OVS_CFG_DPDK_SOCKET_LIMIT,
    TAPI_CFG_OVS_CFG_DPDK_EXTRA,

    TAPI_CFG_OVS_CFG_DPDK_NTYPES, /**< Defines number of valid types */
} tapi_cfg_ovs_cfg_type;

/**
 * Configuration entry name. Names correspond to Open vSwitch configuration
 * entries.
 */
const char *const tapi_cfg_ovs_cfg_name[] = {
    [TAPI_CFG_OVS_CFG_DPDK_ALLOC_MEM] = TAPI_OVS_OTHER_CFG ":dpdk-alloc-mem",
    [TAPI_CFG_OVS_CFG_DPDK_SOCKET_MEM] = TAPI_OVS_OTHER_CFG ":dpdk-socket-mem",
    [TAPI_CFG_OVS_CFG_DPDK_LCORE_MASK] = TAPI_OVS_OTHER_CFG ":dpdk-lcore-mask",
    [TAPI_CFG_OVS_CFG_DPDK_HUGEPAGE_DIR] =
        TAPI_OVS_OTHER_CFG ":dpdk-hugepage-dir",
    [TAPI_CFG_OVS_CFG_DPDK_SOCKET_LIMIT] =
        TAPI_OVS_OTHER_CFG ":dpdk-socket-limit",
    [TAPI_CFG_OVS_CFG_DPDK_EXTRA] = TAPI_OVS_OTHER_CFG ":dpdk-extra",
};

/** Open vSwitch Configuration entry array */
typedef struct tapi_cfg_ovs_cfg {
    char *values[TAPI_CFG_OVS_CFG_DPDK_NTYPES];
} tapi_cfg_ovs_cfg;

/**
 * Convert raw DPDK EAL arguments into Open vSwitch configuration entries.
 *
 * @param[in]  argc     Number of arguments
 * @param[in]  argv     EAL arguments
 * @param[out] ovs_cfg  Open vSwitch configuraion
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ovs_convert_eal_args(int argc, const char *const *argv,
                                              tapi_cfg_ovs_cfg *ovs_cfg);

/**@} <!-- END tapi_conf_ovs --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_OVS_H__ */
