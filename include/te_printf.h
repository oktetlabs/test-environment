/** @file
 * @brief Test Environment common definitions
 *
 * Definitions for portable printf() specifiers
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_PRINTF_H__
#define __TE_PRINTF_H__

#include "te_config.h"

/** printf()-like length modified for 8-bit integers */
#if (SIZEOF_CHAR == 1)
#define TE_PRINTF_8         "hh"
#else
#error Unable to print 8-bit integers
#endif

/** printf()-like length modified for 16-bit integers */
#if (SIZEOF_SHORT == 2)
#define TE_PRINTF_16        "h"
#else
#error Unable to print 16-bit integers
#endif

/** printf()-like length modified for 32-bit integers */
#if (SIZEOF_INT == 4)
#if defined(__CYGWIN__) && (SIZEOF_LONG == 4)
/* Under CYGWIN 'int32_t' is defined as 'long int' */
#define TE_PRINTF_32        "l"
#else
#define TE_PRINTF_32        ""
#endif
#elif (SIZEOF_LONG == 4)
#define TE_PRINTF_32        "l"
#else
#error Unable to print 32-bit integers
#endif

/** printf()-like length modified for 64-bit integers */
#if (SIZEOF_INT == 8)
#define TE_PRINTF_64        ""
#elif (SIZEOF_LONG == 8)
#define TE_PRINTF_64        "l"
#elif (SIZEOF_LONG_LONG == 8)
#define TE_PRINTF_64        "ll"
#else
#error Unable to print 64-bit integers
#endif

/** printf()-like length modified for (s)size_t integers */
#if (SIZEOF_INT == SIZEOF_SIZE_T)
#define TE_PRINTF_SIZE_T    ""
#elif (SIZEOF_LONG == SIZEOF_SIZE_T)
#define TE_PRINTF_SIZE_T    "l"
#elif (SIZEOF_LONG_LONG == SIZEOF_SIZE_T)
#define TE_PRINTF_SIZE_T    "ll"
#else
#error Unable to print (s)size_t integers
#endif

/** printf()-like length modified for socklen_t integers */
#if (SIZEOF_INT == SIZEOF_SOCKLEN_T)
#define TE_PRINTF_SOCKLEN_T ""
#elif (SIZEOF_LONG == SIZEOF_SOCKLEN_T)
#define TE_PRINTF_SOCKLEN_T "l"
#else
#error Unable to print socklen_t integers
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_PRINTF_H__ */
