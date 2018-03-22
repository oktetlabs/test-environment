/** @file
 * @brief SUN RPC Library for Windows
 *
 * Definitions necessary for compiling of SUN RPC library for Windows
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
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
