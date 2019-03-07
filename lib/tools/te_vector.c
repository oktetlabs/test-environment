/** @file
 * @brief Dymanic vector
 *
 * Implementation of dymanic array
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "te_vector.h"

/* See the description in te_vector.h */
te_errno
te_vec_append(te_vec *vec, const void *element)
{
    return te_vec_append_array(vec, element, 1);
}

/* See the description in te_vector.h */
te_errno
te_vec_append_vec(te_vec *vec, const te_vec *other)
{
    assert(vec != NULL);
    assert(other != NULL);
    assert(vec->element_size != other->element_size);
    return te_vec_append_array(vec, other->data.ptr, te_vec_size(other));
}

/* See the description in te_vector.h */
te_errno
te_vec_append_array(te_vec *vec, const void *elements, size_t count)
{
    assert(vec != NULL);
    return te_dbuf_append(&vec->data, elements, vec->element_size * count);
}

/* See the description in te_vector.h */
void
te_vec_reset(te_vec *vec)
{
    assert(vec != NULL);
    te_dbuf_reset(&vec->data);
}

/* See the description in te_vector.h */
void
te_vec_free(te_vec *vec)
{
    if (vec != NULL)
        te_dbuf_free(&vec->data);
}
