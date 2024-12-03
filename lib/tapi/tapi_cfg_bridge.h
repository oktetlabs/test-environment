/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to configure bridge.
 *
 * @defgroup tapi_conf_bridge Bridge configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure bridge.
 */

#ifndef __TE_TAPI_CFG_BRIDGE_H__
#define __TE_TAPI_CFG_BRIDGE_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** List of supported bridge providers */
typedef enum tapi_cfg_bridge_provider {
    TAPI_CFG_BRIDGE_PROVIDER_DEFAULT,
} tapi_cfg_bridge_provider;

/** Bridge configuration */
typedef struct tapi_cfg_bridge {
    /** Bridge name (can't be @c NULL) */
    char *bridge_name;
    /** Bridge provider */
    tapi_cfg_bridge_provider provider;
} tapi_cfg_bridge;

/**
 * Add bridge.
 *
 * @param ta            Test Agent.
 * @param conf          Bridge configuration.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_bridge_add(const char *ta,
                                    const tapi_cfg_bridge *conf);

/**
 * Delete bridge.
 *
 * @param ta            Test Agent.
 * @param bridge_name   Bridge name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_bridge_del(const char *ta, const char *bridge_name);

/**
 * Get bridge configuration information.
 *
 * @param ta            Test Agent.
 * @param bridge_name   Bridge name.
 * @param conf          Pointer of bridge configuration to store.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_bridge_get(const char *ta,
                                    const char *bridge_name,
                                    tapi_cfg_bridge *conf);

/**
 * Add interface in bridge @p bridge_name.
 *
 * @param ta            Test Agent.
 * @param bridge_name   Bridge name.
 * @param if_name       New bridge interface.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_bridge_port_add(const char *ta,
                                         const char *bridge_name,
                                         const char *if_name);

/**
 * Remove interface from bridge @p bridge_name.
 *
 * @param ta            Test Agent.
 * @param bridge_name   Bridge name.
 * @param if_name       Bridge interface.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_bridge_port_del(const char *ta,
                                         const char *bridge_name,
                                         const char *if_name);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_BRIDGE_H__ */
