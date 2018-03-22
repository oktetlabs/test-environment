/** @file
 * @brief Test Environment common definitions
 *
 * Test parameters related
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 *
 * $Id:
 */

#ifndef __TE_PARAM_H__
#define __TE_PARAM_H__

#include "te_config.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif


/** Prefix for values of test arguments which in fact refer to
 * variables */
#define TEST_ARG_VAR_PREFIX "VAR."

/** Prefix for env variables which are connected with test arguments */
#define TEST_ARG_ENV_PREFIX "TE_TEST_VAR_"

/**
 * Function converts name of test variable to name of environment variable
 * which holds it's actual value.
 *
 * actual prefixes are defind above.
 *
 * Rules: VAR.xxx.yyy ->> TE_TEST_VAR_xxx__yyy
 *
 * @param name   [in] Name of the variable
 * @param env    [out] Pointer to memory to store env variable name
 */
static inline void te_var_name2env(const char *name, char *env, int env_size)
{
    char var_name[128];
    int rc = 0;

    char *p;
    char *dot;

    strcpy(var_name, name);
    p = var_name + strlen(TEST_ARG_VAR_PREFIX);

    rc += snprintf(env, env_size,
                   TEST_ARG_ENV_PREFIX);
    do
    {
        dot = strchr(p, '.');
        if (dot != NULL)
            *dot = '\0';

        rc += snprintf(env + rc,
                       env_size - rc,
                       "%s", p);
        if (dot != NULL)
            rc += snprintf(env + rc,
                           env_size - rc,
                           "__");
        p = dot + 1;
    } while ( dot && p && *p != '\0' );
}

/** Define one entry in the list of maping entries */
#define MAPPING_LIST_ENTRY(entry_val_) \
    { #entry_val_, (int)RPC_ ## entry_val_ }

/** Entry for mapping parameter value from string to interger */
struct param_map_entry {
    const char *str_val; /**< value in string format */
    int         num_val; /**< Value in native numberic format */
};

#endif  /* __TE_PARAM_H__ */
