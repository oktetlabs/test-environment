/** @file
 * @brief ACSE memory management API
 *
 * ACSE memory management utils, API
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 */

#ifndef __TE_ACSE_MEM_H__
#define __TE_ACSE_MEM_H__


#include <sys/types.h>

/** Identifier of particular heap. */
typedef unsigned int mheap_t;

/* constants for library */
enum {MHEAP_NONE = (mheap_t)(-1), /**< Undefined heap. */
      MHEAP_MAX_USERS = 8, /**< maximum number of users of one heap */
};

/**
 * Create new heap for user.
 *
 * @param user          user address, which is used only as ID.
 *
 * @return heap ID or MHEAP_NONE on error.
 */
extern mheap_t mheap_create(void *user);

/**
 * Add new user to user-list of specified heap.
 * User-list of heap may contain many users (up to @c MHEAP_MAX_USERS),
 * and any user may be registered for many heaps.
 *
 * @param heap          heap ID.
 * @param user          user address, which is used only as ID.
 *
 * @return 0 on success, -1 on error.
 */
extern int mheap_add_user(mheap_t heap, void *user);

/**
 * Remove user from user-list of specified heap.
 * If heap have no more users after it, all allocated data in the heap
 * is freed.
 * If @p heap is @c MHEAP_NONE, then user is removed from all heaps,
 * where it is registered.
 *
 * @param heap          heap ID.
 * @param user          user address, which is used only as ID.
 */
extern void mheap_free_user(mheap_t heap, const void *user);

/**
 * Allocate memory block in the specified heap.
 *
 * @param heap          heap ID.
 * @param n             size of block.
 *
 * @return pointer to allocated memory block, which first @p n bytes
 *         are available for user, or NULL on error.
 */
extern void * mheap_alloc(mheap_t heap, size_t n);

#endif /* __TE_ACSE_MEM_H__ */
