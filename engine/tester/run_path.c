/** @file
 * @brief Tester Subsystem
 *
 * Run path processing code.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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

#define LGR_USER    "Run Path"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "logger_api.h"

#include "internal.h"


/* See description in run_path.h */
int
tester_run_path_new(tester_run_paths *paths, const char *path)
{
    tester_run_path *p = calloc(1, sizeof(*p));

    if (p != NULL)
    {
        assert(path != NULL);
        p->path = strdup(path);
        if (p->path != NULL)
        {
            p->pos = p->path;
            TAILQ_INSERT_TAIL(paths, p, links);
            return 0;
        }
        free(p);
    }
    ERROR("%s(): Memory allocation failure", __FUNCTION__);   
    return ENOMEM;
}


/* See description in run_path.h */
void
tester_run_path_free(tester_run_path *path)
{
    free(path->path);
    free(path);
}

/* See description in run_path.h */
void
tester_run_paths_free(tester_run_paths *paths)
{
    tester_run_path  *p;

    while ((p = paths->tqh_first) != NULL)
    {
        TAILQ_REMOVE(paths, p, links);
        tester_run_path_free(p);
    }
}


/* See description in run_path.h */
int
tester_run_paths_clone(const tester_run_paths *paths,
                       tester_run_paths *new_paths)
{
    int                     rc;
    const tester_run_path  *p;

    for (p = paths->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        /* Clone from the current path position */
        rc = tester_run_path_new(new_paths, p->pos);
        if (rc != 0)
        {
            ERROR("%s(): tester_run_path_new() failed", __FUNCTION__);
            return rc;
        }
    }
    return 0;
}


/**
 * Move forward on run path.
 * The function updates Tester context on success.
 *
 * @param ctx       Tester context
 * @param name      Name of the node
 *
 * @return Status code.
 * @retval 0        The node is on run path
 * @retval ENOENT   The node is not on run path
 */
int
tester_run_path_forward(tester_ctx *ctx, const char *name)
{
    tester_run_paths *paths = &ctx->paths;
    tester_run_path  *p, *q;
    char             *s;

    assert(name != NULL);
    assert(paths->tqh_first != NULL);

    for (p = paths->tqh_first; p != NULL; p = q)
    {
        q = p->links.tqe_next;
        s = index(p->pos, '/');
        if (s != NULL)
            *s = '\0';
        if (strcmp(name, p->pos) == 0)
        {
            if ((s == NULL) || (strlen(++s) == 0))
            {
                VERB("All tests in '%s' must be run", name);
                /* Path become empty, all nodes below must be run */
                tester_run_paths_free(paths);
                return 0;
            }
            else
                p->pos = s;
        }
        else
        {
            /* Remove all not matching paths */
            TAILQ_REMOVE(paths, p, links);
            tester_run_path_free(p);
        }
    }

    VERB("Item '%s' processed - paths are%s empty", name,
         (paths->tqh_first == NULL) ? "" : " not");
    if (paths->tqh_first != NULL)
        return 0;
    else
        return ENOENT;
}

