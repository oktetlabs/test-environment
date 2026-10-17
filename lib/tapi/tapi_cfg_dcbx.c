/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Data Center Bridging eXchange configuration
 *
 * Data Center Bridging eXchange configuration.
 */

#include "conf_api.h"

#include "tapi_cfg_dcb.h"
#include "tapi_cfg_dcbx.h"

/* See the description in tapi_cfg_dcbx.h. */
te_errno
tapi_cfg_dcbx_is_present(const char *ta, const char *if_name, bool *present)
{
    return tapi_cfg_dcb_node_is_present(ta, if_name, "dcbx", present);
}

/* See the description in tapi_cfg_dcbx.h. */
te_errno
tapi_cfg_dcbx_get(const char *ta, const char *if_name,
                  tapi_dcb_dcbx_state *dcbx)
{
    if (ta == NULL || if_name == NULL || dcbx == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return cfg_get_uint8_sync(dcbx, TAPI_CFG_DCBX_FMT, ta, if_name);
}

/* See the description in tapi_cfg_dcbx.h. */
te_errno
tapi_cfg_dcbx_set(const char *ta, const char *if_name,
                  tapi_dcb_dcbx_state dcbx)
{
    if (ta == NULL || if_name == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return cfg_set_instance_fmt(CFG_VAL(UINT8, dcbx), TAPI_CFG_DCBX_FMT,
                                ta, if_name);
}
