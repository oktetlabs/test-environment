/** @file
 * @brief Unix Test Agent UPnP Control Point support.
 *
 * Implementation of unix TA UPnP Control Point configuring support.
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Conf UPnP Control Point"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "rcf_common.h"
#include "rcf_ch_api.h"
#include "unix_internal.h"


/** Structure for the common UPnP Control Point settings. */
typedef struct upnp_cp_settings {
    pid_t   pid;                   /**< UPnP Control Point PID. */
    te_bool enable;                /**< UPnP Control Point enable flag. */
    char    target[RCF_MAX_VAL];   /**< Search Target for UPnP devices
                                        and/or services. */
    char    iface[RCF_MAX_VAL];    /**< Network interface. */
} upnp_cp_settings;

static upnp_cp_settings upnp_cp_conf;


/**
 * Start the UPnP Control Point process.
 *
 * @return Status code.
 */
static te_errno
upnp_cp_start_process(void)
{
    const char *argv[] = {
        "te_upnp_cp",
        upnp_cp_conf.target,
        ta_upnp_cp_unix_socket,
        upnp_cp_conf.iface,
        NULL
    };
    te_errno rc = 0;

    rc = rcf_ch_start_process(&upnp_cp_conf.pid, -1, argv[0], TRUE,
                              TE_ARRAY_LEN(argv) - 1, (void **)argv);
    if (rc != 0)
    {
        ERROR("Start UPnP Control Point process failed.");
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    RING("The UPnP Control Point started, PID: %d.", upnp_cp_conf.pid);
    return rc;
}

/**
 * Stop the UPnP Control Point process.
 *
 * @return Status code.
 */
static inline te_errno
upnp_cp_stop_process(void)
{
    return rcf_ch_kill_process(upnp_cp_conf.pid);
}

/**
 * Common set function for boolean parameter.
 *
 * @param[in]  oid          Full object instance identifier.
 * @param[in]  new_value    New value to set.
 * @param[out] value        Pointer to the current value.
 *
 * @return Status code.
 */
static inline te_errno
set_boolean(const char *oid, const char *new_value, te_bool *value)
{
    if (strcmp(new_value, "0") == 0)
        *value = FALSE;
    else if (strcmp(new_value, "1") == 0)
        *value = TRUE;
    else
    {
        ERROR("The new value of \"%s\" variable is not set: new value is "
              "invalid. Must be \"0\" or \"1\", but it is %s.",
              oid, new_value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    return 0;
}

/**
 * Common get function for boolean parameter.
 *
 * @param[in]  oid          Full object instance identifier.
 * @param[in]  value        Value to obtain.
 * @param[out] buf          Buffer to allocate the obtained value.
 *
 * @return Status code. De facto, always returns zero.
 */
static inline te_errno
get_boolean(const char *oid, te_bool value, char *buf)
{
    UNUSED(oid);

    strcpy(buf, (value ? "1" : "0"));
    return 0;
}

/**
 * Common set function for string parameter.
 *
 * @param[in]  oid          Full object instance identifier.
 * @param[in]  new_value    New value to set.
 * @param[out] value        Pointer to the current value.
 *
 * @return Status code.
 */
static inline te_errno
set_string(const char *oid, const char *new_value, char *value)
{
    if (strlen(new_value) >= RCF_MAX_VAL)
    {
        ERROR("A buffer to allocate the \"%s\" variable value is too "
              "small.", oid);
        return TE_RC(TE_TA_UNIX, TE_EOVERFLOW);
    }
    strcpy(value, new_value);
    return 0;
}

/**
 * Common get function for string parameter.
 *
 * @param[in]  oid          Full object instance identifier.
 * @param[in]  value        Value to obtain.
 * @param[out] buf          Buffer to allocate the obtained value.
 *
 * @return Status code. De facto, always returns zero.
 */
static inline te_errno
get_string(const char *oid, const char *value, char *buf)
{
    UNUSED(oid);

    strcpy(buf, value);
    return 0;
}

/**
 * Set function for 'iface' parameter of UPnP Control Point.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param value         New value.
 *
 * @return Status code.
 */
static te_errno
upnp_cp_set_iface(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);

    return set_string(oid, value, upnp_cp_conf.iface);
}

/**
 * Get function for 'iface' parameter of UPnP Control Point.
 *
 * @param[in]  gid          Group identifier (unused).
 * @param[in]  oid          Full object instance identifier.
 * @param[out] value        Obtained value.
 *
 * @return Status code.
 */
static te_errno
upnp_cp_get_iface(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);

    return get_string(oid, upnp_cp_conf.iface, value);
}

/**
 * Set function for 'target' parameter of UPnP Control Point.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param value         New value.
 *
 * @return Status code.
 */
static te_errno
upnp_cp_set_target(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);

    return set_string(oid, value, upnp_cp_conf.target);
}

/**
 * Get function for 'target' parameter of UPnP Control Point.
 *
 * @param[in]  gid          Group identifier (unused).
 * @param[in]  oid          Full object instance identifier.
 * @param[out] value        Obtained value.
 *
 * @return Status code.
 */
static te_errno
upnp_cp_get_target(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);

    return get_string(oid, upnp_cp_conf.target, value);
}

/**
 * Set function for 'enable' parameter of UPnP Control Point.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param value         New value.
 *
 * @return Status code.
 */
static te_errno
upnp_cp_set_enable(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    te_bool  new_value;
    te_errno rc;

    rc = set_boolean(oid, value, &new_value);
    if (rc == 0)
    {
        if (upnp_cp_conf.enable != new_value)
        {
            if (new_value)
            {
                rc = upnp_cp_start_process();
                if (rc == 0)
                    upnp_cp_conf.enable = new_value;
            }
            else
            {
                rc = upnp_cp_stop_process();
                if (rc == 0)
                    upnp_cp_conf.enable = new_value;
            }
        }
    }
    return rc;
}

/**
 * Get function for 'enable' parameter of UPnP Control Point.
 *
 * @param[in]  gid          Group identifier (unused).
 * @param[in]  oid          Full object instance identifier.
 * @param[out] value        Obtained value.
 *
 * @return Status code.
 */
static te_errno
upnp_cp_get_enable(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);

    return get_boolean(oid, upnp_cp_conf.enable, value);
}

/*
 * Test Agent /upnp_cp configuration subtree.
 */
RCF_PCH_CFG_NODE_RW(node_iface, "iface", NULL, NULL,
                    upnp_cp_get_iface, upnp_cp_set_iface);

RCF_PCH_CFG_NODE_RW(node_target, "target", NULL, &node_iface,
                    upnp_cp_get_target, upnp_cp_set_target);

RCF_PCH_CFG_NODE_RW(node_enable, "enable", NULL, &node_target,
                    upnp_cp_get_enable, upnp_cp_set_enable);

RCF_PCH_CFG_NODE_RO(node_upnp_cp, "upnp_cp", &node_enable, NULL, NULL);


/* See description in conf_upnp_cp.h. */
te_errno
ta_unix_conf_upnp_cp_init(void)
{
    return rcf_pch_add_node("/agent", &node_upnp_cp);
}

/* See description in conf_upnp_cp.h. */
te_errno
ta_unix_conf_upnp_cp_release(void)
{
    te_errno rc = 0;    /* Kill process result. */

    if (upnp_cp_conf.enable == TRUE)
    {
        rc = upnp_cp_stop_process();
        upnp_cp_conf.enable = FALSE;
    }
    return rc;
}
