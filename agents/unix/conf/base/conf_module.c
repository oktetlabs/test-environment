/** @file
 * @brief System module configuration support
 *
 * Implementation of configuration nodes for system modules.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "Unix Conf System Module"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "conf_common.h"
#include "unix_internal.h"
#include "logger_api.h"

/** Auxiliary buffer used to construct strings. */
static char buf[4096];

static te_errno module_list(unsigned int, const char *, char **);
static te_errno module_param_list(unsigned int, const char *, char **,
                                  const char *);
static te_errno module_param_get(unsigned int, const char *, char *,
                                 const char *, const char *);
static te_errno module_param_set(unsigned int, const char *, const char *,
                                 const char *, const char *);

static rcf_pch_cfg_object node_module_param =
    { "parameter", 0, NULL, NULL,
      (rcf_ch_cfg_get)module_param_get, (rcf_ch_cfg_set)module_param_set,
      NULL, NULL,
      (rcf_ch_cfg_list)module_param_list, NULL, NULL};

RCF_PCH_CFG_NODE_COLLECTION(node_module, "module",
                            &node_module_param, NULL,
                            NULL, NULL, module_list, NULL);

/**
 * Callback used to check whether a given module is grabbed by TA
 * when creating a list of modules with help of get_dir_list().
 *
 * @param module_name       Module name.
 * @param data              Unused.
 *
 * @return @c TRUE if the module is grabbed, @c FALSE otherwise.
 */
static te_bool
module_list_include_callback(const char *module_name, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible("/agent:%s/module:%s",
                                   ta_name, module_name);
}

/**
 * Get list of module names.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param list          Where to save the list.
 *
 * @return Status code.
 */
static te_errno
module_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

#ifdef __linux__
    {
        te_errno  rc;

        rc = get_dir_list("/sys/module/", buf, sizeof(buf), FALSE,
                          &module_list_include_callback, NULL);
        if (rc != 0)
            return rc;
    }
#else
    ERROR("%s(): getting list of system modules "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get list of module parameter names.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param list          Where to save the list.
 * @param module_name   Name of the module.
 *
 * @return Status code.
 */
static te_errno
module_param_list(unsigned int gid, const char *oid, char **list,
                  const char *module_name)
{
    UNUSED(gid);
    UNUSED(oid);

#ifdef __linux__
    {
        int       rc;
        char      path[PATH_MAX];

        rc = snprintf(path, PATH_MAX, "/sys/module/%s/parameters/",
                      module_name);
        if (rc < 0)
        {
            ERROR("%s(): snprintf() failed", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        else if (rc >= PATH_MAX)
        {
            ERROR("%s(): not enough space for module path", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }

        rc = get_dir_list(path, buf, sizeof(buf), TRUE, NULL, NULL);
        if (rc != 0)
            return rc;
    }
#else
    UNUSED(module_name);

    ERROR("%s(): getting list of system module parameters "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get value of module parameter.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param value         Where to save the value.
 * @param module_name   Name of the module.
 * @param param_name    Name of the parameter.
 *
 * @return Status code.
 */
static te_errno
module_param_get(unsigned int gid, const char *oid, char *value,
                 const char *module_name, const char *param_name)
{
    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    return read_sys_value(value, RCF_MAX_VAL, TRUE,
                          "/sys/module/%s/parameters/%s",
                          module_name, param_name);
#else
    UNUSED(value);
    UNUSED(module_name);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set value of module parameter.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param value         Value to set.
 * @param module_name   Name of the module.
 * @param param_name    Name of the parameter.
 *
 * @return Status code.
 */
static te_errno
module_param_set(unsigned int gid, const char *oid, const char *value,
                 const char *module_name, const char *param_name)
{
    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    return write_sys_value(value,
                           "/sys/module/%s/parameters/%s",
                           module_name, param_name);
#else
    UNUSED(value);
    UNUSED(module_name);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Initialize configuration for system module nodes.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_module_init(void)
{
#ifdef __linux__
    te_errno rc;

    rc = rcf_pch_add_node("/agent/", &node_module);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/module",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
#else
    INFO("System module configuration is not supported");
    return 0;
#endif
}
