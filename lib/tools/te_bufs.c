/** @file
 * @brief API to deal with buffers
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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
    assert(min <= max);
    *p_len = rand_range(min, max);
    if (*p_len > 0)
    {
        void   *buf;

        buf = malloc(*p_len);
        if (buf == NULL)
        {
            ERROR("Memory allocation failure - EXIT");
            /* we can do nothing in case we have no memory */
            exit(1);
        }
        te_fill_buf(buf, *p_len);
        return buf;
    }
    else
    {
        ERROR("%s: min > max given for allocation length",
              __FUNCTION__);
        /* why it's good to call exit at this place:
         * all current users of the function think that NULL which
         * is returned is in fact a ENOMEM case, not EINVAL. And they
         * either think that it can't happen or think that they can
         * do nothing. In any case - proper error handling is not
         * implemented. exit(1) will signal that somthing bad happened
         * and print corresponding error message.
         * */
        exit(1);
    }
}
