/** @file
 * @brief Tester Subsystem
 *
 * Requirements management and usage.
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

#define TE_LGR_USER     "Requirements"

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


/* See description in reqs.h */
int
test_requirement_new(test_requirements *reqs, const char *req)
{
    test_requirement *p;

    if (req == NULL)
    {
        ERROR("NULL pointer requirement name");
        return EINVAL;
    }
    p = calloc(1, sizeof(*p));
    if (p != NULL)
    {
        /* Excluded requirements are started from !, skip it */
        p->exclude = (req[0] == '!');
        if (p->exclude)
            req++;
        if (strlen(req) == 0)
        {
            ERROR("Empty requirement ID");
            free(p);
            return EINVAL;
        }
        p->id = strdup(req);
        if (p->id != NULL)
        {
            TAILQ_INSERT_TAIL(reqs, p, links);
            return 0;
        }
        free(p);
    }
    ERROR("%s(): Memory allocation failure", __FUNCTION__);   
    return ENOMEM;
}


/**
 * Clone a requirement. Reffered requirements are not supported.
 *
 * @param r     Requirement to be cloned
 *
 * @return Pointer to allocated requirement clone or NULL.
 */
static test_requirement *
test_requirement_clone(const test_requirement *r)
{
    test_requirement *p = calloc(1, sizeof(*p));
    
    assert(r->ref == NULL);
    if (p != NULL)
    {
        p->id = strdup(r->id);
        if (p->id != NULL)
        {
            p->exclude = r->exclude;
            return p;
        }
        free(p);
    }
    ERROR("%s(): Memory allocation failed", __FUNCTION__);
    return NULL;
}

/**
 * Free requirement.
 *
 * @param req       Requirement to be freed
 */
static void
test_requirement_free(test_requirement *req)
{
    free(req->id);
    free(req->ref);
    free(req);
}


/* See description in reqs.h */
int
test_requirements_clone(const test_requirements *reqs,
                      test_requirements *new_reqs)
{
    test_requirement  *p;
    test_requirement  *q;

    for (p = reqs->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        q = test_requirement_clone(p);
        if (q == NULL)
        {
            ERROR("%s(): test_requirement_clone() failed", __FUNCTION__);
            return ENOMEM;
        }
        TAILQ_INSERT_TAIL(new_reqs, q, links);
    }
    return 0;
}

/* See description in reqs.h */
void
test_requirements_free(test_requirements *reqs)
{
    test_requirement  *p;

    while ((p = reqs->tqh_first) != NULL)
    {
        TAILQ_REMOVE(reqs, p, links);
        test_requirement_free(p);
    }
}


/**
 * Get requirement identifier in specified context of parameters.
 *
 * @param r         A requirement
 * @param params    Parameters contex
 *
 * @return Requirement ID.
 */
static const char *
req_get(const test_requirement *r, const test_params *params)
{
    const test_param *p;

    if (r->id != NULL)
    {
        return r->id;
    }

    assert(r->ref != NULL);
    for (p = params->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (strcmp(p->name, r->ref) == 0)
            return (p->req) ? : p->value;
    }
    return "";
}

/* See description in reqs.h */
te_bool
tester_is_run_required(tester_ctx *ctx, const run_item *test,
                       const test_params *params)
{
    te_bool                     result = TRUE;
    test_requirements          *targets = &ctx->reqs;
    const test_requirements    *reqs;
    test_requirement           *t, *tn;
    const test_requirement     *s;

    switch (test->type)
    {
        case RUN_ITEM_SCRIPT:
            reqs = &test->u.script.reqs;
            break;

        case RUN_ITEM_SESSION:
            return TRUE;

        case RUN_ITEM_PACKAGE:
            reqs = &test->u.package->reqs;
            break;

        default:
            assert(FALSE);
            return FALSE;
    }

    for (t = targets->tqh_first; t != NULL; t = tn)
    {
        tn = t->links.tqe_next;

        assert(t->id != NULL);
        assert(!t->sticky);

        for (s = reqs->tqh_first; s != NULL; s = s->links.tqe_next)
        {
            assert(s->exclude == FALSE);
            assert((test->type != RUN_ITEM_SCRIPT) || !(s->sticky));

            /* 
             * It's NOT exclude target requirement and list of non-sticky
             * test requirementsis not empty, therefore, one of
             * requirements must be complied.
             */
            if (!(t->exclude) && !(s->sticky))
                result = FALSE;

            if (strcmp(t->id, req_get(s, params)) == 0)
            {
                if (t->exclude)
                {
                    if (~ctx->flags & TESTER_QUIET_SKIP)
                        WARN("Excluded because of requirement '%s'", t->id);
                    return FALSE;
                }
                else
                {
                    result = TRUE;
                    if (s->sticky)
                    {
                        /* Remove this target requirement from the list */
                        TAILQ_REMOVE(targets, t, links);
                    }
                    break;
                }
            }
        }
        if (result == FALSE)
        {
            if (~ctx->flags & TESTER_QUIET_SKIP)
                WARN("No matching requirement for '%s' found", t->id);
            return FALSE;
        }
    }

    return result;
}

