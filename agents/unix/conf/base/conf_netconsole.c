/** @file
 * @brief Netconsole support
 *
 * Linux netconsole configure
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

#define TE_LGR_USER     "Conf Netconsole"

#include "te_config.h"
#include "config.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "ta_common.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"

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

extern char configfs_mount_point[RCF_MAX_PATH];

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
    char            local_host_name[HOST_NAME_MAX];

    struct sockaddr_in  local_ipv4_addr;
    struct sockaddr_in  remote_ipv4_addr;
    te_bool             local_ipv4_found = FALSE;
    te_bool             remote_ipv4_found = FALSE;
    int                 rc;
    int                 s = -1;
    struct arpreq       remote_hwaddr_req;

    char            buf[IFS_BUF_SIZE];
    struct ifconf   ifc;
    int             i;
    int             ifs_count;

    char    cmdline[MAX_STR];
    int     tmp_err;

    char    local_ip_str[RCF_MAX_VAL];
    char    remote_ip_str[RCF_MAX_VAL];
    char    remote_hwaddr_str[RCF_MAX_VAL];

    if (gethostname(local_host_name, HOST_NAME_MAX) < 0)
    {
        tmp_err = errno;
        ERROR("%s(): failed to obtain host name", __FUNCTION__);
        return te_rc_os2te(tmp_err);
    }

    rc = te_get_host_addrs(local_host_name, &local_ipv4_addr,
                           &local_ipv4_found, NULL, NULL);
    if (rc != 0)
    {
        ERROR("%s(): failed to obtain addresses of local host",
              __FUNCTION__);
        return rc;
    }

    rc = te_get_host_addrs(remote_host_name, &remote_ipv4_addr,
                           &remote_ipv4_found, NULL, NULL);
    if (rc != 0)
    {
        ERROR("%s(): failed to obtain addresses of remote host",
              __FUNCTION__);
        return rc;
    }

    if (!local_ipv4_found || !remote_ipv4_found)
    {
        ERROR("%s(): failed to find IPv4 address for local and/or "
              "remote host", __FUNCTION__);
        return TE_EADDRNOTAVAIL;
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
    {
        tmp_err = errno;
        ERROR("%s(): failed to create datagram socket, errno '%s'",
              __FUNCTION__, strerror(errno));
        return te_rc_os2te(tmp_err);
    }

    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
    {
        tmp_err = errno;
        ERROR("%s(): ioctl(SIOCGIFCONF) failed, errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return te_rc_os2te(tmp_err);
    }

    ifs_count = ifc.ifc_len / sizeof (struct ifreq);

    for (i = 0; i < ifs_count; i++)
    {
        if (memcmp(&ifc.ifc_req[i].ifr_addr, &local_ipv4_addr,
                   sizeof(local_ipv4_addr)) == 0)
            break;
    }

    if (i == ifs_count)
    {
        ERROR("%s(): local interface not found", __FUNCTION__); 
        close(s);
        return TE_ENOENT;
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
    strncpy(remote_hwaddr_req.arp_dev, ifc.ifc_req[i].ifr_name,
            sizeof(remote_hwaddr_req.arp_dev));
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

    if (strlen(configfs_mount_point) == 0)
    {
        UNUSED(target_name);
        UNUSED(target_dir_path);

        SNPRINTF(cmdline, MAX_STR,
                 "failed to compose command for netconsole module "
                 "configuration",
                 "/sbin/modprobe netconsole netconsole=%d@%s/%s,"
                 "%d@%s/%s",
                 (int)local_port, local_ip_str,
                 ifc.ifc_req[i].ifr_name, (int)remote_port,
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
    }
    else
    {
        char tmp_path[RCF_MAX_PATH];

        if (ta_system("/sbin/modprobe netconsole") != 0)
        {
            ERROR("%s(): failed to do modprobe netconsole",
                  __FUNCTION__);
            return TE_EUNKNOWN;
        }

        SNPRINTF(tmp_path, RCF_MAX_PATH,
                 "failed to compose target directory path",
                 "%s/netconsole/%s_%d_%lu", configfs_mount_point,
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
                 tmp_path, ifc.ifc_req[i].ifr_name, (int)local_port,
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
    UNUSED(argc);
    UNUSED(argv);

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
            if (strlen(configfs_mount_point) == 0)
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

    strncpy(value, target->value, RCF_MAX_VAL);

    return 0;
}

/**
 * Get instance list for object "agent/netconsole".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return              Status code
 */
static te_errno 
netconsole_list(unsigned int gid, const char *oid, char **list)
{
#define BUF_SIZE    2048
    char buf[BUF_SIZE];
    int  n = 0;
    int  rc = 0;
    int  tmp_err;

    netconsole_target   *target = NULL;

    UNUSED(gid);
    UNUSED(oid);

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
      NULL, NULL };

/*
 * Initializing netconsole configuration subtree.
 */
te_errno
ta_unix_conf_netconsole_init(void)
{
    SLIST_INIT(&targets);

    return rcf_pch_add_node("/agent", &node_netconsole);
}
