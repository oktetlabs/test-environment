/** @file
 * @brief Unix Test Agent
 *
 * NTP daemon management
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

#include "conf_daemons.h"

#define SERVICE_INITD "/etc/init.d"

/** Possible daemon names. */
static const char *ntpd_names[] = {"ntpd", "ntp"};

static te_bool ntpd_status = FALSE;

/** Possible action on the daemon. */
typedef enum daemon_action {
    ACT_STATUS,  /**< Retrieve daemon status */
    ACT_START,   /**< Start daemon */
    ACT_STOP     /**< Stop daemon */
} daemon_action;

/**
 * Try to find service binary.
 * 
 * @return Pathname to the service binary or @c NULL
 */
static const char *
service_app(void)
{
    const char *service;

    service = "/usr/sbin/service";
    if (file_exists((char *)service))
        return service;

    service = "/sbin/service";
    if (file_exists((char *)service))
        return service;

    return NULL;
}

/**
 * Apply an action on the daemon.
 * 
 * @param act       Action
 * @param status    Status location or @c NULL
 * 
 * @return Status code
 */
static te_errno
ntpd_apply_action(daemon_action act, te_bool *status)
{
    const char *act_str = NULL;
    char cmd[RCF_MAX_PATH] = {0,};
    const char *service = NULL;
    int res;
    int i;

    switch (act)
    {
        case ACT_STATUS:
            act_str = "status";
            break;

        case ACT_START:
            act_str = "start";
            break;

        case ACT_STOP:
            act_str = "stop";
            break;

        default:
            ERROR("Unknown action to apply to the daemon");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    service = service_app();

#define CHECK_PRINTF_ERR(_res) \
do {                                                                    \
    if (_res < 0 || _res > RCF_MAX_PATH)                                \
    {                                                                   \
        if (_res < 0)                                                   \
            ERROR("Failed to create string with command: %s",           \
                  strerror(errno));                                     \
        else                                                            \
            ERROR("Command buffer is too small for the command line");  \
        return TE_RC(TE_TA_UNIX, TE_EFAIL);                             \
    }                                                                   \
} while (0)

    for (i = 0; (size_t)i < sizeof(ntpd_names)/sizeof(*ntpd_names); i++)
    {
        if (service != NULL)
        {
            res = snprintf(cmd, RCF_MAX_PATH, "%s %s %s >/dev/null 2>&1",
                           service, ntpd_names[i], act_str);
            CHECK_PRINTF_ERR(res);
        }
        else
        {
            res = snprintf(cmd, RCF_MAX_PATH, "%s/%s", SERVICE_INITD,
                           ntpd_names[i]);
            CHECK_PRINTF_ERR(res);

            if (!file_exists(cmd))
                continue;
            res = snprintf(cmd, RCF_MAX_PATH, "%s/%s %s >/dev/null 2>&1",
                           SERVICE_INITD, ntpd_names[i], act_str);
            CHECK_PRINTF_ERR(res);
        }

        res = ta_system(cmd);

        if (WIFEXITED(res))
            res = WEXITSTATUS(res);
        else
            continue;

        if (service != NULL && res != 0 && res != 3)
            continue;

        if (act == ACT_STATUS)
        {
            if (status == NULL)
            {
                ERROR("status argument is NULL");
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }

            if (res == 0)
                *status = 1;
            else if (res == 3)
            {
                *status = 0;
                return 0;
            }
            else
                continue;
        }

        if (res == 0)
        {
            if (act == ACT_START)
                ntpd_status = TRUE;
            if (act == ACT_STOP)
                ntpd_status = FALSE;
            return 0;
        }

        ERROR("Srvice command returned unexpected code %d", res);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

#undef CHECK_PRINTF_ERR

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}

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
ntpd_enable_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    snprintf(value, RCF_MAX_VAL, "%d", ntpd_status);

    return 0;
}

/**
 * Enable/Disable NTP daemon
 * 
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  @c 0 to disable, else - enable
 * 
 * @return Status code
 */
static te_errno
ntpd_enable_set(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ntpd_apply_action(atoi(value) ? ACT_START : ACT_STOP, NULL);
}

RCF_PCH_CFG_NODE_RW(node_ntpd_enable, "enable", NULL, NULL,
                    ntpd_enable_get, ntpd_enable_set);

RCF_PCH_CFG_NODE_RO(node_ntpd, "ntpd", 
                    &node_ntpd_enable, NULL, NULL);

/* See description in conf_daemons.h */
te_errno
ta_unix_conf_ntpd_init(void)
{
    if (ntpd_apply_action(ACT_STATUS, &ntpd_status) == 0)
        return rcf_pch_add_node("/agent", &node_ntpd);

    ERROR("NTP daemon is not under control");
    return 0;
}
