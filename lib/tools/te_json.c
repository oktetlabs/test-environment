/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief JSON routines.
 *
 * Routines to serialize data as JSON.
 */

#include "te_config.h"

#include <assert.h>
#include <ctype.h>

#include "logger_api.h"

#include "te_defs.h"
#include "te_str.h"
#include "te_json.h"

static void
maybe_add_separator(te_json_ctx_t *ctx)
{
    te_json_level_t *level = &ctx->nesting[ctx->current_level];

    if (level->kind == TE_JSON_COMPOUND_OBJECT_VALUE)
    {
        level->kind = TE_JSON_COMPOUND_OBJECT;
    }
    else
    {
        if (level->n_items > 0)
            te_string_append(ctx->dest, ",");
        level->n_items++;
    }
}

void
te_json_add_simple(te_json_ctx_t *ctx, const char *fmt, ...)
{
    va_list args;

    maybe_add_separator(ctx);

    va_start(args, fmt);
    te_string_append_va(ctx->dest, fmt, args);
    va_end(args);
}

void
te_json_append_string_va(te_json_ctx_t *ctx, const char *fmt, va_list args)
{
    te_string inner = TE_STRING_INIT;
    const char *iter;

    assert(ctx->nesting[ctx->current_level].kind ==
                                      TE_JSON_COMPOUND_STRING);

    te_string_append_va(&inner, fmt, args);

    for (iter = inner.ptr; *iter != '\0'; iter++)
    {
        switch (*iter)
        {
            case '\\':
            case '\"':
            case '/':
                te_string_append(ctx->dest, "\\%c", *iter);
                break;
            case '\b':
                te_string_append(ctx->dest, "\\b");
                break;
            case '\f':
                te_string_append(ctx->dest, "\\f");
                break;
            case '\n':
                te_string_append(ctx->dest, "\\n");
                break;
            case '\r':
                te_string_append(ctx->dest, "\\r");
                break;
            case '\t':
                te_string_append(ctx->dest, "\\t");
                break;
            default:
                if (iscntrl(*iter))
                {
                    te_string_append(ctx->dest, "\\u%4.4" PRIx8,
                                     (uint8_t)*iter);
                }
                else
                {
                    te_string_append(ctx->dest, "%c", *iter);
                }
                break;
        }
    }
    te_string_free(&inner);
}

void
te_json_add_string(te_json_ctx_t *ctx, const char *fmt, ...)
{
    va_list args;

    te_json_start_string(ctx);

    va_start(args, fmt);
    te_json_append_string_va(ctx, fmt, args);
    va_end(args);

    te_json_end(ctx);
}

void
te_json_append_string(te_json_ctx_t *ctx, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    te_json_append_string_va(ctx, fmt, args);
    va_end(args);
}

void
te_json_append_raw(te_json_ctx_t *ctx, const char *value, size_t len)
{
    assert(ctx->nesting[ctx->current_level].kind ==
                                      TE_JSON_COMPOUND_RAW);

    if (len == 0)
        te_string_append(ctx->dest, "%s", value);
    else
        te_string_append(ctx->dest, "%.*s", (int)len, value);
}

static void
push_json_level(te_json_ctx_t *ctx, te_json_compound new_kind)
{
    assert(ctx->current_level + 1 < TE_ARRAY_LEN(ctx->nesting));
    ctx->current_level++;
    ctx->nesting[ctx->current_level].kind = new_kind;
    ctx->nesting[ctx->current_level].n_items = 0;
}

void
te_json_start_array(te_json_ctx_t *ctx)
{
    maybe_add_separator(ctx);
    push_json_level(ctx, TE_JSON_COMPOUND_ARRAY);
    te_string_append(ctx->dest, "[");
}

void
te_json_start_object(te_json_ctx_t *ctx)
{
    maybe_add_separator(ctx);
    push_json_level(ctx, TE_JSON_COMPOUND_OBJECT);
    te_string_append(ctx->dest, "{");
}

void
te_json_start_string(te_json_ctx_t *ctx)
{
    maybe_add_separator(ctx);
    push_json_level(ctx, TE_JSON_COMPOUND_STRING);
    te_string_append(ctx->dest, "\"");
}

void
te_json_start_raw(te_json_ctx_t *ctx)
{
    maybe_add_separator(ctx);
    push_json_level(ctx, TE_JSON_COMPOUND_RAW);
}

void
te_json_end(te_json_ctx_t *ctx)
{
    switch (ctx->nesting[ctx->current_level].kind)
    {
        case TE_JSON_COMPOUND_TOPLEVEL:
            /* do nothing */
            break;
        case TE_JSON_COMPOUND_ARRAY:
            te_string_append(ctx->dest, "]");
            break;
        case TE_JSON_COMPOUND_OBJECT:
            te_string_append(ctx->dest, "}");
            break;
        case TE_JSON_COMPOUND_OBJECT_VALUE:
            TE_FATAL_ERROR("Incomplete object value");
            break;
        case TE_JSON_COMPOUND_STRING:
            te_string_append(ctx->dest, "\"");
            break;
        case TE_JSON_COMPOUND_RAW:
            /* do nothing */
            break;
    }
    if (ctx->current_level != 0)
        ctx->current_level--;
}

void
te_json_add_key(te_json_ctx_t *ctx, const char *key)
{
    assert(ctx->nesting[ctx->current_level].kind == TE_JSON_COMPOUND_OBJECT);
    maybe_add_separator(ctx);

    te_string_append(ctx->dest, "\"%s\":", te_str_empty_if_null(key));
    ctx->nesting[ctx->current_level].kind = TE_JSON_COMPOUND_OBJECT_VALUE;
}

void
te_json_add_array_str(te_json_ctx_t *ctx, te_bool skip_null,
                      size_t n_strs, const char *strs[n_strs])
{
    size_t i;

    te_json_start_array(ctx);
    for (i = 0; i < n_strs; i++)
    {
        if (strs[i] != NULL)
            te_json_add_string(ctx, "%s", strs[i]);
        else if (!skip_null)
            te_json_add_null(ctx);
    }
    te_json_end(ctx);
}

void
te_json_add_kvpair(te_json_ctx_t *ctx, const te_kvpair_h *head)
{
    te_kvpair *p;

    assert(head != NULL);

    te_json_start_object(ctx);
    TAILQ_FOREACH(p, head, links)
    {
        te_json_add_key(ctx, p->key);
        te_json_add_string(ctx, "%s", p->value);
    }
    te_json_end(ctx);
}
