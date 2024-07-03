/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief API to safely allocate memory
 *
 * Safe memory allocation.
 */

#define TE_LGR_USER     "TE Alloc"

#include "te_config.h"

#include <stdlib.h>
#include <string.h>

#include "logger_api.h"
#include "te_alloc.h"

/* See description in te_alloc.h */
void *
te_alloc_internal(size_t size, bool initialize,
                  const char *filename, int line)
{
    void *result;

    /*
     * Allocating zero bytes is formally permitted by ISO C but
     * it may either return NULL or some magic non-null pointer
     * that cannot be dereferenced. We force the latter variant
     * by actually allocating a single byte.
     */
    if (size == 0)
    {
        size = 1;
        WARN("Attempted to allocate a zero buffer at %s:%d", filename, line);
    }

    if (initialize)
        result = calloc(1, size);
    else
        result = malloc(size);
    if (result == NULL)
    {
        TE_FATAL_ERROR("Cannot allocate memory of size %zu at %s:%d",
                       size, filename, line);
    }
    return result;
}

void *
te_realloc_internal(void *oldptr, size_t newsize, const char *filename,
                    int line)
{
    void *result;

    if (oldptr == NULL)
        return te_alloc_internal(newsize, true, filename, line);

    /*
     * The allowed behaviour of `realloc(ptr, 0)` when @c ptr is not @c NULL
     * is even more bizarre than that of `malloc(0)`: ISO C not only permits
     * it to return NULL, the original pointer or some unique pointer like
     * `malloc(0)`, but also to **free** the provided memory and then return
     * @c NULL.
     *
     * POSIX 2008 explicitly prohibits such behaviour, and ISO C23 is going
     * to declare it as undefined, but glibc **does** indeed call @c free().
     *
     * See:
     * - https://pubs.opengroup.org/onlinepubs/9699919799/
     *   for POSIX-mandated behaviour;
     * - ISO/IEC 9899:2017 7.22.3.5.3 for C17 behaviour;
     * - ISO/IEC 9899:2023 7.24.3.7.3 for C23 behaviour;
     * - https://www.gnu.org/software/libc/manual/2.38/html_node/Changing-Block-Size.html
     *   for glibc behaviour.
     */
    if (newsize == 0)
    {
        TE_FATAL_ERROR("Attempted to realloc zero bytes at %s:%d",
                       filename, line);
    }
    result = realloc(oldptr, newsize);
    if (result == NULL)
    {
        TE_FATAL_ERROR("Cannot reallocate memory to size %zu at %s:%d",
                       newsize, filename, line);
    }
    return result;
}

/* See description in te_alloc.h */
void *
te_memdup_internal(const void *src, bool zero_terminated,
                   size_t maxsize, const char *filename, int line)
{
    void *copy;
    size_t copy_size = maxsize;

    if (src == NULL)
        return NULL;

    if (zero_terminated)
    {
#if defined(HAVE_STRNLEN)
        copy_size = strnlen(src, maxsize);
#else
        for (copy_size = 0;
             copy_size < maxsize && ((const char *)src)[copy_size] != '\0';
             copy_size++)
            ;
#endif
        maxsize = copy_size + 1;
    }

    copy = te_alloc_internal(maxsize, false, filename, line);
    memcpy(copy, src, copy_size);
    if (zero_terminated)
        ((char *)copy)[copy_size] = '\0';

    return copy;
}
