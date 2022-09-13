/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to deal with buffers
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TE Buffers"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "te_alloc.h"

/* See description in te_bufs.h */
void
te_fill_buf(void *buf, size_t len)
{
    size_t  i;

    for (i = 0; i < len / sizeof(int); ++i)
    {
        ((int *)buf)[i] = rand();
    }
    for (i = (len / sizeof(int)) * sizeof(int); i < len; ++i)
    {
        ((unsigned char *)buf)[i] = rand();
    }
}


/* See description in te_bufs.h */
void *
te_make_buf(size_t min, size_t max, size_t *p_len)
{
    void *buf;

    assert(min <= max);
    *p_len = rand_range(min, max);

    /*
     * There is nothing wrong with asking for a zero-length buffer.
     * However, ISO C and POSIX allow NULL or non-NULL to be returned
     * in this case, so we allocate a single-byte buffer instead.
     */
    buf = TE_ALLOC(*p_len == 0 ? 1 : *p_len);
    if (buf == NULL)
    {
        /* FIXME: see issue #12079 for a consisent solution */
        ERROR("Memory allocation failure - EXIT");
        exit(1);
    }
    te_fill_buf(buf, *p_len);
    return buf;
}
