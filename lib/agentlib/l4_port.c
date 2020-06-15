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
agent_check_l4_port_is_free(int socket_family, int socket_type, uint16_t port)
{
    static const int type[] = { SOCK_STREAM, SOCK_DGRAM };
    static const int pf[] = { PF_INET, PF_INET6 };
    static const int af[] = { AF_INET, AF_INET6 };
    unsigned int family_id;
    unsigned int type_id;

    switch (socket_family)
    {
        case AF_INET:
        case AF_INET6:
            break;

        default:
            ERROR("Invalid soket family");
            return FALSE;
    }

    switch (socket_type)
    {
        case SOCK_STREAM:
        case SOCK_DGRAM:
        case 0:
            break;

        default:
            ERROR("Invalid soket type");
            return FALSE;
    }


    for (family_id = 0; family_id < TE_ARRAY_LEN(af); family_id++)
    {
        if (socket_family != af[family_id])
            continue;

        for (type_id = 0; type_id < TE_ARRAY_LEN(type); type_id++)
        {
            struct sockaddr_storage addr;
            socklen_t addr_len;
            int fd;
            int rc;

            if (socket_type != 0 && socket_type != type[type_id])
                continue;

            fd = socket(pf[family_id], type[type_id], 0);
            if (fd < 0)
            {
                ERROR("Failed to create socket");
                return FALSE;
            }

            memset(&addr, 0, sizeof(addr));
            if (pf[family_id] == PF_INET6)
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
            close(fd);
            if (rc != 0)
                return FALSE;
        }
    }

    return TRUE;
}
