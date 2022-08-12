/** @file
 * @brief Resource limits support
 *
 * Unix TA resource limits configuration
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#define TE_LGR_USER     "Conf Resource Limits"

#include "te_config.h"
#include "config.h"

#include <stdio.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_str.h"
#include "logger_api.h"
#include "rcf_common.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

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
rlimit_get(char *value, int resource, rlimit_val_sel val_sel)
{
    struct rlimit       rlim = { 0 };
    te_errno            rc;
    int                 ret;
    long unsigned int   lim;

    if (getrlimit(resource, &rlim) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): getrlimit() failed with errno %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (val_sel == RLIMIT_VAL_CUR)
        lim = rlim.rlim_cur;
    else
        lim = rlim.rlim_max;

    ret = snprintf(value, RCF_MAX_VAL, "%lu", lim);
    if (ret < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): snprintf() failed with errno %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }
    else if (ret >= RCF_MAX_VAL)
    {
        ERROR("%s(): not enough space to store value", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
    }

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
rlimit_set(const char *value, int resource, rlimit_val_sel val_sel)
{
    struct rlimit        rlim = { 0 };
    long unsigned int    num_value;
    te_errno             rc;

    rc = te_strtoul(value, 10, &num_value);
    if (rc != 0)
    {
        ERROR("%s(): failed to parse value '%s'", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (getrlimit(resource, &rlim) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): getrlimit() failed with errno %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (val_sel == RLIMIT_VAL_CUR)
        rlim.rlim_cur = num_value;
    else
        rlim.rlim_max = num_value;

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

/**
 * Obtain current value of RLIMIT_NOFILE resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_nofile_cur_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_get(value, RLIMIT_NOFILE, RLIMIT_VAL_CUR);
}

/**
 * Set current value of RLIMIT_NOFILE resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_nofile_cur_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_set(value, RLIMIT_NOFILE, RLIMIT_VAL_CUR);
}

/**
 * Obtain maximum value of RLIMIT_NOFILE resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_nofile_max_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_get(value, RLIMIT_NOFILE, RLIMIT_VAL_MAX);
}

/**
 * Set maximum value of RLIMIT_NOFILE resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_nofile_max_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_set(value, RLIMIT_NOFILE, RLIMIT_VAL_MAX);
}


/**
 * Obtain current value of RLIMIT_MEMLOCK resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_memlock_cur_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_get(value, RLIMIT_MEMLOCK, RLIMIT_VAL_CUR);
}

/**
 * Set current value of RLIMIT_MEMLOCK resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_memlock_cur_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_set(value, RLIMIT_MEMLOCK, RLIMIT_VAL_CUR);
}

/**
 * Obtain maximum value of RLIMIT_MEMLOCK resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_memlock_max_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_get(value, RLIMIT_MEMLOCK, RLIMIT_VAL_MAX);
}

/**
 * Set maximum value of RLIMIT_MEMLOCK resource limit.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full object instance identifier (unused)
 * @param value         Value location
 *
 * @return              Status code.
 */
static te_errno
rlimit_memlock_max_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return rlimit_set(value, RLIMIT_MEMLOCK, RLIMIT_VAL_MAX);
}

RCF_PCH_CFG_NODE_RW(node_rlimit_memlock_max, "max",
                    NULL, NULL,
                    rlimit_memlock_max_get, rlimit_memlock_max_set);
RCF_PCH_CFG_NODE_RW(node_rlimit_memlock_cur, "cur",
                    NULL, &node_rlimit_memlock_max,
                    rlimit_memlock_cur_get, rlimit_memlock_cur_set);
RCF_PCH_CFG_NODE_NA(node_rlimit_memlock, "memlock",
                    &node_rlimit_memlock_cur, NULL);

RCF_PCH_CFG_NODE_RW(node_rlimit_nofile_max, "max",
                    NULL, NULL,
                    rlimit_nofile_max_get, rlimit_nofile_max_set);
RCF_PCH_CFG_NODE_RW(node_rlimit_nofile_cur, "cur",
                    NULL, &node_rlimit_nofile_max,
                    rlimit_nofile_cur_get, rlimit_nofile_cur_set);

RCF_PCH_CFG_NODE_NA(node_rlimit_nofile, "nofile",
                    &node_rlimit_nofile_cur, &node_rlimit_memlock);
RCF_PCH_CFG_NODE_NA(node_rlimits, "rlimits", &node_rlimit_nofile,
                    NULL);

/**
 * Add resource limits objects to configuration tree.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_rlimits_init(void)
{
    return rcf_pch_add_node("/agent", &node_rlimits);
}
