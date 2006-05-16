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

#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
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

/* Entity name used to logging Control messages from Tester */
#define TE_LOG_CMSG_ENTITY_TESTER "Tester"
/* User name for all control messages (from Tester and tests) */
#define TE_LOG_CMSG_USER          "Control"

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
/** Cast to 'const struct sockaddr *' type. */
#define CONST_SA(_p)    ((const struct sockaddr *)(_p))
/** Cast to 'struct sockaddr_in *' type. */
#define SIN(_p)         ((struct sockaddr_in *)(_p))
/** Cast to 'const struct sockaddr_in *' type. */
#define CONST_SIN(_p)   ((const struct sockaddr_in *)(_p))
/** Cast to 'struct sockaddr_in6 *' type. */
#define SIN6(_p)        ((struct sockaddr_in6 *)(_p))
/** Cast to 'const struct sockaddr_in6 *' type. */
#define CONST_SIN6(_p)  ((const struct sockaddr_in6 *)(_p))


/** Convert seconds to milliseconds. */
#define TE_SEC2MS(_sec)         ((_sec) * 1000)
/** Convert seconds to microseconds. */
#define TE_SEC2US(_sec)         ((_sec) * 1000000L)
/** Convert seconds to nanoseconds. */
#define TE_SEC2NS(_sec)         ((_sec) * 1000000000L)

/** Convert milliseconds to seconds. */
#define TE_MS2SEC(_ms)          ((_ms) / 1000)
/** Convert milliseconds to microseconds. */
#define TE_MS2US(_ms)           ((_ms) * 1000)
/** Convert milliseconds to nanoseconds. */
#define TE_MS2NS(_ms)           ((_ms) * 1000000L)

/** Convert microseconds to seconds. */
#define TE_US2SEC(_us)          ((_us) / 1000000L)
/** Convert microseconds to milliseconds. */
#define TE_US2MS(_us)           ((_us) / 1000)
/** Convert microseconds to nanoseconds. */
#define TE_US2NS(_us)           ((_us) * 1000)

/** Convert nanoseconds to seconds. */
#define TE_NS2SEC(_ns)          ((_ns) / 1000000000L)
/** Convert nanoseconds to milliseconds. */
#define TE_NS2MS(_ns)           ((_ns) / 1000000L)
/** Convert nanoseconds to microseconds. */
#define TE_NS2US(_ns)           ((_ns) / 1000)

/** Convert microseconds to struct timeval. */
#define TE_US2TV(_us, _p_tv) \
    do {                                        \
        (_p_tv)->tv_sec = TE_US2SEC(_us);       \
        (_p_tv)->tv_usec = (_us) % 1000000L;    \
    } while (0)

/** 
 * Number of elements in array.
 *
 * @param _array    C name of an array
 */
#define TE_ARRAY_LEN(_array)    (sizeof(_array) / sizeof(_array[0]))

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

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_STDLIB_H
/**
 * Generate random number from the range.
 *
 * @param min     range lower limit
 * @param max     range upper limit
 *
 * @return Random number from the range [min, max]
 */
static inline int
rand_range(int min, int max)
{
    return min + (rand() % (max - min + 1));
}
#endif

#if HAVE_STRING_H || HAVE_STRINGS_H
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
#endif

#if HAVE_STDLIB_H && HAVE_ERRNO_H
/**
 * Creates a temporary file base on template passed in @a tmp_name
 * parameter.
 *
 * @param tmp_name  template of the temporary file name, which must
 *                  comply @a templateparameter of mkstemp() function
 *
 * @retval 0    Success
 * @retval -1   Failure
 */
static inline int
te_make_tmp_file(char *tmp_name)
{
    int fd;

    if ((fd = mkstemp(tmp_name)) < 0)
    {
        return -1;
    }

    /* We need to close file descriptor. */
    close(fd);

    return 0;
}
#endif

/** Macro to make safe sprintf to character array */
#define TE_SPRINTF(_buf, _msg...) \
    do {                                                        \
        unsigned _n = snprintf(_buf, sizeof(_buf), _msg);       \
                                                                \
        if (_n >= sizeof(_buf))                                 \
            ERROR("%s: string %s is cut by snprintf()", _buf,   \
                  __FUNCTION__);                                \
    } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_DEFS_H__ */
