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

#include "tq_string.h"


/* See the description in tq_string.h */
void
tq_strings_free(tqh_strings *head, void (*value_free)(void *))
{
    tqe_string *p;

    while ((p = head->tqh_first) != NULL)
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

    for (p1 = s1->tqh_first, p2 = s2->tqh_first;
         p1 != NULL && p2 != NULL && strcmp(p1->v, p2->v) == 0;
         p1 = p1->links.tqe_next, p2 = p2->links.tqe_next);
    
    return (p1 == NULL) && (p2 == NULL);
}
