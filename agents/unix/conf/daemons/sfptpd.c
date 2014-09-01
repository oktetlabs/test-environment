/** @file
 * @brief Unix Test Agent
 *
 * Solarflare PTP daemon configuring
 *
 *
 * Copyright (C) 2014 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "conf_daemons.h"

/** sfptpd daemon process id. */
static pid_t sfptpd_pid = -1;

/** sfptpd daemon pathname. */
static char *sfptpd_path = NULL;

/** sfptpd daemon configuration file pathname. */
static char *sfptpd_config = NULL;

/**
 * Retrieve daemon status
 * 
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  Location for the value
 * 
 * @return Status code
 */
static te_errno
sfptpd_enable_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    if (sfptpd_pid == -1 || kill(sfptpd_pid, 0) != 0)
        snprintf(value, RCF_MAX_VAL, "%d", 0);
    else
        snprintf(value, RCF_MAX_VAL, "%d", 1);

    return 0;
}

/**
 * Enable/Disable sfptpd daemon
 * 
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  @c 0 to disable, else - enable
 * 
 * @return Status code
 */
static te_errno
sfptpd_enable_set(unsigned int gid, const char *oid, char *value)
{
    te_bool en = atoi(value);
    char *s_argv[RCF_MAX_PARAMS] = {sfptpd_path, "-f", sfptpd_config, NULL};
    int rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((en && sfptpd_pid != -1) || (!en && sfptpd_pid == -1))
        return 0;

    if (en == FALSE)
    {
        if ((rc = rcf_ch_kill_process(sfptpd_pid)) != 0)
            ERROR("Failed to kill sfptpd process with pid %d", sfptpd_pid);
        sfptpd_pid = -1;
        return rc;
    }

    if ((rc = rcf_ch_start_process(&sfptpd_pid, -1, sfptpd_path, TRUE, 
                                   RCF_MAX_PARAMS, (void **)s_argv)) != 0)
        ERROR("sfptpd process starting failed.");

    return rc;
}

/**
 * Retrieve the daemon pathname
 * 
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  Location for the value
 * 
 * @return Status code
 */
static te_errno
sfptpd_path_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    *value = 0;
    snprintf(value, RCF_MAX_VAL, "%s", sfptpd_path);

    return 0;
}

/**
 * Set the daemon pathname
 * 
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  String with a new pathname
 * 
 * @return Status code
 */
static te_errno
sfptpd_path_set(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    free(sfptpd_path);
    sfptpd_path = strdup(value);

    return 0;
}

/**
 * Retrieve the daemon config file pathname
 * 
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  Location for the value
 * 
 * @return Status code
 */
static te_errno
sfptpd_config_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    *value = 0;
    snprintf(value, RCF_MAX_VAL, "%s", sfptpd_config);

    return 0;
}

/**
 * Set the daemon config file pathname
 * 
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  String with a new pathname
 * 
 * @return Status code
 */
static te_errno
sfptpd_config_set(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    free(sfptpd_config);
    sfptpd_config = strdup(value);

    return 0;
}

RCF_PCH_CFG_NODE_RW(node_sfptpd_config, "config", NULL, 
                    NULL, sfptpd_config_get, sfptpd_config_set);

RCF_PCH_CFG_NODE_RW(node_sfptpd_path, "path", NULL, 
                    &node_sfptpd_config, sfptpd_path_get, sfptpd_path_set);

RCF_PCH_CFG_NODE_RW(node_sfptpd_enable, "enable", NULL, &node_sfptpd_path,
                    sfptpd_enable_get, sfptpd_enable_set);

RCF_PCH_CFG_NODE_RO(node_sfptpd, "sfptpd", 
                    &node_sfptpd_enable, NULL, NULL);

/* See description in conf_daemons.h */
te_errno
ta_unix_conf_sfptpd_init(void)
{
    sfptpd_path = strdup("");
    sfptpd_config = strdup("");

    return rcf_pch_add_node("/agent", &node_sfptpd);
}

/* See description in conf_daemons.h */
void
ta_unix_conf_sfptpd_release(void)
{
    free(sfptpd_path);
    free(sfptpd_config);
}
