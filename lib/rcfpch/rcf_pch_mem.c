/** @file 
 * @brief RCF Portable Command Handler
 *
 * Memory mapping library. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_stdint.h"
#include "ta_common.h"
#include "rcf_pch_mem.h"
#include "te_rpc_types.h"
#include "te_rpc_types.h"

/** Chunk of reallocation of identifiers array */
#define IDS_CHUNK   128

static void *lock = NULL;

static void   **ids;            /**< Array of identifiers       */
static uint32_t ids_len;        /**< Current array length       */
static uint32_t used;           /**< Number of used identifiers */

/**
 * Assign the identifier to memory.
 *
 * @param mem       location of real memory address (in)
 *
 * @return Memory identifier or 0 in the case of failure
 */
rcf_pch_mem_id 
rcf_pch_mem_alloc(void *mem)
{
    rcf_pch_mem_id id = 0;

    if (mem == NULL)
        return 0;

    if (lock == NULL)
        lock = thread_mutex_create();
        
    thread_mutex_lock(lock);
    
    if (ids_len == used)
    {
        void **tmp = NULL;
        
        if (ids_len + IDS_CHUNK >= RPC_UNKNOWN_ADDR)
        {
            fprintf(stderr, "Too many memory addresses have id now!");
            thread_mutex_unlock(lock);
            return 0;
        }

        tmp = realloc(ids, (ids_len + IDS_CHUNK) * sizeof(void *));

        if (tmp == NULL)
        {
            fprintf(stderr, "Out of memory!");
            thread_mutex_unlock(lock);
            return 0;
        }
            
        memset(tmp + ids_len, 0, IDS_CHUNK * sizeof(void *));
        
        ids = tmp;
        id = ids_len;
        ids_len += IDS_CHUNK;
    }
    
    for (; ids[id] != NULL && id < ids_len; id++);
    
    assert(id < ids_len);
    ids[id] = mem;
    used++;

    thread_mutex_unlock(lock);

    return id + 1;
}

/**
 * Mark the memory identifier as "unused".
 *
 * @param id       memory identifier returned by rcf_pch_mem_alloc
 */     
void 
rcf_pch_mem_free(rcf_pch_mem_id id)
{
    if (lock == NULL)
        lock = thread_mutex_create();
        
    thread_mutex_lock(lock);

    /* Convert id to array index */
    if (id > 0 && (id--, id < ids_len) && ids[id] != NULL)
    {
        ids[id] = NULL;
        used--;
    }

    thread_mutex_unlock(lock);
}

/**
 * Mark the memory identifier corresponding to memory address as "unused".
 *
 * @param mem   memory address
 */     
void 
rcf_pch_mem_free_mem(void *mem)
{
    rcf_pch_mem_id id;
    
    if (lock == NULL)
        lock = thread_mutex_create();
        
    thread_mutex_lock(lock);

    for (id = 0; id < ids_len && ids[id] != mem; id++);
    
    if (id < ids_len)
    {
        ids[id] = NULL;
        used--;
    }

    thread_mutex_unlock(lock);
}

/**
 * Obtain address of the real memory by its identifier.
 *
 * @param id       memory identifier returned by rcf_pch_mem_alloc
 *
 * @return Memory address or NULL
 */
char *
rcf_pch_mem_get(rcf_pch_mem_id id)
{
    void *m = NULL;

    if (lock == NULL)
        lock = thread_mutex_create();
        
    thread_mutex_lock(lock);

    if (id > 0 && (id--, id < ids_len)) 
        m = ids[id];

    thread_mutex_unlock(lock);

    return m;
}

/**
 * Find memory identifier by memory address.
 *
 * @param mem   memory address
 *
 * @return memory identifier or 0
 */     
rcf_pch_mem_id 
rcf_pch_mem_get_id(void *mem)
{
    rcf_pch_mem_id id;

    if (mem == NULL)
        return 0;
    if (lock == NULL)
        lock = thread_mutex_create();
        
    thread_mutex_lock(lock);

    for (id = 0; id < ids_len && ids[id] != mem; id++);
    
    if (id < ids_len)
        id++;
    else
        id = 0;

    thread_mutex_unlock(lock);

    return id;
}
