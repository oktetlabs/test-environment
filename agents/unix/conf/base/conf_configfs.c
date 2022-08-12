/** @file
 * @brief Configfs support
 *
 * Linux configfs configure
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#define TE_LGR_USER     "Conf Configfs"

#include "te_config.h"
#include "config.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_str.h"
#include "rcf_common.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "rcf_ch_api.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch.h"

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
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         Value (unused)
 * @param name          Instance name
 *
 * @return              Status code
 */
static te_errno
configfs_add(unsigned int gid, const char *oid, char *value,
             const char *name)
{
#ifdef HAVE_MKDTEMP
    char    tmp[RCF_MAX_PATH] = "/tmp/te_configfs_mp_XXXXXX";
    char    cmd[RCF_MAX_PATH];
    int     tmp_err;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

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
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(name);
    UNUSED(value);

    ERROR("%s(): not compiled due to lack of system functionality",
          __FUNCTION__);
    return TE_ENOSYS;
#endif
}

/**
 * Unmount configfs.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param name          Instance name (unused)
 *
 * @return              Status code
 */
static te_errno
configfs_del(unsigned int gid, const char *oid, const char *name)
{
    char    cmd[RCF_MAX_PATH];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(name);

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
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         where to store obtained value
 * @param name          name (unused)
 *
 * @return              Status code
 */
static te_errno
configfs_get(unsigned int gid, const char *oid, char *value,
             const char *name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(name);

    te_strlcpy(value, configfs_mount_point, RCF_MAX_VAL);

    return 0;
}

/**
 * Get instance list for object "agent/configfs".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 *
 * @return              Status code
 */
static te_errno
configfs_list(unsigned int gid, const char *oid,
              const char *sub_id, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if (list == NULL)
    {
        ERROR("%s(): null list argument", __FUNCTION__);
        return TE_EINVAL;
    }

    if ((*list = strdup(configfs_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}
/*
 * Configfs configuration tree node.
 */
static rcf_pch_cfg_object node_configfs =
    { "configfs", 0, NULL, NULL,
      (rcf_ch_cfg_get)configfs_get,
      NULL,
      (rcf_ch_cfg_add)configfs_add,
      (rcf_ch_cfg_del)configfs_del,
      (rcf_ch_cfg_list)configfs_list,
      NULL, NULL, NULL };

/*
 * Initializing configfs configuration subtree.
 */
te_errno
ta_unix_conf_configfs_init(void)
{
    return rcf_pch_add_node("/agent", &node_configfs);
}
