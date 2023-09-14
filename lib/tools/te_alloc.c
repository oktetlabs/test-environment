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

#include "logger_api.h"
#include "te_alloc.h"

/* See description in te_alloc.h */
void *
te_alloc_internal(size_t size, const char *filename, int line)
{
    void *result;

    /*
     * Allocating zero bytes is formally permitted by ISO C but
     * it may either return NULL or some magic non-null pointer
     * that cannot be dereferenced.
     */
    if (size == 0)
    {
        TE_FATAL_ERROR("Attempted to allocate a zero buffer at %s:%d",
                       filename, line);
    }

    result = calloc(1, size);
    if (result == NULL)
    {
        TE_FATAL_ERROR("Cannot allocate memory of size %zu at %s:%d",
                       size, filename, line);
    }
    return result;
}
