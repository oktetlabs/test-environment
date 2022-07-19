/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Block devices management
 *
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Block"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <unistd.h>

#if HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#endif

#include<string.h>

#include "te_stdint.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "unix_internal.h"
#include "conf_common.h"

static te_bool
ta_block_is_mine(const char *block_name, void *data)
{
    UNUSED(data);
    return rcf_pch_rsrc_accessible("/agent:%s/block:%s", ta_name, block_name);
}

static te_errno
block_dev_list(unsigned int gid, const char *oid,
               const char *sub_id, char **list)
{
    char buf[4096];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#ifdef __linux__
    {
        te_errno rc;
        rc = get_dir_list("/sys/block", buf, sizeof(buf), FALSE,
                          &ta_block_is_mine, NULL);
        if (rc != 0)
            return rc;
    }
#else
    UNUSED(list);
    ERROR("%s(): getting list of block devices "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

#if HAVE_LINUX_MAJOR_H
static te_errno
check_block_loop(const char *block_name)
{
    te_errno rc = 0;
    char buf[64];

    rc = read_sys_value(buf, sizeof(buf), FALSE, "/sys/block/%s/dev",
                        block_name);
    if (rc != 0)
    {
        /* if we cannot read device numbers, assume it's not a loop device */
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            rc = TE_RC(TE_TA_UNIX, TE_ENOTBLK);
    }
    else
    {
        unsigned major, minor;

        if (sscanf(buf, "%u:%u", &major, &minor) != 2)
        {
            ERROR("Invalid contents of /sys/block/%s/dev: %s", block_name,
                  buf);
            return TE_RC(TE_TA_UNIX, TE_EBADMSG);
        }
        UNUSED(minor);

        if (major != LOOP_MAJOR)
            rc = TE_RC(TE_TA_UNIX, TE_ENOTBLK);
    }

    return rc;
}
#endif

static te_errno
block_dev_loop_get(unsigned int gid, const char *oid, char *value,
                   const char *block_name)
{
    te_errno rc = TE_RC(TE_TA_UNIX, TE_ENOTBLK);

    if (!ta_block_is_mine(block_name, NULL))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

#if HAVE_LINUX_MAJOR_H
    rc  = check_block_loop(block_name);
#else
    UNUSED(block_name);
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (TE_RC_GET_ERROR(rc) == TE_ENOTBLK)
    {
        strcpy(value, "0");
        rc = 0;
    }
    else if (rc == 0)
    {
        strcpy(value, "1");
    }

    return rc;
}

RCF_PCH_CFG_NODE_RO(node_block_dev_loop, "loop", NULL, NULL,
                    block_dev_loop_get);

RCF_PCH_CFG_NODE_COLLECTION(node_block_dev, "block",
                            &node_block_dev_loop, NULL,
                            NULL, NULL, block_dev_list, NULL);

te_errno
ta_unix_conf_block_dev_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent", &node_block_dev);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/block",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
}
