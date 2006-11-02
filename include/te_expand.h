/** @file
 * @brief Environment variable expansion
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_EXPAND_H__
#define __TE_EXPAND_H__

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef TE_EXPAND_XML
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#endif

#include "te_defs.h"


/* Attention: this file must be included AFTER logging related headers */

/**
 * Expands environment variables in a string.
 * The variable names must be between ${ and }.
 * Conditional expansion is supported:
 * - ${NAME:-VALUE} is expanded into VALUE if NAME variable is not set,
 * otherwise to its value
 * - ${NAME:+VALUE} is expanded into VALUE if NAME variable is set,
 * otherwise to an empty string.
 *
 * @note The length of anything between ${...} must be less than 128
 *
 * @param src     Source string
 * @param posargs Positional parameters (expandable via ${[0-9]})
 *                (may be NULL)
 * @param retval  Resulting string (OUT)
 *
 * @return 0 if success, an error code otherwise
 * @retval ENOBUFS The variable name is longer than 128
 * @retval EINVAL Unmatched ${ found
 */
static inline int
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


#ifdef TE_EXPAND_XML
/**
 * A wrapper around xmlGetProp that expands environment variable
 * references.
 *
 * @param node    XML node
 * @param name    XML attribute name
 *
 * @return The expanded attribute value or NULL if no attribute
 * or an error occured while expanding.
 *
 * @sa cfg_expand_env_vars
 */
static inline char *
xmlGetProp_exp(xmlNodePtr node, const xmlChar *name)
{
    xmlChar *value = xmlGetProp(node, name);

    if (value)
    {
        char *result = NULL;
        int   rc;

        rc = te_expand_env_vars((const char *)value, NULL, &result);
        if (rc == 0)
        {
            xmlFree(value);
            value = (xmlChar *)result;
        }
        else
        {
            ERROR("Error substituting variables in %s '%s': %s", 
                  name, value, strerror(rc));
            xmlFree(value);
            value = NULL;
        }
    }
    return (char *)value;
}
#endif /* TE_EXPAND_XML */


#endif /* !__TE_EXPAND_H__ */
