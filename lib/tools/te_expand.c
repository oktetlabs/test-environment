/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Parameters expansion API
 *
 * API that allows to expand parameters in a string
 *
 * Copyright (C) 2018-2023 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TE variable expansion"
#include "te_defs.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "logger_api.h"
#include "te_str.h"
#include "te_expand.h"
#include "te_json.h"

/**
 * @name Variable expansion metacharacters.
 *
 * The abstract syntax of variable expansions is:
 *
 * @code
 * VARREF OPENING varname (FILTER filter-name)*
 *        [MODIFIER_INTRO (DEFAULT_VALUE | ALTERNATE_VALUE) string]
 *        CLOSING
 * @endcode
 *
 * which is represended on surface as:
 * - `${varname}`
 * - `${varname|filter-name}`
 * - `${varname:-default value}`
 * - `${varname:+alternate value}`
 * and so on.
 *
 * te_string_expand_kvpairs() in addition support special syntax
 * of @c varname for list operations:
 *
 * @code
 * COUNT_INTRO basevarname
 * basevarname SUBSCRIPT_START string SUBSCRIPT_END
 * varname LOOPINTRO string
 * @endcode
 *
 * which is represented on surface as:
 * - `${#basevarname}`
 * - `${basevarname[1]}`
 * - `${varname*${othervar[${}}]}`
 * and so on.
 *
 * See the description of te_string_expand_parameters() and
 * te_string_expand_kvpairs() for more details.
 *
 * @{
 */
/** Start of a variable reference. */
#define VARREF "$"
/** Opening brace after VARREF. */
#define OPENING "{"
/** Closing brace. */
#define CLOSING "}"
/** Start of a reference modifier. */
#define MODIFIER_INTRO ":"
/** Default-value modifier. */
#define DEFAULT_VALUE "-"
/** Alternate-value modifier. */
#define ALTERNATE_VALUE "+"
/** Filter separator. */
#define FILTER "|"
/** First character of a counting reference. */
#define COUNT_INTRO "#"
/** Start of a list subscript. */
#define SUBSCRIPT_START "["
/** End of a list subscript. */
#define SUBSCRIPT_END "]"
/** Start of a looping construct. */
#define LOOP_INTRO "*"
/** @} */

static const char *
get_positional_arg(const char *param_name, const char * const *posargs)
{
    if (posargs == NULL || !isdigit(param_name[0]) || param_name[1] != '\0')
        return NULL;

    return posargs[param_name[0] - '0'];
}

/**
 * Expand an environment variable or positional argument into @p dest.
 *
 * @param param_name    Name to expand.
 * @param ctx           Array of positional arguments.
 * @param dest          Destination string.
 *
 * @return @c TRUE if the expansion has happened.
 */
static te_bool
expand_env_value(const char *param_name, const void *ctx, te_string *dest)
{
    const char * const *posargs = ctx;
    const char *value = get_positional_arg(param_name, posargs);

    if (value == NULL)
        value = getenv(param_name);

    te_string_append(dest, "%s", te_str_empty_if_null(value));

    return value != NULL;
}

/**
 * A context for kvpair expansion.
 */
typedef struct kvpairs_expand_ctx {
    /** Array of positional arguments */
    const char **posargs;
    /** Head of key value pairs */
    const te_kvpair_h *kvpairs;
    /** Innermost loop index */
    unsigned int loop_index;
} kvpairs_expand_ctx;

static te_bool expand_kvpairs_value(const char *param_name, const void *ctx,
                                    te_string *dest);

static te_bool
expand_kvpairs_index(const te_kvpair_h *kvpairs, const char *name,
                     long int ival, te_string *dest)
{
    unsigned int idx;
    const char *value;

    if (ival >= 0)
    {
        if (ival > UINT_MAX)
        {
            ERROR("The index %ld is too large", ival);
            return FALSE;
        }
        idx = (unsigned int)ival;
    }
    else
    {
        idx = te_kvpairs_count(kvpairs, name);
        if (-ival > idx)
        {
            ERROR("The index %ls is too small", ival);
            return FALSE;
        }
        idx += (int)ival;
    }

    value = te_kvpairs_get_nth(kvpairs, name, idx);

    te_string_append(dest, "%s", te_str_empty_if_null(value));
    return value != NULL;
}

static te_bool
expand_kvpairs_sep(const te_kvpair_h *kvpairs, const char *name,
                   const char *sep, te_string *dest)
{
    te_vec all_values = TE_VEC_INIT(const char *);
    te_bool result;

    result = (te_kvpairs_get_all(kvpairs, name, &all_values) == 0);

    te_string_join_vec(dest, &all_values, sep);
    te_vec_free(&all_values);

    return result;
}

static te_bool
expand_kvpairs_subscript(const char *param_name, const char *sub_start,
                         const kvpairs_expand_ctx *ctx, te_string *dest)
{
    const char *end;
    char base_param_name[sub_start - param_name];

    memcpy(base_param_name, param_name, sizeof(base_param_name) - 1);
    base_param_name[sizeof(base_param_name) - 1] = '\0';

    te_strpbrk_balanced(sub_start, *OPENING, *CLOSING, '\0',
                        SUBSCRIPT_END, &end);
    if (end == NULL || end[1] != '\0')
    {
        ERROR("Invalid list subscript: %s", sub_start);
        return FALSE;
    }
    else
    {
        char sub_expr[end - sub_start + 1];
        te_string index = TE_STRING_INIT;
        long int ival;
        te_bool result;

        memcpy(sub_expr, sub_start, sizeof(sub_expr) - 1);
        sub_expr[sizeof(sub_expr) - 1] = '\0';

        if (te_string_expand_parameters(sub_expr, expand_kvpairs_value,
                                        ctx, &index) != 0)
        {
            te_string_free(&index);
            return FALSE;
        }

        if (te_strtol_silent(index.ptr, 0, &ival) == 0)
        {
            result = expand_kvpairs_index(ctx->kvpairs, base_param_name,
                                          ival, dest);
        }
        else
        {
            result = expand_kvpairs_sep(ctx->kvpairs, base_param_name,
                                        index.ptr, dest);
        }
        te_string_free(&index);
        return result;
    }
}

static te_bool
expand_kvpairs_loop(const char *param_name, const char *loop_start,
                         const kvpairs_expand_ctx *ctx, te_string *dest)
{
    kvpairs_expand_ctx inner_ctx = *ctx;
    char base_param_name[loop_start - param_name];
    unsigned int count;

    memcpy(base_param_name, param_name, sizeof(base_param_name) - 1);
    base_param_name[sizeof(base_param_name) - 1] = '\0';

    count = te_kvpairs_count(ctx->kvpairs, base_param_name);
    if (count == 0)
        return FALSE;

    for (inner_ctx.loop_index = 0;
         inner_ctx.loop_index < count;
         inner_ctx.loop_index++)
    {
        te_string_expand_parameters(loop_start, expand_kvpairs_value,
                                    &inner_ctx, dest);
    }

    return TRUE;
}

/**
 * Expand a value of a key or a positional argument into @p dest.
 *
 * @param param_name    Name to expand.
 * @param ctx           Context, see kvpairs_expand_ctx.
 * @param dest          Destination string
 *
 * @return @c TRUE if the expansion has happended.
 */
static te_bool
expand_kvpairs_value(const char *param_name, const void *ctx, te_string *dest)
{
    const kvpairs_expand_ctx *kvpairs_ctx = ctx;
    const char *value;

    if (*param_name == '\0')
    {
        te_string_append(dest, "%u", kvpairs_ctx->loop_index);
        return TRUE;
    }

    value = get_positional_arg(param_name, kvpairs_ctx->posargs);

    if (value == NULL)
    {
        if (*param_name == '\0')
        {
            te_string_append(dest, "%u", kvpairs_ctx->loop_index);
            return TRUE;
        }
        else if (*param_name == '#')
        {
            te_string_append(dest, "%u",
                             te_kvpairs_count(kvpairs_ctx->kvpairs,
                                              param_name + 1));
            return TRUE;
        }
        else
        {
            const char *sep = NULL;

            te_strpbrk_balanced(param_name, *OPENING, *CLOSING, '\0',
                                LOOP_INTRO SUBSCRIPT_START, &sep);
            if (sep == NULL)
                value = te_kvpairs_get(kvpairs_ctx->kvpairs, param_name);
            else if (*sep == *SUBSCRIPT_START)
            {
                return expand_kvpairs_subscript(param_name, sep + 1,
                                                kvpairs_ctx, dest);
            }
            else if (*sep == *LOOP_INTRO)
            {
                return expand_kvpairs_loop(param_name, sep + 1,
                                           kvpairs_ctx, dest);
            }
        }
    }

    te_string_append(dest, "%s", te_str_empty_if_null(value));

    return value != NULL;
}

typedef te_errno (*expand_filter)(const te_string *src, te_string *dest);

static te_errno
base64_filter(const te_string *src, te_string *dest)
{
    te_string_encode_base64(dest, src->len, (const uint8_t *)src->ptr,
                            FALSE);
    return 0;
}

static te_errno
base64uri_filter(const te_string *src, te_string *dest)
{
    te_string_encode_base64(dest, src->len, (const uint8_t *)src->ptr,
                            TRUE);
    return 0;
}

static void
c_literal_numeric_escape(te_string *dest, char c)
{
    te_string_append(dest, "\\%3.3" PRIo8, (uint8_t)c);
}

static te_errno
c_literal_filter(const te_string *src, te_string *dest)
{
    static const char *c_esc[UINT8_MAX + 1] = {
        ['\\'] = "\\\\",
        ['\"'] = "\\\"",
        ['\''] = "\\'",
        ['?'] = "\\?",
        ['\a'] = "\\a",
        ['\b'] = "\\b",
        ['\f'] = "\\f",
        ['\n'] = "\\n",
        ['\r'] = "\\r",
        ['\t'] = "\\t",
        ['\v'] = "\\v",
    };

    te_string_generic_escape(dest, te_string_value(src), c_esc,
                             c_literal_numeric_escape,
                             c_literal_numeric_escape);
    return 0;
}

static te_errno
c_identifier_filter(const te_string *src, te_string *dest)
{
    const char *iter;

    if (src->len == 0 || isdigit(src->ptr[0]))
        te_string_append(dest, "_");

    for (iter = te_string_value(src); *iter != '\0'; iter++)
    {
        if (isalnum(*iter))
            te_string_append(dest, "%c", *iter);
        else
            te_string_append(dest, "_");
    }
    return 0;
}

static te_errno
crlf_filter(const te_string *src, te_string *dest)
{
    te_bool after_cr = FALSE;
    const char *iter;

    for (iter = te_string_value(src); *iter != '\0'; iter++)
    {
        if (*iter == '\n' && !after_cr)
            te_string_append(dest, "\r");
        te_string_append(dest, "%c", *iter);
        after_cr = (*iter == '\r');
    }
    return 0;
}

static te_errno
even_filter(const te_string *src, te_string *dest)
{
    intmax_t ival;
    te_errno rc = te_strtoimax(src->ptr, 0, &ival);

    if (rc != 0)
        return rc;

    if (ival % 2 != 0)
        return TE_ENODATA;

    te_string_append_buf(dest, src->ptr, src->len);
    return 0;
}

static te_errno
hex_filter(const te_string *src, te_string *dest)
{
    const char *iter;

    for (iter = te_string_value(src); *iter != '\0'; iter++)
        te_string_append(dest, "%2.2" PRIx8, (uint8_t)*iter);

    return 0;
}

static te_errno
json_filter(const te_string *src, te_string *dest)
{
    te_json_ctx_t ctx = TE_JSON_INIT_STR(dest);

    te_json_add_string(&ctx, "%s", te_string_value(src));
    return 0;
}

static te_errno
length_filter(const te_string *src, te_string *dest)
{
    te_string_append(dest, "%zu", src->len);
    return 0;
}

static te_errno
nonzero_filter(const te_string *src, te_string *dest)
{
    intmax_t ival;
    te_errno rc = te_strtoimax(src->ptr, 0, &ival);

    if (rc != 0)
        return rc;

    if (ival == 0)
        return TE_ENODATA;

    te_string_append_buf(dest, src->ptr, src->len);
    return 0;
}

static te_errno
normalize_filter(const te_string *src, te_string *dest)
{
    te_bool after_space = TRUE;
    const char *iter;
    size_t prev_len = dest->len;

    for (iter = te_string_value(src); *iter != '\0'; iter++)
    {
        if (!isspace(*iter))
            te_string_append(dest, "%c", *iter);
        else if (!after_space)
            te_string_append(dest, " ");

        after_space = isspace(*iter);
    }

    if (after_space && dest->len > prev_len)
        te_string_cut(dest, 1);

    return 0;
}

static te_errno
notempty_filter(const te_string *src, te_string *dest)
{
    if (src->len == 0)
        return TE_ENODATA;

    te_string_append_buf(dest, src->ptr, src->len);
    return 0;
}

static te_errno
odd_filter(const te_string *src, te_string *dest)
{
    intmax_t ival;
    te_errno rc = te_strtoimax(src->ptr, 0, &ival);

    if (rc != 0)
        return rc;

    if (ival % 2 == 0)
        return TE_ENODATA;

    te_string_append_buf(dest, src->ptr, src->len);
    return 0;
}

static te_errno
pred_filter(const te_string *src, te_string *dest)
{
    intmax_t ival;
    te_errno rc = te_strtoimax(src->ptr, 0, &ival);

    if (rc != 0)
        return rc;

    if (ival == 0)
        return TE_EDOM;

    te_string_append(dest, "%jd", ival - 1);
    return 0;
}

static te_errno
shell_filter(const te_string *src, te_string *dest)
{
    te_string_append_shell_arg_as_is(dest, te_string_value(src));
    return 0;
}

static te_errno
succ_filter(const te_string *src, te_string *dest)
{
    intmax_t ival;
    te_errno rc = te_strtoimax(src->ptr, 0, &ival);

    if (rc != 0)
        return rc;

    if (ival == -1)
        return TE_EDOM;

    te_string_append(dest, "%jd", ival + 1);
    return 0;
}

static te_errno
uppercase_filter(const te_string *src, te_string *dest)
{
    const char *iter;

    for (iter = te_string_value(src); *iter != '\0'; iter++)
        te_string_append(dest, "%c", toupper(*iter));

    return 0;
}

static te_errno
uri_filter(const te_string *src, te_string *dest)
{
    te_string_append_escape_uri(dest, TE_STRING_URI_ESCAPE_BASE,
                                te_string_value(src));
    return 0;
}

static void
xml_numeric_escape(te_string *str, char c)
{
    te_string_append(str, "&#x%" PRIx8 ";", (uint8_t)c);
}

static te_errno
xml_filter(const te_string *src, te_string *dest)
{
    static const char *xml_esc[UINT8_MAX + 1] = {
        ['&'] = "&amp;",
        ['<'] = "&lt;",
        ['>'] = "&gt;",
        ['"'] = "&quot;",
        ['\''] = "&apos;",
    };

    te_string_generic_escape(dest, te_string_value(src), xml_esc,
                             xml_numeric_escape, NULL);
    return 0;
}

static expand_filter
lookup_filter(const char *name)
{
    struct {
        const char *name;
        expand_filter fn;
    } filters[] = {
        {"base64", base64_filter},
        {"base64uri", base64uri_filter},
        {"c", c_literal_filter},
        {"cid", c_identifier_filter},
        {"crlf", crlf_filter},
        {"even", even_filter},
        {"hex", hex_filter},
        {"json", json_filter},
        {"length", length_filter},
        {"nonzero", nonzero_filter},
        {"normalize", normalize_filter},
        {"notempty", notempty_filter},
        {"odd", odd_filter},
        {"pred", pred_filter},
        {"shell", shell_filter},
        {"succ", succ_filter},
        {"upper", uppercase_filter},
        {"uri", uri_filter},
        {"xml", xml_filter},
        {NULL, NULL}
    };
    unsigned int i;

    for (i = 0; filters[i].name != NULL; i++)
    {
        if (strcmp(name, filters[i].name) == 0)
            return filters[i].fn;
    }

    return NULL;
}

static te_errno
expand_with_filter(te_string *dest, char *ref,
                   te_expand_param_func expand_param, const void *ctx)
{
    const char *filter = NULL;
    te_errno rc;

    rc = te_strpbrk_rev_balanced(ref, *OPENING, *CLOSING, '\0',
                                 FILTER, &filter);
    if (rc == TE_EILSEQ)
        return rc;

    if (filter == NULL)
    {
        te_bool expanded = expand_param(ref, ctx, dest);

        return expanded ? 0 : TE_ENODATA;
    }
    else
    {
        te_string tmp = TE_STRING_INIT;
        te_errno rc;
        expand_filter fn;

        *TE_CONST_PTR_CAST(char, filter) = '\0';
        fn = lookup_filter(filter + 1);
        if (fn == NULL)
        {
            ERROR("Invalid expansion filter: %s", filter + 1);
            te_string_free(&tmp);
            return TE_EINVAL;
        }

        rc = expand_with_filter(&tmp, ref, expand_param, ctx);
        if (rc != 0)
        {
            te_string_free(&tmp);
            return rc;
        }
        rc = fn(&tmp, dest);
        te_string_free(&tmp);

        return rc;
    }
}

static const char *
process_reference(const char *start, te_expand_param_func expand_param,
                  const void *ctx, te_string *dest)
{
    const char *end;

    if (te_strpbrk_balanced(start, *OPENING, *CLOSING, '\0', NULL, &end) != 0)
        return NULL;
    else
    {
        const char *default_value = NULL;
        char ref[end - start - 1];
        size_t prev_len;
        te_errno expand_rc;

        memcpy(ref, start + 1, end - start - 2);
        ref[end - start - 2] = '\0';

        if (te_strpbrk_balanced(ref, *OPENING, *CLOSING, '\0', MODIFIER_INTRO,
                                &default_value) == TE_EILSEQ)
            return NULL;

        if (default_value != NULL)
        {
            if (default_value[1] != *DEFAULT_VALUE &&
                default_value[1] != *ALTERNATE_VALUE)
                return NULL;

            *TE_CONST_PTR_CAST(char, default_value) = '\0';
            default_value++;
        }

        prev_len = dest->len;
        expand_rc = expand_with_filter(dest, ref, expand_param, ctx);
        if (expand_rc != 0 && expand_rc != TE_ENODATA)
            return NULL;

        if (default_value != NULL)
        {
            if (*default_value == *ALTERNATE_VALUE && expand_rc == 0)
            {
                te_string_cut(dest, dest->len - prev_len);
                if (te_string_expand_parameters(default_value + 1,
                                                expand_param, ctx, dest) != 0)
                    return NULL;
            }
            else if (*default_value == *DEFAULT_VALUE && expand_rc != 0)
            {
                if (te_string_expand_parameters(default_value + 1,
                                                expand_param, ctx, dest) != 0)
                    return NULL;
            }
        }

        return end;
    }
}

/* See description in te_expand.h */
te_errno
te_string_expand_parameters(const char *src,
                            te_expand_param_func expand_param,
                            const void *ctx, te_string *dest)
{
    const char *next = NULL;

    for (;;)
    {
        next = strstr(src, VARREF OPENING);
        if (next == NULL)
        {
            te_string_append(dest, "%s", src);
            break;
        }
        te_string_append(dest, "%.*s", next - src, src);
        next++;

        src = process_reference(next, expand_param, ctx, dest);
        if (src == NULL)
            return TE_EINVAL;
    }

    return 0;
}

/* See description in te_expand.h */
te_errno
te_string_expand_env_vars(const char *src,
                          const char *posargs[TE_EXPAND_MAX_POS_ARGS],
                          te_string *dest)
{
    return te_string_expand_parameters(src, expand_env_value, posargs, dest);
}

/* See description in te_expand.h */
te_errno
te_string_expand_kvpairs(const char *src,
                         const char *posargs[TE_EXPAND_MAX_POS_ARGS],
                         const te_kvpair_h *kvpairs, te_string *dest)
{
    kvpairs_expand_ctx ctx = {.posargs = posargs, .kvpairs = kvpairs};

    return te_string_expand_parameters(src, expand_kvpairs_value, &ctx, dest);
}

typedef struct legacy_expand_ctx {
    const char **posargs;
    te_param_value_getter get_param_value;
    const void *params_ctx;
} legacy_expand_ctx;

static te_bool
legacy_expand(const char *param_name, const void *ctx, te_string *dest)
{
    const legacy_expand_ctx *legacy_ctx = ctx;
    const char *value = get_positional_arg(param_name, legacy_ctx->posargs);

    if (value == NULL)
        value = legacy_ctx->get_param_value(param_name, legacy_ctx->params_ctx);

    return value;
}

te_errno
te_expand_parameters(const char *src, const char **posargs,
                     te_param_value_getter get_param_value,
                     const void *params_ctx, char **retval)
{
    te_errno rc;
    legacy_expand_ctx legacy_ctx = {
        .posargs = posargs,
        .get_param_value = get_param_value,
        .params_ctx = params_ctx,
    };
    te_string str = TE_STRING_INIT;

    rc = te_string_expand_parameters(src, legacy_expand, &legacy_ctx,
                                     &str);

    if (rc != 0)
    {
        te_string_free(&str);
        *retval = NULL;
        return rc;
    }

    te_string_move(retval, &str);

    return 0;
}
