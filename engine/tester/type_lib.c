/** @file
 * @brief Tester Subsystem
 *
 * Types support library.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

/** Logging user name to be used here */
#define TE_LGR_USER     "Types"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_errno.h"
#include "te_queue.h"
#include "tester_conf.h"


/** Boolean 'false' value */
static test_entity_value boolean_false = {
    .links = { NULL, NULL },
    .name = "false",
    .type = NULL,
    .plain = "FALSE",
    .ref = NULL,
    .ext = NULL,
    .reqs = { NULL, NULL }
};

/** Boolean 'true' value */
static test_entity_value boolean_true = {
    .links = { NULL, NULL },
    .name = "true",
    .type = NULL,
    .plain = "TRUE",
    .ref = NULL,
    .ext = NULL,
    .reqs = { NULL, NULL }
};

/** Boolean type */
static test_value_type boolean = {
    { NULL, NULL }, "boolean", NULL, { { NULL, NULL }, 0 }
};

/** List of predefined types */
static test_value_types predefined_types;


/* See the description in type_lib.h */
te_errno
tester_init_types(void)
{
    LIST_INIT(&predefined_types);

    /* Initialize boolean type values */
    TAILQ_INIT(&boolean_false.reqs);
    TAILQ_INIT(&boolean_true.reqs);
    /* Initialize boolean type and add it in predefined types list */
    TAILQ_INIT(&boolean.values.head);
    TAILQ_INSERT_TAIL(&boolean.values.head, &boolean_false, links);
    TAILQ_INSERT_TAIL(&boolean.values.head, &boolean_true, links);
    boolean.values.num = 2;
    LIST_INSERT_HEAD(&predefined_types, &boolean, links);

    return 0;
}

/**
 * Find type in the list of types by name.
 *
 * @param types         List of types
 * @param name          Name of the type to find
 *
 * @return Pointer to found type or NULL.
 */
static const test_value_type *
types_find_type(const test_value_types *types, const char *name)
{
    const test_value_type  *p;

    for (p = types->lh_first; p != NULL; p = p->links.le_next)
    {
        assert(p->name != NULL);
        if (strcmp(name, p->name) == 0)
            return p;
    }
    return NULL;
}

/* See the description in type_lib.h */
const test_value_type *
tester_find_type(const test_session *session, const char *name)
{
    const test_value_type  *p;

    while (session != NULL)
    {
        p = types_find_type(&session->types, name);
        if (p != NULL)
            return p;
        session = session->parent;
    }

    return types_find_type(&predefined_types, name);
}


/* See the description in type_lib.h */
void
tester_add_type(test_session *session, test_value_type *type)
{
    LIST_INSERT_HEAD(&session->types, type, links);
}


/**
 * Callback function to find entity value with specified plain value.
 *
 * The function complies with test_entity_value_enum_cb prototype.
 */
static te_errno
check_plain_value_cb(const test_entity_value *value, void *opaque)
{
    const char *plain = *(const void **)opaque;

    if (value->plain != NULL && strcmp(plain, value->plain) == 0)
    {
        *(const void **)opaque = value;
        return TE_EEXIST;
    }
    else
    {
        return 0;
    }
}

/* See the description in type_lib.h */
const test_entity_value *
tester_type_check_plain_value(const test_value_type *type,
                              const char *plain)
{
    te_errno    rc;
    const void *data = plain;

    rc = test_entity_values_enum(NULL, &type->values,
                                 check_plain_value_cb, &data);
    if (rc == TE_EEXIST)
    {
        return data;
    }
    else
    {
        ERROR("Type '%s' does not have value '%s'", type->name, plain);
        return NULL;
    }
}
