/** @file
 * @brief Test API
 *
 * Procedures that provide access to test parameter.
 *
 * Copyright (C) 2004 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
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
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "te_defs.h"

#define LGR_USER        "TAPI Params"
#include "logger_api.h"

#include "tapi_test.h"


/** Global variable with entity name for logging */
DEFINE_LGR_ENTITY("(unknown)");

/* See description in sockapi-test.h */
char *test_backup_name = NULL;


/* See description in sockapi-ts.h */
void
sigint_handler(int signum)
{
    UNUSED(signum);
    exit(EXIT_FAILURE);
}


/** See the description in sockapi-test.h */
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

/** See the description in sockapi-test.h */
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



