/** @file
 * @brief Parameters expansion API
 *
 * API that allows to expand parameters in a string
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
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

#include "te_expand.h"

/**
 * Get parameter value by name from environment.
 *
 * @param param_name    Name of required value
 *
 * @retval   Requested @p param_name value if found, otherwise @c NULL
 */
static char *
te_get_env_value(const char *param_name, void *params_ctx)
{
    UNUSED(params_ctx);

    return getenv(param_name);
}

/**
 * Get parameter value by name from queue of key-value pairs.
 *
 * @param param_name    Name of required value
 * @param params_ctx    Head of queue of key-value pairs
 *
 * @retval   Requested @p param_name value if found, otherwise @c NULL
 */
static char *
te_get_kvpairs_value(const char *param_name, void *params_ctx)
{
    return te_kvpairs_get((te_kvpair_h *)params_ctx, param_name);
}

/* See description in te_expand.h */
int
te_expand_parameters(const char *src, char **posargs,
                     te_param_value_getter get_param_value,
                     void *params_ctx, char **retval)
{
    const char *next = NULL;
    char       *result = NULL;
    int         result_len = 0;
    int         len;
    char        param_name[TE_EXPAND_PARAM_NAME_LEN];
    char       *default_value;
    char       *param_var;
    int         brace_level = 0;
    te_bool     need_param_free = FALSE;

    for (;;)
    {
        next = strstr(src, "${");
        if (next == NULL)
        {
            len = strlen(src) + 1;
            result = realloc(result, result_len + len);
            if (result == NULL)
                return errno;
            memcpy(result + result_len, src, len);
            break;
        }
        if (next != src)
        {
            result = realloc(result, result_len + (next - src));
            if (result == NULL)
                return errno;
            memcpy(result + result_len, src, next - src);
            result_len += next - src;
        }
        next += 2;

        for (src = next, brace_level = 1; brace_level != 0; src++)
        {
            switch (*src)
            {
                case '\0':
                    free(result);
                    *retval = NULL;
                    return EINVAL;
                case '{':
                    brace_level++;
                    break;
                case '}':
                    brace_level--;
                    break;
            }
        }

        if ((unsigned)(src - next - 1) > sizeof(param_name))
        {
            free(result);
            *retval = NULL;
            return ENOBUFS;
        }
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
        if (isdigit(*param_name) && param_name[1] == '\0')
        {
            param_var = (posargs == NULL ? NULL :
                         posargs[*param_name - '0']);
        }
        else
        {
            param_var = get_param_value(param_name, params_ctx);
        }
        if (default_value != NULL)
        {
            if ((*default_value == '+' && param_var != NULL) ||
                (*default_value == '-' && param_var == NULL))
            {
                int rc = te_expand_parameters(default_value + 1, NULL,
                                              get_param_value, params_ctx,
                                              &param_var);

                if (rc != 0)
                {
                    free(result);
                    *retval = NULL;
                    return rc;
                }
                need_param_free = TRUE;
            }
        }
        if (param_var != NULL)
        {
            len = strlen(param_var);
            result = realloc(result, result_len + len);
            if (result == NULL)
                return errno;
            memcpy(result + result_len, param_var, len);
            result_len += len;
            if (need_param_free)
            {
                free(param_var);
                need_param_free = FALSE;
            }
        }
    }
    *retval = result;
    return 0;
}

/* See description in te_expand.h */
int
te_expand_env_vars(const char *src, char **posargs, char **retval)
{
    return te_expand_parameters(src, posargs, &te_get_env_value,
                                NULL, retval);
}

/* See description in te_expand.h */
int
te_expand_kvpairs(const char *src, char **posargs, te_kvpair_h *head,
                  char **retval)
{
    return te_expand_parameters(src, posargs, &te_get_kvpairs_value,
                                head, retval);
}
