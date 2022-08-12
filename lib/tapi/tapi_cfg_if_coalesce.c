/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Implementation of test API for Network Interface Interrupt
 * Coalescing settings.
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "Config Intr Coalesce"

#include "te_config.h"
#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_if_coalesce.h"

/* See description in tapi_cfg_if_coalesce.h */
te_errno
tapi_cfg_if_coalesce_get(const char *ta, const char *if_name,
                         const char *param, uint64_t *val)
{
    return cfg_get_instance_uint64_fmt(val,
                                   "/agent:%s/interface:%s/coalesce:/param:%s",
                                   ta, if_name, param);
}

/* See description in tapi_cfg_if_coalesce.h */
te_errno
tapi_cfg_if_coalesce_set(const char *ta, const char *if_name,
                         const char *param, uint64_t val)
{
    return cfg_set_instance_fmt(
                CFG_VAL(UINT64, val),
                "/agent:%s/interface:%s/coalesce:/param:%s",
                ta, if_name, param);
}

/* See description in tapi_cfg_if_coalesce.h */
te_errno
tapi_cfg_if_coalesce_set_local(const char *ta, const char *if_name,
                               const char *param, uint64_t val)
{
    return cfg_set_instance_local_fmt(
                CFG_VAL(UINT64, val),
                "/agent:%s/interface:%s/coalesce:/param:%s",
                ta, if_name, param);
}

/* See description in tapi_cfg_if_coalesce.h */
te_errno
tapi_cfg_if_coalesce_commit(const char *ta, const char *if_name)
{
    return cfg_commit_fmt(
                "/agent:%s/interface:%s/coalesce:",
                ta, if_name);
}
