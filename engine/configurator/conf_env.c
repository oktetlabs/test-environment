/** @file
 * @brief Configurator
 *
 * Using environment variables in config files
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

int
cfg_expand_env_vars(const char *src, char **retval)
{
    const char *next = NULL;
    char *result = NULL;
    int result_len = 0;
    int len;
    char env_var_name[128];
    char *env_var;

    for (;;)
    {
        next = strstr(src, "${");
        if(!next)
        {
            len = strlen(src) + 1;
            result = realloc(result, result_len + len);
            memcpy(result + result_len, src, len);
            break;
        }
        if(next != src)
        {
            result = realloc(result, result_len + (next - src));
            memcpy(result + result_len, src, next - src);
            result_len += next - src;
        }
        next += 2;
        src = strchr(next, '}');
        if (!src)
        {
            *retval = NULL;
            return 1; /* actual error code must be added later */
        }
        if ((unsigned)(src - next) > sizeof(env_var_name))
        {
            *retval = NULL;
            return 2; /* see above */
        }
        memcpy(env_var_name, next, src - next);
        env_var_name[src - next] = '\0';
        env_var = getenv(env_var_name);
        if(env_var != NULL)
        {
            len = strlen(env_var);
            result = realloc(result, result_len + len);
            memcpy(result + result_len, env_var, len);
            result_len += len;
        }
        src++;
    }
    *retval = result;
    return 0;
}

