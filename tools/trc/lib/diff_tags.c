/** @file
 * @brief Testing Results Comparator: diff tool
 *
 * Routines to work with TRC diff tags sets.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_queue.h"
#include "logger_api.h"

#include "trc_tags.h"
#include "trc_diff.h"


/* See the description in trc_diff.h */
trc_diff_set *
trc_diff_find_set(trc_diff_sets *sets, unsigned int id, te_bool create)
{
    trc_diff_set *p;

    assert(sets != NULL);
    assert(id < TRC_DIFF_IDS);

    for (p = TAILQ_FIRST(sets);
         p != NULL && p->id != id;
         p = TAILQ_NEXT(p, links));

    if (p == NULL && create)
    {
        p = TE_ALLOC(sizeof(*p));
        if (p == NULL)
            return NULL;

        p->id = id;
        TAILQ_INIT(&p->tags);
        TAILQ_INIT(&p->ignore);
        CIRCLEQ_INIT(&p->keys_stats);

        TAILQ_INSERT_TAIL(sets, p, links);
    }

    return p;
}

/* See the description in trc_diff.h */
te_errno
trc_diff_set_name(trc_diff_sets *sets, unsigned int id,
                  const char *name)
{
    trc_diff_set *p;

    if (sets == NULL || id >= TRC_DIFF_IDS || name == NULL)
        return TE_EINVAL;

    p = trc_diff_find_set(sets, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    p->name = strdup(name);
    if (p->name == NULL)
    {
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, name);
        return TE_ENOMEM;
    }

    return 0;
}

/* See the description in trc_diff.h */
te_errno
trc_diff_set_log(trc_diff_sets *sets, unsigned int id,
                 const char *log)
{
    trc_diff_set *p;

    if (sets == NULL || id >= TRC_DIFF_IDS || log == NULL)
        return TE_EINVAL;

    p = trc_diff_find_set(sets, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    p->log = strdup(log);
    if (p->log == NULL)
    {
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, log);
        return TE_ENOMEM;
    }

    return 0;
}

/* See the description in trc_diff.h */
te_errno
trc_diff_show_keys(trc_diff_sets *sets, unsigned int id)
{
    trc_diff_set *p;

    if (sets == NULL || id >= TRC_DIFF_IDS)
        return TE_EINVAL;

    p = trc_diff_find_set(sets, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    p->show_keys = TRUE;

    return 0;
}

/* See the description in trc_diff.h */
te_errno
trc_diff_add_tag(trc_diff_sets *sets, unsigned int id, const char *tag)
{
    trc_diff_set *p;
    tqe_string   *e;

    if (sets == NULL || id >= TRC_DIFF_IDS || tag == NULL)
        return TE_EINVAL;

    p = trc_diff_find_set(sets, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    e = TE_ALLOC(sizeof(*e));
    if (e == NULL)
        return TE_ENOMEM;

    /* Discard 'const' qualifier, but take it into account on free */
    e->v = (char *)tag;
    TAILQ_INSERT_TAIL(&p->tags, e, links);

    return 0;
}

/* See the description in trc_diff.h */
te_errno
trc_diff_add_ignore(trc_diff_sets *sets, unsigned int id,
                    const char *ignore)
{
    trc_diff_set *p;
    tqe_string   *e;

    if (sets == NULL || id >= TRC_DIFF_IDS || ignore == NULL)
        return TE_EINVAL;

    p = trc_diff_find_set(sets, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    e = TE_ALLOC(sizeof(*e));
    if (e == NULL)
        return TE_ENOMEM;

    /* Discard 'const' qualifier, but take it into account on free */
    e->v = (char *)ignore;
    TAILQ_INSERT_TAIL(&p->ignore, e, links);

    VERB("New ignroe for ID=%u: '%s'\n", id, ignore);

    return 0;
}

/* See the description in trc_diff.h */
void
trc_diff_free_sets(trc_diff_sets *sets)
{
    trc_diff_set   *p;

    while ((p = TAILQ_FIRST(sets)) != NULL)
    {
        TAILQ_REMOVE(sets, p, links);

        /* Tag names are not duplicated */
        tq_strings_free(&p->tags, NULL);
        free(p->name);
        /* Exclude patterns are not duplicated */
        tq_strings_free(&p->ignore, NULL);

        free(p);
    }
}
