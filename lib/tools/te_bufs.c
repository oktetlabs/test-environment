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
#include "te_hex_diff_dump.h"

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

/* See description in te_bufs.h */
void
te_fill_printable_buf(void *buf, size_t len)
{
    size_t i;

    assert(len > 0);
    for (i = 0; i < len - 1; i++)
    {
        /* the range of ASCII printable characters */
        ((char *)buf)[i] = rand_range(' ', '~');
    }
    ((char *)buf)[len - 1] = '\0';
}


/* See description in te_bufs.h */
char *
te_make_printable_buf(size_t min, size_t max, size_t *p_len)
{
    size_t len;
    void *buf;

    assert(min > 0);
    assert(min <= max);
    len = rand_range(min, max);

    buf = TE_ALLOC(len);
    if (buf == NULL)
        TE_FATAL_ERROR("Memory allocation failure");

    te_fill_printable_buf(buf, len);
    if (p_len != NULL)
        *p_len = len;

    return buf;
}


te_bool
te_compare_bufs(const void *exp_buf, size_t exp_len,
                unsigned int n_copies,
                const void *actual_buf, size_t actual_len,
                unsigned int log_level)
{
    te_bool result = TRUE;
    size_t offset = 0;

    if (n_copies * exp_len != actual_len)
    {
        /* If we don't log anything, there's no need to look for more diffs. */
        if (log_level == 0)
            return FALSE;

        LOG_MSG(log_level, "Buffer lengths are not equal: "
                "%zu * %u != %zu", exp_len, n_copies, actual_len);

        result = FALSE;
    }

    while (n_copies != 0)
    {
        size_t chunk_len = MIN(exp_len, actual_len);

        if (memcmp(exp_buf, actual_buf, chunk_len) != 0 ||
            chunk_len < exp_len)
        {
            if (log_level == 0)
                return FALSE;

            result = FALSE;
            LOG_HEX_DIFF_DUMP_AT(log_level, exp_buf, exp_len,
                                 actual_buf, chunk_len, offset);
        }
        offset += chunk_len;
        actual_buf = (const uint8_t *)actual_buf + chunk_len;
        actual_len -= chunk_len;
        n_copies--;
    }

    if (actual_len > 0 && log_level != 0)
    {
        LOG_HEX_DIFF_DUMP_AT(log_level, exp_buf, 0,
                             actual_buf, actual_len, offset);
    }

    return result;
}
