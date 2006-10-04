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
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_queue.h"
#include "logger_api.h"

#include "trc_tags.h"
#include "trc_diff.h"


/* See description in trc_tag.h */
static trc_diff_tags_entry *
trc_diff_find_tags(trc_diff_tags_list *tags, unsigned int id,
                   te_bool create)
{
    trc_diff_tags_entry *p;

    assert(tags != NULL);
    assert(id < TRC_DIFF_IDS);

    for (p = tags->tqh_first;
         p != NULL && p->id != id;
         p = p->links.tqe_next);

    if (p == NULL && create)
    {
        p = calloc(1, sizeof(*p));
        if (p == NULL)
        {
            ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
            return NULL;
        }
        TAILQ_INSERT_TAIL(tags, p, links);

        p->id = id;
        TAILQ_INIT(&p->tags);
    }
 
    return p;
}

/* See description in trc_tag.h */
te_errno
trc_diff_set_name(trc_diff_tags_list *tags, unsigned int id,
                  const char *name)
{
    trc_diff_tags_entry *p;

    if (tags == NULL || id >= TRC_DIFF_IDS || name == NULL)
        return TE_EINVAL;

    p = trc_diff_find_tags(tags, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    free(p->name);
    p->name = strdup(name);
    if (p->name == NULL)
    {
        ERROR("strdup(%s) failed", name);
        return TE_ENOMEM;
    }
 
    return 0;
}

/* See description in trc_tag.h */
te_errno
trc_diff_show_keys(trc_diff_tags_list *tags, unsigned int id)
{
    trc_diff_tags_entry *p;

    if (tags == NULL || id >= TRC_DIFF_IDS)
        return TE_EINVAL;

    p = trc_diff_find_tags(tags, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    p->show_keys = TRUE;
 
    return 0;
}

/* See description in trc_tag.h */
te_errno
trc_diff_add_tag(trc_diff_tags_list *tags, unsigned int id,
                 const char *tag)
{
    trc_diff_tags_entry *p;

    if (tags == NULL || id >= TRC_DIFF_IDS || tag == NULL)
        return TE_EINVAL;

    p = trc_diff_find_tags(tags, id, TRUE);
    if (p == NULL)
        return TE_ENOMEM;

    return trc_add_tag(&p->tags, tag);
}

/* See description in trc_tag.h */
void
trc_diff_free_tags(trc_diff_tags_list *tags)
{
    trc_diff_tags_entry *p;

    while ((p = tags->tqh_first) != NULL)
    {
        TAILQ_REMOVE(tags, p, links);
        free(p->name);
        tq_strings_free(&p->tags, free);
    }
}
