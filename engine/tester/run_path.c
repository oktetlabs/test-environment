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

#define TE_LGR_USER     "Run Path"

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


/**
 * Get 'name' token from run path.
 *
 * @param path      Run path as string
 * @param params    Whether parameters present?
 *
 * @return '\0'-terminated string
 */
static char *
run_path_name_token(char **path, te_bool *params)
{
    char   *s;
    size_t  l;

    assert(path != NULL);
    assert(*path != NULL);

    /* Start of the string with initial path */
    s = *path;
    /* Try to find one of possible separators */
    l = strcspn(s, "/:");
    /* If found separator is open paranthesis, parameters follow */
    *params = (s[l] == ':');
    /* Move path forward */
    *path = s + l + (s[l] != '\0');
    /* Terminate token with '\0' (possibly just overwrite '\0') */
    s[l] = '\0';

    return s;
}

/**
 * Get 'name' of the parameter token from run path.
 *
 * @param path      Run path as string
 *
 * @return '\0'-terminated string
 */
static char *
run_path_param_name(char **path)
{
    char   *s;
    size_t  l;

    assert(path != NULL);
    assert(*path != NULL);

    /* Start of the string with initial path */
    s = *path;
    /* Try to find one of possible separators */
    l = strcspn(s, "=");
    /* Move path forward */
    *path = s + l + (s[l] != '\0');
    /* Terminate token with '\0' (possibly just overwrite '\0') */
    s[l] = '\0';

    return s;
}

/**
 * Get 'value' of the parameter token from run path.
 *
 * @param path      Run path as string
 * @param more      Whether more parameters present?
 *
 * @return '\0'-terminated string
 */
static char *
run_path_param_value(char **path, te_bool *more)
{
    char   *s;
    size_t  l;

    assert(path != NULL);
    assert(*path != NULL);

    /* Start of the string with initial path */
    s = *path;
    /* Try to find one of possible separators */
    l = strcspn(s, ",/");
    /* If found separator is open paranthesis, parameters follow */
    *more = (s[l] == ',');
    /* Move path forward */
    *path = s + l + (s[l] != '\0');
    /* Terminate token with '\0' (possibly just overwrite '\0') */
    s[l] = '\0';

    return s;
}

/**
 * Compare test parameters for equality.
 *
 * @param a     One test parameter
 * @param b     Another test parameter
 *
 * @return Are test parameters equal?
 */
static te_bool
test_param_equal(const test_param *a, const test_param *b)
{
    assert(a != NULL);
    assert(b != NULL);
    return (strcmp(a->name, b->name) == 0) &&
           (strcmp(a->value, b->value) == 0);
}

/**
 * Compare sets of test parameters for equality.
 *
 * @param a     One set of parameters
 * @param b     Another set of parameters
 *
 * @return Are sets equal?
 */
static te_bool
test_params_equal(const test_params *a, const test_params *b)
{
    te_bool             found = TRUE;
    unsigned int        i, j;
    unsigned int        max_j = 0;
    const test_param   *p;
    const test_param   *q;

    assert(a != NULL);
    assert(b != NULL);
    for (p = a->tqh_first, i = 0;
         found && p != NULL;
         p = p->links.tqe_next, ++i)
    {
        found = FALSE;
        for (q = b->tqh_first, j = 0;
             q != NULL;
             q = q->links.tqe_next, ++j)
        {
            if (test_param_equal(p, q))
            {
                found = TRUE;
                /* Count all parameters once */
                if (max_j != 0)
                    break;
            }
        }
        max_j = j;
    }
    return found && (i == max_j);
}


/**
 * Compare Tester run path items for equality.
 *
 * @param a     One run path item
 * @param b     Another run path item
 * 
 * @return Are items equal?
 */
static te_bool
run_path_items_equal(const tester_run_path *a, const tester_run_path *b)
{
    assert(a != NULL);
    assert(b != NULL);
    return (strcmp(a->name, b->name) == 0) &&
           test_params_equal(&a->params, &b->params);
}

/* See description in run_path.h */
int
tester_run_path_new(tester_run_path *root, char *path, unsigned int flags)
{
    tester_run_path *p;
    tester_run_path *q;
    te_bool          params;
    test_param      *param;

    assert(root != NULL);

    while ((strlen(path) > 0) && (strcmp(path, "/") != 0))
    {
        VERB("Processing run path '%s'", path);
        p = calloc(1, sizeof(*p));
        if (p == NULL)
        {
            ERROR("%s(): Memory allocation failure", __FUNCTION__);
            return ENOMEM;
        }

        TAILQ_INIT(&p->params);
        TAILQ_INIT(&p->paths);
        
        p->name = run_path_name_token(&path, &params);
        VERB("%s(): Name got '%s', rest '%s'", __FUNCTION__, p->name, path);
        while (params)
        {
            assert(strlen(path) > 0);
            
            param = calloc(1, sizeof(*param));
            if (param == NULL)
            {
                ERROR("%s(): Memory allocation failure", __FUNCTION__);
                return ENOMEM;
            }

            param->name = run_path_param_name(&path);
            if (strlen(path) == 0)
            {
                ERROR("No value for parameter '%s' on step '%s' specified",
                      param->name, p->name);
                return EINVAL;
            }
            param->value = run_path_param_value(&path, &params);

            TAILQ_INSERT_TAIL(&p->params, param, links);
        }

        /* Try to find equal on this path step */
        for (q = root->paths.tqh_first;
             q != NULL && !run_path_items_equal(p, q);
             q = q->links.tqe_next);

        if (q != NULL)
        {
            /* Merge flags */
            q->flags |= p->flags;
            /* Found */
            tester_run_path_free(p);
            /* Follow the previously added */
            p = q;
        }
        else
        {
            VERB("New run path node '%s'", p->name);
            /* Not found */
            TAILQ_INSERT_TAIL(&root->paths, p, links);
        }
        if (flags & TESTER_RUN)
        {
            /* Trace run path with corresponding flag */
            p->flags |= TESTER_RUNPATH;
        }
        root = p;
    }
    /* Terminate the last run item with specified flags */
    root->flags |= flags;
    
    return 0;
}


/* See description in run_path.h */
void
tester_run_path_free(tester_run_path *path)
{
    test_param *p;

    /* 'name' was not allocated */

    while ((p = path->params.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&path->params, p, links);
        /* 'name' and 'value' was not allocated */
        free(p);
    }
    tester_run_paths_free(&path->paths);
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
tester_run_path_forward(tester_ctx *ctx, const char *name)
{
    tester_run_path  *p;
    te_bool           run_path = FALSE;

    assert(ctx != NULL);
    assert(name != NULL);

    /* Check all children of the current run path node */
    for (p = ctx->path->paths.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        run_path = (run_path || (p->flags & TESTER_RUNPATH));
        if (strcmp(name, p->name) == 0)
        {
            ctx->flags |= p->flags;
            ctx->path = p;
            return 0;
        }
    }

    return (run_path) ? ENOENT : 0;
}

/* See description in run_path.h */
int
tester_run_path_params_match(struct tester_ctx *ctx, test_params *params)
{
    te_bool             found = TRUE;
    const test_param   *p;
    const test_param   *q;

    for (p = ctx->path->params.tqh_first;
         found && p != NULL;
         p = p->links.tqe_next)
    {
        found = FALSE;
        for (q = params->tqh_first;
             !found && q != NULL;
             q = q->links.tqe_next)
        {
            if (test_param_equal(p, q))
                found = TRUE;
        }
    }
    return found ? 0 : ENOENT;
}
