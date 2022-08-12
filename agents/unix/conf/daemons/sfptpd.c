/** @file
 * @brief Unix Test Agent
 *
 * Solarflare PTP daemon configuring
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
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

#include "conf_daemons_internal.h"

/** sfptpd daemon process id. */
static pid_t sfptpd_pid = -1;

/** sfptpd daemon pathname. */
static char *sfptpd_path = NULL;

/** sfptpd daemon configuration file pathname. */
static char *sfptpd_config = NULL;

/** sfptpd daemon interface name to use. */
static char *sfptpd_ifname = NULL;

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
    char *s_argv[RCF_MAX_PARAMS] = {"-i", sfptpd_ifname, "-f", sfptpd_config, NULL};
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

    if (sfptpd_ifname == NULL || strlen(sfptpd_ifname) == 0)
    {
        ERROR("parameter sfptpd_ifname was not set.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (sfptpd_config == NULL || strlen(sfptpd_config) == 0)
    {
        ERROR("parameter sfptpd_config was not set.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = rcf_ch_start_process(&sfptpd_pid, -1, sfptpd_path, TRUE,
                                   RCF_MAX_PARAMS, (void **)s_argv)) != 0)
        ERROR("sfptpd process starting failed.");

    return rc;
}

/**
 * Retrieve the daemon interface name
 *
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  Location for the value
 *
 * @return Status code
 */
static te_errno
sfptpd_ifname_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    *value = 0;
    snprintf(value, RCF_MAX_VAL, "%s", sfptpd_ifname);

    return 0;
}

/**
 * Set the daemon interface name
 *
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  String with a new interface name
 *
 * @return Status code
 */
static te_errno
sfptpd_ifname_set(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    free(sfptpd_ifname);
    if ((sfptpd_ifname = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
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

RCF_PCH_CFG_NODE_RW(node_sfptpd_ifname, "ifname", NULL,
                    &node_sfptpd_config, sfptpd_ifname_get, sfptpd_ifname_set);

RCF_PCH_CFG_NODE_RW(node_sfptpd_path, "path", NULL,
                    &node_sfptpd_ifname, sfptpd_path_get, sfptpd_path_set);

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
    sfptpd_ifname = strdup("");

    return rcf_pch_add_node("/agent", &node_sfptpd);
}

/* See description in conf_daemons.h */
void
ta_unix_conf_sfptpd_release(void)
{
    free(sfptpd_path);
    free(sfptpd_config);
    free(sfptpd_ifname);
}
