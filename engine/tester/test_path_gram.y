/** @file
 * @brief Tester Subsystem
 *
 * Gramma for test path specification.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */
%{
#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#define TE_LGR_USER     "Test Path Parser"
#include "logger_api.h"

#include "test_path.h"
#include "test_path_gram.h"
#include "test_path_lex.h"


/** Current test path item argument */
static test_path_arg *item_arg = NULL;
/** Current test path item */
static test_path_item *item = NULL;
/** Current test path */
static test_path *path = NULL;


/** Current test path string */
const char *test_path_str;
/** Current position in test path string */
int test_path_str_pos;


/**
 * Parser error reporting.
 *
 * @param str       Error description string
 */
static void
test_path_error(const char *str)
{
    ERROR("YACC: %s\nInput=%s\nRest=%s", str, test_path_str,
          test_path_str + test_path_str_pos);
}

/**
 * Allocate a new test path item.
 *
 * @param name          Name of the item or NULL
 *
 * @return Status code.
 *
 * @note @a name is owned by the function in any case.
 */
static te_errno
test_path_new_item(char *name)
{
    item = calloc(1, sizeof(*item));
    if (item == NULL)
    {
        free(name);
        return TE_ENOMEM;
    }

    TAILQ_INIT(&item->args);

    item->name = name;
    item->iterate = 1;

    TAILQ_INSERT_TAIL(&path->head, item, links);

    VERB("New path item with name '%s' allocated", name);

    return 0;
}

/**
 * Allocate a new test path item argument.
 *
 * @param name          Name of the argument or NULL
 *
 * @return Status code.
 *
 * @note @a name is owned by the function in any case.
 */
static te_errno
test_path_new_arg(char *name)
{
    te_errno rc;

    if (item == NULL && (rc = test_path_new_item(NULL)) != 0)
    {
        free(name);
        return rc;
    }
    assert(item != NULL);

    item_arg = calloc(1, sizeof(*item_arg));
    if (item_arg == NULL)
    {
        free(name);
        return TE_ENOMEM;
    }

    TAILQ_INIT(&item_arg->values);

    item_arg->name = name;

    TAILQ_INSERT_TAIL(&item->args, item_arg, links);

    VERB("New path item argument with name '%s' allocated", name);

    return 0;
}

/**
 * Allocate a new test path item argument value.
 *
 * @param value         Value or NULL
 *
 * @return Status code.
 *
 * @note @a value is owned by the function in any case.
 */
static te_errno
test_path_new_arg_value(char *value)
{
    te_errno    rc;
    tqe_string *p;

    if (item_arg == NULL && (rc = test_path_new_arg(NULL)) != 0)
    {
        free(value);
        return rc;
    }
    assert(item_arg != NULL);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        free(value);
        return TE_ENOMEM;
    }

    p->v = value;

    TAILQ_INSERT_TAIL(&item_arg->values, p, links);

    VERB("New path item argument value '%s' allocated", value);

    return 0;
}

%}

%pure-parser
%locations
%name-prefix="test_path_"

%union {
    char           *str;
    unsigned int    num;
}

%token SLASH COLON EQUAL COMMA OPEN CLOSE ITERATE SELECT STEP
%token <num> NUMBER
%token <str> STRING

%%

path: 
    SLASH
    {
        VERB("path -> /");
    }
    |
    item_iter
    {
        VERB("path -> item_iter");
    }
    |
    path SLASH item_iter
    {
        VERB("path -> path SLASH item_iter");
    }
    ;

item_iter:
    item_select ITERATE NUMBER
    {
        VERB("item_iter -> item_select ITERATE NUMBER");
        assert(item != NULL);
        item->iterate = $3;
        if (item->iterate <= 0)
        {
            ERROR("Invalid number (%d) of iterations is requested",
                  (int)(item->iterate));
            YYABORT;
        }
        item = NULL;
    }
    |
    item_select
    {
        VERB("item_iter -> item_select");
        item = NULL;
    }
    ;

item_select:
    item_simple SELECT NUMBER STEP NUMBER
    {
        VERB("item_select -> item_simple SELECT NUMBER STEP NUMBER");
        assert(item != NULL);
        item->select = $3;
        if (item->select <= 0)
        {
            ERROR("Invalid iteration number (%d) is requested",
                  (int)(item->select));
            YYABORT;
        }
        item->step = $5;
        if (item->step <= 0)
        {
            ERROR("Invalid iteration step number (%d) is requested",
                  (int)(item->step));
            YYABORT;
        }
    }
    |
    item_simple SELECT NUMBER
    {
        VERB("item_select -> item_simple SELECT NUMBER");
        assert(item != NULL);
        item->select = $3;
        if (item->select <= 0)
        {
            ERROR("Invalid iteration number (%d) is requested",
                  (int)(item->select));
            YYABORT;
        }
    }
    |
    item_simple
    {
        VERB("item_select -> item_simple");
    }
    ;

item_simple:
    STRING COLON args
    {
        VERB("item_simple -> STRING=%s COLON args", $1);
        assert(item != NULL);
        assert(item->name == NULL);
        item->name = $1;
    }
    |
    STRING
    {
        VERB("item_simple -> STRING=%s", $1);
        if (test_path_new_item($1) != 0)
            YYABORT;
    }
    ;

args:
    arg 
    {
        VERB("args -> arg");
    }
    |
    args COMMA arg 
    {
        VERB("args -> args COMMA arg");
    }
    ;

arg:
    STRING EQUAL values
    {
        VERB("arg=%s", $1);
        assert(item_arg != NULL);
        assert(item_arg->name == NULL);
        item_arg->name = $1;
        item_arg = NULL;
    }
    ;

values:
    STRING
    {
        VERB("values -> STRING=%s", $1);
        if (test_path_new_arg_value($1) != 0)
            YYABORT;
    }
    |
    OPEN values_list CLOSE
    {
        VERB("values -> values_list");
    }
    ;

values_list:
    STRING
    {
        VERB("values_list -> STRING=%s", $1);
        if (test_path_new_arg_value($1) != 0)
            YYABORT;
    }
    |
    values_list COMMA STRING
    {
        VERB("values_list -> values_list COMMA STRING=%s", $3);
        if (test_path_new_arg_value($3) != 0)
            YYABORT;
    }
    ;

%%


/* See description in test_path.h */
te_errno
tester_test_path_parse(test_path *path_location)
{
    test_path_str = path_location->str;
    test_path_str_pos = 0;
    path = path_location;

    if (test_path_parse() != 0)
    {
        ERROR("Failed to parse test path string '%s'",
              path_location->str);
        return TE_EINVAL;
    }

    return 0;
}
