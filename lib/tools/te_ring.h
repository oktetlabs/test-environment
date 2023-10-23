/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Ring buffers.
 *
 * Ring buffers are an efficient way to store
 * N last items from a continuous stream of data.
 *
 * @defgroup te_tools_te_ring Ring buffers.
 * @ingroup te_tools
 * @{
 */

#ifndef __TE_RING_H__
#define __TE_RING_H__

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"
#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ring buffer structure.
 *
 * All the fields may be inspected but must never
 * be directly modified by the user.
 */
typedef struct te_ring {
    /** Ring size. */
    size_t ring_size;
    /** Underlying data vector. */
    te_vec data;
    /** Sequential read pointer. */
    size_t rptr;
    /** Number of items in the ring. */
    size_t fill;
} te_ring;

/**
 * Initialize a ring to store objects of a given @p type_.
 *
 * @param type_       type of elements
 * @param destroy_    element destructor
 * @param ring_size_  initial ring size
 */
#define TE_RING_INIT(type_, destroy_, ring_size_) \
    {                                                       \
        .ring_size = (ring_size_),                          \
        .data = TE_VEC_INIT_DESTROY(type_, destroy_),       \
        .rptr = 0,                                          \
        .fill = 0,                                          \
    }

/**
 * Put the content of @p element into the @p ring.
 *
 * If the ring is already full, the oldest element is discarded.
 * If the ring has non-null destructor, it is called for the discarded
 * element.
 *
 * @param[in,out] ring     ring buffer
 * @param[in]     element  new data
 *
 * @return status code
 * @retval TE_ENOBUFS  The ring buffer has overflowed.
 *                     It is not an error, but the caller may
 *                     take some appropriate action if needed.
 */
extern te_errno te_ring_put(te_ring *ring, const void *element);

/**
 * Get the oldest element from the @p ring.
 *
 * The element is removed from the ring and
 * the read pointer is moved to the next element.
 *
 * The data are put into @p element if it is not @c NULL;
 * otherwise the data are discared (the element destructor is
 * called if it's non-null).
 *
 * If there is no data in @p ring, @p element is not changed.
 *
 * @param[in,out]  ring      ring buffer
 * @param[out]     element   destination buffer (may be @c NULL)
 *
 * @return status code
 * @retval TE_ENODATA    The ring is empty.
 */
extern te_errno te_ring_get(te_ring *ring, void *element);

/**
 * Get the oldest element from the ring @p keeping it.
 *
 * The pointer inside the ring itself is returned, the caller
 * must treat it as a pointer to local storage.
 *
 * The read pointer is not moved.
 *
 * @param ring   ring buffer
 *
 * @return the pointer to the data or @c NULL if the ring is empty
 */
extern const void *te_ring_peek(const te_ring *ring);

/**
 * Copy at most @p count oldest elements from @p ring to @p dest.
 *
 * The read pointer is not moved. The data are appended to @p dest.
 *
 * @param[in]  ring    ring buffer
 * @param[in]  count   maximum number of elements to copy
 * @param[out] dest    destination vector
 *
 * @return the number of elements actually copied (may be 0)
 *
 * @pre @p dest must have a null destructor.
 */
extern size_t te_ring_copy(const te_ring *ring, size_t count, te_vec *dest);

/**
 * Put @p count items from @p elements to @p ring.
 *
 * The ring is never overran, if there is not enough space
 * in @p ring, fewer elements are put.
 *
 * @param[in,out] ring      ring buffer
 * @param[in]     count     maximum number of elements to put
 * @param[in]     elements  new data
 *
 * @return the number of elements actually put (may be 0)
 */
extern size_t te_ring_put_many(te_ring *ring, size_t count,
                               const void *elements);

/**
 * Get at most @p count oldest elements from @p ring and put them to @p dest.
 *
 * The elements are removed from the ring and the read pointer is moved
 * as needed.
 *
 * The data are appended to @p dest,
 * if @p dest is not @c NULL, otherwise the data are discarded (the element
 * destructor is called if it's not null).
 *
 * @param[in,out] ring   ring buffer
 * @param[in]     count  maximum number of elements to put
 * @param[out]    dest   destination vector (may be @c NULL)
 *
 * @return the number of elements actually got.
 */
extern size_t te_ring_get_many(te_ring *ring, size_t count, te_vec *dest);

/**
 * Change the size of @p ring to @p new_ring_size.
 *
 * If @p new_ring_size is less than the current fill of the ring,
 * oldest redundant items are discarded. In all other cases all the data
 * are preserved, but the layout may change.
 *
 * @param[in,out] ring             ring buffer
 * @param[in]     new_ring_size    new size of the ring
 */
extern void te_ring_resize(te_ring *ring, size_t new_ring_size);

/**
 * Free any resources associated with @p ring.
 *
 * @param ring     ring buffer
 */
extern void te_ring_free(te_ring *ring);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RING_H__ */
/**@} <!-- END te_tools --> */
