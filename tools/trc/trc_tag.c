/** @file
 * @brief Testing Results Comparator
 *
 * Routines to work with TRC tags.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#error sys/queue.h is required for TRC tool
#endif

#include "trc_log.h"
#include "trc_tag.h"


/** List of tags to get specific expected result */
trc_tags tags;

/**
 * List of tags to get specific expected result to compare with the
 * first one.
 */
trc_tags_list tags_diff;


/* See description in trc_tag.h */
int
trc_add_tag(trc_tags *tags, const char *name)
{
    trc_tag *p = calloc(1, sizeof(*p));

    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
        return ENOMEM;
    }
    TAILQ_INSERT_TAIL(tags, p, links);
    p->name = strdup(name);
    if (p->name == NULL)
    {
        ERROR("strdup(%s) failed", name);
        return ENOMEM;
    }
    return 0;
}

/* See description in trc_tag.h */
void
trc_free_tags(trc_tags *tags)
{
    trc_tag *p;

    while ((p = tags->tqh_first) != NULL)
    {
        TAILQ_REMOVE(tags, p, links);
        free(p->name);
        free(p);
    }
}


/* See description in trc_tag.h */
int
trc_diff_set_name(trc_tags_list *tags, unsigned int id, const char *name)
{
    trc_tags_entry *p;

    for (p = tags->tqh_first;
         p != NULL && p->id != id;
         p = p->links.tqe_next);

    if (p == NULL)
    {
        p = calloc(1, sizeof(*p));
        if (p == NULL)
        {
            ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
            return ENOMEM;
        }
        TAILQ_INSERT_TAIL(tags, p, links);

        p->id = id;
        TAILQ_INIT(&p->tags);
    }

    free(p->name);
    p->name = strdup(name);
    if (p->name == NULL)
    {
        ERROR("strdup(%s) failed", name);
        return ENOMEM;
    }
 
    return 0;
}

/* See description in trc_tag.h */
int
trc_diff_add_tag(trc_tags_list *tags, unsigned int id, const char *name)
{
    trc_tags_entry *p;

    for (p = tags->tqh_first;
         p != NULL && p->id != id;
         p = p->links.tqe_next);

    if (p == NULL)
    {
        p = calloc(1, sizeof(*p));
        if (p == NULL)
        {
            ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
            return ENOMEM;
        }
        TAILQ_INSERT_TAIL(tags, p, links);

        p->id = id;
        TAILQ_INIT(&p->tags);
    }

    return trc_add_tag(&p->tags, name);
}

/* See description in trc_tag.h */
void
trc_diff_free_tags(trc_tags_list *tags)
{
    trc_tags_entry *p;

    while ((p = tags->tqh_first) != NULL)
    {
        TAILQ_REMOVE(tags, p, links);
        free(p->name);
        trc_free_tags(&p->tags);
    }
}
