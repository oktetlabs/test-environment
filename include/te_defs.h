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

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <limits.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdbool.h>

/** Test Environment copyright to be used in applications output */
#define TE_COPYRIGHT \
"Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS in\n" \
"the root directory of the distribution).\n"                              \
"This is free software; see the source for copying conditions.  There "   \
    "is NO\n"                                                             \
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR "     \
    "PURPOSE.\n"


#ifndef NULL
#define NULL ((void *)0)
#endif

/** Boolean type to be used everywhere in Test Environment */
typedef bool te_bool;

#ifndef FALSE
#define FALSE false
#define TRUE  true
#endif

/** Ternary (tree-valued, trivalent) logic base type */
typedef enum te_bool3 {
    TE_BOOL3_FALSE = -1,
    TE_BOOL3_UNKNOWN = 0,
    TE_BOOL3_ANY = TE_BOOL3_UNKNOWN,
    TE_BOOL3_TRUE = 1,
} te_bool3;


#define C_ASSERT(x)

/** Exit from application because of caught SIGINT signal */
#define TE_EXIT_SIGINT      0x2
/** Exit with biiig problems  */
#define TE_EXIT_ERROR       0x3
/** Exit from application because of caught SIGUSR2 signal */
#define TE_EXIT_SIGUSR2     0x4
/** Script to be run not found */
#define TE_EXIT_NOT_FOUND   0x7f
/**
 * Script tells that the test case does not
 * make sense (can't be run) with passed parameters.
 */
#define TE_EXIT_SKIP        0x5


/**
 * Marks unused argument of the function in order to avoid compilation
 * warning.
 *
 * @param _x    unused argument
 */
#ifndef UNUSED
#define UNUSED(_x)      (void)(_x)
#endif

/* Entity name used to logging Control messages from Tester */
#define TE_LOG_CMSG_ENTITY_TESTER "Tester"
/* User name for all control messages (from Tester and tests) */
#define TE_LOG_CMSG_USER          "Control"

#ifndef __CONCAT
#define __CONCAT(_a, _b) _a##_b
#endif

#define TE_CONCAT(_a, _b) __CONCAT(_a, _b)

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


/** Cast to 'struct sockaddr_storage *' type. */
#define SS(_p)          ((struct sockaddr_storage *)(_p))
/** Cast to 'const struct sockaddr_storage *' type. */
#define CONST_SS(_p)    ((const struct sockaddr_storage *)(_p))
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
/** Cast to 'struct sockaddr_un *' type. */
#define SUN(_p)         ((struct sockaddr_un *)(_p))
/** Cast to 'const struct sockaddr_un *' type. */
#define CONST_SUN(_p)  ((const struct sockaddr_un *)(_p))

/**
 * Platform-independent defines for
 * standard input, output and error
 * file descriptors.
 */
#define RPC_STDIN_FILENO 0
#define RPC_STDOUT_FILENO 1
#define RPC_STDERR_FILENO 2

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

/** Convert nanoseconds to struct timespec. */
#define TE_NS2TS(_ns, _p_tv) \
    do {                                        \
        (_p_tv)->tv_sec = TE_NS2SEC(_ns);       \
        (_p_tv)->tv_nsec = (_ns) % 1000000000L; \
    } while (0)

/**
 * Offset of the field in structure.
 *
 * @param _s        Structure name
 * @param _f        Field name
 */
#define TE_OFFSET_OF(_s, _f)    ((size_t)(&(((_s *)NULL)->_f)))

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

#ifdef HAVE_STDLIB_H
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
#if 0
    return min + (rand() % (max - min + 1));
#else
    /*
     * FIXME: Too simple code above is subject to replacement
     *
     *  As 'man 3 rand' tells:
     *
     * ------------------------------------------------------------
     * In  Numerical  Recipes in C: The Art of Scientific Computing
     * (William H. Press, Brian P. Flannery, Saul A. Teukolsky,
     * William T. Vetterling; New York: Cambridge University Press,
     * 1992 (2nd ed., p. 277)), the following comments are made:
     * "If you want to generate a random integer between 1 and 10,
     * you should always do it by using high-order bits, as in
     *
     *               j=1+(int) (10.0*rand()/(RAND_MAX+1.0));
     *
     *        and never by anything resembling
     *
     *               j=1+(rand() % 10);
     *
     *        (which uses lower-order bits)."
     * ------------------------------------------------------------
     *
     * So, the next implementation sequentially tries to use 'long',
     * 'long long' and 'double' types at compile time basing on RAND_MAX
     * constant value and RAND_RANGE_FORCE_DOUBLE preprocessor symbol and
     * also taking care about avoiding any kind of possible overflow.
     *
     * If RAND_RANGE_FORCE_DOUBLE preprocessor symbol is defined it means
     * the implementation does not try to use 'long' or 'long long' types
     * for calculations implementation; it just uses 'double' type instead.
     *
     * Note: the case when 'max - min > RAND_MAX' is not checked,
     * although in this case pseudo-random values quantity becomes
     * discrete: there are such values from 'min' to 'max' that are never
     * returned.
     */

    /* -RAND_MAX returned value is the range logical error indicator */
    return max < min ? -RAND_MAX :

    /* Try to find suitable type */
#if   LONG_MAX / (INT_MAX - INT_MIN + 1) >= RAND_MAX  && \
      ! defined RAND_RANGE_FORCE_DOUBLE
    (int)(min + (((long)max - min) + 1) * rand() / (RAND_MAX + 1L));
#elif LLONG_MAX / (INT_MAX - INT_MIN + 1) >= RAND_MAX && \
      ! defined RAND_RANGE_FORCE_DOUBLE
    (int)(min + (((long long)max - min) + 1) * rand() / (RAND_MAX + 1LL));
#else
    (int)(min + (((double)max - min) + 1) * rand() / (RAND_MAX + 1.0));
#endif
#endif /* if 0 */
}
#endif

#if defined(HAVE_STRING_H) || defined(HAVE_STRINGS_H)
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

/**
 * Compare strings.
 *
 * @param s1     The first result
 * @param s2     The second result
 *
 * @result value returned by strcmp() if both s1 and s2
 *         are not NULL, 0 if they are both NULL,
 *         1 if only the first one is not NULL,
 *         -1 if only the second one is not NULL.
 */
static inline int strcmp_null(const char *s1, const char *s2)
{
    if (s1 == NULL && s2 == NULL)
        return 0;
    else if (s1 == NULL)
        return -1;
    else if (s2 == NULL)
        return 1;

    return strcmp(s1, s2);
}

#endif

#if defined(HAVE_STDLIB_H) && defined(HAVE_ERRNO_H)
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
#define TE_SPRINTF(_buf, _msg...)                           \
    do {                                                    \
        int _n = snprintf(_buf, sizeof(_buf), _msg);        \
                                                            \
        if (_n < 0)                                         \
        {                                                   \
            ERROR("%s: output error is encountered: %s",    \
                  __func__, strerror(errno));               \
        }                                                   \
        else if ((size_t)_n >= sizeof(_buf))                \
        {                                                   \
            ERROR("%s: string \"%s\" is cut by snprintf()", \
                  __func__, _buf);                          \
        }                                                   \
    } while (0)

/**
 * Check if a pointer is @c NULL. It's to avoid warnings in macroses.
 *
 * @param ptr   A pointer
 *
 * @return @c TRUE if @p ptr is not NULL
 */
static inline te_bool
ptr_is_not_null(const void *ptr)
{
    if (ptr != NULL)
        return TRUE;
    return FALSE;
}

#ifdef __GNUC__
/**
 * Round up to nearest power of 2
 *
 * @param num     The number to round up
 *
 * @note If @p num is equal to @c 0 or @c ULLONG_MAX,
         the result will be @c 2
 *
 * @return Upper power of 2 for @p num
 */
static inline unsigned long long
te_round_up_pow2(unsigned long long num)
{
    unsigned long long n = ((num == 0) || (num == ULLONG_MAX)) ? 1 : num;

    return (1 << (sizeof(num) * CHAR_BIT - __builtin_clzll(n)));
}
#endif

/* Tests related part */
/* fixme kostik: should be moved */

#define TEST_ARG_VAR_PREFIX "VAR."
#define TEST_ARG_ENV_PREFIX "TE_TEST_VAR_"

/* assert() function analog for compilation */
#define TE_COMPILE_TIME_ASSERT(x) \
    switch(0) {case 0: case x:;}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_DEFS_H__ */
