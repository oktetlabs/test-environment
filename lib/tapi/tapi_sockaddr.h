/** @file
 * @brief Functions to opearate with generic "struct sockaddr"
 *
 * Definition of test API for working with struct sockaddr.
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

#ifndef __TE_TAPI_SOCKADDR_H__
#define __TE_TAPI_SOCKADDR_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"

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
#define PREFIX2MASK(prefix) (prefix == 0 ? 0 : (~0) << (32 - (prefix)))

/**
 * Is address family is supported by this TAPI?
 *
 * @param af        Address family
 */
static inline te_bool
sockaddr_is_af_supported(int af)
{
    return (af == AF_INET) || (af == AF_INET6);   
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
extern void sockaddr_clear_port(struct sockaddr *addr);

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
extern uint16_t sockaddr_get_port(const struct sockaddr *addr);

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
extern void sockaddr_set_port(struct sockaddr *addr, uint16_t port);

/**
 * Returns pointer to network address part of sockaddr structure according
 * to 'sa_family' field of the structure
 *
 * @param addr  Generic address structure
 *
 * @return Pointer to corresponding network address
 */
extern const void *sockaddr_get_netaddr(const struct sockaddr *addr);

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
 */
static inline const char *
sockaddr_get_ipstr(const struct sockaddr *addr)
{
    static char addr_buf[INET6_ADDRSTRLEN];
    
    return inet_ntop(addr->sa_family, 
                     sockaddr_get_netaddr(addr), addr_buf, 
                     sizeof(addr_buf));
}

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
extern int sockaddr_set_netaddr(struct sockaddr *addr,
                                const void *net_addr);

/**
 * Set "network address" part of corresponding struct sockaddr to wildcard
 *
 * @param addr  Generic address structure
 *
 * @note The function is not very safe because it does not get the length
 * of the structure assuming that there is enough space for a structure
 * defined from the value of "sa_family" field.
 */
extern void sockaddr_set_wildcard(struct sockaddr *addr);

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
extern te_bool sockaddr_is_wildcard(const struct sockaddr *addr);

/**
 * Check if "network address" part of corresponding struct sockaddr is 
 * multicast address
 *
 * @param addr Generic address struture
 *
 * @return TRUE/FALSE - multicast/other address
 *
 */
extern te_bool sockaddr_is_multicast(const struct sockaddr *addr);

/**
 * Returns the size of a particular sockaddr structure according to
 * 'sa_family' field of the structure
 *
 * @param addr   Generic address structure
 *
 * @return Address size
 */
extern int sockaddr_get_size(const struct sockaddr *addr);

/**
 * Compare 'struct sockaddr'.
 *
 * @param a1    - the first address
 * @param a1len - the first address length
 * @param a2    - the second address
 * @param a2len - the second address length
 *
 * @retval 0    - equal
 * @retval -1   - not equal
 * @retval -2   - comparison of addresses of unsupported family
 */
extern int sockaddrcmp(const struct sockaddr *a1, socklen_t a1len,
                       const struct sockaddr *a2, socklen_t a2len);

/**
 * Compare the content of two 'struct sockaddr' structures till 
 * minimum of two lengths a1len and a2len.
 *
 * @param a1    - the first address
 * @param a1len - the first address length
 * @param a2    - the second address
 * @param a2len - the second address length
 *
 * @retval 0    - equal
 * @retval -1   - not equal
 * @retval -2   - comparison of addresses of unsupported family
 */
extern int sockaddrncmp(const struct sockaddr *a1, socklen_t a1len,
                        const struct sockaddr *a2, socklen_t a2len);

/**
 * Convert 'struct sockaddr' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param sa    - pointer to 'struct sockaddr'
 *
 * @return null-terminated string
 */
extern const char *sockaddr2str(const struct sockaddr *sa);

/**
 * Returns the size of network address from a particular family
 *
 * @param addr_family  Address family
 *
 * @return Number of bytes used under network address
 */
extern int netaddr_get_size(int addr_family);

/**
 * Set multicast address part of XXX_mreq(n) structure
 *
 * @param addr_family  Address family
 * @param mreq         Generic mreq structure
 * @param addr         Multicast address
 */
extern void mreq_set_mr_multiaddr(int addr_family,
                                  void *mreq, const void *addr);

/**
 * Set interface part of XXX_mreq(n) structure
 *
 * @param addr_family  Address family
 * @param mreq         Generic mreq structure
 * @param addr      Interface address
 *
 */
extern void mreq_set_mr_interface(int addr_family,
                                  void *mreq, const void *addr);

/**
 * Set interface index part of XXX_mreq(n) structure
 *
 * @param addr_family  Address family
 * @param mreq         Generic mreq structure
 * @param ifindex      Interface index
 *
 */
extern void mreq_set_mr_ifindex(int addr_family, void *mreq, int ifindex);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SOCKADDR_H__ */
