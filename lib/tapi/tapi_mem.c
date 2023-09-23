/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to use malloc, calloc, realloc, memcpy and strdup
 *
 * Implementation of API to use memory-related functions in a convenient way.
 *
 * Currently these functions delegate all the work to functions from
 * te_alloc.h, but this may change in the future.
 */

#define TE_LGR_USER     "TAPI Mem"

#include "te_config.h"

#include "logger_api.h"
#include "te_alloc.h"
#include "tapi_test_log.h"

void *
tapi_malloc(size_t size)
{
    return TE_ALLOC_UNINITIALIZED(size);
}

void *
tapi_calloc(size_t nmemb, size_t size)
{
    if (!te_is_valid_alloc(nmemb, size))
    {
        TEST_FAIL("%zu * %zu does not fit into size_t",
                  nmemb, size);
    }
    return TE_ALLOC(nmemb * size);
}

void *
tapi_realloc(void *ptr, size_t size)
{
    TE_REALLOC(ptr, size);
    return ptr;
}

void *
tapi_memdup(const void *ptr, size_t size)
{
    return TE_MEMDUP(ptr, size);
}

char *
tapi_strdup(const char *s)
{
    return TE_STRDUP(s);
}

char *
tapi_strndup(const char *s, size_t size)
{
    return TE_STRNDUP(s, size);
}
