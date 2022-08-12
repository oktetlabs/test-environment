/** @file
 * @brief Test Environment common definitions
 *
 * Definitions for portable printf() specifiers
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_PRINTF_H__
#define __TE_PRINTF_H__

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif

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

/*
 * Generic format string for 48bit MAC address.
 *
 * @note It should be used with @b TE_PRINTF_MAC_VAL().
 */
#define TE_PRINTF_MAC_FMT "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"

/*
 * Arguments for @b TE_PRINTF_MAC_FMT.
 *
 * @param _mac      Buffer containing 48bit MAC address.
 */
#define TE_PRINTF_MAC_VAL(_mac) \
      (_mac)[0], (_mac)[1], (_mac)[2], (_mac)[3], (_mac)[4], (_mac)[5]

/**
 * Format string for printing out struct timespec or tarpc_timespec value.
 *
 * @note It should be used with @b TE_PRINTF_TS_VAL().
 */
#define TE_PRINTF_TS_FMT "{%llu s %.9llu ns}"

/**
 * Arguments for @b TE_PRINTF_TS_FMT.
 *
 * @param _ts       tarpc_timespec or struct timespec value.
 */
#define TE_PRINTF_TS_VAL(_ts) \
  (long long unsigned int)((_ts).tv_sec), \
  (long long unsigned int)((_ts).tv_nsec)

#if defined (__QNX__) || !defined(_GNU_SOURCE)
#define TE_USE_SPECIFIC_ASPRINTF 1
#endif /* __QNX__ */

/* Note: enable this for solaris if asprintf is not implemented */
#ifdef TE_USE_SPECIFIC_ASPRINTF
static inline int
te_vasprintf(char **strp, const char *fmt, va_list ap)
{
    va_list aux;
    int len;
    char *buf;
    int ret;

    /* Duplicate va_list, because vprintf functions modify it */
    va_copy(aux, ap);

    /* Calculate length of formatted string */
    if ((len = vsnprintf(NULL, 0, fmt, aux)) < 0)
        return len;

    /* Add space for \0 */
    len++;

    /* Allocate buffer with calculated length */
    if ((buf = malloc(len)) == NULL)
        return -1;

    ret = vsnprintf(buf, len, fmt, ap);
    if (ret < 0)
        free(buf);
    else
        *strp = buf;

    return ret;
}
#else
static inline int
te_vasprintf(char **strp, const char *fmt, va_list ap)
{
    return vasprintf(strp, fmt, ap);
}
#endif

static inline int
te_asprintf(char **strp, const char *fmt, ...)
{
    int     rc;
    va_list ap;

    va_start(ap, fmt);
    rc = te_vasprintf(strp, fmt, ap);
    va_end(ap);

    return rc;
}

static inline char *
te_sprintf(const char *fmt, ...)
{
    va_list  ap;
    char    *c = NULL;
    int      rc;

    va_start(ap, fmt);
    rc = te_vasprintf(&c, fmt, ap);
    va_end(ap);

    if (rc < 0)
        return NULL;

    return c;
}

/**
 * Wrapper around strerror_r() version that does not ignore its return code.
 *
 * It's a bit crazy, cause latest GCC complains that it should not be
 * ignored cause it thinks that in case we're using GNU extension it will
 * store pointed to an important message about errno conversion failure.
 */
static inline char *
te_strerror_r(int in_errno, char *buf, size_t buf_len)
{
    char *ret;

#if defined(_GNU_SOURCE) || (_POSIX_C_SOURCE < 200112L)
    ret = strerror_r(in_errno, buf, buf_len);
#else
    if (strerror_r(in_errno, buf, buf_len) == 0)
    {
        ret = buf;
    }
    else
    {
        (void)snprintf(buf, buf_len, "Unknown errno %d", in_errno);
        ret = buf;
    }
#endif

    return ret;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_PRINTF_H__ */
