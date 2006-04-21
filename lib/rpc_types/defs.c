/** @file
 * @brief RPC types definitions
 *
 * Macros to be used for all RPC types definitions.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
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
