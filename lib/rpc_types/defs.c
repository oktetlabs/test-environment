/** @file
 * @brief RPC types definitions
 *
 * Macros to be used for all RPC types definitions.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_rpc_defs.h"


/* See the description in te_rpc_defs.h */
const char *
bitmask2str(struct rpc_bit_map_entry *maps, unsigned int val)
{
    /* Number of buffers used in the function */
#define N_BUFS 10
#define BUF_SIZE 1024
#define BIT_DELIMETER " | "

    static char buf[N_BUFS][BUF_SIZE];
    static char (*cur_buf)[BUF_SIZE] = (char (*)[BUF_SIZE])buf[0];

    char *ptr;
    int   i;

    /*
     * Firt time the function is called we start from the second buffer, but
     * then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[BUF_SIZE])buf[N_BUFS - 1])
        cur_buf = (char (*)[BUF_SIZE])buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;
    *ptr = '\0';

    for (i = 0; maps[i].str_val != NULL; i++)
    {
        if (val & maps[i].bit_val)
        {
            snprintf(ptr + strlen(ptr), BUF_SIZE - strlen(ptr),
                     "%s" BIT_DELIMETER, maps[i].str_val);
            /* clear processed bit */
            val &= (~maps[i].bit_val);
        }
    }
    if (val != 0)
    {
        /* There are some unprocessed bits */
        snprintf(ptr + strlen(ptr), BUF_SIZE - strlen(ptr),
                 "0x%x" BIT_DELIMETER, val);
    }

    if (strlen(ptr) == 0)
    {
        snprintf(ptr, BUF_SIZE, "0");
    }
    else
    {
        ptr[strlen(ptr) - strlen(BIT_DELIMETER)] = '\0';
    }

    return ptr;

#undef BIT_DELIMETER
#undef N_BUFS
#undef BUF_SIZE
}
