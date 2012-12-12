/** @file
 * @brief Configfs support
 *
 * Linux configfs configure
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 *
 * $Id$
 */

#define TE_LGR_USER     "Conf Configfs"

#include "te_config.h"
#include "config.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
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
    
    strncpy(configfs_mount_point, tmp, RCF_MAX_PATH);
    strncpy(configfs_name, name, RCF_MAX_NAME);

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

    strncpy(configfs_mount_point, "", 2);
    strncpy(configfs_name, "", 2);

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

    strncpy(value, configfs_mount_point, 
            RCF_MAX_VAL);

    return 0;
}

/**
 * Get instance list for object "agent/configfs".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return              Status code
 */
static te_errno 
configfs_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

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
      NULL, NULL };

/*
 * Initializing configfs configuration subtree.
 */
te_errno
ta_unix_conf_configfs_init(void)
{
    return rcf_pch_add_node("/agent", &node_configfs);
}
