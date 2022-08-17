/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Netconsole support
 *
 * Linux netconsole configure
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Netconsole"

#include "te_config.h"
#include "config.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "te_str.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "ta_common.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "netconf.h"
#include "conf_netconf.h"

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#include "te_kernel_log.h"

#ifndef HOST_NAME_MAX
#ifdef _POSIX_HOST_NAME_MAX
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#else
#define HOST_NAME_MAX 64
#endif
#endif

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

#define SYS_KERNEL_CONFIGFS_DIR "/sys/kernel/config"

/** Internal structure for each created netconsole target */
typedef struct netconsole_target {
    SLIST_ENTRY(netconsole_target) links;  /**< List links */

    char *name;                            /**< User-provided name */
    char *value;                           /**< Parameters */
    char *target_dir_path;                 /**< Path of target directory
                                                in configfs (if configfs is
                                                used) */
} netconsole_target;

typedef SLIST_HEAD(netconsole_targets,
                   netconsole_target) netconsole_targets;

static netconsole_targets     targets;

static te_bool netconsole_was_loaded = TRUE;

/**
 * Configure netconsole kernel module.
 *
 * @param local_port        Local port
 * @param remote_host_name  Remote host name
 * @param remote_port       Remote port
 * @param target_name       Name of netconsole target
 * @param target_dir_path   Will be set to the path of target directory
 *                          in configfs if not NULL and if configfs is used
 *
 * @return 0 on success, -1 on failure
 */
static te_errno
configure_netconsole(in_port_t local_port, const char *remote_host_name,
                     in_port_t remote_port, const char *target_name,
                     char **target_dir_path)
{
#define IFS_BUF_SIZE 1024
#define MAX_STR 1024
#if defined(SIOCGARP) && defined(SIOCGIFCONF)

    struct sockaddr_in  local_ipv4_addr;
    struct sockaddr_in  remote_ipv4_addr;
    te_bool             remote_ipv4_found = FALSE;
    int                 rc;
    int                 s = -1;
    struct arpreq       remote_hwaddr_req;

    char    cmdline[MAX_STR];
    int     tmp_err;

    char    local_ip_str[RCF_MAX_VAL];
    char    remote_ip_str[RCF_MAX_VAL];
    char    remote_hwaddr_str[RCF_MAX_VAL];

    char ifname[IF_NAMESIZE];

    rc = te_get_host_addrs(remote_host_name, &remote_ipv4_addr,
                           &remote_ipv4_found, NULL, NULL);
    if (rc != 0)
    {
        ERROR("%s(): failed to obtain addresses of remote host",
              __FUNCTION__);
        return rc;
    }

    if (!remote_ipv4_found)
    {
        ERROR("%s(): failed to find IPv4 address for remote host",
              __FUNCTION__);
        return TE_EADDRNOTAVAIL;
    }

    rc = netconf_route_get_src_addr_and_iface(nh, SA(&remote_ipv4_addr),
                                              SA(&local_ipv4_addr), ifname);
    if (rc != 0)
    {
        ERROR("%s(): failed to get source address and interface, errno '%s'",
              __FUNCTION__, strerror(rc));
        return te_rc_os2te(rc);
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
    {
        tmp_err = errno;
        ERROR("%s(): failed to create datagram socket, errno '%s'",
              __FUNCTION__, strerror(errno));
        return te_rc_os2te(tmp_err);
    }

    local_ipv4_addr.sin_port = htons(local_port);
    remote_ipv4_addr.sin_port = htons(remote_port);

    rc = bind(s, (struct sockaddr *)&local_ipv4_addr,
              sizeof(local_ipv4_addr));
    if (rc < 0)
    {
        tmp_err = errno;
        ERROR("%s(): failed to bind datagram socket, errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return te_rc_os2te(tmp_err);
    }

    rc = sendto(s, "", strlen("") + 1, 0,
                (struct sockaddr *)&remote_ipv4_addr,
                sizeof(remote_ipv4_addr));
    if (rc < 0)
    {
        tmp_err = errno;
        ERROR("%s(): failed to send data from datagram socket, errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return te_rc_os2te(tmp_err);
    }
    usleep(500000);

    memset(&remote_hwaddr_req, 0, sizeof(remote_hwaddr_req));
    memcpy(&remote_hwaddr_req.arp_pa, &remote_ipv4_addr,
           sizeof(remote_ipv4_addr));
    ((struct sockaddr_in *)&remote_hwaddr_req.arp_pa)->sin_port = 0;
#ifdef HAVE_STRUCT_ARPREQ_ARP_DEV
    te_strlcpy(remote_hwaddr_req.arp_dev, ifname,
               sizeof(remote_hwaddr_req.arp_dev));
#endif
    ((struct sockaddr_in *)&remote_hwaddr_req.arp_ha)->sin_family =
                                                            ARPHRD_ETHER;

    if (ioctl(s, SIOCGARP, &remote_hwaddr_req) < 0)
    {
        tmp_err = errno;
        ERROR("%s(): ioctl(SIOCGARP) failed with errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return te_rc_os2te(tmp_err);
    }

    close(s);

    inet_ntop(AF_INET, &local_ipv4_addr.sin_addr, local_ip_str,
              RCF_MAX_VAL);
    inet_ntop(AF_INET, &remote_ipv4_addr.sin_addr, remote_ip_str,
              RCF_MAX_VAL);
    SNPRINTF(remote_hwaddr_str, RCF_MAX_VAL,
             "failed to obtain string representation of remote "
             "hardware address",
             "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
             0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[0]),
             0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[1]),
             0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[2]),
             0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[3]),
             0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[4]),
             0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[5]));

    if (SLIST_EMPTY(&targets) &&
        ta_system("lsmod | grep netconsole || exit 1") != 0)
        netconsole_was_loaded = FALSE;

    if (ta_system("/sbin/modprobe netconsole") != 0)
    {
        ERROR("%s(): failed to do modprobe netconsole",
              __FUNCTION__);
        return TE_EUNKNOWN;
    }

    SNPRINTF(cmdline, MAX_STR,
             "failed to compose netconsole configfs dir checking command",
             "cd %s/netconsole/ >/dev/null 2>&1 || exit 1",
             SYS_KERNEL_CONFIGFS_DIR);

    if (ta_system(cmdline) != 0)
    {
        RING("configfs directory for netconsole is no available, trying to "
             "load module with parameters");
        UNUSED(target_name);
        UNUSED(target_dir_path);

        SNPRINTF(cmdline, MAX_STR,
                 "failed to compose command for netconsole module "
                 "configuration",
                 "/sbin/modprobe netconsole netconsole=%d@%s/%s,"
                 "%d@%s/%s",
                 (int)local_port, local_ip_str,
                 ifname, (int)remote_port,
                 remote_ip_str, remote_hwaddr_str);

        if (ta_system("/sbin/modprobe -r netconsole") != 0)
        {
            usleep(500000);
            if (ta_system("/sbin/modprobe -r netconsole") != 0)
            {
                ERROR("%s(): Failed to unload netconsole module",
                      __FUNCTION__);
                return TE_EUNKNOWN;
            }
        }

        if (ta_system(cmdline) != 0)
        {
            usleep(500000);
            if (ta_system(cmdline) != 0)
            {
                ERROR("%s(): '%s' command failed", __FUNCTION__, cmdline);
                return TE_EUNKNOWN;
            }
        }

        if (target_dir_path != NULL)
            *target_dir_path = NULL;
    }
    else
    {
        char tmp_path[RCF_MAX_PATH];

        SNPRINTF(tmp_path, RCF_MAX_PATH,
                 "failed to compose target directory path",
                 "%s/netconsole/%s_%d_%lu", SYS_KERNEL_CONFIGFS_DIR,
                 target_name, (int)getpid(),
                 (long unsigned int)time(NULL));

        SNPRINTF(cmdline, MAX_STR,
                 "failed to compose directory creation command",
                 "mkdir %s", tmp_path);

        if (ta_system(cmdline) != 0)
        {
            ERROR("%s(): failed to create netconsole target directory",
                  __FUNCTION__);
            return TE_EUNKNOWN;
        }

        SNPRINTF(cmdline, MAX_STR,
                 "failed to compose target configuring command",
                 "cd %s && echo %s > dev_name && echo %d > local_port && "
                 "echo %d > remote_port && echo %s > local_ip && "
                 "echo %s > remote_ip && echo %s > remote_mac &&"
                 "echo 1 > enabled || exit 1",
                 tmp_path, ifname, (int)local_port,
                 (int)remote_port, local_ip_str, remote_ip_str,
                 remote_hwaddr_str);

        if (ta_system(cmdline) != 0)
        {
            ERROR("%s(): failed to configure netconsole target directory",
                  __FUNCTION__);
            return TE_EUNKNOWN;
        }

        if (target_dir_path != NULL)
        {
            *target_dir_path = strdup(tmp_path);
            if (*target_dir_path == NULL)
                return TE_ENOMEM;
        }
    }

    return 0;
#else
    UNUSED(local_port);
    UNUSED(remote_host_name);
    UNUSED(remote_port);
    UNUSED(target_name);
    UNUSED(target_dir_path);

    ERROR("%s(): was not compiled due to lack of system features",
          __FUNCTION__);
    return TE_ENOSYS;
#endif
#undef MAX_STR
#undef IFS_BUF_SIZE
}

/**
 * Load netconsole module with specified parameters
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         module parameters
 * @param name          Instance name (unused)
 *
 * @return              Status code
 */
static te_errno
netconsole_add(unsigned int gid, const char *oid, char *value,
               const char *name)
{
    netconsole_target   *new_target;

    char        *colon = NULL;
    char        *val_dup = NULL;
    char        *tmp = NULL;
    char        *endptr = NULL;

    in_port_t    local_port;
    char        *remote_host_name;
    in_port_t    remote_port;
    te_errno     rc;
    char        *tmp_path = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if (name == NULL)
    {
        ERROR("%s(): name should be set when netconsole object "
              "instance is added", __FUNCTION__);
        return TE_EINVAL;
    }
    if (value == NULL)
    {
        ERROR("%s(): value should be set when netconsole object "
              "instance is added", __FUNCTION__);
        return TE_EINVAL;
    }

    val_dup = strdup(value);
    colon = strchr(val_dup, ':');
    if (colon == NULL)
    {
        ERROR("%s(): local port was not found in a value \"%s\"",
              __FUNCTION__, value);
        return TE_EINVAL;
    }

    *colon = '\0';
    local_port = strtol(val_dup, &endptr, 10);
    if (endptr == NULL || *endptr != '\0')
    {
        ERROR("%s(): failed to process local port value in \"%s\"",
              __FUNCTION__, value);
        return TE_EINVAL;
    }

    tmp = colon + 1;
    colon = strchr(tmp, ':');
    if (colon == NULL)
    {
        ERROR("%s(): remote host was not found in a value \"%s\"",
              __FUNCTION__, value);
        return TE_EINVAL;
    }
    *colon = '\0';
    remote_host_name = tmp;

    tmp = colon + 1;
    remote_port = strtol(tmp, &endptr, 10);
    if (endptr == NULL || *endptr != '\0')
    {
        ERROR("%s(): failed to remote port value in \"%s\"",
              __FUNCTION__, value);
        return TE_EINVAL;
    }

    rc = configure_netconsole(local_port, remote_host_name, remote_port,
                              name, &tmp_path);
    free(val_dup);

    new_target = TE_ALLOC(sizeof(*new_target));
    if (new_target == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    new_target->name = strdup(name);
    new_target->value = strdup(value);
    new_target->target_dir_path = tmp_path;
    SLIST_INSERT_HEAD(&targets, new_target, links);

    return rc;
}

/**
 * Unload netconsole kernel module
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param name          Instance name (unused)
 *
 * @return              Status code
 */
static te_errno
netconsole_del(unsigned int gid, const char *oid, const char *name)
{
    netconsole_target  *target;

    UNUSED(gid);
    UNUSED(oid);

    if (name == NULL)
    {
        ERROR("%s(): name was not specified", __FUNCTION__);
        return TE_EINVAL;
    }

    SLIST_FOREACH(target, &targets, links)
    {
        if (strcmp(target->name, name) == 0)
        {
            if (target->target_dir_path == NULL)
            {
                if (ta_system("/sbin/modprobe -r netconsole") != 0)
                {
                    usleep(500000);
                    if (ta_system("/sbin/modprobe -r netconsole") != 0)
                    {
                        ERROR("%s(): Failed to unload netconsole module",
                              __FUNCTION__);
                        return TE_EUNKNOWN;
                    }
                }
            }
            else
            {
                char cmd[RCF_MAX_PATH];

                SNPRINTF(cmd, RCF_MAX_PATH,
                         "failed to compose target deleting command",
                         "rmdir %s", target->target_dir_path);

                if (ta_system(cmd) != 0)
                {
                    ERROR("%s(): failed to delete target", __FUNCTION__);
                    return TE_EUNKNOWN;
                }
            }

            SLIST_REMOVE(&targets, target, netconsole_target, links);
            if (target->target_dir_path != NULL)
            {
                if (SLIST_EMPTY(&targets) && !netconsole_was_loaded)
                {
                    if (ta_system("/sbin/modprobe -r netconsole") != 0)
                        ERROR("%s(): failed to unload netconsole module",
                              __FUNCTION__);
                }
            }

            free(target->name);
            free(target->value);
            free(target->target_dir_path);
            free(target);
            break;
        }
    }

    if (target == NULL)
    {
        ERROR("%s(): netconsole target was not found", __FUNCTION__);
        return TE_ENOENT;
    }

    return 0;
}

/**
 * Get netconsole value (i.e. its parameters)
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         where to store obtained value
 * @param name          name of netconsole node
 *
 * @return              Status code
 */
static te_errno
netconsole_get(unsigned int gid, const char *oid, char *value,
               const char *name)
{
    netconsole_target   *target;

    UNUSED(gid);
    UNUSED(oid);

    SLIST_FOREACH(target, &targets, links)
        if (strcmp(target->name, name) == 0)
            break;

    if (target == NULL)
    {
        ERROR("%s(): netconsole target was not found", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    te_strlcpy(value, target->value, RCF_MAX_VAL);

    return 0;
}

/**
 * Get instance list for object "agent/netconsole".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 *
 * @return              Status code
 */
static te_errno
netconsole_list(unsigned int gid, const char *oid,
                const char *sub_id, char **list)
{
#define BUF_SIZE    2048
    char buf[BUF_SIZE] = "";
    int  n = 0;
    int  rc = 0;
    int  tmp_err;

    netconsole_target   *target = NULL;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if (list == NULL)
    {
        ERROR("%s(): null list argument", __FUNCTION__);
        return TE_EINVAL;
    }

    SLIST_FOREACH(target, &targets, links)
    {
        rc = snprintf(buf + n, BUF_SIZE - n, "%s ", target->name);
        if (rc > 0 && rc < BUF_SIZE - n)
            n += rc;
        else if (rc < 0)
        {
            tmp_err = errno;
            ERROR("%s(): snprintf() failed", __FUNCTION__);
            return te_rc_os2te(tmp_err);
        }
        else
        {

            ERROR("%s(): not enough space in buffer", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
    }
    if (strlen(buf) > 0)
        buf[strlen(buf) - 1] = '\0';

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
#undef BUF_SIZE
}

/*
 * Netconsole configuration tree node.
 */
static rcf_pch_cfg_object node_netconsole =
    { "netconsole", 0, NULL, NULL,
      (rcf_ch_cfg_get)netconsole_get,
      NULL,
      (rcf_ch_cfg_add)netconsole_add,
      (rcf_ch_cfg_del)netconsole_del,
      (rcf_ch_cfg_list)netconsole_list,
      NULL, NULL, NULL };

/*
 * Initializing netconsole configuration subtree.
 */
te_errno
ta_unix_conf_netconsole_init(void)
{
    SLIST_INIT(&targets);

    return rcf_pch_add_node("/agent", &node_netconsole);
}
