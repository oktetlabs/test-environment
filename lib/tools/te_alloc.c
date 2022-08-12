/** @file
 * @brief API to safely allocate memory
 *
 * Safe memory allocation
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TE Alloc"

#include "te_config.h"

#ifndef WINDOWS
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#endif
#endif

#include "logger_api.h"
#include "te_alloc.h"

#if SIZEOF_SIZE_T > SIZEOF_LONG
#define SIZE_T_FMT "%Lu"
#define SIZE_T_CAST(x) ((unsigned long long)(x))
#else
#define SIZE_T_FMT "%lu"
#define SIZE_T_CAST(x) ((unsigned long)(x))
#endif

/* See description in te_alloc.h */
void *
te_alloc_internal(size_t size, const char *filename, int line)
{
    void *result = calloc(1, size);

    if (result == NULL)
    {
        ERROR("Cannot allocate memory of size " SIZE_T_FMT " at %s:%d",
              SIZE_T_CAST(size),
              filename, line);
    }
    return result;
}
