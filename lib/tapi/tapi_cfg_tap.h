/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to configure TAP interface.
 *
 * @defgroup tapi_conf_tap TAP interface configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure TAP interface.
 */

#ifndef __TE_TAPI_CFG_TAP_H__
#define __TE_TAPI_CFG_TAP_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add TAP interface.
 *
 * @param ta            Test Agent.
 * @param ifname        Interface name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tap_add(const char *ta, const char *ifname);

/**
 * Delete TAP interface.
 *
 * @param ta            Test Agent.
 * @param ifname        Interface name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tap_del(const char *ta, const char *ifname);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_TAP_H__ */
