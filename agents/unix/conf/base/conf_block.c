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
#include <fcntl.h>

#if HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#endif

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_LINUX_LOOP_H
#include <linux/loop.h>
#endif

#include<string.h>

#include "te_stdint.h"
#include "te_alloc.h"
#include "te_string.h"
#include "te_vector.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_pch_tree.h"
#include "unix_internal.h"
#include "conf_common.h"

static bool
ta_block_is_mine(const char *block_name, void *data)
{
    UNUSED(data);
    return rcf_pch_rsrc_accessible("/agent:%s/block:%s", ta_name, block_name);
}

static te_errno
block_dev_list(const ta_conf_ctx *ctx, te_vec *names)
{
    UNUSED(ctx);

#ifdef __linux__
    return get_dir_list_vec("/sys/block", names, false,
                            &ta_block_is_mine, NULL, NULL);
#else
    UNUSED(names);

    ERROR("%s(): getting list of block devices "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

#if HAVE_LINUX_MAJOR_H
static te_errno
check_block_loop(const char *block_name)
{
    te_errno rc = 0;
    char buf[64];

    rc = read_sys_value(buf, sizeof(buf), false, "/sys/block/%s/dev",
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
block_dev_loop_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *block_name = ta_conf_ctx_inst(ctx, "block");
    te_errno rc = TE_RC(TE_TA_UNIX, TE_ENOTBLK);

    if (!ta_block_is_mine(block_name, NULL))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

#if HAVE_LINUX_MAJOR_H
    rc  = check_block_loop(block_name);
#else
    UNUSED(block_name);
#endif

    if (TE_RC_GET_ERROR(rc) == TE_ENOTBLK)
    {
        *val = 0;
        rc = 0;
    }
    else if (rc == 0)
    {
        *val = 1;
    }

    return rc;
}

static te_errno
block_dev_loop_backing_file_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *block_name = ta_conf_ctx_inst(ctx, "block");
    char filename[RCF_MAX_VAL];
    te_errno rc;

    if (!ta_block_is_mine(block_name, NULL))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    /*
     * We don't check explicitly for loop device, as in set accessor,
     * because a non-loop device won't have a corresponding sysfs file
     * anyway.
     */
    rc = read_sys_value(filename, sizeof(filename),
                        false, "/sys/block/%s/loop/backing_file",
                        block_name);
    if (rc != 0)
    {
        /*
         * Missing backing_file in sysfs is not an error:
         * it means there is no backing file configured
         */
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            return 0;
        return rc;
    }

    te_string_append(val, "%s", filename);
    return 0;
}

#if HAVE_LINUX_LOOP_H
static te_errno
attach_loop_device(const char *devname, const char *filename)
{
    int devfd = open(devname, O_RDWR, 0);
    int filefd;
    te_errno rc = 0;

    if (devfd < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open block device %s: %r", devname, rc);
        return rc;
    }

    filefd = open(filename, O_RDWR, 0);
    if (filefd < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        close(devfd);
        ERROR("Cannot open backing file %s: %r", filename, rc);
        return rc;
    }

    if (ioctl(devfd, LOOP_SET_FD, filefd) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Attaching %s to %s failed: %r", filename, devname, rc);
    }
    close(devfd);
    close(filefd);

    return rc;
}

static te_errno
detach_loop_device(const char *devname)
{
    int devfd = open(devname, O_RDWR, 0);
    te_errno rc = 0;

    if (devfd < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open block device %s: %r", devname, rc);
        return rc;
    }

    if (ioctl(devfd, LOOP_CLR_FD) != 0)
    {
        /* Detaching an already detached loop is not an error */
        if (errno != ENXIO)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("Detaching %s failed: %r", devname, rc);
        }
    }
    close(devfd);

    return rc;

}
#endif

static te_errno
block_dev_loop_backing_file_set(const ta_conf_ctx *ctx, const char *val)
{
    const char *block_name = ta_conf_ctx_inst(ctx, "block");
    te_errno rc;
    char devname[RCF_MAX_VAL];

    if (!ta_block_is_mine(block_name, NULL))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

#if HAVE_LINUX_MAJOR_H
    rc = check_block_loop(block_name);
    if (rc != 0)
        return *val == '\0' ? 0 : rc;
#endif

#if HAVE_LINUX_LOOP_H
    /* FIXME: we will need to discover a real device file name */
    TE_SPRINTF(devname, "/dev/%s", block_name);
    if (*val == '\0')
        rc = detach_loop_device(devname);
    else
        rc = attach_loop_device(devname, val);

    return rc;
#else
    UNUSED(devname);
    UNUSED(rc);

    return *val == '\0' ? 0 : TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

static const ta_conf_node *const node_block_dev =
    TA_CONF_LIST("block", block_dev_list,
        TA_CONF_RO_INT32("loop", block_dev_loop_get,
            TA_CONF_RW_STR("backing_file", block_dev_loop_backing_file_get,
                           block_dev_loop_backing_file_set)));

te_errno
ta_unix_conf_block_dev_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent", node_block_dev);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/block",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
}
