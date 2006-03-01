/** @file
 * @brief Functions to opearate with generic "struct sockaddr"
 *
 * Implementation of test API for working with struct sockaddr.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER      "TAPI SockAddr"

#include "te_config.h"

#include <stdio.h>
#if HAVE_STRING_H
#include <string.h>
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

#include "logger_api.h"
#include "conf_api.h"

#include "tapi_sockaddr.h"


/* See the description in tapi_sockaddr.h */
void
sockaddr_clear_port(struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            SIN(addr)->sin_port = 0;
            break;

        case AF_INET6:
            SIN6(addr)->sin6_port = 0;
            break;
            
        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }
}

/* See the description in sockapi-ts.h */
uint16_t *
sockaddr_get_port_ptr(const struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            return &SIN(addr)->sin_port;

        case AF_INET6:
            return &SIN6(addr)->sin6_port;
            
        default:
            ERROR("%s(): Address family %d is not supported",
                  __FUNCTION__, addr->sa_family);
            return NULL;
    }
}

/* See the description in tapi_sockaddr.h */
void
sockaddr_set_port(struct sockaddr *addr, uint16_t port)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            SIN(addr)->sin_port = port;
            break;

        case AF_INET6:
            SIN6(addr)->sin6_port = port;
            break;
            
        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }    
}

/* See the description in tapi_sockaddr.h */
const void *
sockaddr_get_netaddr(const struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            return &(SIN(addr)->sin_addr);
            break;

        case AF_INET6:
            return &(SIN6(addr)->sin6_addr);
            break;
            
        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }
    return NULL;    
}

/* See the description in tapi_sockaddr.h */
int
sockaddr_set_netaddr(struct sockaddr *addr, const void *net_addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            memcpy(&(SIN(addr)->sin_addr), net_addr,
                   sizeof(SIN(addr)->sin_addr));
            break;

        case AF_INET6:
            memcpy(&(SIN6(addr)->sin6_addr), net_addr,
                   sizeof(SIN6(addr)->sin6_addr));
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            return -1;
    }

    return 0;
}

/* See the description in tapi_sockaddr.h */
void
sockaddr_set_wildcard(struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            SIN(addr)->sin_addr.s_addr = htonl(INADDR_ANY);
            break;

        case AF_INET6:
            memcpy(&(SIN6(addr)->sin6_addr), &in6addr_any,
                   sizeof(in6addr_any));
            break;
            
        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }
}

/* See the description in tapi_sockaddr.h */
te_bool
sockaddr_is_wildcard(const struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            return (SIN(addr)->sin_addr.s_addr == htonl(INADDR_ANY));

        case AF_INET6:
            return (IN6_IS_ADDR_UNSPECIFIED(&(SIN6(addr)->sin6_addr)));
            
        default:
            ERROR("%s(): Address family %d is not supported, ",
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            return FALSE;
    }
}

/* See the description in tapi_sockaddr.h */
te_bool
sockaddr_is_multicast(const struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            return (IN_MULTICAST(ntohl(SIN(addr)->sin_addr.s_addr)));

        case AF_INET6:
            return (IN6_IS_ADDR_MULTICAST(&(SIN6(addr)->sin6_addr)));

        default:
            ERROR("%s(): Address family %d is not supported, ",
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            return FALSE;
    }
}

/* See the description in tapi_sockaddr.h */
int
sockaddr_get_size_by_af(int af)
{
    switch (af)
    {
        case AF_INET:
            return sizeof(struct sockaddr_in);

        case AF_INET6:
            return sizeof(struct sockaddr_in6);
            
        default:
            ERROR("%s(): Address family %d is not supported, ",
                  "operation has no effect", __FUNCTION__, af);
    }

    return 0;
}

/* See the description in tapi_sockaddr.h */
int
sockaddr_get_size(const struct sockaddr *addr)
{
    return sockaddr_get_size_by_af(addr->sa_family);
}

/* See description in tapi_sockaaddr.h */
int
sockaddrcmp(const struct sockaddr *a1, socklen_t a1len,
            const struct sockaddr *a2, socklen_t a2len)
{
    if (a1 != NULL && a2 != NULL &&
        a1->sa_family == a2->sa_family)
    {
        switch (a1->sa_family)
        {
            case AF_INET:
            {
                if (a1len < sizeof(struct sockaddr_in) ||
                    a2len < sizeof(struct sockaddr_in))
                {
                    ERROR("One of sockaddr structures is shorter "
                          "than it should be");
                    return -2;
                }

                if ((SIN(a1)->sin_port == SIN(a2)->sin_port) &&
                    (memcmp(&(SIN(a1)->sin_addr), &(SIN(a2)->sin_addr),
                            sizeof(SIN(a1)->sin_addr)) == 0))
                {
                    return 0;
                }
                break;
            }

            case AF_INET6:
            {
                if (a1len < sizeof(struct sockaddr_in6) ||
                    a2len < sizeof(struct sockaddr_in6))
                {
                    ERROR("One of sockaddr structures is shorter "
                          "than it should be");
                    return -2;
                }

                if ((SIN6(a1)->sin6_port == SIN6(a2)->sin6_port) &&
                    (memcmp(&(SIN6(a1)->sin6_addr), &(SIN6(a2)->sin6_addr),
                            sizeof(SIN6(a1)->sin6_addr)) == 0))
                {
                    return 0;
                }
                break;
            }

            default:
                ERROR("Comparison of addresses with unsupported "
                      "family %d", a1->sa_family);
                return -2;
        }
    }
    return -1;
}

/* See description in tapi_sockaaddr.h */
int
sockaddrncmp(const struct sockaddr *a1, socklen_t a1len,
             const struct sockaddr *a2, socklen_t a2len)
{
    socklen_t min_len = ((a1len < a2len) ? a1len : a2len);

    if (a1 == NULL && a1len != 0)
    {
        ERROR("%s(): The first address is NULL, but its length is not zero",
              __FUNCTION__);
    }
    if (a2 == NULL && a2len != 0)
    {
        ERROR("%s(): The second address is NULL, but its length is not "
              "zero", __FUNCTION__);
    }

    if (a1 == NULL && a2 == NULL)
    {
        RING("%s(): Both addresses are NULL", __FUNCTION__);
        return 0;
    }

    if ((a1 == NULL && a2 != NULL) || (a2 == NULL && a1 != NULL))
    {
        RING("%s(): The %s address is NULL", __FUNCTION__,
             (a1 == NULL) ? "first" : "second");
        return -1;
    }

    /* Compare 'sa_family' field */

#define CMP_FIELD(field_name_, field_addr_, field_size_) \
    do {                                                                \
        unsigned int i;                                                 \
                                                                        \
        if ((socklen_t)((uint8_t *)(field_addr_) - (uint8_t *)a1) >=    \
                min_len)                                                \
        {                                                               \
            RING("No one byte of '" field_name_ "' field can be "       \
                 "compared");                                           \
            return 0;                                                   \
        }                                                               \
        for (i = ((uint8_t *)(field_addr_) - (uint8_t *)a1);            \
             i < (((uint8_t *)(field_addr_) - (uint8_t *)a1) +          \
                     (field_size_)) &&                                  \
             i < min_len;                                               \
             i++)                                                       \
        {                                                               \
            if (((uint8_t *)a1)[i] != ((uint8_t *)a2)[i])               \
            {                                                           \
                return -1;                                              \
            }                                                           \
        }                                                               \
        if (i == min_len)                                               \
            return 0;                                                   \
    } while (0)

    CMP_FIELD("sa_family", &(a1->sa_family), sizeof(a1->sa_family));

    switch (a1->sa_family)
    {
        case AF_INET:
        {
            struct sockaddr_in *in_a1 = SIN(a1);

            /*
             * AF_INET address contains port and IPv4 address
             */
            if ((size_t)&(in_a1->sin_port) < (size_t)&(in_a1->sin_addr))
            {
                /* Compare port first */
                CMP_FIELD("sin_port", &(in_a1->sin_port),
                          sizeof(in_a1->sin_port));
                CMP_FIELD("sin_addr", &(in_a1->sin_addr),
                          sizeof(in_a1->sin_addr));
            }
            else
            {
                /* Compare address first */
                CMP_FIELD("sin_addr", &(in_a1->sin_addr),
                          sizeof(in_a1->sin_addr));
                CMP_FIELD("sin_port", &(in_a1->sin_port),
                          sizeof(in_a1->sin_port));
            }

            /* We do not compare padding field ('sin_zero') */

            return 0;

            break;
        }

        case AF_INET6:
        {
            /*
             * AF_INET6 address contains port, flowinfo, 
             * IPv6 address and scope_id
             */
            
            /* Pass through: Not supported yet */
        }

        default:
            ERROR("Comparison of addresses with unsupported "
                  "family %d", a1->sa_family);
            return -2;
    }

    return -1;
}

/* See description in tapi_sockaaddr.h */
const char *
sockaddr2str(const struct sockaddr *sa)
{

#define SOCKADDR2STR_ADDRSTRLEN 128
/* Number of buffers used in the function */
#define N_BUFS 10

    static char  buf[N_BUFS][SOCKADDR2STR_ADDRSTRLEN];
    static char  (*cur_buf)[SOCKADDR2STR_ADDRSTRLEN] = 
                                (char (*)[SOCKADDR2STR_ADDRSTRLEN])buf[0];

    char       *ptr;
    const void *addr_ptr;
    char        addr_buf[INET6_ADDRSTRLEN];
    uint16_t    port;

    /*
     * Firt time the function is called we start from the second buffer,
     * but then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[SOCKADDR2STR_ADDRSTRLEN])buf[N_BUFS - 1])
        cur_buf = (char (*)[SOCKADDR2STR_ADDRSTRLEN])buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;

    if (sa == NULL)
    {
        snprintf(ptr, SOCKADDR2STR_ADDRSTRLEN, "(nil)");
        return ptr;
    }

    if (!sockaddr_is_af_supported(sa->sa_family))
    {
        return "<Not supported address family>";
    }

    addr_ptr = sockaddr_get_netaddr(sa);
    assert(addr_ptr != NULL);

    port = sockaddr_get_port(sa);

    if (inet_ntop(sa->sa_family, addr_ptr,
                  addr_buf, sizeof(addr_buf)) == NULL)
    {
        return "<Cannot convert network address>";
    }
    snprintf(ptr, SOCKADDR2STR_ADDRSTRLEN, "%s:%hu", addr_buf, ntohs(port));
    if (sa->sa_family == AF_INET6 &&
        IN6_IS_ADDR_LINKLOCAL(&SIN6(sa)->sin6_addr))
    {
        size_t len = strlen(ptr);

        snprintf(ptr + len, SOCKADDR2STR_ADDRSTRLEN - len,
                 "<%u>", (unsigned)(SIN6(sa)->sin6_scope_id));
    }

#undef N_BUFS
#undef SOCKADDR2STR_ADDRSTRLEN

    return ptr;
}

/* See the description in tapi_sockaddr.h */
int
netaddr_get_size(int addr_family)
{
    switch (addr_family)
    {
        case AF_INET:
            return sizeof(struct in_addr);

        case AF_INET6:
            return sizeof(struct in6_addr);

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr_family);
    }

    return -1;
}

void
mreq_set_mr_multiaddr(int addr_family, void *mreq, const void *addr)
{
    switch (addr_family)
    {
        case AF_INET:
            memcpy(&(((struct ip_mreq *)mreq)->imr_multiaddr), addr,
                   sizeof(struct in_addr));
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr_family);
    }
}

void
mreq_set_mr_interface(int addr_family, void *mreq, const void *addr)
{
    switch (addr_family)
    {
        case AF_INET:
            memcpy(&(((struct ip_mreq *)mreq)->imr_interface), addr,
                   sizeof(struct in_addr));
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr_family);
    }
}

void
mreq_set_mr_ifindex(int addr_family, void *mreq, int ifindex)
{
    switch (addr_family)
    {
        case AF_INET:
            ((struct ip_mreqn *)mreq)->imr_ifindex = ifindex;
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr_family);
    }
}

/* See description in tapi_env.h */
te_errno
tapi_allocate_port(uint16_t *p_port)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    int             rc;
    cfg_val_type    val_type;
    int             port;
    
    pthread_mutex_lock(&mutex);

    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type, &port,
                              "/volatile:/sockaddr_port:");
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
    if ((port < 20000) || (port >= 30000))
    {
        /* Random numbers generator should be initialized earlier */
        port = 20000 + rand_range(0, 10000);
    }
    else
    {
        port++;
    }
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
te_errno
sockaddr_netaddr_from_string(const char      *addr_str,
                             struct sockaddr *addr)
{
    if (addr_str == NULL || addr == NULL)
        return TE_RC(TE_TAPI, TE_EFAULT);

    if (inet_pton(AF_INET, addr_str, &SIN(addr)->sin_addr) > 0)
    {
        addr->sa_family = AF_INET;
        return 0;
    }
    if (inet_pton(AF_INET6, addr_str, &SIN6(addr)->sin6_addr) > 0)
    {
        addr->sa_family = AF_INET6;
        return 0;
    }
    return TE_RC(TE_TAPI, TE_EINVAL);
}

te_errno
sockaddr_ip4_to_ip6_mapped(struct sockaddr *addr)
{
    uint32_t    ip4_addr;
    uint16_t    port;
    
    if (addr->sa_family != AF_INET)
    {
        ERROR("Specified address is not IPv4 one");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    ip4_addr = (uint32_t)SIN(addr)->sin_addr.s_addr;
    port = (uint32_t)SIN(addr)->sin_port;
    
    memset(addr, 0, sizeof(struct sockaddr_in6));

    SIN6(addr)->sin6_family = AF_INET6;
    SIN6(addr)->sin6_port = port;
    SIN6(addr)->sin6_addr.s6_addr32[3] = ip4_addr;
    SIN6(addr)->sin6_addr.s6_addr16[5] = htons(0xFFFF);
    return 0;
}

