/** @file
 * @brief Test Environment: RGT chunked output - manager.
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 * 
 * $Id$
 */

#include "rgt_co_mngr.h"


te_bool
rgt_co_mngr_valid(const rgt_co_mngr *mngr)
{
    return mngr != NULL &&
           mngr->used_mem <= mngr->max_mem;
}

rgt_co_mngr *
rgt_co_mngr_init(rgt_co_mngr *mngr, size_t max_mem)
{
    assert(mngr != NULL);

    mngr->max_mem     = max_mem;
    mngr->used_mem    = 0;
    mngr->first       = NULL;
    mngr->free        = NULL;

    return mngr;
}


rgt_co_chunk *
rgt_co_mngr_add_chunk(rgt_co_mngr *mngr, rgt_co_chunk *prev, size_t depth)
{
    rgt_co_chunk   *chunk;

    assert(rgt_co_mngr_valid(mngr));
    assert(prev == NULL || rgt_co_chunk_valid(prev));

    /* If there are any free chunks */
    if (mngr->free != NULL)
    {
        /* Take a free chunk */
        chunk = mngr->free;
        mngr->free = chunk->next;
    }
    else
    {
        /* Allocate a new chunk */
        chunk = malloc(sizeof(*chunk));
        if (chunk == NULL)
            return NULL;
    }

    /* Initialize chunk */
    rgt_co_chunk_init(chunk, depth);

    /* Link new chunk */
    if (prev == NULL)
    {
        chunk->next = mngr->first;
        mngr->first = chunk;
    }
    else
    {
        chunk->next = prev->next;
        prev->next = chunk;
    }

    return chunk;
}


void
rgt_co_mngr_del_chunk(rgt_co_mngr  *mngr,
                      rgt_co_chunk *prev,
                      rgt_co_chunk *chunk)
{
    assert(rgt_co_mngr_valid(mngr));
    assert(prev == NULL || rgt_co_chunk_valid(prev));
    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_co_chunk_is_void(chunk));
    assert((prev == NULL && mngr->first == chunk) ||
           (prev != NULL && prev->next == chunk));

    /* Unlink the chunk */
    if (prev == NULL)
        mngr->first = chunk->next;
    else
        prev->next = chunk->next;

    /* Link the chunk to the free list */
    chunk->next = mngr->free;
    mngr->free = chunk;
}


te_bool
rgt_co_mngr_chunk_append(rgt_co_mngr   *mngr,
                         rgt_co_chunk  *chunk,
                         const void    *ptr,
                         size_t         len)
{
    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(chunk));
    assert(ptr != NULL || len == 0);

    if (!rgt_co_chunk_append(chunk, ptr, len))
        return FALSE;

    if (rgt_co_chunk_is_mem(chunk))
        mngr->used_mem += len;

    return TRUE;
}


te_bool
rgt_co_mngr_chunk_finish(rgt_co_mngr *mngr, rgt_co_chunk *chunk)
{
    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(chunk));

    return rgt_co_chunk_finish(chunk);
}


te_bool
rgt_co_mngr_finished(const rgt_co_mngr *mngr)
{
    assert(rgt_co_mngr_valid(mngr));

    return mngr->first != NULL &&
           rgt_co_chunk_finished(mngr->first) &&
           mngr->first->next == NULL;
}


te_bool
rgt_co_mngr_clnp(rgt_co_mngr *mngr)
{
    rgt_co_chunk   *chunk;
    rgt_co_chunk   *next_chunk;

    for (chunk = mngr->free; chunk != NULL; chunk = next_chunk)
    {
        next_chunk = chunk->next;
        assert(rgt_co_chunk_is_void(chunk));
        /* Cleaning void chunk never fails */
        rgt_co_chunk_clnp(chunk);
        free(chunk);
    }

    for (chunk = mngr->first; chunk != NULL; chunk = next_chunk)
    {
        next_chunk = chunk->next;
        if (!rgt_co_chunk_clnp(chunk))
        {
            /* Just so we can resume later */
            mngr->first = chunk;
            return FALSE;
        }
        free(chunk);
    }

    return TRUE;
}
