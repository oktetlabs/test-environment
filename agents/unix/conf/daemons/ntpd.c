/** @file
 * @brief Unix Test Agent
 *
 * NTP daemon management
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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

#include "conf_daemons_internal.h"

#define SERVICE_INITD "/etc/init.d"

/** Possible daemon names. */
static const char *ntpd_names[] = {"chronyd", "chrony", "ntpd", "ntp"};

static te_bool ntpd_status = FALSE;

/** Possible action on the daemon. */
typedef enum daemon_action {
    ACT_STATUS,  /**< Retrieve daemon status */
    ACT_START,   /**< Start daemon */
    ACT_STOP     /**< Stop daemon */
} daemon_action;

/**
 * Try to find systemctl binary.
 *
 * @note When systemctl is present, then:
 * (1) service works via systemctl;
 * (2) systemctl/service show the end of log, which takes up to 30s;
 * (3) systemctl has a parameter to stop reading log and exist immediately.
 * So systemctl is the preferred way to manipulate the services.
 *
 * @return Pathname to the service binary or @c NULL
 */
static const char *
systemctl_app(void)
{
    const char *systemctl;

    systemctl = "/usr/bin/systemctl";
    if (file_exists((char *)systemctl))
        return systemctl;

    systemctl = "/bin/systemctl";
    if (file_exists((char *)systemctl))
        return systemctl;

    return NULL;
}

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
    const char *service = service_app();
    const char *systemctl = systemctl_app();
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
        if (systemctl != NULL)
        {
            res = snprintf(cmd, RCF_MAX_PATH,
                           "%s -n0 --no-pager %s %s " ">/dev/null 2>&1",
                           systemctl, act_str, ntpd_names[i]);
            CHECK_PRINTF_ERR(res);
        }
        else if (service != NULL)
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

        if ((systemctl != NULL || service != NULL) && res != 0 && res != 3)
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
