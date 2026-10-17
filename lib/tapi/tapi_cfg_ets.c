/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Enhanced Transmission Selection configuration
 *
 * Enhanced Transmission Selection configuration.
 */

#include "conf_api.h"

#include "tapi_cfg_dcb.h"
#include "tapi_cfg_ets.h"

#define CHECK_RC(exp_)      \
    do {                    \
        te_errno rc;        \
                            \
        rc = (exp_);        \
        if (rc != 0)        \
            return rc;      \
    } while (0)

/**
 * Get all values of an indexed ETS parameter.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[in] param     ETS parameter name.
 * @param[out] values   Retrieved parameter values.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_ets_array_get(const char *ta, const char *if_name, const char *param,
                       uint8_t values[TAPI_DCB_NUM_TCS_MAX])
{
    uint8_t i;

    if (ta == NULL || if_name == NULL || param == NULL || values == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
        CHECK_RC(cfg_get_uint8(&values[i], TAPI_CFG_ETS_PARAM_ID_FMT,
                               ta, if_name, param, i));

    return 0;
}

/**
 * Set all values of an indexed ETS parameter locally.
 *
 * @param[in] ta         Test Agent.
 * @param[in] if_name    Interface name.
 * @param[in] param      ETS parameter name.
 * @param[in] values     Parameter values to set.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_ets_array_local_set(const char *ta, const char *if_name,
                             const char *param,
                             const uint8_t values[TAPI_DCB_NUM_TCS_MAX])
{
    uint8_t i;

    if (ta == NULL || if_name == NULL || param == NULL || values == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
        CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(UINT8, values[i]),
                                            TAPI_CFG_ETS_PARAM_ID_FMT,
                                            ta, if_name, param, i));

    return 0;
}

/* See the description in tapi_cfg_ets.h. */
te_errno
tapi_cfg_ets_is_present(const char *ta, const char *if_name, bool *present)
{
    return tapi_cfg_dcb_node_is_present(ta, if_name, "ets", present);
}

/* See the description in tapi_cfg_ets.h. */
te_errno
tapi_cfg_ets_get(const char *ta, const char *if_name, tapi_dcb_ets_state *ets)
{
    struct tapi_dcb_ets_state e;

    if (ta == NULL || if_name == NULL || ets == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(cfg_synchronize_fmt(true, TAPI_CFG_ETS_FMT, ta, if_name));

    CHECK_RC(cfg_get_bool(&e.config.willing, TAPI_CFG_ETS_PARAM_FMT,
                          ta, if_name, "willing"));
    CHECK_RC(cfg_get_uint8(&e.ets_cap, TAPI_CFG_ETS_PARAM_FMT,
                           ta, if_name, "ets_cap"));
    CHECK_RC(cfg_get_bool(&e.cbs, TAPI_CFG_ETS_PARAM_FMT,
                          ta, if_name, "cbs"));
    CHECK_RC(tapi_cfg_ets_array_get(ta, if_name, "tc_bw",
                                    e.config.tc_bw));
    CHECK_RC(tapi_cfg_ets_array_get(ta, if_name, "pg_bw",
                                    e.config.pg_bw));
    CHECK_RC(tapi_cfg_ets_array_get(ta, if_name, "tc_tsa",
                                    e.config.tc_tsa));
    CHECK_RC(tapi_cfg_ets_array_get(ta, if_name, "prio_tc",
                                    e.config.prio_tc));
    CHECK_RC(tapi_cfg_ets_array_get(ta, if_name, "reco_tc_bw",
                                    e.config.reco_tc_bw));
    CHECK_RC(tapi_cfg_ets_array_get(ta, if_name, "reco_tc_tsa",
                                    e.config.reco_tc_tsa));
    CHECK_RC(tapi_cfg_ets_array_get(ta, if_name, "reco_prio_tc",
                                    e.config.reco_prio_tc));

    *ets = e;

    return 0;
}

/* See the description in tapi_cfg_ets.h. */
te_errno
tapi_cfg_ets_set(const char *ta, const char *if_name,
                 const tapi_dcb_ets_config *ets)
{
    if (ta == NULL || if_name == NULL || ets == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(BOOL, ets->willing),
                                        TAPI_CFG_ETS_PARAM_FMT,
                                        ta, if_name, "willing"));
    CHECK_RC(tapi_cfg_ets_array_local_set(ta, if_name, "tc_bw",
                                          ets->tc_bw));
    CHECK_RC(tapi_cfg_ets_array_local_set(ta, if_name, "pg_bw",
                                          ets->pg_bw));
    CHECK_RC(tapi_cfg_ets_array_local_set(ta, if_name, "tc_tsa",
                                          ets->tc_tsa));
    CHECK_RC(tapi_cfg_ets_array_local_set(ta, if_name, "prio_tc",
                                          ets->prio_tc));
    CHECK_RC(tapi_cfg_ets_array_local_set(ta, if_name, "reco_tc_bw",
                                          ets->reco_tc_bw));
    CHECK_RC(tapi_cfg_ets_array_local_set(ta, if_name, "reco_tc_tsa",
                                          ets->reco_tc_tsa));
    CHECK_RC(tapi_cfg_ets_array_local_set(ta, if_name, "reco_prio_tc",
                                          ets->reco_prio_tc));

    return cfg_commit_fmt(TAPI_CFG_ETS_FMT, ta, if_name);
}
