/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Unix TA PFC configuration support
 *
 * Configuration nodes of PFC.
 *
 * This API does not attempt to restrict the user to any specific standard
 * features. The user is free to specify any parameter values they deem
 * necessary.
 */

#define TE_LGR_USER     "Unix Conf PFC"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "te_alloc.h"
#include "te_str.h"

#include "rcf_common.h"
#include "rcf_pch_tree.h"
#include "rcf_pch_ta_cfg.h"

#include "conf_dcb.h"
#include "conf_pfc.h"

#include <linux/dcbnl.h>

#define TA_OBJ_TYPE_IF_PFC "if_pfc"

/*
 * Read the PFC configuration and frame counters of an interface. The
 * driver is asked once per configuration group (see
 * ta_unix_conf_dcb_data_get_group()), so the counters of all priorities
 * read in a group come from one snapshot.
 */
static te_errno
pfc_get(const ta_conf_ctx *ctx, struct ieee_pfc *pfc)
{
    te_errno rc;

    if (pfc == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    memset(pfc, 0, sizeof(*pfc));

    rc = ta_unix_conf_dcb_data_get_group(ta_conf_ctx_gid(ctx),
                                         ta_conf_ctx_inst(ctx, "interface"),
                                         TA_UNIX_CONF_DCB_TYPE_PFC, pfc);
    if (rc != 0)
    {
        /* The interface is gone, so is the instance. */
        if (TE_RC_GET_ERROR(rc) == TE_ENODEV)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        return rc;
    }

    return 0;
}

static te_errno
pfc_set(const char *if_name, const struct ieee_pfc *pfc)
{
    return ta_unix_conf_dcb_data_set(if_name, TA_UNIX_CONF_DCB_TYPE_PFC, pfc);
}

static te_errno
pfc_local_value_get(const ta_conf_ctx *ctx, struct ieee_pfc **pfc_out)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    ta_cfg_obj_t *obj;
    struct ieee_pfc *pfc;
    te_errno rc;

    if (pfc_out == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *pfc_out = NULL;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_PFC, if_name, gid);
    if (obj != NULL)
    {
        *pfc_out = obj->user_data;
        return 0;
    }

    pfc = TE_ALLOC(sizeof(*pfc));

    rc = pfc_get(ctx, pfc);
    if (rc != 0)
    {
        free(pfc);
        return rc;
    }

    rc = ta_obj_add(TA_OBJ_TYPE_IF_PFC, if_name, "", gid,
                    pfc, &free, &obj);
    if (rc != 0)
    {
        free(pfc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    *pfc_out = pfc;
    return 0;
}

/**
 * Get a PFC frame counter of a priority.
 *
 * @param[in] ctx       Context of the request; the priority is the
 *                      instance name of the counter node.
 * @param[in] requests  Get the counter of transmitted frames (node
 *                      @c requests) rather than of received ones (node
 *                      @c indications).
 * @param[out] val      Location for the counter.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_EINVAL        Invalid priority instance.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_counter_get(const ta_conf_ctx *ctx, bool requests, uint64_t *val)
{
    struct ieee_pfc pfc;
    unsigned int prio;
    te_errno rc;

    rc = ta_unix_conf_dcb_index_parse(
             ta_conf_ctx_inst(ctx, requests ? "requests" : "indications"),
             &prio);
    if (rc != 0)
        return rc;

    rc = pfc_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    *val = requests ? pfc.requests[prio] : pfc.indications[prio];

    return 0;
}

/**
 * Get the number of received PFC frames for Priority.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the received PFC frame count.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_EINVAL        Invalid Priority instance.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_indications_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    return pfc_counter_get(ctx, false, val);
}

/**
 * Get the number of transmitted PFC frames for Priority.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the transmitted PFC frame count.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_EINVAL        Invalid priority instance.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_requests_get(const ta_conf_ctx *ctx, uint64_t *val)
{
    return pfc_counter_get(ctx, true, val);
}

/**
 * Get the bitmap of priorities for which PFC is enabled.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the PFC-enabled priority bitmap.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_en_get(const ta_conf_ctx *ctx, uint8_t *val)
{
    struct ieee_pfc pfc;
    te_errno rc;

    rc = pfc_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    *val = pfc.pfc_en;
    return 0;
}

/**
 * Set the bitmap of priorities for which PFC is enabled.
 *
 * @param[in] ctx       Context of the request.
 * @param[in] val       Required PFC-enabled priority bitmap.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_en_set(const ta_conf_ctx *ctx, uint8_t val)
{
    struct ieee_pfc *pfc;
    te_errno rc;

    rc = pfc_local_value_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    pfc->pfc_en = val;

    return 0;
}

/**
 * Get the PFC delay value.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the PFC delay value.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_delay_get(const ta_conf_ctx *ctx, uint16_t *val)
{
    struct ieee_pfc pfc;
    te_errno rc;

    rc = pfc_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    *val = pfc.delay;
    return 0;
}

/**
 * Set the PFC delay value.
 *
 * @param[in] ctx       Context of the request.
 * @param[in] val       PFC delay value.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_delay_set(const ta_conf_ctx *ctx, uint16_t val)
{
    struct ieee_pfc *pfc;
    te_errno rc;

    rc = pfc_local_value_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    pfc->delay = val;

    return 0;
}

/**
 * Get the status of a PFC MACsec Bypass.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the PFC MACsec Bypass status.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_macsec_bypass_get(const ta_conf_ctx *ctx, bool *val)
{
    struct ieee_pfc pfc;
    te_errno rc;

    rc = pfc_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    *val = (pfc.mbc != 0);
    return 0;
}

/**
 * Set the status of a PFC MACsec Bypass.
 *
 * @param[in] ctx       Context of the request.
 * @param[in] val       PFC MACsec Bypass status.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_macsec_bypass_set(const ta_conf_ctx *ctx, bool val)
{
    struct ieee_pfc *pfc;
    te_errno rc;

    rc = pfc_local_value_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    pfc->mbc = val ? 1 : 0;

    return 0;
}

/**
 * Get the number of Traffic Classes that may simultaneously have PFC enabled.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the number of PFC TCs that may
 *                      simultaneously have PFC enabled.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
pfc_cap_get(const ta_conf_ctx *ctx, uint8_t *val)
{
    struct ieee_pfc pfc;
    te_errno rc;

    rc = pfc_get(ctx, &pfc);
    if (rc != 0)
        return rc;

    *val = pfc.pfc_cap;
    return 0;
}

/**
 * Commit PFC structure.
 *
 * @param[in] ctx   Context of the commit.
 *
 * @return Status code.
 */
static te_errno
pfc_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    struct ieee_pfc *pfc;
    ta_cfg_obj_t *obj;
    te_errno rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_PFC, if_name, ta_conf_ctx_gid(ctx));
    if (obj == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    pfc = obj->user_data;

    /*
     * struct ieee_pfc is used both to get and to set PFC, and the kernel
     * passes the whole structure to the driver. The frame counters are
     * filled by the driver on get and are not configuration, so drivers
     * ignore them on set. They were read when the group started: clear
     * them, so that the request does not carry a stale snapshot of
     * statistics.
     */
    memset(pfc->requests, 0, sizeof(pfc->requests));
    memset(pfc->indications, 0, sizeof(pfc->indications));

    rc = pfc_set(if_name, pfc);

    ta_obj_free(obj);
    return rc;
}

/**
 * List the PFC node: it exists if the driver provides PFC data.
 *
 * @param[in] ctx       Context of the list request.
 * @param[out] names    Vector to append the instance names to.
 *
 * @return Status code.
 */
static te_errno
pfc_list(const ta_conf_ctx *ctx, te_vec *names)
{
    return ta_unix_conf_dcb_list(ctx, TA_UNIX_CONF_DCB_TYPE_PFC, names);
}

static const ta_conf_node *const node_pfc =
    TA_CONF_NODE((.name = "pfc", .list = pfc_list, .commit = pfc_commit),
        TA_CONF_RO_UINT8("pfc_cap", pfc_cap_get),
        TA_CONF_RW_BOOL("macsec_bypass", pfc_macsec_bypass_get,
                        pfc_macsec_bypass_set),
        TA_CONF_RW_UINT16("delay", pfc_delay_get, pfc_delay_set),
        TA_CONF_RW_UINT8("pfc_en", pfc_en_get, pfc_en_set),
        TA_CONF_RO_COLL_UINT64("requests", pfc_requests_get,
                               ta_unix_conf_dcb_index_list),
        TA_CONF_RO_COLL_UINT64("indications", pfc_indications_get,
                               ta_unix_conf_dcb_index_list));

/* See the description in conf_pfc.h. */
te_errno
ta_unix_conf_pfc_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent/interface/dcb", node_pfc);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
}
