/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Unix TA ETS configuration support
 *
 * Configuration nodes of ETS.
 *
 * This API does not attempt to restrict the user to any specific standard
 * features. The user is free to specify any parameter values they deem
 * necessary.
 */

#define TE_LGR_USER     "Unix Conf ETS"

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
#include "conf_ets.h"

#include <linux/dcbnl.h>

#define TA_OBJ_TYPE_IF_ETS "if_ets"

typedef enum ets_array {
    ETS_ARRAY_TC_BW,
    ETS_ARRAY_PG_BW,
    ETS_ARRAY_TC_TSA,
    ETS_ARRAY_PRIO_TC,
    ETS_ARRAY_RECO_TC_BW,
    ETS_ARRAY_RECO_TC_TSA,
    ETS_ARRAY_RECO_PRIO_TC,
} ets_array;

/** Names of the nodes of the ETS arrays, indexed by ets_array. */
static const char *const ets_array_names[] = {
    [ETS_ARRAY_TC_BW] = "tc_bw",
    [ETS_ARRAY_PG_BW] = "pg_bw",
    [ETS_ARRAY_TC_TSA] = "tc_tsa",
    [ETS_ARRAY_PRIO_TC] = "prio_tc",
    [ETS_ARRAY_RECO_TC_BW] = "reco_tc_bw",
    [ETS_ARRAY_RECO_TC_TSA] = "reco_tc_tsa",
    [ETS_ARRAY_RECO_PRIO_TC] = "reco_prio_tc",
};

/*
 * Read the ETS configuration of an interface. The driver is asked once per
 * configuration group (see ta_unix_conf_dcb_data_get_group()).
 */
static te_errno
ets_get(const ta_conf_ctx *ctx, struct ieee_ets *ets)
{
    te_errno rc;

    if (ets == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    memset(ets, 0, sizeof(*ets));

    rc = ta_unix_conf_dcb_data_get_group(ta_conf_ctx_gid(ctx),
                                         ta_conf_ctx_inst(ctx, "interface"),
                                         TA_UNIX_CONF_DCB_TYPE_ETS, ets);
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
ets_local_value_get(const ta_conf_ctx *ctx, struct ieee_ets **ets_out)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    ta_cfg_obj_t *obj;
    struct ieee_ets *ets;
    te_errno rc;

    if (ets_out == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *ets_out = NULL;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_ETS, if_name, gid);
    if (obj != NULL)
    {
        *ets_out = obj->user_data;
        return 0;
    }

    ets = TE_ALLOC(sizeof(*ets));

    rc = ets_get(ctx, ets);
    if (rc != 0)
    {
        free(ets);
        return rc;
    }

    rc = ta_obj_add(TA_OBJ_TYPE_IF_ETS, if_name, "", gid,
                    ets, &free, &obj);
    if (rc != 0)
    {
        free(ets);
        return TE_RC(TE_TA_UNIX, rc);
    }

    *ets_out = ets;
    return 0;
}

static te_errno
ets_set(const char *if_name, const struct ieee_ets *ets)
{
    return ta_unix_conf_dcb_data_set(if_name, TA_UNIX_CONF_DCB_TYPE_ETS, ets);
}

static uint8_t *
ets_array_get(struct ieee_ets *ets, ets_array array)
{
    switch (array)
    {
        case ETS_ARRAY_TC_BW:
            return ets->tc_tx_bw;

        case ETS_ARRAY_PG_BW:
            return ets->tc_rx_bw;

        case ETS_ARRAY_TC_TSA:
            return ets->tc_tsa;

        case ETS_ARRAY_PRIO_TC:
            return ets->prio_tc;

        case ETS_ARRAY_RECO_TC_BW:
            return ets->tc_reco_bw;

        case ETS_ARRAY_RECO_TC_TSA:
            return ets->tc_reco_tsa;

        case ETS_ARRAY_RECO_PRIO_TC:
            return ets->reco_prio_tc;
    }

    return NULL;
}

/**
 * Get an element of an ETS array.
 *
 * @param[in] ctx       Context of the request.
 * @param[in] array     Array to get the element of; the element index
 *                      is the instance name of its node.
 * @param[out] val      Location for the element.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_EINVAL        Invalid index.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
ets_array_u8_get(const ta_conf_ctx *ctx, ets_array array, uint8_t *val)
{
    struct ieee_ets ets;
    uint8_t *values;
    unsigned int index;
    te_errno rc;

    rc = ta_unix_conf_dcb_index_parse(ta_conf_ctx_inst(ctx,
                                                       ets_array_names[array]),
                                      &index);
    if (rc != 0)
        return rc;

    rc = ets_get(ctx, &ets);
    if (rc != 0)
        return rc;

    values = ets_array_get(&ets, array);
    if (values == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *val = values[index];
    return 0;
}

/**
 * Set an element of an ETS array locally, to be sent on commit.
 *
 * @param[in] ctx       Context of the request.
 * @param[in] array     Array to set the element of; the element index
 *                      is the instance name of its node.
 * @param[in] val       New value of the element.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_EINVAL        Invalid index.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
ets_local_array_u8_set(const ta_conf_ctx *ctx, ets_array array, uint8_t val)
{
    struct ieee_ets *ets;
    uint8_t *values;
    unsigned int index;
    te_errno rc;

    rc = ta_unix_conf_dcb_index_parse(ta_conf_ctx_inst(ctx,
                                                       ets_array_names[array]),
                                      &index);
    if (rc != 0)
        return rc;

    rc = ets_local_value_get(ctx, &ets);
    if (rc != 0)
        return rc;

    values = ets_array_get(ets, array);
    if (values == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    values[index] = val;

    return 0;
}

/**
 * Get the number of Traffic Classes supported by ETS.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the number of supported ETS TCs.
 *
 * @return Status code.
 * @retval 0                  Success.
 * @retval TE_ENOENT          The interface is gone.
 */
static te_errno
ets_cap_get(const ta_conf_ctx *ctx, uint8_t *val)
{
    struct ieee_ets ets;
    te_errno rc;

    rc = ets_get(ctx, &ets);
    if (rc != 0)
        return rc;

    *val = ets.ets_cap;
    return 0;
}

/**
 * Get the Credit-Based Shaper TSA capability.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the ETS CBS.
 *
 * @return Status code.
 * @retval 0                  Success.
 * @retval TE_ENOENT          The interface is gone.
 */
static te_errno
ets_cbs_get(const ta_conf_ctx *ctx, bool *val)
{
    struct ieee_ets ets;
    te_errno rc;

    rc = ets_get(ctx, &ets);
    if (rc != 0)
        return rc;

    *val = (ets.cbs != 0);
    return 0;
}

/**
 * Get the ETS willing state.
 *
 * @param[in] ctx       Context of the request.
 * @param[out] val      Location for the ETS willing state.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
ets_willing_get(const ta_conf_ctx *ctx, bool *val)
{
    struct ieee_ets ets;
    te_errno rc;

    rc = ets_get(ctx, &ets);
    if (rc != 0)
        return rc;

    *val = (ets.willing != 0);
    return 0;
}

/**
 * Set the ETS willing state locally, to be sent on commit.
 *
 * @param[in] ctx       Context of the request.
 * @param[in] val       ETS willing state.
 *
 * @return Status code.
 * @retval 0                Success.
 * @retval TE_ENOENT        The interface is gone.
 */
static te_errno
ets_willing_set(const ta_conf_ctx *ctx, bool val)
{
    struct ieee_ets *ets;
    te_errno rc;

    rc = ets_local_value_get(ctx, &ets);
    if (rc != 0)
        return rc;

    ets->willing = val ? 1 : 0;

    return 0;
}

/**
 * Define the get and set methods of an ETS array node.
 *
 * Elements of the arrays are:
 * - tc_bw: transmit bandwidth of a Traffic Class;
 * - pg_bw: receive bandwidth of a Priority Group;
 * - tc_tsa: TSA of a Traffic Class;
 * - prio_tc: Traffic Class of a priority;
 * - reco_tc_bw, reco_tc_tsa, reco_prio_tc: the same, recommended.
 *
 * Elements are set locally and sent on commit.
 *
 * @param name_     Node name used in the method names.
 * @param array_    Array (ets_array).
 */
#define ETS_ARRAY_METHODS(name_, array_) \
    static te_errno                                                     \
    ets_##name_##_get(const ta_conf_ctx *ctx, uint8_t *val)             \
    {                                                                   \
        return ets_array_u8_get(ctx, array_, val);                      \
    }                                                                   \
                                                                        \
    static te_errno                                                     \
    ets_##name_##_set(const ta_conf_ctx *ctx, uint8_t val)              \
    {                                                                   \
        return ets_local_array_u8_set(ctx, array_, val);                \
    }

ETS_ARRAY_METHODS(tc_bw, ETS_ARRAY_TC_BW)
ETS_ARRAY_METHODS(pg_bw, ETS_ARRAY_PG_BW)
ETS_ARRAY_METHODS(tc_tsa, ETS_ARRAY_TC_TSA)
ETS_ARRAY_METHODS(prio_tc, ETS_ARRAY_PRIO_TC)
ETS_ARRAY_METHODS(reco_tc_bw, ETS_ARRAY_RECO_TC_BW)
ETS_ARRAY_METHODS(reco_tc_tsa, ETS_ARRAY_RECO_TC_TSA)
ETS_ARRAY_METHODS(reco_prio_tc, ETS_ARRAY_RECO_PRIO_TC)

#undef ETS_ARRAY_METHODS

/**
 * Commit ETS structure.
 *
 * @param[in] ctx   Context of the commit.
 *
 * @return Status code.
 */
static te_errno
ets_commit(const ta_conf_ctx *ctx)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    ta_cfg_obj_t *obj;
    te_errno rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_ETS, if_name, ta_conf_ctx_gid(ctx));
    if (obj == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = ets_set(if_name, obj->user_data);

    ta_obj_free(obj);

    return rc;
}

/**
 * List the ETS node: it exists if the driver provides ETS data.
 *
 * @param[in] ctx       Context of the list request.
 * @param[out] names    Vector to append the instance names to.
 *
 * @return Status code.
 */
static te_errno
ets_list(const ta_conf_ctx *ctx, te_vec *names)
{
    return ta_unix_conf_dcb_list(ctx, TA_UNIX_CONF_DCB_TYPE_ETS, names);
}

static const ta_conf_node *const node_ets =
    TA_CONF_NODE((.name = "ets", .list = ets_list, .commit = ets_commit),
        TA_CONF_RO_UINT8("ets_cap", ets_cap_get),
        TA_CONF_RO_BOOL("cbs", ets_cbs_get),
        TA_CONF_RW_BOOL("willing", ets_willing_get, ets_willing_set),
        TA_CONF_RW_COLL_UINT8("tc_bw", ets_tc_bw_get, ets_tc_bw_set,
                              ta_unix_conf_dcb_index_list),
        TA_CONF_RW_COLL_UINT8("pg_bw", ets_pg_bw_get, ets_pg_bw_set,
                              ta_unix_conf_dcb_index_list),
        TA_CONF_RW_COLL_UINT8("tc_tsa", ets_tc_tsa_get, ets_tc_tsa_set,
                              ta_unix_conf_dcb_index_list),
        TA_CONF_RW_COLL_UINT8("prio_tc", ets_prio_tc_get, ets_prio_tc_set,
                              ta_unix_conf_dcb_index_list),
        TA_CONF_RW_COLL_UINT8("reco_tc_bw", ets_reco_tc_bw_get,
                              ets_reco_tc_bw_set,
                              ta_unix_conf_dcb_index_list),
        TA_CONF_RW_COLL_UINT8("reco_tc_tsa", ets_reco_tc_tsa_get,
                              ets_reco_tc_tsa_set,
                              ta_unix_conf_dcb_index_list),
        TA_CONF_RW_COLL_UINT8("reco_prio_tc", ets_reco_prio_tc_get,
                              ets_reco_prio_tc_set,
                              ta_unix_conf_dcb_index_list));

/* See the description in conf_ets.h. */
te_errno
ta_unix_conf_ets_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent/interface/dcb", node_ets);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
}
