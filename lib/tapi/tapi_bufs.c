/** @file
 * @brief Test API to deal with buffers
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Buffers"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "logger_api.h"


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
