/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment common definitions
 *
 * Common usefull definition
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_DEFS_H__
#define __TE_DEFS_H__

#include "te_config.h"

#include <stddef.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#include <stdbool.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
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

/** Test Environment copyright to be used in applications output */
#define TE_COPYRIGHT \
"Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.\n"

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
/*
 * User name for all control messages (from Tester and tests).
 * Usage of this name for logging verdicts from tests is
 * deprecated now. Use instead more specific user names listed
 * below.
 */
#define TE_LOG_CMSG_USER          "Control"
/* User name for verdict control messages from tests */
#define TE_LOG_VERDICT_USER       "Verdict"
/* User name for artifact control messages from tests */
#define TE_LOG_ARTIFACT_USER      "Artifact"
/* User name for the message with the execution plan */
#define TE_LOG_EXEC_PLAN_USER     "Execution Plan"
/* User name for the message with the TRC tags */
#define TE_LOG_TRC_TAGS_USER      "TRC tags"
/* User name for the message with proocess info, e.g. PID */
#define TE_LOG_PROC_INFO_USER     "Process Info"

#ifndef __CONCAT
#define __CONCAT(_a, _b) _a##_b
#endif

#define TE_CONCAT(_a, _b) __CONCAT(_a, _b)


/**
 * Helper to get the first (mandatory) argument of the variadic macro.
 *
 * For example:
 * @code{.c}
 * #define MY_VA_MACRO(...) \
 *     do {                                                 \
 *         if (TE_VA_HEAD(__VA_ARGS__)[0] != '\0')          \
 *              // do something format string is not empty  \
 *     while (0)
 * @endcode
 */
#define TE_VA_HEAD(first, ...)  first

/**
 * Helper to get all arguments of the variadic macro except the first one.
 *
 * If there is no arguments except the first one, nothing is substututed.
 */
#define TE_VA_TAIL(first, ...)  __VA_ARGS__


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
 * @deprecated Use standard C `offsetof`
 */
#define TE_OFFSET_OF(_s, _f)    (offsetof(_s, _f))

/**
 * Size of a @p field in a structure @p type.
 *
 * A complement to a C standard `offsetof`.
 * One usecase for such a macro is to define
 * compile-time assertions.
 *
 * @param type   Struct or union type name
 * @param field  Field name
 */
#define TE_SIZEOF_FIELD(type, field) (sizeof(((type *)NULL)->field))

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

/** Align up a value to a power of two specified by mask */
#define TE_ALIGN_MASK(_v, _m)       (((_v) + (_m)) & ~(_m))

/** Align up a value to a power of two */
#define TE_ALIGN(_v, _a)            TE_ALIGN_MASK((_v), (typeof(_v))(_a) - 1)

/* Tests related part */
/* fixme kostik: should be moved */

#define TEST_ARG_VAR_PREFIX "VAR."
#define TEST_ARG_ENV_PREFIX "TE_TEST_VAR_"

/**
 * Build-time analog of an assert().
 *
 * @note Syntactically this is a boolean expression that always
 * return true, so it can be used in conditional expressions.
 * This is important because a comma operator always makes
 * an expression non-constant, so e.g. to insert a check into
 * a static variable initializer, one would need to do smth like:
 * `static int x = TE_COMPILE_TIME_ASSERT_EXPR(test) ? value : 0`
 */
#define TE_COMPILE_TIME_ASSERT_EXPR(x) \
    (!!sizeof((char [!!(x) ? 1 : -1]){0}))

/**
 * The same as TE_COMPILE_TIME_ASSERT_EXPR(), but suitable
 * as a standalone statement without compiler warnings
 */
#define TE_COMPILE_TIME_ASSERT(x) ((void)TE_COMPILE_TIME_ASSERT_EXPR(x))

/**
 * Cast away constness from a pointer.
 *
 * The rationale for the macro is twofold:
 * - the only legal ISO C way to cast away constness is through
 *   an intermediate cast to `uintptr_t` and some compilers in some
 *   modes __do__ enforce this
 * - the macro tries to be type-safe and assert at compile time that
 *   the object type is indeed compatible
 *
 * @note Casting away constness may still be dangerous, use with caution
 *
 * @param _type  The expected type of the pointer @p _ptr
 * @param _ptr   Object pointer
 */
#ifdef __GNUC__
#define TE_CONST_PTR_CAST(_type, _ptr) \
    (TE_COMPILE_TIME_ASSERT_EXPR(                                        \
        __builtin_types_compatible_p(__typeof__(_ptr), const _type *)) ? \
     ((_type *)(uintptr_t)(_ptr)) : NULL)
#else
#define TE_CONST_PTR_CAST(_type, _ptr) ((_type *)(uintptr_t)(_ptr))
#endif

/**
 * Void function pointer. Can be casted to any other function
 * pointer type without cast-function-type warning.
 */
typedef void (*te_void_func)(void);

/**
 * This macro can be used to suppress -Wcast-function-type warning
 * when casting function pointer to incompatible function pointer type.
 * Usually it is not a good idea to do this, use it only if you are
 * sure it is safe and there is no better alternative.
 *
 * @param _func_type      Desired function pointer type.
 * @param _func_ptr       Function pointer.
 */
#define TE_FUNC_CAST(_func_type, _func_ptr) \
    ((_func_type)(te_void_func)(_func_ptr))

/**
 * Round-towards-zero to nearest multiple.
 *
 * @param n      Number to be rounded.
 * @param m      Number to multiple of which to round.
 */
static inline unsigned int
te_round_to_zero(unsigned int n, unsigned int m)
{
    return (n / m) * m;
}

#define TE_DIV_ROUND_UP(_n, _d) (((_n) + (_d) - 1) / (_d))

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_DEFS_H__ */
