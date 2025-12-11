/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RCF interface via CS
 */

#define TE_LGR_USER     "Conf RCF TAPI"

#include "te_config.h"
#include "te_errno.h"
#include "logger_api.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "tapi_cfg_rcf.h"

te_errno
tapi_cfg_rcf_add_ta(const char *ta, const char *type, const char *rcflib,
                    const te_kvpair_h *conf, unsigned int flags)
{
    te_kvpair *p;
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, type),
                              "/rcf:/agent:%s", ta);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, rcflib),
                              "/rcf:/agent:%s/rcflib:", ta);
    if (rc != 0)
        goto fail;

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, !(flags & RCF_TA_NO_SYNC_TIME)),
                              "/rcf:/agent:%s/synch_time:", ta);
    if (rc != 0)
        goto fail;

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, !!(flags & RCF_TA_REBOOTABLE)),
                              "/rcf:/agent:%s/rebootable:", ta);
    if (rc != 0)
        goto fail;

    TAILQ_FOREACH(p, conf, links)
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, p->value),
                                  "/rcf:/agent:%s/conf:%s", ta, p->key);
        if (rc != 0)
            goto fail;
    }

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, 1),
                              "/rcf:/agent:%s/status:", ta);
    if (rc != 0)
    {
        ERROR("Failed to start TA '%s': %r", ta, rc);
        goto fail;
    }

    return 0;

fail:
    (void)tapi_cfg_rcf_del_ta(ta);
    return rc;
}

te_errno
tapi_cfg_rcf_del_ta(const char *ta)
{
    return cfg_del_instance_fmt(true, "/rcf:/agent:%s", ta);
}
