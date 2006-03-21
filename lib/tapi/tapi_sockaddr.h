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
#include "te_errno.h"
#include "te_sockaddr.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Retrieve unused in system port in host order.
 *
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 */
extern te_errno tapi_allocate_port(uint16_t *p_port);

/**
 * Retrieve unused in system port in network order.
 *
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 */
static inline te_errno
tapi_allocate_port_htons(uint16_t *p_port)
{
    uint16_t port;
    int      rc;
    
    if ((rc = tapi_allocate_port(&port)) != 0)
        return rc;
        
    *p_port = htons(port);
    
    return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SOCKADDR_H__ */
