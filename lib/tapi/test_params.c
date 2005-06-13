/** @file
 * @brief Test API
 *
 * Procedures that provide access to test parameter.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "te_defs.h"

#define TE_LGR_USER     "TAPI Params"
#include "logger_api.h"

#include "tapi_test.h"


/** Global variable with entity name for logging */
DEFINE_LGR_ENTITY("(unknown)");


/* See description in sockapi-ts.h */
void
sigint_handler(int signum)
{
    UNUSED(signum);
    exit(TE_EXIT_SIGINT);
}


/** See the description in tapi_test.h */
const char *
test_get_param(int argc, char *argv[], const char *name)
{
    int         i;
    const char *ptr;

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], name, strlen(name)) != 0)
            continue;

        ptr = argv[i] + strlen(name);
        while (isspace(*ptr))
            ptr++;
        if (*(ptr++) != '=')
        {
            ERROR("Error while parsing '%s' parameter value: "
                  "cannot find '=' delimeter", name);
            return NULL;
        }
        while (isspace(*ptr))
            ptr++;

        return ptr;
    }
    WARN("There is no '%s' parameter specified", name);

    return NULL;
}

/** See the description in tapi_test.h */
int
test_map_param_value(const char *var_name,
                     struct param_map_entry *maps, const char *str_val,
                     int *num_val)
{
    int i;

    for (i = 0; maps[i].str_val != NULL; i++)
    {
        if (strcmp(str_val, maps[i].str_val) == 0)
        {
            *num_val = maps[i].num_val;
            return 0;
        }
    }

    {
        char buf[2048];

        snprintf(buf, sizeof(buf),
                 "'%s' parameter has incorrect value '%s'. "
                 "It can have the following value: {",
                 var_name, str_val);

        for (i = 0; maps[i].str_val != NULL; i++)
        {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "'%s', ", maps[i].str_val);
        }
        if (i > 0)
        {
            /* Delete comma after the last item in the list */
            buf[strlen(buf) - 2] = '\0';
        }
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "}");

        ERROR(buf);
    }

    return -1;
}

/** See the description in tapi_test.h */
uint8_t *
test_get_octet_string_param(const char *str_val, size_t len)
{
    uint8_t        *oct_string, *ptr;
    const char     *nptr, *endptr;
    unsigned int    i = 0;

    if (str_val == NULL)
    {
        ERROR("Invalid parameter: NULL pointer");
        return NULL;
    }

    oct_string = (uint8_t *)calloc(len, sizeof(uint8_t));
    if (oct_string == NULL)
    {
        ERROR("Error in memory allocation");
        return NULL;
    }
    ptr = oct_string;
    nptr = NULL;
    endptr = str_val;

    while ((nptr != endptr) && (endptr) && (i < len))
    {
        /* How should I fight args list ?*/
        if (*endptr == ':')
            endptr++;
        nptr = endptr;
        *ptr++ = (unsigned char)strtol(nptr, (char **)&endptr, 16);
        i++;
    }

    if (*endptr != '\0')
    {
        ERROR("Error in parsing octet string %s or bad given length %u",
              str_val, len);
        free(oct_string);
        return NULL;
    }

    if (i != len)
    {
        ERROR("Bad given length %u for octet string %s", len, str_val);
        free(oct_string);
        return NULL;
    }

    return oct_string;
}

/** See the description in tapi_test.h */
const char*
print_octet_string(const uint8_t *oct_string, size_t len)
{
    static char     buf[256];

    char           *p = buf;
    unsigned int    i;

    if (oct_string == NULL || len == 0)
        strncpy (buf, "<null octet string>", sizeof(buf));

    memset(buf, 0, sizeof(buf));

    for (i = 0; i < len; i++)
        p += sprintf(p, " 0x%02x", (unsigned int)(*oct_string++));

    return buf;
}
