/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Resource limits support
 *
 * Unix TA resource limits configuration
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Resource Limits"

#include "te_config.h"
#include "config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "rcf_pch_tree.h"

/**
 * RLIMIT value selector
 */
typedef enum rlimit_val_sel {
    RLIMIT_VAL_CUR, /**< Use current value */
    RLIMIT_VAL_MAX  /**< Use maximum value */
} rlimit_val_sel;

/**
 * Get value of a resource limit (as reported by getrlimit()).
 *
 * @param value     Where to save value
 * @param resource  Which resource limit to get
 * @param val_sel   Which value to get (@ref rlimit_val_sel)
 *
 * @return Status code.
 */
static te_errno
rlimit_uint_get(uintmax_t *value, int resource, rlimit_val_sel val_sel)
{
    struct rlimit       rlim = { 0 };
    te_errno            rc;

    if (getrlimit(resource, &rlim) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): getrlimit() failed with errno %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    *value = (val_sel == RLIMIT_VAL_CUR) ? rlim.rlim_cur : rlim.rlim_max;

    return 0;
}

/**
 * Set value of a resource limit (with setrlimit()).
 *
 * @param value     Value to set
 * @param resource  Which resource limit to set
 * @param val_sel   Which value to set (@ref rlimit_val_sel)
 *
 * @return Status code.
 */
static te_errno
rlimit_uint_set(uintmax_t value, int resource, rlimit_val_sel val_sel)
{
    struct rlimit        rlim = { 0 };
    te_errno             rc;

    if (getrlimit(resource, &rlim) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): getrlimit() failed with errno %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (val_sel == RLIMIT_VAL_CUR)
        rlim.rlim_cur = value;
    else
        rlim.rlim_max = value;

    if (val_sel == RLIMIT_VAL_CUR)
    {
        if (rlim.rlim_max < rlim.rlim_cur)
            rlim.rlim_max = rlim.rlim_cur;
    }
    else
    {
        if (rlim.rlim_cur > rlim.rlim_max)
            rlim.rlim_cur = rlim.rlim_max;
    }

    if (setrlimit(resource, &rlim) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): setrlimit() failed with errno %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

static te_errno
rlimit_nofile_cur_get(const ta_conf_ctx *ctx, uint64_t *value)
{
    UNUSED(ctx);
    return rlimit_uint_get(value, RLIMIT_NOFILE, RLIMIT_VAL_CUR);
}

static te_errno
rlimit_nofile_cur_set(const ta_conf_ctx *ctx, uint64_t value)
{
    UNUSED(ctx);
    return rlimit_uint_set(value, RLIMIT_NOFILE, RLIMIT_VAL_CUR);
}

static te_errno
rlimit_nofile_max_get(const ta_conf_ctx *ctx, uint64_t *value)
{
    UNUSED(ctx);
    return rlimit_uint_get(value, RLIMIT_NOFILE, RLIMIT_VAL_MAX);
}

static te_errno
rlimit_nofile_max_set(const ta_conf_ctx *ctx, uint64_t value)
{
    UNUSED(ctx);
    return rlimit_uint_set(value, RLIMIT_NOFILE, RLIMIT_VAL_MAX);
}

static te_errno
rlimit_memlock_cur_get(const ta_conf_ctx *ctx, uint64_t *value)
{
    UNUSED(ctx);
    return rlimit_uint_get(value, RLIMIT_MEMLOCK, RLIMIT_VAL_CUR);
}

static te_errno
rlimit_memlock_cur_set(const ta_conf_ctx *ctx, uint64_t value)
{
    UNUSED(ctx);
    return rlimit_uint_set(value, RLIMIT_MEMLOCK, RLIMIT_VAL_CUR);
}

static te_errno
rlimit_memlock_max_get(const ta_conf_ctx *ctx, uint64_t *value)
{
    UNUSED(ctx);
    return rlimit_uint_get(value, RLIMIT_MEMLOCK, RLIMIT_VAL_MAX);
}

static te_errno
rlimit_memlock_max_set(const ta_conf_ctx *ctx, uint64_t value)
{
    UNUSED(ctx);
    return rlimit_uint_set(value, RLIMIT_MEMLOCK, RLIMIT_VAL_MAX);
}

static const ta_conf_node *const node_rlimits =
    TA_CONF_NA("rlimits",
        TA_CONF_NA("nofile",
            TA_CONF_RW_UINT64("cur", rlimit_nofile_cur_get,
                            rlimit_nofile_cur_set),
            TA_CONF_RW_UINT64("max", rlimit_nofile_max_get,
                            rlimit_nofile_max_set)),
        TA_CONF_NA("memlock",
            TA_CONF_RW_UINT64("cur", rlimit_memlock_cur_get,
                            rlimit_memlock_cur_set),
            TA_CONF_RW_UINT64("max", rlimit_memlock_max_get,
                            rlimit_memlock_max_set)));

/**
 * Add resource limits objects to configuration tree.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_rlimits_init(void)
{
    return ta_conf_register("/agent", node_rlimits);
}
