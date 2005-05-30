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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_pch.h"

/** Chunk of reallocation of identifiers array */
#define IDS_CHUNK   128

#ifdef HAVE_PTHREAD_H
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void   **ids;            /**< Array of identifiers       */
static uint32_t ids_len;        /**< Current array length       */
static uint32_t used;           /**< Number of used identifiers */

/**
 * Allocate a piece of memory (if necessary) and assign the identifier 
 * to it.
 *
 * @param size      size of memory to be allocated 
 *                  (ignored if allocate is FALSE)
 * @param mem       if allocate is TRUE, location for real memory address 
 *                  (out); otherwise - location of real memory address (in)
 * @param allocate  if TRUE, allocate real memory in the routine; 
 *                  otherwise - register the memory provided by the user
 *
 * @return Memory identifier or 0 in the case of failure
 */
rcf_pch_mem_id 
rcf_pch_mem_alloc(size_t size, void **mem, te_bool allocate)
{
    void *m;
    
    rcf_pch_mem_id id = 0;
    
    if (allocate && (m = malloc(size)) == NULL)
        return 0;
    else if ((m = *mem) == NULL)
        return 0;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lock);
#endif

    if (ids_len == used)
    {
        void **tmp = realloc(ids, ids_len + IDS_CHUNK * sizeof(void *));
        
        if (tmp== NULL)
        {
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&lock);
#endif            
            if (allocate)
                free(m);
            return 0;
        }
            
        memset(tmp + ids_len, 0, IDS_CHUNK * sizeof(void *));
        
        *ids = tmp;
        id = ids_len;
        ids_len += IDS_CHUNK;
    }
    
    for (; ids[id] == NULL && id < ids_len; id++);
    
    assert(id < ids_len);
    ids[id] = m;
    used++;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lock);
#endif            
    
    if (allocate)
        *mem = m;
        
    return id + 1;
}

/**
 * Mark the memory identifier as "unused" and release the memory, 
 * if necessary.
 *
 * @param id       memory identifier returned by rcf_pch_mem_alloc
 * @param release  if TRUE, release the real memory
 */     
void 
rcf_pch_mem_free(rcf_pch_mem_id id, te_bool release)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lock);
#endif

    if (id > 0 && (id--, id < ids_len) && ids[id] != NULL)
    {
        if (release)
            free(ids[id]);
        ids[id] = NULL;
        used--;
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lock);
#endif
}

/**
 * Obtain address of the real memory by its identifier.
 *
 * @param id       memory identifier returned by rcf_pch_mem_alloc
 *
 * @return Memory address or NULL
 */
void *
rcf_pch_mem_get(rcf_pch_mem_id id)
{
    void *m = NULL;
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lock);
#endif

    if (id > 0 && (id--, id < ids_len)) 
        m = ids[id];

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lock);
#endif

    return m;
}

