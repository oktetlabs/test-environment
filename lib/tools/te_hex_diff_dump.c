/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to log a diff between two binary blocks of memory
 *
 * Implementation of functions to create and log hex diff dumps of two
 * binary memory blocks.
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <inttypes.h>
#include "te_defs.h"
#include "te_hex_diff_dump.h"

#define BYTES_PER_LINE 8

/* Left mark + 2 hex digits + right mark. */
#define CHARS_PER_BYTE 4

#define COLUMN_WIDTH (BYTES_PER_LINE * CHARS_PER_BYTE)

static void
hex_diff_half_line(te_string *dest,
                   const uint8_t *this_side, size_t this_len,
                   const uint8_t *other_side, size_t other_len,
                   int indent)
{
    size_t i;

    te_string_append(dest, "|%*s", indent * CHARS_PER_BYTE, "");

    for (i = 0; i < this_len; i++)
    {
        bool equal = i < other_len && other_side[i] == this_side[i];

        te_string_append(dest, "%c%02" PRIx8 "%c",
                         equal ? ' ' : '>',
                         this_side[i],
                         equal ? ' ' : '<');
    }

    te_string_append(dest, "%*s", (BYTES_PER_LINE - indent - this_len) *
                     CHARS_PER_BYTE, "");
}

void
te_hex_diff_dump(const void *expected, size_t exp_len,
                 const void *actual, size_t actual_len, size_t offset,
                 te_string *dest)
{

    const uint8_t *exp_ptr = expected;
    const uint8_t *act_ptr = actual;
    bool skip = false;
    size_t max_len = MAX(exp_len, actual_len);

    te_string_append(dest, "%8s|", "");
    te_string_add_centered(dest, " Expected ", COLUMN_WIDTH, '=');
    te_string_append(dest, "|");
    te_string_add_centered(dest, " Actual ", COLUMN_WIDTH, '=');
    te_string_append(dest, "\n");

    while (max_len > 0)
    {
        int indent = (int)(offset % BYTES_PER_LINE);
        size_t frag_len = MIN(max_len, BYTES_PER_LINE - indent);

        if (exp_len > frag_len && actual_len > frag_len &&
            memcmp(exp_ptr, act_ptr,
                   MIN(frag_len, MIN(exp_len, actual_len))) == 0)
        {
            if (!skip)
            {
                te_string_append(dest, "%9s%*s\n", "...",
                                 COLUMN_WIDTH + 1, "...");
            }
            skip = true;
        }
        else
        {
            te_string_append(dest, "%08zx", offset / BYTES_PER_LINE *
                             BYTES_PER_LINE);
            hex_diff_half_line(dest, exp_ptr, MIN(exp_len, frag_len),
                               act_ptr, MIN(actual_len, frag_len), indent);
            hex_diff_half_line(dest, act_ptr, MIN(actual_len, frag_len),
                               exp_ptr, MIN(exp_len, frag_len), indent);
            te_string_append(dest, "\n");

            skip = false;
        }

        if (exp_len > 0)
        {
            exp_ptr += MIN(frag_len, exp_len);
            exp_len -= MIN(frag_len, exp_len);
        }
        if (actual_len > 0)
        {
            act_ptr += MIN(frag_len, actual_len);
            actual_len -= MIN(frag_len, actual_len);
        }
        offset += frag_len;
        max_len -= frag_len;
    }
}

void
te_hex_diff_log(const void *expected, size_t exp_len,
                const void *actual, size_t actual_len,
                size_t offset,
                const char *file, int line, unsigned int level,
                const char *entity, const char *user)
{
    te_string str = TE_STRING_INIT;

    te_hex_diff_dump(expected, exp_len, actual, actual_len, offset, &str);
    te_log_message(file, line, level, entity, user, "%s", str.ptr);
    te_string_free(&str);
}
