/** @file
 * @brief SUN RPC Library for Windows
 *
 * Definitions necessary for compiling of SUN RPC library for Windows
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2006 Level5 Networks Corp.
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __RPC_INTERNAL_H__
#define __RPC_INTERNAL_H__

#ifndef WINDOWS

#include <stdint.h>
#include <limits.h>
#include <stdio.h>

#endif


#define INTUSE(name) name
#define INTDEF(name)
#define INTVARDEF(name)
#define INTDEF2(name, newname)
#define INTVARDEF2(name, newname)

#define _(x)    x
#define __bzero(s, n)           memset(s, 0, n)

#define ntohl(n) \
    ((uint32_t)(((uint8_t *)&(n))[0] << 24) + \
     (uint32_t)(((uint8_t *)&(n))[1] << 16) + \
     (uint32_t)(((uint8_t *)&(n))[2] << 8) + \
     (uint32_t)(((uint8_t *)&(n))[3]))

#define htonl(n)                ntohl(n)     

#include "types.h"
#include "xdr.h"

#endif 
