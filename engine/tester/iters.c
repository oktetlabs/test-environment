/** @file
 * @brief Tester Subsystem
 *
 * Test parameters (variables, arguments) iteration.
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

#define TE_LGR_USER     "Iterations"

#if HAVE_CONFIG_H
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

#include "iters.h"


/**
 * Clone a test parameter.
 *
 * @param p     Test parameter to be cloned
 *
 * @return Clone of the test parameter or NULL.
 */
static test_param *
test_param_clone(const test_param *p)
{
    test_param *pc = calloc(1, sizeof(*pc));

    ENTRY("%s=%s, clone=%d, reqs=%p", p->name, p->value, p->clone, p->reqs);

    if (pc == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(*pc));
        EXIT("ENOMEM");
        return NULL;
    }

    pc->name = p->name;
    pc->value = strdup(p->value);
    if (pc->value == NULL)
    {
        ERROR("strdup() failed");
        free(pc);
        EXIT("ENOMEM");
        return NULL;
    }
    pc->clone = p->clone;
    pc->reqs = p->reqs;

    return pc;
}

/**
 * Free a test parameter.
 *
 * @param p     Test parameter to be freed 
 */
static void
test_param_free(test_param *p)
{
    free(p->value);
    free(p);
}

/**
 * Free list of test parameters.
 *
 * @param params    List of test parameters to be freed    
 */
static void 
test_params_free(test_params *params)
{
    test_param *p;

    while ((p = params->tqh_first) != NULL)
    {
        TAILQ_REMOVE(params, p, links);
        test_param_free(p);
    }
}


/* See description in iters.h */
test_param_iteration *
test_param_iteration_new(void)
{
    test_param_iteration *p = calloc(1, sizeof(*p));

    ENTRY();

    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(*p));
        EXIT("ENOMEM");
        return NULL;
    }
    
    TAILQ_INIT(&p->params);

    return p;
}


/* See description in iters.h */
test_param_iteration *
test_param_iteration_clone(const test_param_iteration *i)
{
    test_param_iteration *ic = test_param_iteration_new();
    test_param           *p;
    test_param           *pc;

    ENTRY("0x%x", (size_t)i);

    if (ic == NULL)
    {
        EXIT("ENOMEM");
        return NULL;
    }
    for (p = i->params.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->clone)
        {
            pc = test_param_clone(p);
            if (pc == NULL)
            {
                EXIT("ENOMEM");
                return NULL;
            }
            TAILQ_INSERT_TAIL(&ic->params, pc, links);
        }
    }
    ic->base = (i->base) ? : i;
    EXIT("OK 0x%x", (size_t)ic);
    return ic;
}

/* See description in iters.h */
void
test_param_iteration_free(test_param_iteration *p)
{
    test_params_free(&p->params);
    free(p);
}

/**
 * Free list of test parameters iterations.
 *
 * @param iters     List of test parameters iterations to be freed    
 */
void 
test_param_iterations_free(test_param_iterations *iters)
{
    test_param_iteration *p;

    while ((p = iters->tqh_first) != NULL)
    {
        TAILQ_REMOVE(iters, p, links);
        test_param_iteration_free(p);
    }
}

#if 0
/* See description in iters.h */
int
test_param_iterations_mult(test_param_iterations *a,
                           const test_param_iterations *b)
{
    test_param_iteration       *p;
    const test_param_iteration *q;

    /* Multiplication by zero is not supported (required) yet */
    assert(b->tqh_first != NULL);

    for (p = a->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        for (q = b->tqh_first; q != NULL; q = q->links.tqe_next)
        {
        }
    }
    return 0;
}
#endif
