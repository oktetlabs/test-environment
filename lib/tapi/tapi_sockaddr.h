/** @file
 * @brief Functions to opearate with generic "struct sockaddr"
 *
 * Definition of test API for working with struct sockaddr.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#include "te_stdint.h"
#include "te_errno.h"
#include "te_sockaddr.h"
#include "rcf_rpc.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieve unused in system port in host order.
 *
 * @param pco       RPC server to check that port is free
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 */
extern te_errno tapi_allocate_port(struct rcf_rpc_server *pco,
                                   uint16_t *p_port);

/**
 * Retrieve range of ports unused in system, in host order.
 *
 * @param pco       RPC server to check that port is free
 * @param p_port    Location for allocated ports, pointer to array,
 *                  should have enough place for @p items. 
 * @param num       Number of ports requests, i.e. length of range.
 *
 * @return Status code.
 */
extern te_errno tapi_allocate_port_range(struct rcf_rpc_server *pco,
                                         uint16_t *p_port, int num);

/**
 * Retrieve unused in system port in network order.
 *
 * @param pco       RPC server to check that port is free
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 */
extern te_errno tapi_allocate_port_htons(rcf_rpc_server *pco,
                                         uint16_t *p_port);

/**
 * Generate new sockaddr basing on existing one (copy data and 
 * allocate new port).
 *
 * @param pco   RPC server to check that port is free
 * @param src   existing sockaddr
 * @param dst   location for new sockaddr
 *
 * @return Status code.
 */
static inline te_errno
tapi_sockaddr_clone(rcf_rpc_server *pco,
                    const struct sockaddr *src, 
                    struct sockaddr_storage *dst)
{
    memcpy(dst, src, te_sockaddr_get_size(src));
    return tapi_allocate_port_htons(pco, te_sockaddr_get_port_ptr(SA(dst)));
}

/**
 * Obtain an exact copy of a given socket address. 
 *
 * @param src   existing sockaddr
 * @param dst   location for a clone
 *
 * @return Status code.
 */
static inline void
tapi_sockaddr_clone_exact(const struct sockaddr *src, 
                          struct sockaddr_storage *dst)
{
    memcpy(dst, src, te_sockaddr_get_size(src));
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SOCKADDR_H__ */
