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

static te_errno
block_dev_loop_backing_file_get(unsigned int gid, const char *oid, char *value,
                                const char *block_name)
{
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
                        FALSE, "/sys/block/%s/loop/backing_file",
                        block_name);
    if (rc != 0)
    {
        /*
         * Missing backing_file in sysfs is not an error:
         * it means there is no backing file configured
         */
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            *value = '\0';
            return 0;
        }
        return rc;
    }

    strcpy(value, filename);
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
block_dev_loop_backing_file_set(unsigned int gid, const char *oid,
                                const char *value, const char *block_name)
{
    te_errno rc;
    char devname[RCF_MAX_VAL];

    if (!ta_block_is_mine(block_name, NULL))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

#if HAVE_LINUX_MAJOR_H
    rc = check_block_loop(block_name);
    if (rc != 0)
        return *value == '\0' ? 0 : rc;
#endif

#if HAVE_LINUX_LOOP_H
    /* FIXME: we will need to discover a real device file name */
    TE_SPRINTF(devname, "/dev/%s", block_name);
    if (*value == '\0')
        rc = detach_loop_device(devname);
    else
        rc = attach_loop_device(devname, value);

    return rc;
#else
    UNUSED(devname);
    UNUSED(rc);

    return *value == '\0' ? 0 : TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

RCF_PCH_CFG_NODE_RW(node_block_dev_loop_backing_file,
                    "backing_file", NULL, NULL,
                    block_dev_loop_backing_file_get,
                    block_dev_loop_backing_file_set);

RCF_PCH_CFG_NODE_RO(node_block_dev_loop, "loop",
                    &node_block_dev_loop_backing_file, NULL,
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
