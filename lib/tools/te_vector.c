/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Dymanic vector
 *
 * Implementation of dymanic array
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include "te_defs.h"
#include "te_alloc.h"

#include "te_defs.h"
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
    assert(vec->element_size == other->element_size);
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

    TE_VEC_APPEND(vec, opt.ptr);

    return 0;
}

/* See the description in te_vector.h */
void
te_vec_remove(te_vec *vec, size_t start_index, size_t count)
{
    assert(vec != NULL);

    te_dbuf_cut(&vec->data, vec->element_size * start_index,
                vec->element_size * count);
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

/* See the description in te_vector.h */
te_errno
te_vec_append_strarray(te_vec *vec, const char **elements)
{
    size_t   i;

    assert(vec != NULL);
    assert(vec->element_size == sizeof(char *));
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
