/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */

#include "te_config.h"

#include "conf_api.h"
#include "logger_api.h"

#include "tapi_cfg_base.h"
#include "tapi_cfg_tap.h"

#define TE_CFG_TA_TAP            "/agent:%s/tap:%s"

/* See description in tapi_cfg_tap.h */
te_errno
tapi_cfg_tap_add(const char *ta, const char *ifname)
{
    te_errno rc;

    rc = tapi_cfg_base_if_add_rsrc(ta, ifname);
    if (rc != 0)
    {
        ERROR("Failed to add interface '%s' from the TA '%s' resources: %r",
              ifname, ta, rc);
        return rc;
    }

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                              TE_CFG_TA_TAP, ta, ifname);
    if (rc != 0)
    {
        ERROR("Failed to add TAP interface '%s': %r", ifname, rc);
        goto err_add_if;
    }

    return 0;

err_add_if:
    tapi_cfg_base_if_del_rsrc(ta, ifname);

    return rc;
}

/* See description in tapi_cfg_tap.h */
te_errno
tapi_cfg_tap_del(const char *ta, const char *ifname)
{
    te_errno rc;

    rc = cfg_del_instance_fmt(false, TE_CFG_TA_TAP, ta, ifname);
    if (rc != 0)
    {
        ERROR("Failed to remove TAP interface '%s': %r", ifname, rc);
        return rc;
    }

    rc = tapi_cfg_base_if_del_rsrc(ta, ifname);
    if (rc != 0)
    {
        ERROR("Failed to remove interface '%s' from the TA '%s' resources: %r",
              ifname, ta, rc);
        return rc;
    }

    return 0;
}
