/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Parameters expansion API
 *
 * API that allows to expand parameters in a string
 *
 * Copyright (C) 2018-2023 OKTET Labs Ltd. All rights reserved.
 */

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

#include "te_str.h"
#include "te_expand.h"

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
} kvpairs_expand_ctx;

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
    const char *value = get_positional_arg(param_name, kvpairs_ctx->posargs);

    if (value == NULL)
        value = te_kvpairs_get(kvpairs_ctx->kvpairs, param_name);

    te_string_append(dest, "%s", te_str_empty_if_null(value));

    return value != NULL;
}

/* See description in te_expand.h */
te_errno
te_string_expand_parameters(const char *src,
                            te_expand_param_func expand_param,
                            const void *ctx, te_string *dest)
{
    const char *next = NULL;
    char        param_name[TE_EXPAND_PARAM_NAME_LEN];
    char       *default_value;
    int         brace_level = 0;
    te_bool expanded;
    size_t prev_len;
    te_errno rc;

    for (;;)
    {
        next = strstr(src, "${");
        if (next == NULL)
        {
            te_string_append(dest, "%s", src);
            break;
        }
        te_string_append(dest, "%.*s", next - src, src);
        next += 2;

        for (src = next, brace_level = 1; brace_level != 0; src++)
        {
            switch (*src)
            {
                case '\0':
                    return TE_EINVAL;
                case '{':
                    brace_level++;
                    break;
                case '}':
                    brace_level--;
                    break;
            }
        }

        if ((size_t)(src - next - 1) >= sizeof(param_name))
            return TE_ENOBUFS;

        memcpy(param_name, next, src - next - 1);
        param_name[src - next - 1] = '\0';
        default_value = strchr(param_name, ':');
        if (default_value != NULL)
        {
            if (default_value[1] == '+' || default_value[1] == '-')
            {
                *default_value++ = '\0';
            }
            else
            {
                default_value = NULL;
            }
        }

        prev_len = dest->len;
        expanded = expand_param(param_name, ctx, dest);
        if (default_value != NULL)
        {
            if (*default_value == '+' && expanded)
            {
                te_string_cut(dest, dest->len - prev_len);
                rc = te_string_expand_parameters(default_value + 1,
                                                 expand_param, ctx, dest);
                if (rc != 0)
                    return rc;
            }
            else if (*default_value == '-' && !expanded)
            {
                rc = te_string_expand_parameters(default_value + 1,
                                                 expand_param, ctx, dest);
                if (rc != 0)
                    return rc;
            }
        }
    }

    return 0;
}

/* See description in te_expand.h */
te_errno
te_string_expand_env_vars(const char *src,
                          const char *posargs[static TE_EXPAND_MAX_POS_ARGS],
                          te_string *dest)
{
    return te_string_expand_parameters(src, expand_env_value, posargs, dest);
}

/* See description in te_expand.h */
te_errno
te_string_expand_kvpairs(const char *src,
                         const char *posargs[static TE_EXPAND_MAX_POS_ARGS],
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
