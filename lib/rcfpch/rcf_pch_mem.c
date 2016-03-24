/** @file
 * @brief RCF Portable Command Handler
 *
 * Memory mapping library.
 *
 * Copyright (C) 2003-2016 Test Environment authors (see file AUTHORS in the
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
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#define TE_LGR_USER "rcfpch"

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

#include "logger_api.h"

/** The initial size of an array */
#define INITIAL_SIZE_OF_ARRAY       128

/** The multiplier of an array size */
#define MULTIPLIER_OF_ARRAY_SIZE    2

/**
 * Wrapper to call @b reallocate_memory to increase the number of available
 * elements in @b ids
 */
#define IDS_REALLOCATE()                        \
    reallocate_memory(sizeof(id_node),          \
                      RPC_PTR_ID_INDEX_LIMIT,   \
                      (void *)&ids, &ids_len)

/** Maximum index value in @b namespaces in a pointer id */
#define RPC_PTR_ID_NS_LIMIT         (1 << RPC_PTR_ID_NS_BITCOUNT)

/**
 * Wrapper to call @b reallocate_memory to increase the number of available
 * elements in @b namespaces
 */
#define NAMESPACES_REALLOCATE()                             \
    reallocate_memory(sizeof(char *),                       \
                      RPC_PTR_ID_NS_LIMIT,                  \
                      (void *)&namespaces, &namespaces_len)

/** Attributes of id structure (memory pointer) */
typedef struct {
    rpc_ptr_id_namespace    ns;     /**< Memory pointer namespace */
    void                   *memory; /**< Memory pointer */
    te_bool                 used;   /**< If the node is used */
} id_node;

/** Synchronization object */
static void *lock = NULL;

/** Array of identifiers */
static id_node *ids = NULL;

/** Current array @ids length */
static size_t ids_len = 0;

/** Number of used identifiers */
static size_t ids_used = 0;

/**
 * The index from which to start looking for the next free element in
 * @p ids
 */
static rpc_ptr_id_index next_free = 0;

/** Array of namespaces */
static char **namespaces = NULL;

/** Current array @b namespaces length */
static size_t namespaces_len = 0;

/**
 * Compare @p node with given values @p ns and @p memory
 *
 * @param [in] node     Compared node
 * @param [in] ns       Namespace
 * @param [in] memory   Memory pointer
 *
 * @return @c TRUE in case of equality and @c FALSE in other cases
 */
static te_bool
id_nodes_equal(const id_node *node, const rpc_ptr_id_namespace ns,
               const void *memory)
{
    return node->ns == ns && node->memory == memory;
}

/**
 * Allocate a new chunk of memory
 *
 * @param [in]    item_size     Size of one element of the @b array
 * @param [in]    limit         Limit maximum number of elements in the
 *                              @b array
 * @param [inout] array         Pointer to an array for reallocate memory
 * @param [inout] array_len     Number of elements in the @b array
 *
 * @return Status code
 */
static te_errno
reallocate_memory(size_t item_size, size_t limit,
                  void **array, size_t *array_len)
{
    void           *tmp         = NULL;
    const size_t    current     = *array_len;
    const size_t    required    = (current == 0)
            ? INITIAL_SIZE_OF_ARRAY : (current * MULTIPLIER_OF_ARRAY_SIZE);

    if (required > limit)
    {
        ERROR("Elements limit is reached");
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }

    tmp = realloc(*array, required * item_size);

    if (tmp == NULL)
    {
        ERROR("Out of memory!");
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }

    memset(tmp + current * item_size, 0, (required - current) * item_size);

    *array = tmp;
    *array_len = required;
    return 0;
}

/**
 * Find the first unused element
 *
 * @return Index of unused element or
 *         @b next_free (if all elements are used)
 */
static rpc_ptr_id_index
search_id_index()
{
    rpc_ptr_id_index i;
    for (i = next_free; i < ids_len; i++)
    {
        if (!ids[i].used)
            return i;
    }
    for (i = 0; i < next_free; i++)
    {
        if (!ids[i].used)
            return i;
    }
    return next_free;
}

/**
 * Acquire a @p index of free node
 *
 * @param [out] index   Returned @p index of the free @b id_node in the
 *                      @b ids
 *
 * @return Status code
 */
static te_errno
take_index(rpc_ptr_id_index *index)
{
    te_errno            rc;
    rpc_ptr_id_index    id_index;

    if (ids_used < ids_len)
        id_index = search_id_index();
    else
    {
        id_index = ids_len;
        rc = IDS_REALLOCATE();
        if (rc != 0)
            return rc;
    }

    if (ids[id_index].used)
        return TE_RC(TE_RCF_PCH, TE_EFAIL);

    *index = id_index;
    next_free = (id_index + 1) % ids_len;
    return 0;
}

/**
 * Release a @p index of used node
 *
 * @param index     @p index of the used @b id_node in the @b ids
 *
 * @return Status code
 */
static te_errno
give_index(rpc_ptr_id_index index)
{
    if (ids[index].memory  == NULL ||
        ids[index].ns      == RPC_PTR_ID_NS_INVALID ||
        !ids[index].used)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    ids[index].memory   = NULL;
    ids[index].ns       = RPC_PTR_ID_NS_INVALID;
    ids[index].used     = FALSE;
    ids_used--;

    return 0;
}

/**
 * Determine a namespace id by the string of namespace without
 * synchronization
 *
 * @param [in]  ns_string   Namespace as string
 * @param [out] ns_id       Namespace id
 *
 * @return Status code
 */
static te_errno
ns_get_index(const char *ns_string, rpc_ptr_id_namespace *ns_id)
{
    rpc_ptr_id_namespace    i = 0;
    te_errno                rc;

    if (namespaces != NULL)
    {
        for (; i < namespaces_len && namespaces[i] != NULL; i++)
        {
            if (strcmp(namespaces[i], ns_string) == 0)
            {
                *ns_id = i;
                return 0;
            }
        }
    }
    if (namespaces == NULL || i == namespaces_len)
    {
        rc = NAMESPACES_REALLOCATE();
        if (rc != 0)
            return rc;
    }

    namespaces[i] = strdup(ns_string);
    if (namespaces[i] == NULL)
    {
        ERROR("Not enough memory! (%d, '%s')", i, ns_string);
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }

    *ns_id = i;
    return 0;
}

/* See description in rcf_pch_mem.h */
void
rcf_pch_mem_init()
{
    if (lock == NULL)
        lock = thread_mutex_create();
}

/* See description in rcf_pch_mem.h */
rpc_ptr
rcf_pch_mem_index_alloc(void *mem, rpc_ptr_id_namespace ns,
                        const char *caller_func, int caller_line)
{
    te_errno            rc;
    rpc_ptr_id_index    index;

    if (mem == NULL)
    {
        VERB("%s:%d: Don't try to allocate the null pointer",
             caller_func, caller_line);
        return 0;
    }

    thread_mutex_lock(lock);

    rc = take_index(&index);
    if (rc != 0)
    {
        ERROR("%s:%d: Taking index fails (rc=%r)",
              caller_func, caller_line, rc);
        errno = TE_RC_GET_ERROR(rc);
        thread_mutex_unlock(lock);
        return 0;
    }

    assert(index < ids_len);
    ids[index].memory   = mem;
    ids[index].ns       = ns;
    ids[index].used     = TRUE;
    ids_used++;

    thread_mutex_unlock(lock);
    return RPC_PTR_ID_MAKE(ns, index);
}

/* See description in rcf_pch_mem.h */
te_errno
rcf_pch_mem_index_free(rpc_ptr id, rpc_ptr_id_namespace ns,
                       const char *caller_func, int caller_line)
{
    rpc_ptr_id_index    index;
    te_errno            rc;

    if (id == 0)
    {
        VERB("%s:%d: Don't try to find id with 0 value",
             caller_func, caller_line);
        return 0;
    }

    if (RPC_PTR_ID_GET_NS(id) != ns)
    {
        ERROR("%s:%d: Incorrect namespace %d != %d (id = %d)",
              caller_func, caller_line, RPC_PTR_ID_GET_NS(id), ns, id);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    thread_mutex_lock(lock);

    /* Convert id to array index */
    index = RPC_PTR_ID_GET_INDEX(id);
    if (index < ids_len && ids[index].memory != NULL && ids[index].ns == ns)
        rc = give_index(index);
    else
    {
        if (ids[index].memory == NULL &&
                ids[index].ns == RPC_PTR_ID_NS_INVALID &&
                !ids[index].used)
        {
            ERROR("%s:%d: Possible double free or corruption "
                  "(id=%d, ns=%d)",
                  caller_func, caller_line, id, ns);
        }
        else
        {
            ERROR("%s:%d: Incorrect namespace for the memory id "
                  "(%p, %d != %d)",
                  caller_func, caller_line, ids[index].memory,
                  ids[index].ns, ns);
        }
        rc = TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    thread_mutex_unlock(lock);
    return rc;
}

/* See description in rcf_pch_mem.h */
te_errno
rcf_pch_mem_index_free_mem(void *mem, rpc_ptr_id_namespace ns,
                           const char *caller_func, int caller_line)
{
    rpc_ptr_id_index    index = 0;
    te_errno            rc;

    if (mem == NULL)
    {
        VERB("%s:%d: Don't try to find the null pointer",
             caller_func, caller_line);
        return 0;
    }

    thread_mutex_lock(lock);

    while (index < ids_len && !id_nodes_equal(&ids[index], ns, mem))
        index++;

    if (index < ids_len)
        rc = give_index(index);
    else
    {
        ERROR("%s:%d: The memory pointer isn't found (%p, %d)",
              caller_func, caller_line, mem, ns);
        rc = TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    thread_mutex_unlock(lock);
    return rc;
}

/* See description in rcf_pch_mem.h */
void *
rcf_pch_mem_index_mem_to_ptr(rpc_ptr id, rpc_ptr_id_namespace ns,
                             const char *caller_func, int caller_line)
{
    rpc_ptr_id_index    index;
    void               *memory = NULL;

    if (id == 0)
        return NULL;

    if (RPC_PTR_ID_GET_NS(id) != ns)
    {
        ERROR("%s:%d: Incorrect namespace %d != %d (id = %d)",
              caller_func, caller_line, RPC_PTR_ID_GET_NS(id), ns, id);
        return NULL;
    }

    thread_mutex_lock(lock);

    /* Convert id to array index */
    index = RPC_PTR_ID_GET_INDEX(id);
    if (index < ids_len && ids[index].ns == ns)
        memory = ids[index].memory;
    else
    {
        if (ids[index].memory == NULL &&
                ids[index].ns == RPC_PTR_ID_NS_INVALID &&
                !ids[index].used)
        {
            ERROR("%s:%d: Incorrect access to released object "
                  "(%d, %d)",
                  caller_func, caller_line, id, ns);
        }
        else
        {
            ERROR("%s:%d: Incorrect namespace for the memory id "
                  "(%p, %d != %d)",
                  caller_func, caller_line, ids[index].memory,
                  ids[index].ns, ns);
        }
    }

    thread_mutex_unlock(lock);
    return memory;
}

/* See description in rcf_pch_mem.h */
rpc_ptr
rcf_pch_mem_index_ptr_to_mem(void *mem, rpc_ptr_id_namespace ns,
                             const char *caller_func, int caller_line)
{
    rpc_ptr             id = 0;
    rpc_ptr_id_index    index = 0;

    if (mem == NULL)
        return 0;

    thread_mutex_lock(lock);

    while (index < ids_len && !id_nodes_equal(&ids[index], ns, mem))
        index++;

    if (index < ids_len)
        id = RPC_PTR_ID_MAKE(ns, index);
    else
    {
        ERROR("%s:%d: The memory pointer isn't found (%p, %d)",
              caller_func, caller_line, mem, ns);
    }

    thread_mutex_unlock(lock);
    return id;
}

/* See description in rcf_pch_mem.h */
rpc_ptr_id_namespace
rcf_pch_mem_ns_generic()
{
    static rpc_ptr_id_namespace ns = RPC_PTR_ID_NS_INVALID;
    if (RCF_PCH_MEM_NS_CREATE_IF_NEEDED(&ns, RPC_TYPE_NS_GENERIC) != 0)
        return RPC_PTR_ID_NS_INVALID;
    return ns;
}

/* See description in rcf_pch_mem.h */
te_errno
rcf_pch_mem_ns_get_index(const char *ns_string, rpc_ptr_id_namespace *ns_id)
{
    te_errno rc;
    thread_mutex_lock(lock);
    rc = ns_get_index(ns_string, ns_id);
    thread_mutex_unlock(lock);
    return rc;
}

/* See description in rcf_pch_mem.h */
te_errno
rcf_pch_mem_ns_get_string(
        rpc_ptr_id_namespace ns_id, const char **ns_string)
{
    if (ns_id >= namespaces_len || namespaces == NULL ||
            namespaces[ns_id] == NULL)
    {
        ERROR("Invalid namespace index (%d < %d)",
              ns_id, namespaces_len);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    *ns_string = namespaces[ns_id];
    return 0;
}
