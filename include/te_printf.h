/** @file
 * @brief Test Environment common definitions
 *
 * Definitions for portable printf() specifiers
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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

#if defined (__QNX__)
#define TE_USE_SPECIFIC_ASPRINTF 1
#endif /* __QNX__ */

/* Note: enable this for solaris if asprintf is not implemented */
#ifdef TE_USE_SPECIFIC_ASPRINTF
static inline int
te_vasprintf(char **strp, const char *fmt, va_list ap)
{
    va_list aux;
    int len;

    /* Duplicate va_list, because vprintf functions modify it */
    va_copy(aux, ap);

    /* Calculate length of formatted string */
    if ((len = vsnprintf(NULL, 0, fmt, aux) + 1) <= 0)
        return len;

    /* Allocate buffer with calculated length */
    if ((*strp = calloc(1, len)) == NULL)
        return 0;

    return vsnprintf(*strp, len, fmt, ap);
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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_PRINTF_H__ */
