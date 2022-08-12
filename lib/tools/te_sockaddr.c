/** @file
 * @brief Functions to opearate with generic "struct sockaddr"
 *
 * Implementation of API for working with struct sockaddr.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

//#ifndef __CYGWIN__

#define TE_LGR_USER      "SockAddr"

#include "te_config.h"

#ifndef __CYGWIN__

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
#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#else /* __CYGWIN__ */

#ifndef WINDOWS
#include "winsock2.h"
#include "mswsock.h"
#include "ws2tcpip.h"
#undef ERROR
#else
INCLUDE(te_win_defs.h)
#endif
#define _NETINET_IN_H 1

extern int inet_pton (int, const char *, void *);
extern const char *inet_ntop (int, const void *, char *, size_t);

#endif /* __CYGWIN__ */

#include "logger_api.h"
#include "te_sockaddr.h"
#include "te_ethernet.h"
#include "te_string.h"
#include "te_errno.h"


/* See the description in te_sockaddr.h */
void
te_sockaddr_clear_port(struct sockaddr *addr)
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
te_sockaddr_get_port_ptr(const struct sockaddr *addr)
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

/* See the description in te_sockaddr.h */
void
te_sockaddr_set_port(struct sockaddr *addr, uint16_t port)
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

/* See the description in te_sockaddr.h */
void *
te_sockaddr_get_netaddr(const struct sockaddr *addr)
{
    if (addr == NULL)
        return NULL;

    switch (addr->sa_family)
    {
        case AF_INET:
            return &(SIN(addr)->sin_addr);
            break;

        case AF_INET6:
            return &(SIN6(addr)->sin6_addr);
            break;

        case AF_LOCAL:
            return (void *)(addr->sa_data);

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }
    return NULL;
}

/* See the description in te_sockaddr.h */
int
te_sockaddr_set_netaddr(struct sockaddr *addr, const void *net_addr)
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

        case AF_LOCAL:
            memcpy(addr->sa_data, net_addr, ETHER_ADDR_LEN);
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            return -1;
    }

    return 0;
}

/* See the description in te_sockaddr.h */
void
te_sockaddr_set_wildcard(struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            SIN(addr)->sin_addr.s_addr = htonl(INADDR_ANY);
            break;

        case AF_INET6:
#if 1
        {
            char buf[16] = { 0, };
            memcpy(&(SIN6(addr)->sin6_addr), buf, sizeof(buf));
        }
#else
            memcpy(&(SIN6(addr)->sin6_addr), &in6addr_any,
                   sizeof(in6addr_any));
#endif
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }
}

/* See the description in te_sockaddr.h */
void
te_sockaddr_set_loopback(struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            SIN(addr)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            break;

        case AF_INET6:
        {
            char buf[16] = { 0, };
            buf[15] = 1;
            memcpy(&(SIN6(addr)->sin6_addr), buf, sizeof(buf));
        }
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }
}

/* See the description in te_sockaddr.h */
void
te_sockaddr_set_multicast(struct sockaddr *addr)
{
    unsigned int i;

    switch (addr->sa_family)
    {
        case AF_INET:
            SIN(addr)->sin_addr.s_addr =
                htonl(rand_range(0xe0000100, 0xefffffff));
            break;

        case AF_INET6:
            SIN6(addr)->sin6_addr.s6_addr[0] = 0xff;
            SIN6(addr)->sin6_addr.s6_addr[1] = 0x0e;

            for (i = 2; i < sizeof(SIN6(addr)->sin6_addr.s6_addr); i++)
            {
                SIN6(addr)->sin6_addr.s6_addr[i] = rand_range(0x00, 0xff);
            }

            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            break;
    }
}

/* See the description in te_sockaddr.h */
te_bool
te_sockaddr_is_wildcard(const struct sockaddr *addr)
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

/* See the description in te_sockaddr.h */
te_bool
te_sockaddr_is_multicast(const struct sockaddr *addr)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            return (IN_MULTICAST(ntohl(SIN(addr)->sin_addr.s_addr)));

        case AF_INET6:
            return (IN6_IS_ADDR_MULTICAST(&(SIN6(addr)->sin6_addr)));

        case AF_LOCAL:
            return !!(addr->sa_data[0] & 1);

        default:
            ERROR("%s(): Address family %d is not supported, ",
                  "operation has no effect", __FUNCTION__, addr->sa_family);
            return FALSE;
    }
}


/* See the description in te_sockaddr.h */
size_t
te_netaddr_get_size(int af)
{
    switch (af)
    {
        case AF_INET:
            return sizeof(struct in_addr);

        case AF_INET6:
            return sizeof(struct in6_addr);

        case AF_LOCAL:
            return ETHER_ADDR_LEN;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, (int)af);
    }

    return 0;
}

/* See the description in te_sockaddr.h */
size_t
te_netaddr_get_bitsize(int af)
{
    return te_netaddr_get_size(af) * 8;
}

/* See the description in te_sockaddr.h */
size_t
te_sockaddr_get_size_by_af(int af)
{
    switch (af)
    {
        case AF_INET:
            return sizeof(struct sockaddr_in);

        case AF_INET6:
            return sizeof(struct sockaddr_in6);

        case AF_LOCAL:
            return sizeof(struct sockaddr);

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, (int)af);
    }

    return 0;
}

/* See the description in te_sockaddr.h */
size_t
te_sockaddr_get_size(const struct sockaddr *addr)
{
#if HAVE_STRUCT_SOCKADDR_SA_LEN
    if (addr->sa_len > 0)
        return addr->sa_len;
#endif
    return te_sockaddr_get_size_by_af(addr->sa_family);
}

/* See the description in te_sockaddr.h */
te_errno
te_sockaddr_mask_by_prefix(struct sockaddr *mask, socklen_t masklen,
                           int af, unsigned int prefix)
{
    size_t   max = te_netaddr_get_size(af);
    uint8_t *ptr;

    if (max == 0)
        return TE_EAFNOSUPPORT;
    if (prefix > (max << 3))
        return TE_EINVAL;

    memset(mask, 0, masklen);
    mask->sa_family = af;

    ptr = te_sockaddr_get_netaddr(mask);
    assert(ptr != NULL);
    if (masklen <
        (socklen_t)((ptr - (uint8_t *)mask) + ((prefix + 7) >> 3)))
    {
        return TE_ESMALLBUF;
    }

    memset(ptr, 0xff, prefix >> 3);
    if (prefix & 7)
        ptr[prefix >> 3] = 0xff << (8 - (prefix & 7));

    return 0;
}

/* See description in te_sockaaddr.h */
te_errno
te_sockaddr_cleanup_to_prefix(struct sockaddr *addr, unsigned int prefix)
{
    switch (addr->sa_family)
    {
        case AF_INET:
            if (prefix > (sizeof(struct in_addr) << 3))
            {
                ERROR("%s: Too long IPv4 prefix length %u",
                      __FUNCTION__, prefix);
                return TE_RC(TE_TAPI, TE_E2BIG);
            }
            SIN(addr)->sin_addr.s_addr =
                htonl(ntohl(SIN(addr)->sin_addr.s_addr) &
                      PREFIX2MASK(prefix));
            break;

        case AF_INET6:
        {
            unsigned int  i;
            uint8_t      *p = (uint8_t *)&SIN6(addr)->sin6_addr;

            if (prefix > (sizeof(struct in6_addr) << 3))
            {
                ERROR("%s: Too long IPv6 prefix length %u",
                      __FUNCTION__, prefix);
                return TE_RC(TE_TAPI, TE_E2BIG);
            }
            for (i = 0; i < sizeof(struct in6_addr); ++i)
            {
                if (prefix >= 8)
                {
                    prefix -= 8;
                }
                else
                {
                    *p &= ((~0u) << (8u - prefix));
                    prefix = 0;
                }
                p++;
            }
            break;
        }

        default:
            ERROR("%s: Address family %u is not supported",
                  __FUNCTION__, addr->sa_family);
            return TE_RC(TE_TAPI, TE_ENOSYS);
    }
    return 0;
}


/* See description in tapi_sockaaddr.h */
int
te_sockaddrcmp(const struct sockaddr *a1, socklen_t a1len,
               const struct sockaddr *a2, socklen_t a2len)
{
    if (a1 != NULL && a2 != NULL &&
        a1->sa_family == a2->sa_family)
    {
        switch (a1->sa_family)
        {
            case AF_INET:
            {
                if (a1len < (socklen_t)sizeof(struct sockaddr_in) ||
                    a2len < (socklen_t)sizeof(struct sockaddr_in))
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
                if (a1len < (socklen_t)sizeof(struct sockaddr_in6) ||
                    a2len < (socklen_t)sizeof(struct sockaddr_in6))
                {
                    ERROR("One of sockaddr structures is shorter "
                          "than it should be");
                    return -2;
                }

                /** All fields of struct sockaddr_in6 should be compared
                 * except sin6_flowinfo, because it changes during
                 * sequence: bind(), connect(), getsockname().
                 */
                if ((SIN6(a1)->sin6_port == SIN6(a2)->sin6_port) &&
                    (memcmp(&(SIN6(a1)->sin6_addr), &(SIN6(a2)->sin6_addr),
                            sizeof(SIN6(a1)->sin6_addr)) == 0) &&
                    (SIN6(a1)->sin6_scope_id == SIN6(a2)->sin6_scope_id))
                {
                    return 0;
                }
                break;
            }

            case AF_LOCAL:
                if (a1len < (socklen_t)sizeof(struct sockaddr) ||
                    a2len < (socklen_t)sizeof(struct sockaddr))
                {
                    ERROR("One of sockaddr structures is shorter "
                          "than it should be");
                    return -2;
                }
                if (memcmp(a1->sa_data, a2->sa_data, ETHER_ADDR_LEN) == 0)
                    return 0;
                break;

            default:
                ERROR("Comparison of addresses with unsupported "
                      "family %d", a1->sa_family);
                return -2;
        }
    }
    return -1;
}

/* See description in tapi_sockaddr.h */
int
te_sockaddrcmp_no_ports(const struct sockaddr *a1, socklen_t a1len,
                        const struct sockaddr *a2, socklen_t a2len)
{
    struct sockaddr_storage a1_copy;
    struct sockaddr_storage a2_copy;

    memcpy(&a1_copy, a1, a1len);
    memcpy(&a2_copy, a2, a2len);

    te_sockaddr_clear_port(SA(&a1_copy));
    te_sockaddr_clear_port(SA(&a2_copy));

    return te_sockaddrcmp(SA(&a1_copy), a1len,
                          SA(&a2_copy), a2len);
}

/* See description in tapi_sockaaddr.h */
int
te_sockaddrncmp(const struct sockaddr *a1, socklen_t a1len,
                const struct sockaddr *a2, socklen_t a2len)
{
    socklen_t min_len = ((a1len < a2len) ? a1len : a2len);

    if (min_len == 0)
    {
        RING("%s(): Addresses length to compare is 0", __FUNCTION__);
        return 0;
    }

    if (a1 == NULL)
    {
        ERROR("%s(): The first address is NULL, but its length is not zero",
              __FUNCTION__);
    }
    if (a2 == NULL)
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
        socklen_t   i;                                                  \
                                                                        \
        if ((socklen_t)((uint8_t *)(field_addr_) - (uint8_t *)a1) >=    \
                min_len)                                                \
        {                                                               \
            RING("No one byte of '" field_name_ "' field can be "       \
                 "compared");                                           \
            break;                                                      \
        }                                                               \
        for (i = ((uint8_t *)(field_addr_) - (uint8_t *)a1);            \
             i < (socklen_t)(((uint8_t *)(field_addr_) -                \
                              (uint8_t *)a1) + (field_size_)) &&        \
             i < min_len;                                               \
             i++)                                                       \
        {                                                               \
            if (((uint8_t *)a1)[i] != ((uint8_t *)a2)[i])               \
            {                                                           \
                return -1;                                              \
            }                                                           \
        }                                                               \
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

            CMP_FIELD("sin_port", &(in_a1->sin_port),
                      sizeof(in_a1->sin_port));
            CMP_FIELD("sin_addr", &(in_a1->sin_addr),
                      sizeof(in_a1->sin_addr));

            /* We do not compare padding field ('sin_zero') */

            return 0;
        }

        case AF_INET6:
        {
            struct sockaddr_in6 *in_a1 = SIN6(a1);

            /*
             * AF_INET6 address contains port, flowinfo,
             * IPv6 address and scope_id
             */

            CMP_FIELD("sin6_port", &(in_a1->sin6_port),
                      sizeof(in_a1->sin6_port));
            CMP_FIELD("sin6_flowinfo", &(in_a1->sin6_flowinfo),
                      sizeof(in_a1->sin6_flowinfo));
            CMP_FIELD("sin6_addr", &(in_a1->sin6_addr),
                      sizeof(in_a1->sin6_addr));
            CMP_FIELD("sin6_scope_id", &(in_a1->sin6_scope_id),
                      sizeof(in_a1->sin6_scope_id));

            return 0;
        }

        default:
            ERROR("Comparison of addresses with unsupported "
                  "family %d", a1->sa_family);
            return -2;
    }

#undef CMP_FIELD

    return -1;
}

/* See description in tapi_sockaddr.h */
te_errno
te_sockaddr2str_buf(const struct sockaddr *sa,
                    char *buf, size_t len)
{
    const void *addr_ptr;
    char        addr_buf[INET6_ADDRSTRLEN];
    uint16_t    port;

    te_string   str = TE_STRING_EXT_BUF_INIT(buf, len);

    if (sa == NULL)
    {
        CHECK_NZ_RETURN(te_string_append(&str, "(nil)"));
        return 0;
    }

    if (!te_sockaddr_is_af_supported(sa->sa_family))
    {
        if (sa->sa_family == AF_UNSPEC)
        {
            int i;

            CHECK_NZ_RETURN(te_string_append(
                              &str,
                              "<Address family is AF_UNSPEC raw value="));
            for (i = 0; i < (int)sizeof(*sa); i++)
            {
                CHECK_NZ_RETURN(te_string_append(
                                    &str, "%2.2x",
                                    *((uint8_t *)sa + i)));
            }
            CHECK_NZ_RETURN(te_string_append(&str, ">"));

            return 0;
        }
        else
        {
            return TE_RC(TE_TOOL_EXT, TE_EAFNOSUPPORT);
        }
    }

#ifdef AF_LOCAL
    if (sa->sa_family == AF_LOCAL)
    {
        CHECK_NZ_RETURN(te_string_append(&str, "%s", SUN(sa)->sun_path));
        return 0;
    }
#endif

    addr_ptr = te_sockaddr_get_netaddr(sa);
    assert(addr_ptr != NULL);

    port = te_sockaddr_get_port(sa);

    if (inet_ntop(sa->sa_family, addr_ptr,
                  addr_buf, sizeof(addr_buf)) == NULL)
    {
        return TE_OS_RC(TE_TOOL_EXT, errno);
    }
    CHECK_NZ_RETURN(te_string_append(&str, "%s:%hu",
                                     addr_buf, ntohs(port)));

    if (sa->sa_family == AF_INET6 &&
        IN6_IS_ADDR_LINKLOCAL(&SIN6(sa)->sin6_addr))
    {

        CHECK_NZ_RETURN(te_string_append(
                            &str, "<%u>",
                            (unsigned)(SIN6(sa)->sin6_scope_id)));
    }

    return 0;
}

/* See description in tapi_sockaddr.h */
const char *
te_sockaddr2str(const struct sockaddr *sa)
{
/* Number of buffers used in the function */
#define N_BUFS 10

    static char  buf[N_BUFS][TE_SOCKADDR_STR_LEN];
    static int   i = -1;
    te_errno     rc;

    i++;
    if (i >= N_BUFS)
        i = 0;

    rc = te_sockaddr2str_buf(sa, buf[i], TE_SOCKADDR_STR_LEN);
    if (rc != 0)
    {
        ERROR("%s(): te_sockaddr2str_buf() returned %r", rc);
        return "<Failed to convert address to string>";
    }

    return buf[i];
#undef N_BUFS
}

/* See the description in te_sockaddr.h */
te_errno
te_sockaddr_h2str_buf(const struct sockaddr *sa, char *buf, size_t len)
{
    const char *result = NULL;

    switch(sa->sa_family)
    {
        case AF_INET:
        case AF_INET6:
            result = inet_ntop(sa->sa_family,
                               sa->sa_family == AF_INET ?
                               (const void *)&(CONST_SIN (sa)->sin_addr ) :
                               (const void *)&(CONST_SIN6(sa)->sin6_addr),
                               buf, len);
            break;

        default:
            return TE_RC(TE_TOOL_EXT, TE_EAFNOSUPPORT);
    }

    if (result == NULL)
        return TE_OS_RC(TE_TOOL_EXT, errno);

    return 0;
}

/* See the description in te_sockaddr.h */
te_errno
te_sockaddr_h2str(const struct sockaddr *sa, char **string)
{
    char      buf[INET6_ADDRSTRLEN];
    te_errno  rc;
    char     *s;

    rc = te_sockaddr_h2str_buf(sa, buf, sizeof(buf));

    if (rc == 0)
    {
        s = strdup(buf);
        if (s == NULL)
            rc = TE_OS_RC(TE_TOOL_EXT, errno);
        else
            *string = s;
    }

    return rc;
}


/* See the description in te_sockaddr.h */
te_errno
te_sockaddr_str2h(const char *string, struct sockaddr *sa)
{
    int result;
    /*
     * It's an simple check on an address type (127.0.0.1 or ff01::01).
     * Then `inet_pton` function performs a full check and parse value.
     */
    if (strchr(string, ':') != NULL)
    {
        sa->sa_family = AF_INET6;
        result = inet_pton(AF_INET6, string, &SIN6(sa)->sin6_addr);
    }
    else
    {
        sa->sa_family = AF_INET;
        result = inet_pton(AF_INET, string, &SIN(sa)->sin_addr);
    }

    switch(result)
    {
        case  1: return 0;
        case -1: return TE_RC(TE_TOOL_EXT, TE_EAFNOSUPPORT);
        default: return TE_RC(TE_TOOL_EXT, TE_EINVAL);
    }
}


/* See the description in te_sockaddr.h */
void
te_mreq_set_mr_multiaddr(int af, void *mreq, const void *addr)
{
    switch (af)
    {
        case AF_INET:
            memcpy(&(((struct ip_mreq *)mreq)->imr_multiaddr), addr,
                   sizeof(struct in_addr));
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, (int)af);
    }
}

/* See the description in te_sockaddr.h */
void
te_mreq_set_mr_interface(int af, void *mreq, const void *addr)
{
    switch (af)
    {
        case AF_INET:
            memcpy(&(((struct ip_mreq *)mreq)->imr_interface), addr,
                   sizeof(struct in_addr));
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, (int)af);
    }
}

#if 0
/* See the description in te_sockaddr.h */
void
te_mreq_set_mr_ifindex(int af, void *mreq, int ifindex)
{
    switch (af)
    {
        case AF_INET:
            ((struct ip_mreqn *)mreq)->imr_ifindex = ifindex;
            break;

        default:
            ERROR("%s(): Address family %d is not supported, "
                  "operation has no effect", __FUNCTION__, (int)af);
    }
}
#endif

/* See description in te_sockaddr.h */
const char *
te_sockaddr_get_ipstr(const struct sockaddr *addr)
{
/* Number of buffers used in the function */
#define N_BUFS 10

    static char addr_buf[N_BUFS][INET6_ADDRSTRLEN];
    static char (*cur_buf)[INET6_ADDRSTRLEN] =
                                (char (*)[INET6_ADDRSTRLEN])addr_buf[0];

    char       *ptr;

    /*
     * Firt time the function is called we start from the second buffer,
     * but then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[INET6_ADDRSTRLEN])addr_buf[N_BUFS - 1])
        cur_buf = (char (*)[INET6_ADDRSTRLEN])addr_buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;

    return inet_ntop(addr->sa_family,
                     te_sockaddr_get_netaddr(addr), ptr,
                     INET6_ADDRSTRLEN);
#undef N_BUFS
}

/* See description in te_sockaddr.h */
char *
te_ip2str(const struct sockaddr *addr)
{
    char        buf[INET6_ADDRSTRLEN];
    const char *res;

    if (addr == NULL)
        return NULL;

    res = inet_ntop(addr->sa_family, te_sockaddr_get_netaddr(addr),
                    buf, sizeof(buf));
    if (res == NULL)
    {
        ERROR("Failed to convert address to string: %s", strerror(errno));
        return NULL;
    }

    return strdup(buf);
}

/* See description in te_sockaddr.h */
te_errno
te_ip_addr2te_str(te_string *str, const void *ip_addr, int af)
{
#define MAX_STR_LEN MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)
    char ip_addr_str[MAX_STR_LEN];
    const char *result;

    result = inet_ntop(af, ip_addr, ip_addr_str, MAX_STR_LEN);
    if (result == NULL)
        return te_rc_os2te(errno);

    return te_string_append(str, "%s", ip_addr_str);
#undef MAX_STR_LEN
}

/* See description in te_sockaddr.h */
te_errno
te_mac_addr2te_str(te_string *str, const uint8_t *mac_addr)
{
    if (mac_addr == NULL)
        return te_string_append(str, "<NULL>");

    return te_string_append(str, TE_PRINTF_MAC_FMT,
                            TE_PRINTF_MAC_VAL(mac_addr));
}

/* See description in te_sockaddr.h */
te_errno
te_sockaddr_netaddr_from_string(const char      *addr_str,
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

/* See description in te_sockaddr.h */
const char *
te_sockaddr_netaddr_to_string(int af, const void *net_addr)
{
    struct sockaddr_storage addr;

    addr.ss_family = af;
    if (te_sockaddr_set_netaddr(SA(&addr), net_addr) == 0)
        return te_sockaddr_get_ipstr(SA(&addr));

    ERROR("%s(): Failed to convert network address to sockaddr", __FUNCTION__);
    return NULL;
}

/* See description in te_sockaddr.h */
te_errno
te_sockaddr_ip4_to_ip6_mapped(struct sockaddr *addr)
{
    uint32_t    ip4_addr;
    uint16_t    port;

    if (addr->sa_family != AF_INET)
    {
        ERROR("Specified address is not IPv4 one");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    ip4_addr = (uint32_t)SIN(addr)->sin_addr.s_addr;
    port = SIN(addr)->sin_port;

    memset(addr, 0, sizeof(struct sockaddr_in6));

    SIN6(addr)->sin6_family = AF_INET6;
    SIN6(addr)->sin6_port = port;
    SIN6(addr)->sin6_addr.s6_addr32[3] = ip4_addr;
    SIN6(addr)->sin6_addr.s6_addr16[5] = htons(0xFFFF);

    return 0;
}

//#endif /* __CYGWIN__ */
