/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Compound strings.
 *
 * Routines to work with compound strings.
 *
 */

#define TE_LGR_USER "Compound strings"
#include "te_config.h"

#include <ctype.h>
#include "logger_api.h"
#include "te_str.h"
#include "te_alloc.h"
#include "te_compound.h"

te_compound_kind
te_compound_classify(const te_string *comp)
{
    if (comp == NULL || te_str_is_null_or_empty(comp->ptr))
        return TE_COMPOUND_NULL;

    if (strchr(comp->ptr, TE_COMPOUND_ITEM_SEP) == NULL)
        return TE_COMPOUND_PLAIN;

    if (strchr(comp->ptr, TE_COMPOUND_KEY_SEP) == NULL)
        return TE_COMPOUND_ARRAY;

    return TE_COMPOUND_OBJECT;
}

static bool
parse_next_item(te_substring_t *iter,
                te_substring_t *field, te_substring_t *value)
{
    char sep;
    te_substring_t next = *iter;

    if (te_substring_past_end(iter))
        return false;

    sep = te_substring_span(&next,
                            (const char[]){TE_COMPOUND_ITEM_SEP,
                                           TE_COMPOUND_KEY_SEP, '\0'},
                            true);
    switch (sep)
    {
        case '\0':
        case TE_COMPOUND_ITEM_SEP:
            te_substring_invalidate(field);
            break;
        case TE_COMPOUND_KEY_SEP:
            if (field)
                *field = next;
            te_substring_advance(&next);
            te_substring_skip(&next, TE_COMPOUND_KEY_SEP, 1);
            te_substring_span(&next,
                              (const char[]){TE_COMPOUND_ITEM_SEP, '\0'},
                              true);
            break;
        default:
            assert(0);
    }

    if (value)
        *value = next;
    te_substring_advance(&next);
    te_substring_skip(&next, TE_COMPOUND_ITEM_SEP, 1);
    te_substring_limit(iter, &next);
    return true;
}

bool
te_compound_validate(const te_string *comp)
{
    te_substring_t iter = TE_SUBSTRING_INIT(comp);
    te_substring_t field = TE_SUBSTRING_INIT(comp);
    te_substring_t value = TE_SUBSTRING_INIT(comp);
    te_substring_t prev_field = TE_SUBSTRING_INIT(comp);
    size_t count = 0;

    te_substring_invalidate(&prev_field);

    while (parse_next_item(&iter, &field, &value))
    {
        size_t i;

        if (te_substring_compare(&prev_field, &field) > 0)
        {
            ERROR("Invalid ordering of field keys");
            return false;
        }
        prev_field = field;
        for (i = value.start; i < value.start + value.len; i++)
        {
            if (value.base->ptr[i] == TE_COMPOUND_KEY_SEP)
            {
                ERROR("The value contains a key separator character");
                return false;
            }
        }
        te_substring_advance(&iter);
        count++;
    }

    if (count > 1 && comp->ptr[comp->len - 1] != TE_COMPOUND_ITEM_SEP)
    {
        ERROR("A multi-value compound is not properly terminated");
        return false;
    }

    return true;
}

bool
te_compound_validate_str(const char *comp)
{
    te_string tmp = TE_STRING_INIT_RO_PTR(comp);
    bool result = te_compound_validate(&tmp);

    te_string_free(&tmp);
    return result;
}

bool
te_compound_extract(te_string *dst, const te_string *comp,
                    const char *key, size_t idx)
{
    te_substring_t iter = TE_SUBSTRING_INIT(comp);
    te_substring_t field = TE_SUBSTRING_INIT(comp);
    te_substring_t value = TE_SUBSTRING_INIT(comp);

    while (parse_next_item(&iter, &field, &value))
    {
        int cmp = te_substring_compare_str(&field, key);

        if (cmp > 0)
            break;
        if (cmp == 0)
        {
            if (idx == 0)
            {
                te_substring_extract(dst, &value);
                return true;
            }
            idx--;
        }
        te_substring_advance(&iter);
    }

    return false;
}

size_t
te_compound_count(const te_string *comp, const char *key)
{
    te_substring_t iter = TE_SUBSTRING_INIT(comp);
    te_substring_t field = TE_SUBSTRING_INIT(comp);
    size_t count = 0;

    while (parse_next_item(&iter, &field, NULL))
    {
        int cmp = te_substring_compare_str(&field, key);

        if (cmp > 0)
            break;
        if (cmp == 0)
            count++;
        te_substring_advance(&iter);
    }

    return count;
}

static void
cover_equal_fields(te_substring_t *start, const te_substring_t *field)
{
    te_substring_t iter = *start;
    te_substring_t span_field = *field;

    te_substring_advance(&iter);
    while (parse_next_item(&iter, &span_field, NULL))
    {
        if (te_substring_compare(&span_field, field) != 0)
            break;
        te_substring_advance(&iter);
    }
    te_substring_limit(start, &iter);
}

void
te_compound_set_va(te_string *comp, const char *key,
                   te_compound_mod_op mod_op,
                   const char *val_fmt, va_list args)
{
    te_substring_t iter = TE_SUBSTRING_INIT(comp);
    te_substring_t field = TE_SUBSTRING_INIT(comp);

    /* Deletion is always replacement */
    if (val_fmt == NULL)
        mod_op = TE_COMPOUND_MOD_OP_REPLACE;

    while (parse_next_item(&iter, &field, NULL))
    {
        int cmp = te_substring_compare_str(&field, key);

        if (cmp >= 0)
        {
            /*
             * If there is no such key, it is inserted _before_
             * the nearest greater key.
             */
            if (cmp > 0)
                mod_op = TE_COMPOUND_MOD_OP_PREPEND;
            break;
        }
        te_substring_advance(&iter);
    }

    switch (mod_op)
    {
        case TE_COMPOUND_MOD_OP_PREPEND:
            iter.len = 0;
            break;
        case TE_COMPOUND_MOD_OP_REPLACE:
            cover_equal_fields(&iter, &field);
            te_substring_modify(&iter, TE_SUBSTRING_MOD_OP_REPLACE,
                                NULL);
            break;
        case TE_COMPOUND_MOD_OP_APPEND:
            cover_equal_fields(&iter, &field);
            te_substring_advance(&iter);
            break;
    }
    if (val_fmt != NULL)
    {
        te_substring_insert_sep(&iter, TE_COMPOUND_ITEM_SEP, false);
        if (key != NULL)
        {
            te_substring_modify(&iter, TE_SUBSTRING_MOD_OP_APPEND,
                                "%s%c", key, TE_COMPOUND_KEY_SEP);
        }
        te_substring_modify_va(&iter, TE_SUBSTRING_MOD_OP_APPEND,
                               val_fmt, args);
        te_substring_modify(&iter, TE_SUBSTRING_MOD_OP_APPEND,
                            "%c", TE_COMPOUND_ITEM_SEP);
    }
}

void
te_compound_set(te_string *comp, const char *key,
                te_compound_mod_op mod_op, const char *val_fmt, ...)
{
    va_list args;

    va_start(args, val_fmt);
    te_compound_set_va(comp, key, mod_op, val_fmt, args);
    va_end(args);
}

void
te_compound_merge(te_string *dst, const te_string *src,
                  te_compound_mod_op mod_op)
{
    te_substring_t left = TE_SUBSTRING_INIT(dst);
    te_substring_t right = TE_SUBSTRING_INIT(src);
    te_substring_t left_field = TE_SUBSTRING_INIT(dst);
    te_substring_t right_field = TE_SUBSTRING_INIT(src);
    bool advance_left = true;
    bool advance_right = true;
    static const te_substring_mod_op compound2substring_op[] = {
        [TE_COMPOUND_MOD_OP_APPEND] = TE_SUBSTRING_MOD_OP_APPEND,
        [TE_COMPOUND_MOD_OP_PREPEND] = TE_SUBSTRING_MOD_OP_PREPEND,
        [TE_COMPOUND_MOD_OP_REPLACE] = TE_SUBSTRING_MOD_OP_REPLACE,
    };

    if (dst == NULL || src == NULL)
        return;

    for (;;)
    {
        int cmp;

        if (advance_left)
            te_substring_advance(&left);
        if (advance_right)
            te_substring_advance(&right);

        if (advance_left)
        {
            if (!parse_next_item(&left, &left_field, NULL))
                break;
        }

        if (advance_right)
        {
            if (!parse_next_item(&right, &right_field, NULL))
                break;
        }

        cmp = te_substring_compare(&left_field, &right_field);
        if (cmp < 0)
        {
            advance_left = true;
            advance_right = false;
        }
        else if (cmp > 0)
        {
            advance_left = false;
            advance_right = true;
            te_substring_copy(&left, &right, TE_SUBSTRING_MOD_OP_PREPEND);
            left.start += right.len;
            left.len -= right.len;
            left_field.start += right.len;
        }
        else
        {
            cover_equal_fields(&left, &left_field);
            cover_equal_fields(&right, &right_field);

            te_substring_copy(&left, &right, compound2substring_op[mod_op]);

            advance_left = true;
            advance_right = true;
        }
    }

    te_substring_till_end(&right);
    te_substring_copy(&left, &right, TE_SUBSTRING_MOD_OP_APPEND);
}

te_errno
te_compound_iterate(const te_string *src, te_compound_iter_fn *callback,
                    void *user)
{
    te_substring_t iter = TE_SUBSTRING_INIT(src);
    te_substring_t prev_field = TE_SUBSTRING_INIT(src);
    te_substring_t field = TE_SUBSTRING_INIT(src);
    te_substring_t value = TE_SUBSTRING_INIT(src);
    size_t index = 0;
    te_errno rc = 0;

    if (te_substring_past_end(&iter))
        return TE_ENODATA;
    while (parse_next_item(&iter, &field, &value))
    {
        char value_buf[value.len + 1];

        te_substring_extract_buf(value_buf, &value);
        if (te_substring_compare(&prev_field, &field) != 0)
        {
            prev_field = field;
            index = 0;
        }
        te_substring_advance(&iter);
        if (te_substring_is_valid(&field))
        {
            char field_buf[field.len + 1];
            te_substring_extract_buf(field_buf, &field);

            rc = callback(field_buf, index, value_buf,
                          !te_substring_past_end(&iter), user);
        }
        else
        {
            rc = callback(NULL, index, value_buf,
                          !te_substring_past_end(&iter), user);
        }
        if (rc != 0)
            break;

        index++;
    }

    return rc == TE_EOK ? 0 : rc;
}

te_errno
te_compound_iterate_str(const char *src, te_compound_iter_fn *callback,
                        void *user)
{
    te_string tmp = TE_STRING_INIT_RO_PTR(src);
    te_errno rc = te_compound_iterate(&tmp, callback, user);

    te_string_free(&tmp);
    return rc;
}

void
te_vec2compound(te_string *dst, const te_vec *vec)
{
    const char * const *iter;

    if (dst == NULL || vec == NULL)
        return;

    assert(vec->element_size == sizeof(char *));
    TE_VEC_FOREACH(vec, iter)
        te_compound_set(dst, NULL, TE_COMPOUND_MOD_OP_APPEND, "%s", *iter);
}

static te_errno
copy_kv_to_compound(const char *key, const char *value, void *user)
{
    te_string *dst = user;

    te_compound_set(dst, key, TE_COMPOUND_MOD_OP_APPEND, "%s", value);
    return 0;
}

void
te_kvpair2compound(te_string *dst, const te_kvpair_h *kv)
{
    te_kvpairs_foreach(kv, copy_kv_to_compound, NULL, dst);
}

static te_errno
copy_values_vec(char *key, size_t idx, char *value, bool has_more, void *user)
{
    te_vec *dst = user;

    TE_VEC_APPEND_RVALUE(dst, char *, TE_STRDUP(value));

    UNUSED(key);
    UNUSED(idx);
    UNUSED(has_more);

    return 0;
}

void
te_compound2vec(te_vec *dst, const te_string *compound)
{
    if (dst == NULL || compound == NULL)
        return;
    assert(dst->element_size == sizeof(char *));
    te_compound_iterate(compound, copy_values_vec, dst);
}

static te_errno
copy_values_kv(char *key, size_t idx, char *value, bool has_more, void *user)
{
    te_kvpair_h *dst = user;

    if (key != NULL)
        te_kvpair_push(dst, key, "%s", value);
    else
    {
        char idx_buf[16];
        TE_SPRINTF(idx_buf, "%zu", idx);

        te_kvpair_push(dst, idx_buf, "%s", value);
    }

    return 0;
}

void
te_compound2kvpair(te_kvpair_h *dst, const te_string *compound)
{
    if (dst == NULL || compound == NULL)
        return;
    te_compound_iterate(compound, copy_values_kv, dst);
}

static te_errno
compound_array_to_json(char *key, size_t idx, char *value, bool has_more,
                       void *user)
{
    te_json_ctx_t *ctx = user;

    assert(key == NULL);
    te_json_add_string(ctx, "%s", value);

    UNUSED(idx);
    UNUSED(has_more);
    return 0;
}

static te_errno
compound_object_to_json(char *key, size_t idx, char *value, bool has_more,
                        void *user)
{
    te_json_ctx_t *ctx = user;

    if (key != NULL && idx == 0)
        te_json_add_key_str(ctx, key, value);
    else
    {
        te_string key_buf = TE_STRING_INIT;

        te_string_append(&key_buf, "%s%zu", te_str_empty_if_null(key), idx);
        te_json_add_key_str(ctx, key_buf.ptr, value);
        te_string_free(&key_buf);
    }

    return 0;
}

void
te_json_add_compound(te_json_ctx_t *ctx, const te_string *compound)
{
    switch (te_compound_classify(compound))
    {
        case TE_COMPOUND_NULL:
            te_json_add_null(ctx);
            break;
        case TE_COMPOUND_PLAIN:
            te_json_add_string(ctx, "%s", compound->ptr);
            break;
        case TE_COMPOUND_ARRAY:
            te_json_start_array(ctx);
            te_compound_iterate(compound, compound_array_to_json, ctx);
            te_json_end(ctx);
            break;
        case TE_COMPOUND_OBJECT:
            te_json_start_object(ctx);
            te_compound_iterate(compound, compound_object_to_json, ctx);
            te_json_end(ctx);
            break;
    }
}

typedef struct compound_update_data {
    const char *inner_key;
    te_compound_mod_op mod_op;
    const char *val_fmt;
    va_list args;
} compound_update_data;

static char *
compound_update(const te_kvpair_h *dst, const char *outer_key,
                const char *old_value, void *user)
{
    compound_update_data *data = user;
    te_string compound = TE_STRING_INIT;

    if (data->inner_key == NULL && old_value == NULL)
    {
        te_string_append_va(&compound, data->val_fmt, data->args);
        return compound.ptr;
    }

    if (old_value != NULL)
        te_string_append(&compound, "%s", old_value);

    te_compound_set_va(&compound, data->inner_key, data->mod_op,
                       data->val_fmt, data->args);

    if (compound.len == 0)
    {
        te_string_free(&compound);
        return NULL;
    }

    UNUSED(dst);
    UNUSED(outer_key);
    return compound.ptr;
}

void
te_kvpair_set_compound_va(te_kvpair_h *dst, const char *outer_key,
                          const char *inner_key, te_compound_mod_op mod_op,
                          const char *val_fmt, va_list args)
{
    compound_update_data data = {
        .inner_key = inner_key,
        .mod_op = mod_op,
        .val_fmt = val_fmt,
    };

    va_copy(data.args, args);
    te_kvpair_update(dst, outer_key, compound_update, &data);
    va_end(data.args);
}

void
te_kvpair_set_compound(te_kvpair_h *dst, const char *outer_key,
                       const char *inner_key,
                       te_compound_mod_op mod_op,
                       const char *val_fmt, ...)
{
    va_list args;

    va_start(args, val_fmt);
    te_kvpair_set_compound_va(dst, outer_key, inner_key, mod_op, val_fmt, args);
    va_end(args);
}

void
te_compound_build_name(te_string *dst, const char *stem,
                       const char *key, size_t idx)
{
    te_string_append(dst, "%s%s%s", stem, key == NULL ? "" : "_",
                     te_str_empty_if_null(key));
    if (idx > 0)
    {
        te_string_append(dst, "%s%zu",
                         dst->len > 0 &&
                         (isdigit(dst->ptr[dst->len - 1]) ||
                          dst->ptr[dst->len - 1] == '_') ? "_" : "",
                         idx);
    }
}

static te_errno
compound_dereference_int(const te_string *compound,
                         te_substring_t *need_field, size_t idx,
                         te_compound_iter_fn *callback,
                         void *user)
{
    te_substring_t iter = TE_SUBSTRING_INIT(compound);
    te_substring_t field = TE_SUBSTRING_INIT(compound);
    te_substring_t value = TE_SUBSTRING_INIT(compound);
    size_t i = 0;

    while (parse_next_item(&iter, &field, &value))
    {
        int cmp = te_substring_compare(&field, need_field);

        if (cmp > 0)
            break;
        if (cmp == 0)
        {
            if (i == idx)
            {
                char value_buf[value.len + 1];
                te_substring_extract_buf(value_buf, &value);

                if (!te_substring_is_valid(&field))
                    return callback(NULL, i, value_buf,
                                    !te_substring_past_end(&iter), user);
                else
                {
                    char field_buf[field.len + 1];

                    te_substring_extract_buf(field_buf, &field);
                    return callback(field_buf, i, value_buf,
                                    !te_substring_past_end(&iter), user);
                }
            }
            i++;
        }
        te_substring_advance(&iter);
    }

    return TE_ENODATA;
}

te_errno
te_compound_dereference(const te_string *compound,
                        const char *stem, const char *key,
                        te_compound_iter_fn *callback,
                        void *user)
{
    te_string key_str = TE_STRING_INIT_RO_PTR(key);
    te_substring_t key_sub = TE_SUBSTRING_INIT(&key_str);
    uintmax_t idx;
    bool has_idx;
    te_errno rc = 0;

    key_sub.len = key_str.len;
    if (!te_substring_strip_prefix(&key_sub, stem))
        return TE_ENODATA;

    has_idx = te_substring_strip_uint_suffix(&key_sub, &idx);
    if (has_idx)
        te_substring_strip_suffix(&key_sub, "_");

    if (key_sub.len == 0)
        rc = compound_dereference_int(compound, NULL, idx, callback, user);
    else
    {
        if (!te_substring_strip_prefix(&key_sub, "_"))
            return TE_ENODATA;

        rc = compound_dereference_int(compound, &key_sub, idx,
                                      callback, user);
        if (rc == TE_ENODATA && has_idx)
        {
            te_substring_till_end(&key_sub);
            rc = compound_dereference_int(compound, &key_sub, 0,
                                          callback, user);
        }
    }

    return rc == TE_EOK ? 0 : rc;
}

te_errno
te_compound_dereference_str(const char *src,
                            const char *stem, const char *key,
                            te_compound_iter_fn *callback,
                            void *user)
{
    te_string tmp = TE_STRING_INIT_RO_PTR(src);
    te_errno rc = te_compound_dereference(&tmp, stem, key, callback, user);

    te_string_free(&tmp);
    return rc;
}
