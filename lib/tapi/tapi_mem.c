/** @file
 * @brief Test API to use malloc, calloc, realloc, memcpy and strdup
 *
 * Implementation of API to use memory-related functions in a convenient way
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TAPI Mem"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#include "logger_api.h"
#include "tapi_test_log.h"

void *
tapi_malloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    if (ptr == NULL)
    {
        TEST_FAIL("Failed to allocate memory");
    }

    return ptr;
}

void *
tapi_calloc(size_t nmemb, size_t size)
{
    void *ptr;

    ptr = calloc(nmemb, size);
    if (ptr == NULL)
    {
        TEST_FAIL("Failed to allocate memory");
    }

    return ptr;
}

void *
tapi_realloc(void *ptr, size_t size)
{
    void *new_ptr;

    new_ptr = realloc(ptr, size);
    if (new_ptr == NULL)
    {
        TEST_FAIL("Failed to re-allocate memory");
    }

    return new_ptr;
}

void *
tapi_memdup(const void *ptr, size_t size)
{
    void *new_ptr;

    new_ptr = tapi_malloc(size);
    new_ptr = memcpy(new_ptr, ptr, size);

    return new_ptr;
}

char *
tapi_strdup(const char *s)
{
    char *dup;

    dup = strdup(s);
    if (dup == NULL)
    {
        TEST_FAIL("Failed to duplicate string");
    }

    return dup;
}

char *
tapi_strndup(const char *s, size_t size)
{
    char *dup;

    dup = strndup(s, size);
    if (dup == NULL)
    {
        TEST_FAIL("Failed to duplicate string");
    }

    return dup;
}
