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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * Returns pointer to network address part of sockaddr structure according to
 * 'sa_family' field of the structure
 *
 * @param addr  Generic address structure
 *
 * @return Pointer to corresponding network address
 */
extern const void *sockaddr_get_netaddr(const struct sockaddr *addr);

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
extern int sockaddr_set_netaddr(struct sockaddr *addr, const void *net_addr);

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
 * @param if_addr      Interface address
 *
 */
extern void mreq_set_mr_interface(int addr_family,
                                  void *mreq, const void *addr);

/**
 * Set interface index part of XXX_mreq(n) structure
 *
 * @param addr_family  Address family
 * @param mreq         Generic mreq structure
 * @param if_addr      Interface address
 *
 */
extern void mreq_set_mr_ifindex(int addr_family, void *mreq, int ifindex);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SOCKADDR_H__ */
