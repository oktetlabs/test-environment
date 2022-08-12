/** @file
 * @brief API to log a diff between two binary blocks of memory
 *
 * Implementation of functions to create and log hex diff dumps of two
 * binary memory blocks.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 * @author Sergey Nikitin <Sergey.Nikitin@oktetlabs.ru>
 */

#include <stdio.h>

#include "te_hex_diff_dump.h"
#include "te_str.h"

/* Number of printed bytes per line. */
#define DIFF_LINE_WIDTH 8

static void
diff_dump_line(const void *ex_frag, const void *rx_frag, size_t size,
               char *ex_str, char *rx_str)
{
    int len = 0;
    const uint8_t *ex_frag_p = ex_frag;
    const uint8_t *rx_frag_p = rx_frag;

    while (size > 0)
    {
        te_bool equal = *ex_frag_p == *rx_frag_p;
        char sep_left = equal ? ' ' : '>';
        char sep_right = equal ? ' ' : '<';

        sprintf(ex_str + len, "%c%02x%c", sep_left, *ex_frag_p, sep_right);
        len += sprintf(rx_str + len, "%c%02x%c", sep_left, *rx_frag_p, sep_right);
        ++ex_frag_p;
        ++rx_frag_p;
        --size;
    }
}

void
te_hex_diff_dump(const void *ex_pkt, const void *rx_pkt, size_t size,
                 int (*log_func)(const char *s))
{
    /* 2 symbols for a hexbyte, 2 symbols for separators, + terminating null. */
    char exp_line_str[DIFF_LINE_WIDTH * 4 + 1];
    char rx_line_str[DIFF_LINE_WIDTH * 4 + 1];
    int offset = 0;

    log_func("======================Expected============="
             "|=============Received=============");

    while (size > 0)
    {
        char line[128];
        size_t frag_len = (DIFF_LINE_WIDTH < size) ? DIFF_LINE_WIDTH : size;

        diff_dump_line(ex_pkt, rx_pkt, frag_len, exp_line_str, rx_line_str);
        te_snprintf(line, sizeof(line), "%08x %-32s  |  %-32s", offset,
                    exp_line_str, rx_line_str);
        log_func(line);

        offset += DIFF_LINE_WIDTH;
        ex_pkt = (const void *)((uintptr_t)ex_pkt + frag_len);
        rx_pkt = (const void *)((uintptr_t)rx_pkt + frag_len);
        size -= frag_len;
    }
}
