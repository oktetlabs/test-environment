/** @file
 * @brief ACSE memory management lib
 *
 * ACSE memory management lib, implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include "acse_mem.h"

/** Descriptor for memory block in heap.
  This struct is placed to the end of any allocated block.
*/
typedef struct {
    void   *ptr;       /**< start of next block in the heap or NULL */
    size_t  user_size; /**< size of user-part of next block or 0. */
} next_block_descr_t;

/** Descriptor of memory heap. */
typedef struct {
    mheap_t             id;    /**< Heap ID, really index in array */

    next_block_descr_t  first; /**< Descriptor of first block in heap */

    void   *users[MHEAP_MAX_USERS];  /**< users of this heap */
    size_t  n_users;                 /**< number of users */
} mheap_descr_t;

static mheap_descr_t *heaps_table = NULL;
static size_t heaps_table_size = 0;

/** Quantum to increase heaps table */
#define TABLE_SIZE_BLOCK 32


/** Clear particular heap descriptor */
static inline void
mheap_clear(mheap_t heap)
{
    size_t i;

    heaps_table[heap].id = MHEAP_NONE;
    heaps_table[heap].first.ptr = NULL;
    heaps_table[heap].first.user_size = 0;
    heaps_table[heap].n_users = 0;

    for (i = 0; i < MHEAP_MAX_USERS; i++)
        heaps_table[heap].users[i] = NULL;
}

/** Increase heaps table by @c TABLE_SIZE_BLOCK */
void
increase_heaps_table(void)
{
    size_t i;
    size_t new_size = heaps_table_size + TABLE_SIZE_BLOCK;

    heaps_table = realloc(heaps_table, new_size * sizeof(mheap_descr_t));
    for (i = heaps_table_size; i < new_size; i++)
        mheap_clear(i);

    heaps_table_size = new_size;
}

mheap_t
mheap_create(void *user)
{
    size_t i;

    for (i = 0; i < heaps_table_size; i++)
    {
        if (MHEAP_NONE == heaps_table[i].id)
            break;
    }
    if (i == heaps_table_size)
        increase_heaps_table();

    heaps_table[i].id = i;
    heaps_table[i].users[0] = user;
    heaps_table[i].n_users = 1;

    return i;
}

int
mheap_add_user(mheap_t heap, void *user)
{
    size_t i;

    if (heap >= heaps_table_size || heaps_table[heap].id != heap)
        return -1;

    for (i = 0; i < MHEAP_MAX_USERS; i++)
        if (user == heaps_table[heap].users[i])
            return 0;

    if (heaps_table[heap].n_users >= MHEAP_MAX_USERS)
        return -1;

    for (i = 0; i < MHEAP_MAX_USERS; i++)
    {
        if (NULL == heaps_table[heap].users[i])
        {
            heaps_table[heap].users[i] = user;
            heaps_table[heap].n_users++;
            return 0;
        }
    }
    return -1;
}

/** Free all blocks in the heap, clear the heap.
 *
 * @param heap          heap ID
 */
void
mheap_free_heap(mheap_t heap)
{
    next_block_descr_t bd;

    if (heap >= heaps_table_size)
        return;

    bd = heaps_table[heap].first;
    while (NULL != bd.ptr)
    {
        void *ptr = bd.ptr;
        memcpy(&bd, bd.ptr + bd.user_size, sizeof(bd));
        free(ptr);
    }
    mheap_clear(heap);
}


void *
mheap_alloc(mheap_t heap, size_t n)
{
    next_block_descr_t nbd;
    void *ptr;

    if (heap >= heaps_table_size || heaps_table[heap].id != heap)
        return NULL;

    ptr = malloc(n + sizeof(next_block_descr_t));

    if (NULL == ptr)
        return NULL;

    /* now insert new block to the head of list in heap */
    nbd.ptr = heaps_table[heap].first.ptr;
    nbd.user_size = heaps_table[heap].first.user_size;
    memcpy(ptr + n, &nbd, sizeof(nbd));

    heaps_table[heap].first.ptr = ptr;
    heaps_table[heap].first.user_size = n;

    return ptr;
}


void
mheap_free_user(mheap_t heap, const void *user)
{
    size_t i;

    if (MHEAP_NONE == heap)
    {
        for (heap = 0; heap < heaps_table_size; heap++)
            mheap_free_user(heap, user);
        return;
    }

    if (heap >= heaps_table_size)
        return;

    for (i = 0; i < MHEAP_MAX_USERS; i++)
    {
        if (heaps_table[heap].users[i] == user)
        {
            heaps_table[heap].users[i] = NULL;
            heaps_table[heap].n_users--;

            if (0 == heaps_table[heap].n_users)
                mheap_free_heap(heap);

            break;
        }
    }
}

