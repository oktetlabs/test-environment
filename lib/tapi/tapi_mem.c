/** @file
 * @brief Test API to use malloc, calloc, realloc, memcpy and strdup
 *
 * Implementation of API to use memory-related functions in a convenient way
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
