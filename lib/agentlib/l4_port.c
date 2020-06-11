/** @file
 * @brief Agent library
 *
 * Network port related routines
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER "Agent library"

#include "agentlib.h"
#include "logger_api.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IF_ETHER_H
/* Required on OpenBSD */
#if HAVE_NET_IF_ARP_H
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if_arp.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <netinet/if_ether.h>
#endif

te_bool
agent_check_l4_port_is_free(uint16_t port)
{
    int pf = PF_INET6;
    int fd;
    int rc;
    struct sockaddr_storage addr;
    socklen_t addr_len;

    fd = socket(pf, SOCK_STREAM, 0);
    if (fd < 0 && errno == EAFNOSUPPORT) {
        pf = PF_INET;
        fd = socket(pf, SOCK_STREAM, 0);
    }
    if (fd < 0)
    {
        ERROR("Failed to create TCP socket");
        return FALSE;
    }

    memset(&addr, 0, sizeof(addr));
    if (pf == PF_INET6)
    {
        addr_len = sizeof(struct sockaddr_in6);
        SIN6(&addr)->sin6_family = AF_INET6;
        SIN6(&addr)->sin6_port = htons(port);
    }
    else
    {
        addr_len = sizeof(struct sockaddr_in);
        SIN(&addr)->sin_family = AF_INET;
        SIN(&addr)->sin_port = htons(port);
    }
    rc = bind(fd, SA(&addr), addr_len);
    if (rc != 0)
    {
        close(fd);
        return FALSE;
    }

    close(fd);
    fd = socket(pf, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        ERROR("Failed to create UDP socket");
        return FALSE;
    }

    rc = bind(fd, SA(&addr), addr_len);
    if (rc != 0)
    {
        close(fd);
        return FALSE;
    }
    close(fd);

    return TRUE;
}
