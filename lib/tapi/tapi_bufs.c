/** @file
 * @brief Test API to deal with buffers
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 * Copyright (C) 2004 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#define LGR_USER        "TAPI Buffers"
#include "logger_api.h"

#include "tapi_test.h"


/* See description in tapi_bufs.h */
void
tapi_fill_buf(void *buf, size_t len)
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


/* See description in tapi_bufs.h */
void *
tapi_make_buf(size_t min, size_t max, size_t *p_len)
{
    *p_len = rand_range(min, max);
    if (*p_len > 0)
    {
        void   *buf;

        buf = malloc(*p_len);
        if (buf == NULL)
        {
            ERROR("Memory allocation failure - EXIT");
            return NULL;
        }
        tapi_fill_buf(buf, *p_len);
        return buf;
    }
    else
    {
        return NULL;
    }
}
