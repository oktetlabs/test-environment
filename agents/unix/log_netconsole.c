/** @file
 * @brief Configuring netconsole
 *
 * Configuring netconsole support in TA
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER      "Netconsole Configuration"

#include "te_config.h"
#include "config.h"

#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "comm_agent.h"
#include "te_raw_log.h"
#include "logger_api.h"
#include "rcf_common.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "unix_internal.h"

#include "te_kernel_log.h"

te_bool ta_netconsole_configured = FALSE;

/**
 * Configure netconsole kernel module.
 *
 * @param argc Number of arguments
 * @param argv Array of arguments
 *
 * @return 0 on success, -1 on failure
 */
int
configure_netconsole(int argc, char *argv[])
{
#define IFS_BUF_SIZE 1024
#define MAX_STR 1024
#if defined(SIOCGARP) && defined(SIOCGIFCONF)
    in_port_t       local_port;
    in_port_t       remote_port;
    char           *remote_host_name;
    char           *endptr = NULL;
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
    int     n = 0;

    if (argc != 3)
    {
        ERROR("%s(): wrong number of arguments", __FUNCTION__);
        return -1;
    }

    remote_host_name = (char *)argv[1];
    local_port = strtol(argv[0], &endptr, 10);
    if (endptr == NULL || *endptr != '\0')
    {
        ERROR("%s(): failed to process local port", __FUNCTION__);
        return -1;
    }
    remote_port = strtol(argv[2], &endptr, 10);
    if (endptr == NULL || *endptr != '\0')
    {
        ERROR("%s(): failed to process remote port", __FUNCTION__);
        return -1;
    }

    if (gethostname(local_host_name, HOST_NAME_MAX) < 0)
    {
        ERROR("%s(): failed to obtain host name", __FUNCTION__);
        return -1;
    }

    rc = te_get_host_addrs(local_host_name, &local_ipv4_addr,
                           &local_ipv4_found, NULL, NULL);
    if (rc < 0)
    {
        ERROR("%s(): failed to obtain addresses of local host",
              __FUNCTION__);
        return -1;
    }

    rc = te_get_host_addrs(remote_host_name, &remote_ipv4_addr,
                           &remote_ipv4_found, NULL, NULL);
    if (rc < 0)
    {
        ERROR("%s(): failed to obtain addresses of remote host",
              __FUNCTION__);
        return -1;
    }

    if (!local_ipv4_found || !remote_ipv4_found)
    {
        ERROR("%s(): failed to find IPv4 address for local and/or "
              "remote host", __FUNCTION__);
        return -1;
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
    {
        ERROR("%s(): failed to create datagram socket, errno '%s'",
              __FUNCTION__, strerror(errno));
        return -1;
    }

    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
    {
        ERROR("%s(): ioctl(SIOCGIFCONF) failed, errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return -1;
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
        return -1;
        close(s);
    }

    local_ipv4_addr.sin_port = htons(local_port);
    remote_ipv4_addr.sin_port = htons(remote_port);

    rc = bind(s, (struct sockaddr *)&local_ipv4_addr,
              sizeof(local_ipv4_addr)); 
    if (rc < 0)
    {
        ERROR("%s(): failed to bind datagram socket, errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return -1;
    }

    rc = sendto(s, "", strlen("") + 1, 0,
                (struct sockaddr *)&remote_ipv4_addr,
                sizeof(remote_ipv4_addr));
    if (rc < 0)
    {
        ERROR("%s(): failed to send data from datagram socket, errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return -1;
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
        ERROR("%s(): ioctl(SIOCGARP) failed with errno '%s'",
              __FUNCTION__, strerror(errno));
        close(s);
        return -1;
    }

    close(s);

    n = snprintf(cmdline, MAX_STR,
                 "/sbin/modprobe netconsole netconsole=%d@",
                 (int)local_port);
    inet_ntop(AF_INET, &local_ipv4_addr.sin_addr, cmdline + n,
              MAX_STR - n);
    n = strlen(cmdline);
    n += snprintf(cmdline + n, MAX_STR - n, "/%s,%d@",
                  ifc.ifc_req[i].ifr_name, (int)remote_port);
    inet_ntop(AF_INET, &remote_ipv4_addr.sin_addr, cmdline + n,
              MAX_STR - n);
    n = strlen(cmdline);

    n += snprintf(cmdline + n, MAX_STR - n,
                  "/%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                  0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[0]),
                  0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[1]),
                  0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[2]),
                  0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[3]),
                  0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[4]),
                  0xff & (int)(remote_hwaddr_req.arp_ha.sa_data[5]));

    if (system("/sbin/modprobe -r netconsole") != 0)
    {
        usleep(500000);
        if (system("/sbin/modprobe -r netconsole"))
        {
            ERROR("%s(): Failed to unload netconsole module",
                  __FUNCTION__);
            return -1;
        }
    }

    if (system(cmdline) != 0)
    {
        usleep(500000);
        if (system(cmdline) != 0)
        {
            ERROR("%s(): '%s' command failed", __FUNCTION__, cmdline);
            return -1;
        }
    }

    ta_netconsole_configured = TRUE;

    return 0;
#else
    UNUSED(argc);
    UNUSED(argv);

    ERROR("%s(): was not compiled due to lack of system features",
          __FUNCTION__);
    return -1;
#endif
#undef MAX_STR
#undef IFS_BUF_SIZE
}
