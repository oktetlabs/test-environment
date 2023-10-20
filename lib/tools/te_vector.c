/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2019-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Dymanic vector
 *
 * Implementation of dymanic array
 */

#define TE_LGR_USER     "TE vectors"

#include "te_config.h"
#include "te_defs.h"
#include "te_alloc.h"

#include "te_defs.h"
#include "te_vector.h"
#include "te_string.h"

#include "logger_api.h"

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
    assert(vec->element_size == other->element_size);
    assert(vec->destroy == NULL);
    assert(other->destroy == NULL);
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

    va_start(ap, fmt);
    te_string_append_va(&opt, fmt, ap);
    va_end(ap);

    te_vec_set_destroy_fn_safe(vec, te_vec_item_free_ptr);
    TE_VEC_APPEND(vec, opt.ptr);

    return 0;
}

static void
vec_destroy_elements(const te_vec *vec, size_t start, size_t count)
{
    size_t i;

    if (vec->destroy == NULL)
        return;

    for (i = start; i < start + count; i++)
    {
        const void *data = te_vec_get_immutable(vec, i);

        vec->destroy(data);
    }
}

/* See the description in te_vector.h */
void
te_vec_transfer(te_vec *vec, size_t index, void *dest)
{
    void *src = te_vec_get_mutable(vec, index);

    if (dest != NULL)
        memcpy(dest, src, vec->element_size);
    else
        vec_destroy_elements(vec, index, 1);
    memset(src, 0, vec->element_size);
}

/* See the description in te_vector.h */
size_t
te_vec_transfer_append(te_vec *vec, size_t start_index,
                       size_t count, te_vec *dest_vec)
{
    void *src;

    if (count == 0)
        return 0;

    src = te_vec_get_mutable(vec, start_index);
    te_alloc_adjust_extent(te_vec_size(vec), start_index, &count);

    if (dest_vec == NULL)
    {
        vec_destroy_elements(vec, start_index, count);
    }
    else
    {
        assert(dest_vec->element_size == vec->element_size);
        assert(dest_vec->destroy == NULL || dest_vec->destroy == vec->destroy);
        te_vec_append_array(dest_vec, src, count);
    }

    memset(src, 0, vec->element_size * count);

    return count;
}

/* See the description in te_vector.h */
void
te_vec_remove(te_vec *vec, size_t start_index, size_t count)
{
    assert(vec != NULL);

    vec_destroy_elements(vec, start_index, count);
    te_dbuf_cut(&vec->data, vec->element_size * start_index,
                vec->element_size * count);
}

/* See the description in te_vector.h */
void *
te_vec_replace(te_vec *vec, size_t index, const void *new_val)
{
    size_t size = te_vec_size(vec);
    void *element;
    assert(vec != NULL);

    if (index < size)
        vec_destroy_elements(vec, index, 1);
    else
    {
        te_dbuf_append(&vec->data, NULL,
                       (index - size + 1) * vec->element_size);
    }

    element = te_vec_get_mutable(vec, index);
    if (new_val != NULL)
        memcpy(element, new_val, vec->element_size);
    else
        memset(element, 0, vec->element_size);

    return element;
}

/* See the description in te_vector.h */
void
te_vec_reset(te_vec *vec)
{
    assert(vec != NULL);
    vec_destroy_elements(vec, 0, te_vec_size(vec));
    te_dbuf_reset(&vec->data);
}

/* See the description in te_vector.h */
void
te_vec_free(te_vec *vec)
{
    if (vec != NULL)
    {
        vec_destroy_elements(vec, 0, te_vec_size(vec));
        te_dbuf_free(&vec->data);
    }
}

/* See the description in te_vector.h */
void
te_vec_deep_free(te_vec *vec)
{
    if (vec->destroy == NULL)
    {
        void **item;

        TE_VEC_FOREACH(vec, item)
        {
            free(*item);
        }
    }
    te_vec_free(vec);
}

/* See the description in te_vector.h */
void
te_vec_item_free_ptr(const void *item)
{
    void *buf = *(void * const *)item;
    free(buf);
}

/* See the description in te_vector.h */
void
te_vec_set_destroy_fn_safe(te_vec *vec, te_vec_item_destroy_fn *destroy)
{
    if (vec->destroy == destroy || destroy == NULL)
        return;

    if (vec->destroy != NULL)
    {
        TE_FATAL_ERROR("Incompatible element destructor: %p != %p",
                           vec->destroy, destroy);
    }
    else
    {
        if (te_vec_size(vec) == 0)
            vec->destroy = destroy;
    }
}

/* See the description in te_vector.h */
te_errno
te_vec_append_strarray(te_vec *vec, const char **elements)
{
    size_t   i;

    assert(vec != NULL);
    assert(vec->element_size == sizeof(char *));
    te_vec_set_destroy_fn_safe(vec, te_vec_item_free_ptr);

    for (i = 0; elements[i] != NULL; i++)
    {
        char *tmp = TE_STRDUP(elements[i]);

        TE_VEC_APPEND(vec, tmp);
    }
    return 0;
}

te_errno
te_vec_split_string(const char *str, te_vec *strvec, char sep,
                    te_bool empty_is_none)
{
    const char *next = str;

    assert(strvec != NULL);
    assert(strvec->element_size == sizeof(char *));
    te_vec_set_destroy_fn_safe(strvec, te_vec_item_free_ptr);

    if (str == NULL || (*str == '\0' && empty_is_none))
        return 0;

    while (next != NULL)
    {
        char *dup;

        next = strchr(str, sep);
        if (next == NULL)
        {
            dup = TE_STRDUP(str);
        }
        else
        {
            dup = TE_ALLOC(next - str + 1);
            memcpy(dup, str, next - str);
            dup[next - str] = '\0';
            str = next + 1;
        }
        TE_VEC_APPEND(strvec, dup);
    }

    return 0;
}

void
te_vec_sort(te_vec *vec, int (*compar)(const void *, const void *))
{
    qsort(vec->data.ptr, te_vec_size(vec), vec->element_size, compar);
}

te_bool
te_vec_search(const te_vec *vec, const void *key,
              int (*compar)(const void *, const void *),
              unsigned int *minpos, unsigned int *maxpos)
{
    const void *found = bsearch(key, vec->data.ptr, te_vec_size(vec),
                                vec->element_size, compar);

    if (found == NULL)
        return FALSE;

    if (minpos != NULL)
    {
        const uint8_t *iter = found;

        while (iter != vec->data.ptr)
        {
            if (compar(key, iter - vec->element_size) != 0)
                break;
            iter -= vec->element_size;
        }
        *minpos = te_vec_get_index(vec, iter);
    }

    if (maxpos != NULL)
    {
        const uint8_t *iter = found;

        while (iter != vec->data.ptr + vec->data.len - vec->element_size)
        {
            if (compar(key, iter + vec->element_size) != 0)
                break;
            iter += vec->element_size;
        }
        *maxpos = te_vec_get_index(vec, iter);
    }
    return TRUE;
}
