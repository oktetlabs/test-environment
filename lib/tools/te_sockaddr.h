/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Functions to opearate generic "struct sockaddr"
 *
 * @defgroup te_tools_te_sockaddr Sockaddr
 * @ingroup te_tools
 * @{
 *
 * Definition of API for working with struct sockaddr.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TOOLS_SOCKADDR_H__
#define __TE_TOOLS_SOCKADDR_H__

#ifndef __CYGWIN__
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fast conversion of IPv4 the network mask to prefix */
#define MASK2PREFIX(mask, prefix)            \
    switch (mask)                            \
    {                                        \
        case 0x0: prefix = 0; break;         \
        case 0x80000000: prefix = 1; break;  \
        case 0xc0000000: prefix = 2; break;  \
        case 0xe0000000: prefix = 3; break;  \
        case 0xf0000000: prefix = 4; break;  \
        case 0xf8000000: prefix = 5; break;  \
        case 0xfc000000: prefix = 6; break;  \
        case 0xfe000000: prefix = 7; break;  \
        case 0xff000000: prefix = 8; break;  \
        case 0xff800000: prefix = 9; break;  \
        case 0xffc00000: prefix = 10; break; \
        case 0xffe00000: prefix = 11; break; \
        case 0xfff00000: prefix = 12; break; \
        case 0xfff80000: prefix = 13; break; \
        case 0xfffc0000: prefix = 14; break; \
        case 0xfffe0000: prefix = 15; break; \
        case 0xffff0000: prefix = 16; break; \
        case 0xffff8000: prefix = 17; break; \
        case 0xffffc000: prefix = 18; break; \
        case 0xffffe000: prefix = 19; break; \
        case 0xfffff000: prefix = 20; break; \
        case 0xfffff800: prefix = 21; break; \
        case 0xfffffc00: prefix = 22; break; \
        case 0xfffffe00: prefix = 23; break; \
        case 0xffffff00: prefix = 24; break; \
        case 0xffffff80: prefix = 25; break; \
        case 0xffffffc0: prefix = 26; break; \
        case 0xffffffe0: prefix = 27; break; \
        case 0xfffffff0: prefix = 28; break; \
        case 0xfffffff8: prefix = 29; break; \
        case 0xfffffffc: prefix = 30; break; \
        case 0xfffffffe: prefix = 31; break; \
        case 0xffffffff: prefix = 32; break; \
         /* Error indication */              \
        default: prefix = 33; break;         \
    }

/** Fast conversion of the IPv4 prefix to network mask */
#define PREFIX2MASK(prefix) \
    (prefix == 0 ? 0U : (~0U) << (unsigned)(32 - (prefix)))

/**
 * Number of bytes which should be enough for string representation
 * of sockaddr structure
 */
#define TE_SOCKADDR_STR_LEN 300

/**
 * Is address family is supported by this TAPI?
 *
 * @param af        Address family
 */
static inline te_bool
te_sockaddr_is_af_supported(int af)
{
    return (af == AF_INET) || (af == AF_INET6) || (af == AF_LOCAL);
}

/**
 * Set "port" part of corresponding struct sockaddr to zero (wildcard)
 *
 * @param addr  Generic address structure
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
extern void te_sockaddr_clear_port(struct sockaddr *addr);

/**
 * Get pointer to "port" part of corresponding struct sockaddr.
 *
 * @param addr  Generic address structure
 *
 * @return Pointer to "port" part
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
extern uint16_t *te_sockaddr_get_port_ptr(const struct sockaddr *addr);

/**
 * Get "port" part of corresponding struct sockaddr.
 *
 * @param addr  Generic address structure
 *
 * @return Port number in network byte order
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
static inline uint16_t
te_sockaddr_get_port(const struct sockaddr *addr)
{
    uint16_t *port = te_sockaddr_get_port_ptr(addr);

    return (port != NULL) ? *port : 0;
}

/**
 * Update "port" part of corresponding struct sockaddr.
 *
 * @param addr  Generic address structure
 * @param port  A new port value (port should be in network byte order)
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
extern void te_sockaddr_set_port(struct sockaddr *addr, uint16_t port);

/**
 * Returns pointer to network address part of sockaddr structure according
 * to 'sa_family' field of the structure.
 *
 * @note If @p addr is @c NULL, this function simply returns @c NULL.
 *
 * @param addr  Generic address structure
 *
 * @return Pointer to corresponding network address or @c NULL.
 */
extern void *te_sockaddr_get_netaddr(const struct sockaddr *addr);

/**
 * Returns pointer to the IP 4/6 address in human representation
 * (without port).
 *
 * @param addr  address to be converted
 *
 * @return address string
 *
 * @note non-rentarable
 * @note non-thread-safe
 * @note does not perform any checks
 *
 * @sa te_ip2str, te_sockaddr2str
 */
extern const char * te_sockaddr_get_ipstr(const struct sockaddr *addr);

/**
 * Convert IPv4 and IPv6 addresses from binary to text form.
 *
 * @note Return value should be freed with free(3) when it is no longer needed.
 *
 * @param addr      IP address.
 *
 * @return String representation of IP address, or @c NULL in case of error.
 *
 * @sa te_sockaddr_get_ipstr, te_sockaddr2str
 */
extern char *te_ip2str(const struct sockaddr *addr);

/**
 * Append IPv4 or IPv6 address to TE string.
 *
 * @param str            Pointer to TE string
 * @param ip_addr        Pointer to in_addr or in6_addr
 * @param af             @c AF_INET or @c AF_INET6
 *
 * @return Status code.
 */
extern te_errno te_ip_addr2te_str(te_string *str,
                                  const void *ip_addr,
                                  int af);

/**
 * Append MAC address to TE string.
 *
 * @param str       Pointer to the TE string
 * @param mac_addr  Pointer to the MAC address
 *
 * @return Status code.
 */
extern te_errno te_mac_addr2te_str(te_string *str,
                                   const uint8_t *mac_addr);

/**
 * Update network address part of sockaddr structure according to
 * 'sa_family' field of the structure
 *
 * @param addr      Generic address structure
 * @param net_addr  Pointer to the network address to be set
 *
 * @return Result of the operation
 *
 * @retval  0  on success
 * @retval -1  on failure
 */
extern int te_sockaddr_set_netaddr(struct sockaddr *addr,
                                   const void      *net_addr);

/**
 * Set "network address" part of corresponding struct sockaddr to wildcard
 *
 * @param addr  Generic address structure
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
extern void te_sockaddr_set_wildcard(struct sockaddr *addr);

/**
 * Set "network address" part of corresponding struct sockaddr to loopback
 *
 * @param addr  Generic address structure
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
extern void te_sockaddr_set_loopback(struct sockaddr *addr);

/**
 * Set "network address" part of corresponding struct sockaddr to randomly
 * chosen multicast address.
 *
 * @param addr  Generic address structure
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for an address.
 */
extern void te_sockaddr_set_multicast(struct sockaddr *addr);

/**
 * Check if "network address" part of corresponding struct sockaddr is
 * wildcard
 *
 * @param addr  Generic address structure
 *
 * @return Is "network address" part wildcard?
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
extern te_bool te_sockaddr_is_wildcard(const struct sockaddr *addr);

/**
 * Check if "network address" part of corresponding struct sockaddr is
 * multicast address
 *
 * @param addr Generic address struture
 *
 * @return TRUE/FALSE - multicast/other address
 *
 */
extern te_bool te_sockaddr_is_multicast(const struct sockaddr *addr);

/**
 * Returns the size of a particular sockaddr structure according to
 * its family.
 *
 * @param af    Address family
 *
 * @return Address size
 */
extern size_t te_sockaddr_get_size_by_af(int af);

/**
 * Returns the size of a particular sockaddr structure according to
 * 'sa_family' field of the structure
 *
 * @param addr   Generic address structure
 *
 * @return Address size
 */
extern size_t te_sockaddr_get_size(const struct sockaddr *addr);

/**
 * Create mask in 'struct sockaddr' by prefix length.
 *
 * @param mask      Location for mask
 * @param masklen   Size of the memory for mask
 * @param af        Address family
 * @param prefix    Prefix length
 *
 * @return Status code.
 */
extern te_errno te_sockaddr_mask_by_prefix(struct sockaddr *mask,
                                           socklen_t masklen,
                                           int af, unsigned int prefix);

/**
 * Clean up network address part to be prefix of specified length.
 *
 * @param addr      address
 * @param prefix    prefix length
 *
 * @return Status code.
 */
extern te_errno te_sockaddr_cleanup_to_prefix(struct sockaddr *addr,
                                              unsigned int prefix);

/**
 * Compare 'struct sockaddr'.
 *
 * @param a1        the first address
 * @param a1len     the first address length
 * @param a2        the second address
 * @param a2len     the second address length
 *
 * @retval 0        equal
 * @retval -1       not equal
 * @retval -2       comparison of addresses of unsupported family
 */
extern int te_sockaddrcmp(const struct sockaddr *a1, socklen_t a1len,
                          const struct sockaddr *a2, socklen_t a2len);


/**
 * Compare 'struct sockaddr', not taking ports into account.
 *
 * @param a1        the first address
 * @param a1len     the first address length
 * @param a2        the second address
 * @param a2len     the second address length
 *
 * @retval 0        equal
 * @retval -1       not equal
 * @retval -2       comparison of addresses of unsupported family
 */
extern int te_sockaddrcmp_no_ports(const struct sockaddr *a1,
                                   socklen_t a1len,
                                   const struct sockaddr *a2,
                                   socklen_t a2len);

/**
 * Compare the content of two 'struct sockaddr' structures till
 * minimum of two lengths a1len and a2len.
 *
 * @param a1        the first address
 * @param a1len     the first address length
 * @param a2        the second address
 * @param a2len     the second address length
 *
 * @retval 0        equal
 * @retval -1       not equal
 * @retval -2       comparison of addresses of unsupported family
 */
extern int te_sockaddrncmp(const struct sockaddr *a1, socklen_t a1len,
                           const struct sockaddr *a2, socklen_t a2len);

/**
 * Convert 'struct sockaddr' to string (it prints not only address
 * but also port and other fields).
 *
 * @param sa      Pointer to 'struct sockaddr'.
 * @param buf     Where to save string.
 * @param len     Length of the buffer.
 *
 * @return Status code.
 */
extern te_errno te_sockaddr2str_buf(const struct sockaddr *sa,
                                    char *buf, size_t len);

/**
 * Convert 'struct sockaddr' to string (it prints not only address
 * but also port and other fields).
 *
 * @note Static buffer is used for return value.
 *
 * @param sa    - pointer to 'struct sockaddr'
 *
 * @return null-terminated string
 *
 * @sa te_sockaddr_get_ipstr, te_ip2str
 */
extern const char *te_sockaddr2str(const struct sockaddr *sa);

/**
 * Convert sockaddr to string, save result in a provided buffer.
 *
 * @param sa      Pointer to struct sockaddr.
 * @param buf     Where to save a string representation.
 * @param len     Number of bytes available for a string.
 *
 * @reutnr Status code.
 */
extern te_errno te_sockaddr_h2str_buf(const struct sockaddr *sa,
                                      char *buf, size_t len);

/**
 * Convert a @b sockaddr to a string or return an error.
 *
 * @param [in]sa        source @b sockaddr
 * @param [out]string   result of conversion
 *
 * @return                      status code
 * @retval 0                    successful
 * @retval TE_EAFNOSUPPORT      family is not supported
 * @retval value of @b errno    @b inet_ntop was fail
 *
 * @note Free allocated memory on @b string
 */
extern te_errno te_sockaddr_h2str(const struct sockaddr *sa,
                                  char **string);

/**
 * Convert a string to a @b sockaddr or return an error.
 *
 * @param [in]string    source string
 * @param [out]sa       result @b sockaddr
 *
 * @return                      status code
 * @retval 0                    successful
 * @retval TE_EAFNOSUPPORT      family is not supported
 * @retval TE_EINVAL            @p string doesn't contain a valid network
 *                              address
 */
extern te_errno te_sockaddr_str2h(const char *string, struct sockaddr *sa);

/**
 * Returns the size of network address from a particular family
 * (in bytes).
 *
 * @param af  Address family
 *
 * @return Number of bytes used under network address
 */
extern size_t te_netaddr_get_size(int af);

/**
 * Returns the size of network address from a particular family
 * (in bits).
 *
 * @param af  Address family
 *
 * @return Number of bits required to store network address.
 */
extern size_t te_netaddr_get_bitsize(int af);

/**
 * Set multicast address part of XXX_mreq(n) structure
 *
 * @param af            Address family
 * @param mreq          Generic mreq structure
 * @param addr          Multicast address
 */
extern void te_mreq_set_mr_multiaddr(int af,
                                     void *mreq, const void *addr);

/**
 * Set interface part of XXX_mreq(n) structure
 *
 * @param af            Address family
 * @param mreq          Generic mreq structure
 * @param addr          Interface address
 */
extern void te_mreq_set_mr_interface(int af, void *mreq, const void *addr);

/**
 * Set interface index part of XXX_mreq(n) structure
 *
 * @param af            Address family
 * @param mreq          Generic mreq structure
 * @param ifindex       Interface index
 */
extern void te_mreq_set_mr_ifindex(int af, void *mreq, int ifindex);


/**
 * Convert network address from string and put it in provided sockaddr
 * structure. Set address family appropriately.
 *
 * @param addr_str      Address in string format
 * @param addr          Location for the address (should be sufficient
 *                      for sockadddr structure which corresponds to
 *                      provided address in string format, e.g.
 *                      struct te_sockaddr_storage)
 *
 * @return Status code.
 */
extern te_errno te_sockaddr_netaddr_from_string(const char      *addr_str,
                                                struct sockaddr *addr);

/**
 * Convert network address to its string representation. It returns statically
 * allocated string using te_sockaddr_get_ipstr()
 *
 * @param af            Address family
 * @param net_addr      Network address in form acceptable to
 *                      te_sockaddr_set_netaddr()
 *
 * @return Network address string representation, or @c NULL in case of error
 */
extern const char *te_sockaddr_netaddr_to_string(int af, const void *net_addr);

/**
 * Convert IPv4 address to IPv4-mapped IPv6 one.
 *
 * @param addr         Sockaddr structure containing address.
 *                     It must be large enough to hold IPv6 address.
 *
 * @return             Status code.
 */
extern te_errno te_sockaddr_ip4_to_ip6_mapped(struct sockaddr *addr);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_SOCKADDR_H__ */
/**@} <!-- END te_tools_te_sockaddr --> */
