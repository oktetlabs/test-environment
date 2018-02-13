/** @file
 * @brief Resource limits support
 *
 * Unix TA resource limits configuration
 *
 *
 * Copyright (C) 2018 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
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
 * Get value of a resource limit (as reported by getrlimit()).
 *
 * @param value     Where to save value
 * @param resource  Which resource limit to get
 * @param current   If @c TRUE, get current value, otherwise
 *                  get maximum value
 *
 * @return Status code.
 */
static te_errno
rlimit_get(char *value, int resource, te_bool current)
{
    struct rlimit       rlim = { 0 };
    te_errno            rc;
    int                 ret;
    long unsigned int   lim;

    if (getrlimit(resource, &rlim) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): getrlimit() failed with errno %r", __FUNCTION__, rc);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    if (current)
        lim = rlim.rlim_cur;
    else
        lim = rlim.rlim_max;

    ret = snprintf(value, RCF_MAX_VAL, "%lu", lim);
    if (ret < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): snprintf() failed with errno %r", __FUNCTION__, rc);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    else if (ret >= RCF_MAX_VAL)
    {
        ERROR("%s(): not enough space to store value", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, TE_ESMALLBUF);
    }

    return 0;
}

/**
 * Set value of a resource limit (with setrlimit()).
 *
 * @param value     Value to set
 * @param resource  Which resource limit to set
 * @param current   If @c TRUE, set current value, otherwise
 *                  set maximum value
 *
 * @return Status code.
 */
static te_errno
rlimit_set(const char *value, int resource, te_bool current)
{
    struct rlimit        rlim = { 0 };
    long unsigned int    num_value;
    te_errno             rc;

    rc = te_strtoul(value, 10, &num_value);
    if (rc != 0)
    {
        ERROR("%s(): failed to parse value '%s'", __FUNCTION__, value);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    if (getrlimit(resource, &rlim) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): getrlimit() failed with errno %r", __FUNCTION__, rc);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    if (current)
        rlim.rlim_cur = num_value;
    else
        rlim.rlim_max = num_value;

    if (current)
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
        return TE_OS_RC(TE_TA_UNIX, rc);
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

    return rlimit_get(value, RLIMIT_NOFILE, TRUE);
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

    return rlimit_set(value, RLIMIT_NOFILE, TRUE);
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

    return rlimit_get(value, RLIMIT_NOFILE, FALSE);
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

    return rlimit_set(value, RLIMIT_NOFILE, FALSE);
}

RCF_PCH_CFG_NODE_RW(node_rlimit_nofile_max, "max",
                    NULL, NULL,
                    rlimit_nofile_max_get, rlimit_nofile_max_set);
RCF_PCH_CFG_NODE_RW(node_rlimit_nofile_cur, "cur",
                    NULL, &node_rlimit_nofile_max,
                    rlimit_nofile_cur_get, rlimit_nofile_cur_set);

RCF_PCH_CFG_NODE_NA(node_rlimit_nofile, "nofile",
                    &node_rlimit_nofile_cur, NULL);
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
