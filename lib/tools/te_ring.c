/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Ring buffers.
 *
 * Implementation of ring buffers.
 */

#define TE_LGR_USER     "TE ring buffers"

#include "te_config.h"

#include <assert.h>

#include "te_defs.h"
#include "te_errno.h"

#include "te_ring.h"

#include "logger_api.h"

te_errno
te_ring_put(te_ring *ring, const void *element)
{
    size_t wptr = (ring->rptr + ring->fill) % ring->ring_size;

    assert(element != NULL);

    te_vec_replace(&ring->data, wptr, element);
    if (ring->fill == ring->ring_size)
    {
        ring->rptr++;
        return TE_ENOBUFS;
    }
    else
    {
        ring->fill++;
        return 0;
    }
}

te_errno
te_ring_get(te_ring *ring, void *element)
{
    if (ring->fill == 0)
        return TE_ENODATA;

    te_vec_transfer(&ring->data, ring->rptr % ring->ring_size, element);
    ring->rptr++;
    ring->fill--;

    return 0;
}

const void *
te_ring_peek(const te_ring *ring)
{
    if (ring->fill == 0)
        return NULL;

    return te_vec_get_immutable(&ring->data, ring->rptr % ring->ring_size);
}

size_t
te_ring_copy(const te_ring *ring, size_t count, te_vec *dest)
{
    size_t index = ring->rptr % ring->ring_size;

    if (count > ring->fill)
        count = ring->fill;

    if (count == 0)
        return 0;

    assert(dest->destroy == NULL);
    assert(dest->element_size == ring->data.element_size);

    if (index + count <= ring->ring_size)
    {
        te_vec_append_array(dest, te_vec_get_immutable(&ring->data, index),
                            count);
    }
    else
    {
        size_t tail_count = ring->ring_size - index;

        te_vec_append_array(dest, te_vec_get_immutable(&ring->data, index),
                            tail_count);
        te_vec_append_array(dest, te_vec_get_immutable(&ring->data, 0),
                            count - tail_count);
    }

    return count;
}

size_t
te_ring_put_many(te_ring *ring, size_t count, const void *elements)
{
    size_t n = 0;
    const uint8_t *iter = elements;

    assert(count == 0 || elements != NULL);

    while (ring->fill < ring->ring_size && n < count)
    {
        te_ring_put(ring, iter);
        iter += ring->data.element_size;
        n++;
    }

    return n;
}

static void
ring_linearize(te_ring *ring, size_t count, te_vec *dest)
{
    size_t index = ring->rptr % ring->ring_size;

    if (index + count <= ring->ring_size)
        te_vec_transfer_append(&ring->data, index, count, dest);
    else
    {
        size_t tail_count = ring->ring_size - index;

        te_vec_transfer_append(&ring->data, index, tail_count, dest);
        te_vec_transfer_append(&ring->data, 0, count - tail_count, dest);
    }
}

size_t
te_ring_get_many(te_ring *ring, size_t count, te_vec *dest)
{
    if (count > ring->fill)
        count = ring->fill;

    if (count == 0)
        return 0;

    ring_linearize(ring, count, dest);

    ring->rptr += count;
    ring->fill -= count;

    return count;
}

void
te_ring_resize(te_ring *ring, size_t new_ring_size)
{
    size_t old_index;
    size_t new_index;

    if (new_ring_size < ring->fill)
        te_ring_get_many(ring, ring->fill - new_ring_size, NULL);

    old_index = ring->rptr % ring->ring_size;
    new_index = ring->rptr % new_ring_size;

    if (old_index == new_index &&
        old_index + ring->fill <= ring->ring_size &&
        new_index + ring->fill <= new_ring_size)
    {
        ring->ring_size = new_ring_size;
    }
    else
    {
        te_vec tmp = TE_VEC_INIT_LIKE(&ring->data);

        tmp.destroy = NULL;
        ring_linearize(ring, ring->fill, &tmp);

        ring->fill = 0;
        ring->ring_size = new_ring_size;
        te_ring_put_many(ring, te_vec_size(&tmp), tmp.data.ptr);

        te_vec_free(&tmp);
    }

}

void
te_ring_free(te_ring *ring)
{
    te_vec_free(&ring->data);
}
