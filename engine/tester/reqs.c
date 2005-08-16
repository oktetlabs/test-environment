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

/** Logging user name to be used here */
#define TE_LGR_USER     "Requirements"

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

#include "internal.h"
#include "reqs.h"


/* See description in reqs.h */
int
tester_new_target_reqs(reqs_expr **targets, const char *req)
{
    int        rc;
    reqs_expr *parent;
    reqs_expr *parsed;

    if (targets == NULL || req == NULL)
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    rc = tester_reqs_expr_parse(req, &parsed);
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
            tester_reqs_expr_free(parsed);
            ERROR("%s(): calloc(1, %u) failed",
                  __FUNCTION__, sizeof(*parent));
            return rc;
        } 

        parent->type = TESTER_REQS_EXPR_AND;
        parent->u.binary.lhv = *targets;
        parent->u.binary.rhv = parsed;
        *targets = parent;
    }

    return 0;
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
            return TE_RC(TE_TESTER, TE_ENOMEM);
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


/* See description in reqs.h */
reqs_expr *
reqs_expr_binary(reqs_expr_type type, reqs_expr *lhv, reqs_expr *rhv)
{
    reqs_expr *p;

    assert(type == TESTER_REQS_EXPR_AND ||
           type == TESTER_REQS_EXPR_OR);
    assert(lhv != NULL);
    assert(rhv != NULL);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("%s(): calloc(1, %u) failed", __FUNCTION__, sizeof(*p));
        return NULL;
    }
    p->type = type;
    p->u.binary.lhv = lhv;
    p->u.binary.rhv = rhv;
    
    return p;
}


/* See description in reqs.h */
void
tester_reqs_expr_free_nr(reqs_expr *p)
{
    free(p);
}

/* See description in reqs.h */
void
tester_reqs_expr_free(reqs_expr *p)
{
    if (p == NULL)
        return;

    switch (p->type)
    {
        case TESTER_REQS_EXPR_VALUE:
            free(p->u.value);
            break;

        case TESTER_REQS_EXPR_NOT:
            tester_reqs_expr_free(p->u.unary);
            break;

        case TESTER_REQS_EXPR_AND:
        case TESTER_REQS_EXPR_OR:
            tester_reqs_expr_free(p->u.binary.lhv);
            tester_reqs_expr_free(p->u.binary.rhv);
            break;

        default:
            ERROR("Invalid type of requirements expression");
            assert(FALSE);
            break;
    }
    tester_reqs_expr_free_nr(p);
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

    assert(r != NULL);
    assert(params != NULL);

    if (r->id != NULL)
    {
        return r->id;
    }

    assert(r->ref != NULL);
    for (p = params->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (strcmp(p->name, r->ref) == 0)
            return p->value;
    }
    return "";
}

/**
 * Has the set requirement with specified ID?
 *
 * @param req       Requirement ID
 * @param set       Set of requirements
 * @param params    Current parameters to interpret references
 */
static te_bool
is_req_in_set(const char *req, const test_requirements *set,
              const test_params *params)
{
    const test_requirement *s;

    assert(req != NULL);

    for (s = (set != NULL) ? set->tqh_first : NULL;
         s != NULL;
         s = s->links.tqe_next)
    {
        if (strcmp(req, req_get(s, params)) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Has one of parameters specified requirments in its set?
 *
 * @param req       Requirement ID
 * @param params    Current parameters to interpret references
 */
static te_bool
is_req_in_params(const char *req, const test_params *params)
{
    const test_param   *p;

    assert(params != NULL);
    for (p = params->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (is_req_in_set(req, p->reqs, params))
            return TRUE;
    }
    return FALSE;
}

/**
 * Is union of context, test and parameters requirements match logical
 * expression of requirements?
 *
 * @param re        Logical expression of requirements
 * @param ctx_set   Context set of requirements
 * @param test_set  Test set of requirements
 * @param params    Current parameters
 * @param force     Is the force verdict or weak?
 */
static te_bool
is_reqs_expr_match(const reqs_expr         *re,
                   const test_requirements *ctx_set,
                   const test_requirements *test_set,
                   const test_params       *params,
                   te_bool                 *force)
{
    te_bool result;

    assert(re != NULL);
    switch (re->type)
    {
        case TESTER_REQS_EXPR_VALUE:
            result = is_req_in_set(re->u.value, ctx_set, params) ||
                     is_req_in_set(re->u.value, test_set, params) ||
                     is_req_in_params(re->u.value, params);
            VERB("%s(): %s -> %u(%u)", __FUNCTION__, re->u.value,
                 result, *force);
            break;

        case TESTER_REQS_EXPR_NOT:
            result = !is_reqs_expr_match(re->u.unary,
                                         ctx_set, test_set, params,
                                         force);
            if (!result)
                *force = TRUE;
            VERB("%s(): ! -> %u(%u)", __FUNCTION__, result, *force);
            break;

        case TESTER_REQS_EXPR_AND:
        {
            te_bool lhr = is_reqs_expr_match(re->u.binary.lhv,
                                             ctx_set, test_set, params,
                                             force);
            te_bool rhr = (lhr || !*force) ?
                              is_reqs_expr_match(re->u.binary.rhv,
                                                 ctx_set, test_set, params,
                                                 force) : FALSE;

            result = lhr && rhr;
            VERB("%s(): && -> %u(%u)", __FUNCTION__, result, *force);
            break;
        }

        case TESTER_REQS_EXPR_OR:
            result = is_reqs_expr_match(re->u.binary.lhv,
                                        ctx_set, test_set, params,
                                        force) ||
                     is_reqs_expr_match(re->u.binary.rhv,
                                        ctx_set, test_set, params,
                                        force);
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
reqs_expr_to_string_buf(const reqs_expr *expr, char **buf, ssize_t *left,
                        reqs_expr_type prev_op)
{
    int     out;
    te_bool enclose;

    switch (expr->type)
    {
        case TESTER_REQS_EXPR_VALUE:
            out = snprintf(*buf, *left, "%s", expr->u.value);
            *buf += out; *left -= out;
            break;

        case TESTER_REQS_EXPR_NOT:
            out = snprintf(*buf, *left, "!");
            *buf += out; *left -= out;
            reqs_expr_to_string_buf(expr->u.unary, buf, left,
                                    TESTER_REQS_EXPR_NOT);
            break;

        case TESTER_REQS_EXPR_AND:
            enclose = (prev_op == TESTER_REQS_EXPR_NOT);
            if (enclose)
            {
                out = snprintf(*buf, *left, "(");
                *buf += out; *left -= out;
            }
            reqs_expr_to_string_buf(expr->u.binary.lhv, buf, left,
                                    TESTER_REQS_EXPR_AND);
            out = snprintf(*buf, *left, " & ");
            *buf += out; *left -= out;
            reqs_expr_to_string_buf(expr->u.binary.rhv, buf, left,
                                    TESTER_REQS_EXPR_AND);
            if (enclose)
            {
                out = snprintf(*buf, *left, ")");
                *buf += out; *left -= out;
            }
            break;

        case TESTER_REQS_EXPR_OR:
            enclose = (prev_op == TESTER_REQS_EXPR_NOT ||
                       prev_op == TESTER_REQS_EXPR_AND);
            if (enclose)
            {
                out = snprintf(*buf, *left, "(");
                *buf += out; *left -= out;
            }
            reqs_expr_to_string_buf(expr->u.binary.lhv, buf, left,
                                    TESTER_REQS_EXPR_OR);
            out = snprintf(*buf, *left, " | ");
            *buf += out; *left -= out;
            reqs_expr_to_string_buf(expr->u.binary.rhv, buf, left,
                                    TESTER_REQS_EXPR_OR);
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

/**
 * Print requirements expression to static buffer.
 *
 * @param expr      Requirements expression
 *
 * @return Pointer to the static buffer with printed expression
 */
static const char *
reqs_expr_to_string(const reqs_expr *expr)
{
    static char buf[1024];

    char       *s = buf;
    ssize_t     left = sizeof(buf);

    s[0] = '\0';
    reqs_expr_to_string_buf(expr, &s, &left, TESTER_REQS_EXPR_VALUE);

    return buf;
}


/**
 * Print list of requirements into string.
 *
 * @param reqs      List of requirements
 * @param params    Parameters context to get referred requirements ID
 * @param buf       Location of the pointer to the buffer
 * @param left      Location of the space left in the buffer
 */
static void
reqs_list_to_string_buf(const test_requirements  *reqs,
                        const test_params        *params,
                        char                    **buf,
                        ssize_t                  *left)
{
    const test_requirement *p; 
    char                   *s = *buf;
    int                     out;

    for (p = (reqs != NULL) ? reqs->tqh_first : NULL;
         p != NULL && *left > 0;
         p = p->links.tqe_next, s += out, *left -= out)
    {
        out = snprintf(s, *left, "%s%s",
                       (p == reqs->tqh_first) ? "" : ", ",
                       req_get(p, params));
    }
    *buf = s;
}

/**
 * Print list of requirements into string.
 *
 * @param reqs      List of requirements
 * @param params    Parameters context to get referred requirements ID
 *
 * @return Pointer to one of few static buffers with printed
 *         requirements
 */
static const char *
reqs_list_to_string(const test_requirements *reqs,
                    const test_params       *params)
{
    static char         bufs[4][1024];
    static unsigned int index = 0;

    char       *out_buf = bufs[index++ & 0x3];
    char       *s = out_buf;
    ssize_t     left = sizeof(bufs[0]);

    out_buf[0] = '\0';
    reqs_list_to_string_buf(reqs, params, &s, &left);

    return out_buf;
}

/**
 * Print requirements attached to each parameter in the list.
 *
 * @param params        List of test parameters
 *
 * @return Pointer to the static buffer with printed requirements per
 *         test parameter. Parameter is skipped, if it has no attached
 *         requirements.
 */
static const char *
params_reqs_list_to_string(const test_params *params)
{
    static char out_buf[1024];

    const test_param   *p;
    char               *s;
    ssize_t             left; 
    int                 out;

    out_buf[0] = '\0';
    for (p = params->tqh_first, s = out_buf, left = sizeof(out_buf);
         p != NULL && left > 0;
         p = p->links.tqe_next)
    {
        if (p->reqs != NULL)
        {
            out = snprintf(s, left, " %s=", p->name);
            s += out;
            left -= out;
            reqs_list_to_string_buf(p->reqs, params, &s, &left);
        }
    }

    return out_buf;
}


/* See description in reqs.h */
te_bool
tester_is_run_required(const tester_ctx *ctx, const run_item *test,
                       const test_params *params, te_bool quiet)
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
            reqs = NULL;
            break;

        case RUN_ITEM_PACKAGE:
            reqs = &test->u.package->reqs;
            break;

        default:
            assert(FALSE);
            return FALSE;
    }

    if (ctx->targets != NULL)
    {
        result = is_reqs_expr_match(ctx->targets, &ctx->reqs, reqs,
                                    params, &force);
        if (!force)
            result = result || (test->type != RUN_ITEM_SCRIPT);
        if (!result && !quiet)
        {
            RING("Skipped because of expression: %s\n"
                 "Collected sticky requirements: %s\n"
                 "Test node requirements: %s\n"
                 "Requirements attached to parameters:%s\n",
                 reqs_expr_to_string(ctx->targets),
                 reqs_list_to_string(&ctx->reqs, params),
                 reqs_list_to_string(reqs, params),
                 params_reqs_list_to_string(params));
        }
    }
    else
    {
        result = TRUE;
    }

    return result;
}

/* See description in reqs.h */
int
tester_ctx_get_sticky_reqs(struct tester_ctx       *ctx,
                           const test_requirements *reqs)
{
    const test_requirement *p;
    test_requirement       *q;

    for (p = reqs->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->sticky)
        {
            q = test_requirement_clone(p);
            if (q == NULL)
                return TE_RC(TE_TESTER, TE_ENOMEM);
            TAILQ_INSERT_TAIL(&ctx->reqs, q, links);
        }
    }

    return 0;
}
