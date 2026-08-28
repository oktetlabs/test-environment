/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configfs support
 *
 * Linux configfs configure
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Configfs"

#include "te_config.h"
#include "config.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_str.h"
#include "te_alloc.h"
#include "te_vector.h"
#include "rcf_common.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"

/**
 * Configfs mounting point.
 */
char        configfs_mount_point[RCF_MAX_PATH] = "";

/**
 * Configfs configuration tree node name.
 */
static char configfs_name[RCF_MAX_NAME];

/*
 * snprintf() wrapper.
 *
 * @param str_          String pointer
 * @param size_         String size
 * @param err_msg_      Error message
 * @param format_...    Format string and list of arguments
 */
#define SNPRINTF(str_, size_, err_msg_, format_...) \
    do {                                                \
        int rc_ = snprintf((str_), (size_), format_);   \
        if (rc_ >= (size_) || rc_ < 0)                  \
        {                                               \
            int tmp_err_ = errno;                       \
                                                        \
            ERROR("%s(): %s",                           \
                  __FUNCTION__, (err_msg_));            \
            if (rc_ >= RCF_MAX_PATH)                    \
                return TE_ENOMEM;                       \
            else                                        \
                return te_rc_os2te(tmp_err_);           \
        }                                               \
    } while (0)

/**
 * Mount configfs.
 *
 * @param ctx           Request context
 * @param val           Value (unused)
 *
 * @return              Status code
 */
static te_errno
configfs_add(const ta_conf_ctx *ctx, const char *val)
{
#ifdef HAVE_MKDTEMP
    const char *name = ta_conf_ctx_inst(ctx, "configfs");
    char    tmp[RCF_MAX_PATH] = "/tmp/te_configfs_mp_XXXXXX";
    char    cmd[RCF_MAX_PATH];
    int     tmp_err;

    UNUSED(val);

    if (strlen(configfs_mount_point) != 0)
    {
        ERROR("%s(): there can be only one configfs per TA",
              __FUNCTION__);
        return TE_EEXIST;
    }

    if (mkdtemp(tmp) == NULL)
    {
        tmp_err = errno;
        ERROR("%s(): failed to create temporary directory",
              __FUNCTION__);
        return te_rc_os2te(tmp_err);
    }

    SNPRINTF(cmd, RCF_MAX_PATH,
             "failed to compose mounting command",
             "mount none -t configfs %s", tmp);

    if (ta_system(cmd) != 0)
    {
        ERROR("%s(): failed to mount configfs", __FUNCTION__);
        return TE_EUNKNOWN;
    }

    te_strlcpy(configfs_mount_point, tmp, sizeof(configfs_mount_point));
    te_strlcpy(configfs_name, name, sizeof(configfs_name));

    return 0;
#else
    UNUSED(ctx);
    UNUSED(val);

    ERROR("%s(): not compiled due to lack of system functionality",
          __FUNCTION__);
    return TE_ENOSYS;
#endif
}

/**
 * Unmount configfs.
 *
 * @param ctx           Request context (unused)
 *
 * @return              Status code
 */
static te_errno
configfs_del(const ta_conf_ctx *ctx)
{
    char    cmd[RCF_MAX_PATH];

    UNUSED(ctx);

    SNPRINTF(cmd, RCF_MAX_PATH,
             "failed to compose unmounting command",
             "umount %s", configfs_mount_point);

    if (ta_system(cmd) != 0)
    {
        ERROR("%s(): failed to unmount configfs", __FUNCTION__);
        return TE_EUNKNOWN;
    }

    SNPRINTF(cmd, RCF_MAX_PATH,
             "failed to compose deleting command",
             "rm -rf %s", configfs_mount_point);

    if (ta_system(cmd) != 0)
    {
        ERROR("%s(): failed to delete temporary directory",
              __FUNCTION__);
        return TE_EUNKNOWN;
    }

    configfs_mount_point[0] = '\0';
    configfs_name[0] = '\0';

    return 0;
}

/**
 * Get configfs mounting point
 *
 * @param ctx           Request context (unused)
 * @param val           Location for the retrieved value
 *
 * @return              Status code
 */
static te_errno
configfs_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);

    te_string_append(val, "%s", configfs_mount_point);

    return 0;
}

/**
 * Get instance list for object "agent/configfs".
 *
 * @param ctx           Request context (unused)
 * @param names         Vector of heap-allocated names to append to
 *
 * @return              Status code
 */
static te_errno
configfs_list(const ta_conf_ctx *ctx, te_vec *names)
{
    char *name = TE_STRDUP(configfs_name);

    UNUSED(ctx);

    TE_VEC_APPEND(names, name);

    return 0;
}

/*
 * Configfs configuration tree node.
 */
static const ta_conf_node *const node_configfs =
    TA_CONF_COLL_STR("configfs", configfs_get, configfs_add,
                     configfs_del, configfs_list);

/*
 * Initializing configfs configuration subtree.
 */
te_errno
ta_unix_conf_configfs_init(void)
{
    return ta_conf_register("/agent", node_configfs);
}
