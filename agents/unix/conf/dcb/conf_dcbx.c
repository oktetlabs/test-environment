/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Unix TA DCBX configuration support
 *
 * Configuration nodes of the DCBX mode.
 *
 * This API does not attempt to restrict the user to any specific standard
 * features. The user is free to specify any parameter values they deem
 * necessary.
 */

#define TE_LGR_USER     "Unix Conf DCBX"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "te_str.h"

#include "rcf_common.h"
#include "rcf_pch_tree.h"

#include "logger_api.h"

#include "conf_dcb.h"
#include "conf_dcbx.h"

#include <linux/dcbnl.h>

/**
 * Retrieve the DCBX mode bitmask from an interface.
 *
 * The mode is read from the driver once per configuration group.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the DCBX mode.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_EINVAL        An argument is invalid.
 * @retval TE_ENOTCONN      Netlink socket is unavailable.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
dcbx_get(const ta_conf_ctx *ctx, uint8_t *val)
{
    te_errno rc;

    rc = ta_unix_conf_dcb_data_get_group(ta_conf_ctx_gid(ctx),
                                         ta_conf_ctx_inst(ctx, "interface"),
                                         TA_UNIX_CONF_DCB_TYPE_DCBX, val);
    if (rc != 0)
    {
        /* The interface is gone, so is the instance. */
        if (TE_RC_GET_ERROR(rc) == TE_ENODEV)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        return rc;
    }

    return 0;
}

/**
 * Configure the DCBX mode bitmask for an interface.
 *
 * Setting the mode that the driver already reports succeeds without
 * a request to the driver.
 *
 * @param[in] ctx       Context of the request.
 * @param[in] dcbx      DCBX mode to set.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_EINVAL        An argument is invalid.
 * @retval TE_ENOTCONN      Netlink socket is unavailable.
 * @retval Other            Error reported by the kernel or by the driver.
 */
static te_errno
dcbx_set(const ta_conf_ctx *ctx, uint8_t dcbx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    uint8_t current;

    /*
     * Some drivers fail a request that does not change the mode (ice
     * returns 1), and a mode reported by the driver may be impossible to
     * request (mlx5 firmware mode). Rollback and tests set the current
     * mode to restore it, so do not ask the driver. If the mode cannot be
     * read, send the request and let it report the error.
     */
    if (ta_unix_conf_dcb_data_get(if_name, TA_UNIX_CONF_DCB_TYPE_DCBX,
                                  &current) == 0 && current == dcbx)
    {
        VERB("%s: DCBX mode %" TE_PRINTF_8 "u is already set, skip",
             if_name, dcbx);
        return 0;
    }

    return ta_unix_conf_dcb_data_set(if_name, TA_UNIX_CONF_DCB_TYPE_DCBX,
                                     &dcbx);
}

/**
 * List the DCBX node: it exists if the driver provides the DCBX mode.
 *
 * @param[in] ctx       Context of the list request.
 * @param[out] names    Vector to append the instance names to.
 *
 * @return Status code.
 */
static te_errno
dcbx_list(const ta_conf_ctx *ctx, te_vec *names)
{
    return ta_unix_conf_dcb_list(ctx, TA_UNIX_CONF_DCB_TYPE_DCBX, names);
}

static const ta_conf_node *const node_dcbx =
    TA_CONF_RW_COLL_UINT8("dcbx", dcbx_get, dcbx_set, dcbx_list);

/* See the description in conf_dcbx.h. */
te_errno
ta_unix_conf_dcbx_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent/interface/dcb", node_dcbx);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
}
