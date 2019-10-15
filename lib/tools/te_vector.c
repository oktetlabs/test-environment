/** @file
 * @brief Dymanic vector
 *
 * Implementation of dymanic array
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "te_config.h"

#include "te_vector.h"
#include "te_string.h"

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
te_errno
te_vec_append_str_fmt(te_vec *vec, const char *fmt, ...)
{
    te_string opt = TE_STRING_INIT;
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = te_string_append_va(&opt, fmt, ap);
    va_end(ap);

    if (rc == 0)
        rc = TE_VEC_APPEND(vec, opt.ptr);

    if (rc != 0)
        te_string_free(&opt);

    return rc;
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

/* See the description in te_vector.h */
void
te_vec_deep_free(te_vec *args)
{
    void **arg;

    TE_VEC_FOREACH(args, arg)
    {
        free(*arg);
    }

    te_vec_free(args);
}
