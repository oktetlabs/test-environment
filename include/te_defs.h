/** @file
 * @brief Test Environment common definitions
 *
 * Common usefull definition
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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

#ifndef __TE_DEFS_H__
#define __TE_DEFS_H__

#include "te_config.h"

#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif


/** Test Environment copyright to be used in applications output */
#define TE_COPYRIGHT \
"Copyright (C) 2004 Test Environment authors (see file AUTHORS in the " \
    "root\n"                                                            \
"directory of the distribution).\n"                                     \
"This is free software; see the source for copying conditions.  There " \
    "is NO\n"                                                           \
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR "   \
    "PURPOSE.\n"
    

/** Boolean type to be used everywhere in Test Environment */
typedef unsigned char te_bool;

#ifndef FALSE
#define FALSE   0
#define TRUE    (!FALSE)
#endif


#define C_ASSERT(x)

/** Exit from application because of caught SIGINT signal */
#define TE_EXIT_SIGINT      0x2
/** Script to be run not found */
#define TE_EXIT_NOT_FOUND   0x7f


/**
 * Marks unused argument of the function in order to avoid compilation
 * warning.
 *
 * @param _x    unused argument 
 */
#define UNUSED(_x)      (void)(_x)


/**
 * Determines minimum from two arguments. If arguments are equal,
 * preference is given to the first one.
 *
 * @param _x    the first argument
 * @param _y    the second argument
 *
 * @note Macro user is responsible for appropriateness of the argument
 *       types.
 *
 * @sa MAX()
 */
#ifndef MIN
#define MIN(_x, _y)     (((_x) > (_y)) ? (_y) : (_x))
#endif

/**
 * Determines maximum from two arguments. If arguments are equal,
 * preference is given to the first one.
 *
 * @param _x    the first argument
 * @param _y    the second argument
 *
 * @note Macro user is responsible for appropriateness of the argument
 *       types.
 *
 * @sa MIN()
 */
#ifndef MAX
#define MAX(_x, _y)     (((_x) < (_y)) ? (_y) : (_x))
#endif


/** Cast to 'struct sockaddr *' type. */
#define SA(_p)          ((struct sockaddr *)(_p))
/** Cast to 'struct sockaddr_in *' type. */
#define SIN(_p)         ((struct sockaddr_in *)(_p))
/** Cast to 'struct sockaddr_in6 *' type. */
#define SIN6(_p)        ((struct sockaddr_in6 *)(_p))


/**
 * Swap two pointers.
 *
 * @param _p1       one pointer
 * @param _p2       another pointer
 */
#define SWAP_PTRS(_p1, _p2) \
    do {                    \
        void *tmp = (_p1);  \
                            \
        (_p1) = (_p2);      \
        (_p2) = tmp;        \
    } while (FALSE)


/** Useful for debugging */
#if 0 /* BUG here */
#define malloc(_x)   (((_x) < 0x04000000) ? (malloc)(_x) : \
                                            (({assert(FALSE);}), NULL))
#endif

/** Prefix for tester user name */
#define TE_USER_PREFIX  "te"


/**
 * Macro to log RING before sleep and sleep.
 *
 * @param x     Seconds to sleep
 */
#define SLEEP(x) \
    do {                                        \
        unsigned int _to_sleep = (x);           \
                                                \
        RING("Sleeping %u seconds", _to_sleep); \
        (void)sleep(_to_sleep);                 \
    } while (0)


#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_STDLIB_H
/**
 * Generate random number from the range.
 *
 * @param min   - range lower limit
 * @param max   - range upper limit
 *
 * @return Random number from the range [min, max]
 */
static inline int
rand_range(int min, int max)
{
    return min + (rand() % (max - min + 1));
}
#endif

/**
 * Check that string starts fro the specified substring.
 *
 * @param pattern       pattern which str should begin from
 * @param str           string to be checked
 *
 * @return result of strncmp
 */
static inline int
strcmp_start(const char *pattern, const char *str)
{
    return strncmp(pattern, str, strlen(pattern));
}


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
#define TE_PRINTF_32        ""
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


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_DEFS_H__ */
