/** @file
 * @brief Tester Subsystem
 *
 * Requirements management and usage.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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
#define TE_LGR_USER     "Requirements"

#include "te_config.h"
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

#include "tester_conf.h"
#include "tester_reqs.h"
#include "tester_run.h"

#if 0
#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL (TE_LL_WARN | TE_LL_ERROR | \
                      TE_LL_VERB | TE_LL_ENTRY_EXIT | TE_LL_RING)
#endif

/* See description in tester_reqs.h */
te_errno
tester_new_target_reqs(logic_expr **targets, const char *req)
{
    te_errno    rc;
    logic_expr *parent;
    logic_expr *parsed;

    if (targets == NULL || req == NULL)
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    rc = logic_expr_parse(req, &parsed);
    if (rc != 0)
    {
        ERROR("Failed to parse requirements expression '%s'", req);
        return rc;
    }

    if (*targets == NULL)
    {
        *targets = parsed;
    }
    else
    {
        parent = calloc(1, sizeof(*parent));
        if (parent == NULL)
        {
            rc = TE_OS_RC(TE_TESTER, errno);;
            logic_expr_free(parsed);
            ERROR("%s(): calloc(1, %u) failed",
                  __FUNCTION__, sizeof(*parent));
            return rc;
        } 

        parent->type = LOGIC_EXPR_AND;
        parent->u.binary.lhv = *targets;
        parent->u.binary.rhv = parsed;
        *targets = parent;
    }

    return 0;
}


/**
 * Clone a requirement. Reffered requirements are not supported.
 *
 * @param req             Requirement to be cloned
 *
 * @return Pointer to allocated requirement clone or NULL.
 */
static test_requirement *
test_requirement_clone(const test_requirement *req)
{
    test_requirement *p = calloc(1, sizeof(*p));
    
    assert(req->ref == NULL);
    if (p != NULL)
    {
        p->id = strdup(req->id);
        if (p->id != NULL)
        {
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
 * @param req           Requirement to be freed
 */
static void
test_requirement_free(test_requirement *req)
{
    free(req->id);
    free(req->ref);
    free(req);
}


/* See description in tester_reqs.h */
te_errno
test_requirements_clone(const test_requirements *reqs,
                        test_requirements *new_reqs)
{
    test_requirement  *p;
    test_requirement  *q;

    TAILQ_FOREACH(p, reqs, links)
    {
        q = test_requirement_clone(p);
        if (q == NULL)
        {
            ERROR("%s(): test_requirement_clone() failed", __FUNCTION__);
            return TE_RC(TE_TESTER, TE_ENOMEM);
        }
        TAILQ_INSERT_TAIL(new_reqs, q, links);
    }
    return 0;
}

/* See description in tester_reqs.h */
void
test_requirements_free(test_requirements *reqs)
{
    test_requirement  *p;

    while ((p = TAILQ_FIRST(reqs)) != NULL)
    {
        TAILQ_REMOVE(reqs, p, links);
        test_requirement_free(p);
    }
}


/**
 * Get requirement identifier in specified context of parameters.
 *
 * @param req           A requirement
 * @param n_args        Number of arguments
 * @param args          Test iteration arguments
 *
 * @return Requirement ID.
 */
static const char *
req_get(const test_requirement *req,
        unsigned int n_args, const test_iter_arg *args)
{
    assert(req != NULL);
    assert((args == NULL) == (n_args == 0));

    if (req->id != NULL)
    {
        return req->id;
    }

    assert(req->ref != NULL);
    for (; n_args-- > 0; ++args)
    {
        if (strcmp(args->name, req->ref) == 0)
            return args->value;
    }
    return "";
}

/**
 * Has the set requirement with specified ID?
 *
 * @param req           Requirement ID
 * @param set           Set of requirements
 * @param n_args        Number of arguments
 * @param args          Test iteration arguments
 */
static te_bool
is_req_in_set(const char *req, const test_requirements *set,
              const unsigned int n_args, const test_iter_arg *args)
{
    const test_requirement *s;

    assert(req != NULL);

    for (s = (set != NULL) ? TAILQ_FIRST(set) : NULL;
         s != NULL;
         s = TAILQ_NEXT(s, links))
    {
        if (strcmp(req, req_get(s, n_args, args)) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Has one of arguments specified requirments in its set?
 *
 * @param req           Requirement ID
 * @param n_args        Number of arguments
 * @param args          Test iteration arguments
 */
static te_bool
is_req_in_args(const char *req, const unsigned int n_args,
               const test_iter_arg *args)
{
    const test_iter_arg   *p;
    unsigned int            i;

    for (p = args, i = 0; i < n_args; ++i, ++p)
    {
        if (p->variable)
            continue;

        if (is_req_in_set(req, &p->reqs, n_args, args))
            return TRUE;
    }
    return FALSE;
}

/**
 * Is union of context, test and parameters requirements match logical
 * expression of requirements?
 *
 * @param re            Logical expression of requirements
 * @param ctx_set       Context set of requirements
 * @param test_set      Test set of requirements
 * @param n_args        Number of arguments
 * @param args          Test iteration arguments
 * @param force         Is the force verdict or weak?
 */
static te_bool
is_reqs_expr_match(const logic_expr        *re,
                   const test_requirements *ctx_set,
                   const test_requirements *test_set,
                   const unsigned int       n_args,
                   const test_iter_arg     *args,
                   te_bool                 *force)
{
    te_bool result;

    assert(re != NULL);
    switch (re->type)
    {
        case LOGIC_EXPR_VALUE:
            result = is_req_in_set(re->u.value, ctx_set, n_args, args) ||
                     is_req_in_set(re->u.value, test_set, n_args, args) ||
                     is_req_in_args(re->u.value, n_args, args);
            VERB("%s(): %s -> %u(%u)", __FUNCTION__, re->u.value,
                 result, *force);
            break;

        case LOGIC_EXPR_NOT:
            result = !is_reqs_expr_match(re->u.unary,
                                         ctx_set, test_set, n_args, args,
                                         force);
            if (!result)
                *force = TRUE;
            VERB("%s(): ! -> %u(%u)", __FUNCTION__, result, *force);
            break;

        case LOGIC_EXPR_AND:
        {
            te_bool lhr = is_reqs_expr_match(re->u.binary.lhv,
                                             ctx_set, test_set,
                                             n_args, args, force);
            te_bool rhr = (lhr || !*force) ?
                              is_reqs_expr_match(re->u.binary.rhv,
                                                 ctx_set, test_set,
                                                 n_args, args,
                                                 force) : FALSE;

            result = lhr && rhr;
            VERB("%s(): && -> %u(%u)", __FUNCTION__, result, *force);
            break;
        }

        case LOGIC_EXPR_OR:
            result = is_reqs_expr_match(re->u.binary.lhv,
                                        ctx_set, test_set,
                                        n_args, args, force) ||
                     is_reqs_expr_match(re->u.binary.rhv,
                                        ctx_set, test_set,
                                        n_args, args, force);
            VERB("%s(): || -> %u(%u)", __FUNCTION__, result, *force);
            break;

        default:
            ERROR("Invalid type of requirements expression");
            assert(FALSE);
            result = FALSE;
            *force = TRUE;
            break;
    }
    return result;
}


/**
 * Print requirements expression to buffer provided by caller.
 *
 * @param expr      Requirements expression
 * @param buf       Location of the pointer to the buffer
 * @param left      Location of the space left in the buffer
 * @param prev_op   Previous operation
 */
static void
reqs_expr_to_string_buf(const logic_expr *expr, char **buf, ssize_t *left,
                        logic_expr_type prev_op)
{
    int     out;
    te_bool enclose;

    switch (expr->type)
    {
        case LOGIC_EXPR_VALUE:
            out = snprintf(*buf, *left, "%s", expr->u.value);
            *buf += out; *left -= out;
            break;

        case LOGIC_EXPR_NOT:
            out = snprintf(*buf, *left, "!");
            *buf += out; *left -= out;
            reqs_expr_to_string_buf(expr->u.unary, buf, left,
                                    LOGIC_EXPR_NOT);
            break;

        case LOGIC_EXPR_AND:
            enclose = (prev_op == LOGIC_EXPR_NOT);
            if (enclose)
            {
                out = snprintf(*buf, *left, "(");
                *buf += out; *left -= out;
            }
            reqs_expr_to_string_buf(expr->u.binary.lhv, buf, left,
                                    LOGIC_EXPR_AND);
            out = snprintf(*buf, *left, " & ");
            *buf += out; *left -= out;
            reqs_expr_to_string_buf(expr->u.binary.rhv, buf, left,
                                    LOGIC_EXPR_AND);
            if (enclose)
            {
                out = snprintf(*buf, *left, ")");
                *buf += out; *left -= out;
            }
            break;

        case LOGIC_EXPR_OR:
            enclose = (prev_op == LOGIC_EXPR_NOT ||
                       prev_op == LOGIC_EXPR_AND);
            if (enclose)
            {
                out = snprintf(*buf, *left, "(");
                *buf += out; *left -= out;
            }
            reqs_expr_to_string_buf(expr->u.binary.lhv, buf, left,
                                    LOGIC_EXPR_OR);
            out = snprintf(*buf, *left, " | ");
            *buf += out; *left -= out;
            reqs_expr_to_string_buf(expr->u.binary.rhv, buf, left,
                                    LOGIC_EXPR_OR);
            if (enclose)
            {
                out = snprintf(*buf, *left, ")");
                *buf += out; *left -= out;
            }
            break;

        default:
            ERROR("Invalid type of requirements expression");
            assert(FALSE);
            break;
    }
}

/* See the description in tester_reqs.h */
const char *
tester_reqs_expr_to_string(const logic_expr *expr)
{
    static char buf[0x10000];

    char       *s = buf;
    ssize_t     left = sizeof(buf);

    s[0] = '\0';
    reqs_expr_to_string_buf(expr, &s, &left, LOGIC_EXPR_VALUE);

    return buf;
}


/**
 * Print list of requirements into string.
 *
 * @param reqs          List of requirements
 * @param n_args        Number of arguments
 * @param args          Test iteration arguments to get indirect
 *                      requirements ID
 * @param buf           Location of the pointer to the buffer
 * @param left          Location of the space left in the buffer
 */
static void
reqs_list_to_string_buf(const test_requirements  *reqs,
                        const unsigned int        n_args,
                        const test_iter_arg      *args,
                        char                    **buf,
                        ssize_t                  *left)
{
    const test_requirement *p; 
    char                   *s = *buf;
    int                     out;

    for (p = (reqs != NULL) ? TAILQ_FIRST(reqs) : NULL;
         p != NULL && *left > 0;
         p = TAILQ_NEXT(p, links), s += out, *left -= out)
    {
        out = snprintf(s, *left, "%s%s",
                       (p == TAILQ_FIRST(reqs)) ? "" : ", ",
                       req_get(p, n_args, args));
    }
    *buf = s;
}

/**
 * Print list of requirements into string.
 *
 * @param reqs          List of requirements
 * @param n_args        Number of arguments
 * @param args          Test iteration arguments to get indirect
 *                      requirements ID
 *
 * @return Pointer to one of few static buffers with printed
 *         requirements
 */
static const char *
reqs_list_to_string(const test_requirements *reqs,
                    const unsigned int       n_args,
                    const test_iter_arg     *args)
{
    static char         bufs[4][1024];
    static unsigned int index = 0;

    char       *out_buf = bufs[index++ & 0x3];
    char       *s = out_buf;
    ssize_t     left = sizeof(bufs[0]);

    out_buf[0] = '\0';
    reqs_list_to_string_buf(reqs, n_args, args, &s, &left);

    return out_buf;
}

/**
 * Print requirements attached to each parameter of the test iteration.
 *
 * @param n_args        Number of arguments
 * @param args          Test iteration arguments
 *
 * @return Pointer to the static buffer with printed requirements per
 *         test parameter. Parameter is skipped, if it has no attached
 *         requirements.
 */
static const char *
params_reqs_list_to_string(const unsigned int   n_args,
                           const test_iter_arg *args)
{
    static char out_buf[1024];

    const test_iter_arg   *p;
    unsigned int            i;
    char                   *s;
    ssize_t                 left; 
    int                     out;


    out_buf[0] = '\0';
    for (p = args, s = out_buf, left = sizeof(out_buf), i = 0;
         i < n_args && left > 0;
         ++p, ++i)
    {
        if (!TAILQ_EMPTY(&p->reqs))
        {
            out = snprintf(s, left, " %s=", p->name);
            s += out;
            left -= out;
            reqs_list_to_string_buf(&p->reqs, n_args, args, &s, &left);
        }
    }

    return out_buf;
}


/* See description in tester_reqs.h */
te_bool
tester_is_run_required(const logic_expr        *targets,
                       const test_requirements *sticky_reqs,
                       const run_item          *test,
                       const test_iter_arg     *args,
                       unsigned int             flags,
                       te_bool                  quiet)
{
    te_bool                     result;
    te_bool                     force = FALSE;
    const test_requirements    *reqs;

    switch (test->type)
    {
        case RUN_ITEM_SCRIPT:
            reqs = &test->u.script.reqs;
            break;

        case RUN_ITEM_SESSION:
            reqs = &test->u.session.reqs;
            break;

        case RUN_ITEM_PACKAGE:
            reqs = &test->u.package->reqs;
            break;

        default:
            assert(FALSE);
            return FALSE;
    }

    if (targets != NULL)
    {
        result = is_reqs_expr_match(targets, sticky_reqs, reqs,
                                    test->n_args, args, &force);
        if (!force)
            result = result || (test->type != RUN_ITEM_SCRIPT) ||
                     !!(flags & TESTER_INLOGUE);
        if (!result && !quiet)
        {
            RING("Skipped because of expression: %s\n"
                 "Collected sticky requirements: %s\n"
                 "Test node requirements: %s\n"
                 "Requirements attached to parameters:%s\n",
                 tester_reqs_expr_to_string(targets),
                 reqs_list_to_string(sticky_reqs, test->n_args, args),
                 reqs_list_to_string(reqs, test->n_args, args),
                 params_reqs_list_to_string(test->n_args, args));
        }
    }
    else
    {
        result = TRUE;
    }

    return result;
}


/* See description in tester_reqs.h */
te_errno
tester_get_sticky_reqs(test_requirements       *sticky_reqs,
                       const test_requirements *reqs)
{
    const test_requirement *p;
    test_requirement       *q;

    TAILQ_FOREACH(p, reqs, links)
    {
        if (p->sticky)
        {
            q = test_requirement_clone(p);
            if (q == NULL)
                return TE_RC(TE_TESTER, TE_ENOMEM);
            TAILQ_INSERT_TAIL(sticky_reqs, q, links);
        }
    }

    return 0;
}
