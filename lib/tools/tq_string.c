/** @file
 * @brief Tail queue of strings (char *).
 *
 * Implementation of API for working with tail queue of strings.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TQ String"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "tq_string.h"


/* See the description in tq_string.h */
void
tq_strings_free(tqh_strings *head, void (*value_free)(void *))
{
    tqe_string *p;

    if (head == NULL)
        return;

    while ((p = TAILQ_FIRST(head)) != NULL)
    {
        TAILQ_REMOVE(head, p, links);
        if (value_free != NULL)
            value_free(p->v);
        free(p);
    }
}

/* See the description in tq_string.h */
te_bool
tq_strings_equal(const tqh_strings *s1, const tqh_strings *s2)
{
    const tqe_string *p1;
    const tqe_string *p2;

    if (s1 == s2)
        return TRUE;
    if (s1 == NULL || s2 == NULL)
        return FALSE;

    for (p1 = TAILQ_FIRST(s1), p2 = TAILQ_FIRST(s2);
         p1 != NULL && p2 != NULL && strcmp(p1->v, p2->v) == 0;
         p1 = TAILQ_NEXT(p1, links), p2 = TAILQ_NEXT(p2, links));

    return (p1 == NULL) && (p2 == NULL);
}

/* See the description in tq_string.h */
te_errno
tq_strings_add_uniq_gen(tqh_strings *list, const char *value,
                        te_bool duplicate)
{
    tqe_string *p;

    assert(list != NULL);
    assert(value != NULL);

    for (p = TAILQ_FIRST(list);
         p != NULL && strcmp(value, p->v) != 0;
         p = TAILQ_NEXT(p, links));

    if (p == NULL)
    {
        p = TE_ALLOC(sizeof(*p));
        if (p == NULL)
            return TE_ENOMEM;

        if (duplicate)
        {
            p->v = strdup((char *)value);
            if (p->v == NULL)
            {
                free(p);
                return TE_ENOMEM;
            }
        }
        else
        {
            p->v = (char *)value;
        }

        TAILQ_INSERT_TAIL(list, p, links);

        return 0;
    }

    return 1;
}

/* See the description in tq_string.h */
te_errno
tq_strings_add_uniq(tqh_strings *list, const char *value)
{
    return tq_strings_add_uniq_gen(list, value, FALSE);
}

/* See the description in tq_string.h */
te_errno
tq_strings_add_uniq_dup(tqh_strings *list, const char *value)
{
    return tq_strings_add_uniq_gen(list, value, TRUE);
}
