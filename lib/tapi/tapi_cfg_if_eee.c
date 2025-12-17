/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for Energy Efficient Ethernet
 * configuration (storage/cm/cm_base.xml).
 */

#define TE_LGR_USER "Config EEE"

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_if_eee.h"

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_enabled_get(const char *ta, const char *if_name,
                            bool *enabled)
{
    return cfg_get_bool(enabled, "/agent:%s/interface:%s/eee:/eee_enabled:",
                        ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_enabled_set(const char *ta, const char *if_name,
                            bool enabled)
{
    return cfg_set_instance_fmt(CFG_VAL(BOOL, enabled),
                                "/agent:%s/interface:%s/eee:/eee_enabled:",
                                ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_enabled_set_local(const char *ta, const char *if_name,
                                  bool enabled)
{
    return cfg_set_instance_local_fmt(
                                CFG_VAL(BOOL, enabled),
                                "/agent:%s/interface:%s/eee:/eee_enabled:",
                                ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_active_get(const char *ta, const char *if_name,
                           bool *active)
{
    return cfg_get_bool(active, "/agent:%s/interface:%s/eee:/eee_active:",
                        ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_tx_lpi_enabled_get(const char *ta, const char *if_name,
                                   bool *enabled)
{
    return cfg_get_bool(enabled,
                        "/agent:%s/interface:%s/eee:/tx_lpi_enabled:",
                        ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_tx_lpi_enabled_set(const char *ta, const char *if_name,
                                   bool enabled)
{
    return cfg_set_instance_fmt(
                          CFG_VAL(BOOL, enabled),
                          "/agent:%s/interface:%s/eee:/tx_lpi_enabled:",
                          ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_tx_lpi_enabled_set_local(const char *ta,
                                         const char *if_name,
                                         bool enabled)
{
    return cfg_set_instance_local_fmt(
                          CFG_VAL(BOOL, enabled),
                          "/agent:%s/interface:%s/eee:/tx_lpi_enabled:",
                          ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_tx_lpi_timer_get(const char *ta, const char *if_name,
                                 uint32_t *timer)
{
    return cfg_get_uint32(timer,
                          "/agent:%s/interface:%s/eee:/tx_lpi_timer:",
                          ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_tx_lpi_timer_set(const char *ta, const char *if_name,
                                 uint32_t timer)
{
    return cfg_set_instance_fmt(
                          CFG_VAL(UINT32, timer),
                          "/agent:%s/interface:%s/eee:/tx_lpi_timer:",
                          ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_tx_lpi_timer_set_local(const char *ta,
                                       const char *if_name,
                                       uint32_t timer)
{
    return cfg_set_instance_local_fmt(
                          CFG_VAL(UINT32, timer),
                          "/agent:%s/interface:%s/eee:/tx_lpi_timer:",
                          ta, if_name);
}

/* See description in tapi_cfg_if_eee.h */
te_errno
tapi_cfg_if_eee_commit(const char *ta, const char *if_name)
{
    return cfg_commit_fmt("/agent:%s/interface:%s/eee:",
                          ta, if_name);
}
