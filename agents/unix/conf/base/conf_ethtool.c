/** @file
 * @brief Unix Test Agent
 *
 * Implementation of common API for SIOCETHTOOL usage in Unix TA
 * configuration
 *
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "te_config.h"
#include "config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"

#ifdef HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#include "unix_internal.h"

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H

/**
 * Get string representation of ethtool command.
 *
 * @param cmd     Ethtool command number
 *
 * @return String representation.
 */
static const char *
ethtool_cmd2str(int cmd)
{
#define CASE_CMD(_cmd) \
    case _cmd:               \
        return #_cmd

    switch (cmd)
    {
        CASE_CMD(ETHTOOL_GCOALESCE);
        CASE_CMD(ETHTOOL_SCOALESCE);
        CASE_CMD(ETHTOOL_GPAUSEPARAM);
        CASE_CMD(ETHTOOL_SPAUSEPARAM);
    }

    return "<UNKNOWN>";
#undef CASE_CMD
}

/* See description in conf_ethtool.h */
te_errno
call_ethtool_ioctl(const char *if_name, int cmd, void *value)
{
    struct ifreq ifr;
    int rc;
    te_errno te_rc;

    memset(&ifr, 0, sizeof(ifr));

    te_rc = te_snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
    if (te_rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, te_rc);
    }

    *(uint32_t *)value = cmd;
    ifr.ifr_data = (caddr_t)value;

    rc = ioctl(cfg_socket, SIOCETHTOOL, &ifr);
    if (rc < 0)
    {
        /*
         * Avoid extra logs if this command is simply not supported
         * for a given interface.
         */
        if (errno != EOPNOTSUPP)
        {
            ERROR("%s(if_name=%s, cmd=%s): ioctl() failed", __FUNCTION__,
                  if_name, ethtool_cmd2str(cmd));
        }
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/* See description in conf_ethtool.h */
te_errno
get_ethtool_value(const char *if_name, unsigned int gid,
                  int cmd, void *val_local, void **ptr_out,
                  te_bool do_set)
{
    ta_cfg_obj_t *obj;
    te_errno rc;

    const char *obj_type;
    size_t val_size;

    switch (cmd)
    {
        case ETHTOOL_GCOALESCE:
            obj_type = TA_OBJ_TYPE_IF_COALESCE;
            val_size = sizeof(struct ethtool_coalesce);
            break;

        case ETHTOOL_GPAUSEPARAM:
            obj_type = TA_OBJ_TYPE_IF_PAUSE;
            val_size = sizeof(struct ethtool_pauseparam);
            break;

        default:
            ERROR("%s(): unknown ethtool command %d", __FUNCTION__, cmd);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    obj = ta_obj_find(obj_type, if_name);
    if (obj != NULL)
    {
        *ptr_out = obj->user_data;
    }
    else
    {
        memset(val_local, 0, val_size);
        rc = call_ethtool_ioctl(if_name, cmd, val_local);
        if (rc != 0)
            return rc;

        if (do_set)
        {
            *ptr_out = calloc(val_size, 1);
            if (*ptr_out == NULL)
            {
                ERROR("%s(): not enough memory", __FUNCTION__);
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }

            rc = ta_obj_add(obj_type, if_name, "", gid, *ptr_out, NULL);
            if (rc != 0)
            {
                ERROR("%s(): failed to add a new object", __FUNCTION__);
                free(*ptr_out);
                *ptr_out = NULL;
                return TE_RC(TE_TA_UNIX, rc);
            }

            memcpy(*ptr_out, val_local, val_size);
        }
        else
        {
            *ptr_out = val_local;
        }
    }

    return 0;
}

/* See description in conf_ethtool.h */
te_errno
commit_ethtool_value(const char *if_name, int cmd)
{
    const char *obj_type;
    ta_cfg_obj_t *obj;
    te_errno rc;

    switch (cmd)
    {
        case ETHTOOL_SCOALESCE:
            obj_type = TA_OBJ_TYPE_IF_COALESCE;
            break;

        case ETHTOOL_SPAUSEPARAM:
            obj_type = TA_OBJ_TYPE_IF_PAUSE;
            break;

        default:
            ERROR("%s(): unknown ethtool command %d", __FUNCTION__, cmd);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    obj = ta_obj_find(obj_type, if_name);
    if (obj == NULL)
    {
        ERROR("%s(): object of type '%s' was not found for "
              "interface '%s'", __FUNCTION__, obj_type, if_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = call_ethtool_ioctl(if_name, cmd, obj->user_data);

    free(obj->user_data);
    ta_obj_free(obj);

    return rc;
}

#endif
