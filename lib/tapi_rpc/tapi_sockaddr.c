/** @file
 * @brief Functions to opearate with generic "struct sockaddr"
 *
 * Implementation of test API for working with struct sockaddr.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER      "TAPI SockAddr"

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_errno.h"
#include "te_stdint.h"
#include "te_alloc.h"

#include "tapi_sockaddr.h"
#include "tapi_rpc_socket.h"
#include "tapi_test.h"

/**
 * The minimum available port number
 * Ports below may be used by standard services
 */
#define MIN_AVAILABLE_PORT  20000

/**
 * The maximum available port number
 * Ports above can be used when the linux allocates a dynamic port
 */
#define MAX_AVAILABLE_PORT  30000

/* See description in tapi_sockaddr.h */
te_errno
tapi_allocate_port(struct rcf_rpc_server *pco, uint16_t *p_port)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    int  rc;
    int  port;

    /* NOTE: if scheme of port allocation will be changed,
       implementation of tapi_allocate_port_range() also should be fixed! */
    pthread_mutex_lock(&mutex);

    rc = cfg_get_instance_int_fmt(&port, "/volatile:/sockaddr_port:");
    if (rc != 0)
    {
        pthread_mutex_unlock(&mutex);
        ERROR("Failed to get /volatile:/sockaddr_port:: %r", rc);
        return rc;
    }
    if (port < 0 || port > 0xffff)
    {
        pthread_mutex_unlock(&mutex);
        ERROR("Wrong value %d is got from /volatile:/sockaddr_port:", port);
        return TE_EINVAL;
    }
    if ((port < MIN_AVAILABLE_PORT) || (port >= MAX_AVAILABLE_PORT))
    {
        /* Random numbers generator should be initialized earlier */
        port = MIN_AVAILABLE_PORT +
                rand_range(0, MAX_AVAILABLE_PORT - MIN_AVAILABLE_PORT);
    }
    else
    {
        port++;
    }

    /* Check that port is free */
    if (pco != NULL)
    {
        int port_max = MAX_AVAILABLE_PORT;
        int port_base = port;
        while (!rpc_check_port_is_free(pco, port))
        {
            port++;
            if (port >= port_max)
            {
                port_max = port_base;
                if (port_max == MIN_AVAILABLE_PORT)
                    break;
                port = MIN_AVAILABLE_PORT +
                        rand_range(0, port_max - MIN_AVAILABLE_PORT);
                port_base = port;
            }
        }
    }

    /* Set /volatile:/sockaddr_port */
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, port),
                              "/volatile:/sockaddr_port:");
    if (rc != 0)
    {
        pthread_mutex_unlock(&mutex);
        ERROR("Failed to set /volatile:/sockaddr_port:: %r", rc);
        return rc;
    }

    pthread_mutex_unlock(&mutex);

    *p_port = (uint16_t)port;

    return 0;
}

/* See description in tapi_sockaddr.h */
uint16_t
tapi_get_port(rcf_rpc_server *rpcs)
{
    uint16_t port;

    CHECK_RC(tapi_allocate_port(rpcs, &port));

    return port;
}

/* See description in tapi_sockaddr.h */
te_errno
tapi_allocate_port_range(struct rcf_rpc_server *pco,
                         uint16_t *p_port, int num)
{
    te_errno    rc;
    int         i;
    int         j;

    /** Try 3 times */
    for (i = 0; i < 3; i++)
    {
        uint16_t   *tmp = calloc(num, sizeof(*tmp));
        te_bool     ok = TRUE;

        for (j = 0; j < num; j++)
        {
            if ((rc = tapi_allocate_port(pco, &tmp[j])) != 0)
                return rc;

            /* check that new port is subsequent for previous */
            if (j > 0 && (tmp[j] - tmp[j - 1] != 1))
            {
                WARN("%s: Attempt: %d - allocated ports are "
                     "not subsequent: p[%d]: %u, p[%d]: %u",
                     __FUNCTION__, i + 1, j - 1, tmp[j - 1], j, tmp[j]);

                free(tmp);
                ok = FALSE;
                break;
            }
        }

        if (ok)
        {
            memcpy(p_port, tmp, num * sizeof(*tmp));
            free(tmp);
            return 0;
        }
    }

    return TE_EFAIL;
}


/* See description in tapi_sockaddr.h */
te_errno
tapi_allocate_port_htons(rcf_rpc_server *pco, uint16_t *p_port)
{
    uint16_t port;
    int      rc;

    if ((rc = tapi_allocate_port(pco, &port)) != 0)
        return rc;

    *p_port = htons(port);

    return 0;
}

/* See description in tapi_sockaddr.h */
struct sockaddr *
tapi_sockaddr_clone_typed(const struct sockaddr *addr,
                          tapi_address_type type)
{
    struct sockaddr_storage *res_addr = NULL;

    if (type == TAPI_ADDRESS_NULL)
        return NULL;

    res_addr = TE_ALLOC(sizeof(*res_addr));
    if (res_addr == NULL)
        TEST_STOP;

    tapi_sockaddr_clone_exact(addr, res_addr);

    if (type == TAPI_ADDRESS_WILDCARD ||
        type == TAPI_ADDRESS_WILDCARD_ZERO_PORT)
        te_sockaddr_set_wildcard(SA(res_addr));

    if (type == TAPI_ADDRESS_SPECIFIC_ZERO_PORT ||
        type == TAPI_ADDRESS_WILDCARD_ZERO_PORT)
    {
        uint16_t *port = te_sockaddr_get_port_ptr(SA(res_addr));
        *port = 0;
    }

    return SA(res_addr);
}

/* See description in tapi_sockaddr.h */
te_errno
tapi_allocate_set_port(rcf_rpc_server *rpcs, const struct sockaddr *addr)
{
    uint16_t *port_ptr;
    te_errno rc;

    if ((port_ptr = te_sockaddr_get_port_ptr(addr)) == NULL)
    {
        ERROR("Failed to get pointer to port");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_allocate_port_htons(rpcs, port_ptr);
    if (rc != 0)
        ERROR("Failed to allocate a free port: %r", rc);

    return rc;
}

/* See description in tapi_sockaddr.h */
te_errno
tapi_sockaddr_clone2(const struct sockaddr  *src,
                     struct sockaddr       **dst)
{
    assert(dst != NULL);
    if (src == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    *dst = TE_ALLOC(sizeof(struct sockaddr_storage));
    if (*dst == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    memcpy(*dst, src, te_sockaddr_get_size(src));
    return 0;
}
