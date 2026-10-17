/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Priority-based Flow Control configuration
 *
 * Priority-based Flow Control configuration
 */

#include "conf_api.h"

#include "tapi_cfg_dcb.h"
#include "tapi_cfg_pfc.h"

#define CHECK_RC(exp_)      \
    do {                    \
        te_errno rc;        \
                            \
        rc = (exp_);        \
        if (rc != 0)        \
            return rc;      \
    } while (0)

/* See the description in tapi_cfg_pfc.h. */
te_errno
tapi_cfg_pfc_is_present(const char *ta, const char *if_name, bool *present)
{
    return tapi_cfg_dcb_node_is_present(ta, if_name, "pfc", present);
}

/* See the description in tapi_cfg_pfc.h. */
te_errno
tapi_cfg_pfc_get(const char *ta, const char *if_name, tapi_dcb_pfc_state *pfc)
{
    struct tapi_dcb_pfc_state p;
    uint8_t pfc_en;
    uint8_t tc;

    if (ta == NULL || if_name == NULL || pfc == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(cfg_synchronize_fmt(true, TAPI_CFG_PFC_FMT, ta, if_name));

    CHECK_RC(cfg_get_uint8(&p.pfc_cap, TAPI_CFG_PFC_PARAM_FMT,
                           ta, if_name, "pfc_cap"));
    CHECK_RC(cfg_get_uint8(&pfc_en, TAPI_CFG_PFC_PARAM_FMT,
                           ta, if_name, "pfc_en"));
    CHECK_RC(cfg_get_bool(&p.config.mbc, TAPI_CFG_PFC_PARAM_FMT,
                          ta, if_name, "macsec_bypass"));
    CHECK_RC(cfg_get_uint16(&p.config.delay, TAPI_CFG_PFC_PARAM_FMT,
                            ta, if_name, "delay"));

    for (tc = 0; tc < TAPI_DCB_NUM_PRIORITIES; tc++)
    {
        p.config.pfc_en[tc] = pfc_en & (1U << tc);

        CHECK_RC(cfg_get_uint64(&p.requests[tc],
                                TAPI_CFG_PFC_PARAM_ID_FMT,
                                ta, if_name, "requests", tc));
        CHECK_RC(cfg_get_uint64(&p.indications[tc],
                                TAPI_CFG_PFC_PARAM_ID_FMT,
                                ta, if_name, "indications", tc));
    }

    *pfc = p;

    return 0;
}

/* See the description in tapi_cfg_pfc.h. */
te_errno
tapi_cfg_pfc_set(const char *ta, const char *if_name,
                 const tapi_dcb_pfc_config *pfc)
{
    uint8_t pfc_en = 0;
    uint8_t pr;

    if (ta == NULL || if_name == NULL || pfc == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    for (pr = 0; pr < TAPI_DCB_NUM_PRIORITIES; pr++)
        pfc_en |= ((1U << pr) * pfc->pfc_en[pr]);

    CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(UINT8, pfc_en),
                                        TAPI_CFG_PFC_PARAM_FMT,
                                        ta, if_name, "pfc_en"));
    CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(BOOL, pfc->mbc),
                                        TAPI_CFG_PFC_PARAM_FMT,
                                        ta, if_name, "macsec_bypass"));
    CHECK_RC(cfg_set_instance_local_fmt(CFG_VAL(UINT16, pfc->delay),
                                        TAPI_CFG_PFC_PARAM_FMT,
                                        ta, if_name, "delay"));

    return cfg_commit_fmt(TAPI_CFG_PFC_FMT, ta, if_name);
}
