/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Data Center Bridging configuration common helpers
 *
 * Helpers shared by the DCBX, ETS and PFC configuration TAPIs.
 */

#include "conf_api.h"

#include "tapi_cfg_dcb.h"

/* See the description in tapi_cfg_dcb.h. */
te_errno
tapi_cfg_dcb_node_is_present(const char *ta, const char *if_name,
                             const char *node, bool *present)
{
    cfg_handle handle;
    te_errno rc;

    if (ta == NULL || if_name == NULL || node == NULL || present == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    *present = false;

    rc = cfg_find_fmt(&handle, TAPI_CFG_DCB_FMT, ta, if_name);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        return 0;
    if (rc != 0)
        return rc;

    rc = cfg_synchronize_fmt(true, TAPI_CFG_DCB_FMT, ta, if_name);
    if (rc != 0)
        return rc;

    rc = cfg_find_fmt(&handle, TAPI_CFG_DCB_FMT "/%s:", ta, if_name, node);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        return 0;
    if (rc != 0)
        return rc;

    *present = true;

    return 0;
}
