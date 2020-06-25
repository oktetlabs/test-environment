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
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

/**
 * The minimum available port number
 * Ports below may be used by standard services
 */
#define MIN_AVAILABLE_PORT  20000

/**
 * The maximum available port number
 * Ports above can be used when the linux allocates a dynamic port
 */
#define MAX_AVAILABLE_PORT  (30000 - 1)

#define AVAILABLE_PORT_COUNT (MAX_AVAILABLE_PORT - MIN_AVAILABLE_PORT + 1)

/* Number of port in each bucket */
#define PORTS_PER_BUCKET_COUNT 100

/* Number of buckets */
#define BUCKETS_COUNT (AVAILABLE_PORT_COUNT / PORTS_PER_BUCKET_COUNT)

/* Used to initialize state only once for the TA */
static te_bool initialization_needed = TRUE;
/* Number of allocated ports for the TA */
static unsigned int allocated_ports = 0;
/* Current offset of the next port to allocate for the TA */
static uint16_t port_offset;
/* Mutex used to make port allocation thread-safe for the TA */
static pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;

static te_errno
agent_port_alloc_init(void)
{
    struct random_data buf = {0};
    char statebuf[8];
    int32_t rnd;

    if (initstate_r(getpid(), statebuf, sizeof(statebuf), &buf) != 0 ||
        random_r(&buf, &rnd) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to initialize random number");
        return rc;
    }

    port_offset = (rnd % BUCKETS_COUNT) * PORTS_PER_BUCKET_COUNT;
    initialization_needed = FALSE;

    return 0;
}

te_errno
agent_alloc_l4_port(int socket_family, int socket_type, uint16_t *port)
{
    unsigned int n_ports_tried = 0;
    uint16_t result;
    te_errno rc;

    pthread_mutex_lock(&alloc_lock);

    if (initialization_needed)
    {
        rc = agent_port_alloc_init();
        if (rc != 0)
        {
            pthread_mutex_unlock(&alloc_lock);
            return rc;
        }
    }

    do {
        if (n_ports_tried++ > AVAILABLE_PORT_COUNT)
        {
            ERROR("Failed to allocate port from all available");
            pthread_mutex_unlock(&alloc_lock);
            return TE_ENOBUFS;
        }

        result = MIN_AVAILABLE_PORT + port_offset;
        port_offset = (port_offset + 1) % AVAILABLE_PORT_COUNT;
    } while (!agent_check_l4_port_is_free(socket_family, socket_type, result));

    allocated_ports++;
    *port = result;

    pthread_mutex_unlock(&alloc_lock);

    return 0;
}

void
agent_free_l4_port(uint16_t port)
{
    UNUSED(port);

    pthread_mutex_lock(&alloc_lock);

    if (allocated_ports > 0)
        allocated_ports--;
    else
        ERROR("Failed to free a port, number of frees is greater than allocs");

    pthread_mutex_unlock(&alloc_lock);
}

te_bool
agent_check_l4_port_is_free(int socket_family, int socket_type, uint16_t port)
{
    static const int type[] = { SOCK_STREAM, SOCK_DGRAM };
    static const int pf[] = { PF_INET6, PF_INET };
    static const int af[] = { AF_INET6, AF_INET };
    /* Whether fallback to IPv4 is required (when using 0 socket family) */
    te_bool all_families_fallback_inet4 = FALSE;
    unsigned int family_id;
    unsigned int type_id;

    switch (socket_family)
    {
        case 0:
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
        if (socket_family != 0 && socket_family != af[family_id])
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
                if (errno == EAFNOSUPPORT && socket_family == 0 &&
                    af[family_id] == AF_INET6)
                {
                    /*
                     * Fallback to IPv4 since IPv6 is not supported
                     * and requested family is 0 (all supported families).
                     */
                    all_families_fallback_inet4 = TRUE;
                    break;
                }

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

        /*
         * Checking for IPv6 is enough when socket family 0 is specified,
         * but if fallback to IPv4 is requested, continue checking with IPv4.
         */
        if (socket_family == 0 && !all_families_fallback_inet4)
            return TRUE;
    }

    return TRUE;
}
