/** @file
 * @brief Parameters expansion API
 *
 * API that allows to expand parameters in a string
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
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

#include "logger_api.h"
#include "te_expand.h"

int
te_expand_env_vars(const char *src, char **posargs, char **retval)
{
    const char *next = NULL;
    char       *result = NULL;
    int         result_len = 0;
    int         len;
    char        env_var_name[128];
    char       *default_value;
    char       *env_var;
    int         brace_level = 0;
    te_bool     need_free_env = FALSE;

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

        if ((unsigned)(src - next - 1) > sizeof(env_var_name))
        {
            free(result);
            *retval = NULL;
            return ENOBUFS;
        }
        memcpy(env_var_name, next, src - next - 1);
        env_var_name[src - next - 1] = '\0';
        default_value = strchr(env_var_name, ':');
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
        if (isdigit(*env_var_name) && env_var_name[1] == '\0')
        {
            env_var = (posargs == NULL ? NULL :
                       posargs[*env_var_name - '0']);
        }
        else
        {
            env_var = getenv(env_var_name);
        }
        if (default_value != NULL)
        {
            if ((*default_value == '+' && env_var != NULL) ||
                (*default_value == '-' && env_var == NULL))
            {
                int rc = te_expand_env_vars(default_value + 1, NULL,
                                            &env_var);

                if (rc != 0)
                {
                    free(result);
                    *retval = NULL;
                    return rc;
                }
                need_free_env = TRUE;
            }
        }
        if (env_var != NULL)
        {
            len = strlen(env_var);
            result = realloc(result, result_len + len);
            if (result == NULL)
                return errno;
            memcpy(result + result_len, env_var, len);
            result_len += len;
            if (need_free_env)
            {
                free(env_var);
                need_free_env = FALSE;
            }
        }
    }
    *retval = result;
    return 0;
}
