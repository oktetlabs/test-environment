/** @file
 * @brief Tester Subsystem
 *
 * Run path processing code.
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
#define TE_LGR_USER     "Run Path"

#include "te_config.h"
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

#include "te_alloc.h"
#include "logger_api.h"

#include "tester_flags.h"
#include "tester_conf.h"
#include "test_path.h"


/* See the description in test_path.h */
te_errno
test_path_new(test_paths *paths, const char *path_str, test_path_type type)
{
    test_path *path;
    te_errno   rc;

    ENTRY("path_str=%s type=%u", path_str, type);

    path = TE_ALLOC(sizeof(*path));
    if (path == NULL)
        return TE_ENOMEM;

    TAILQ_INIT(&path->head);
    TAILQ_INIT(&path->scen);
    path->type = type;
    path->str = strdup(path_str);
    if (path->str == NULL)
    {
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, path_str);
        return TE_ENOMEM;
    }

    TAILQ_INSERT_TAIL(paths, path, links);

    rc = tester_test_path_parse(path);
    if (rc == 0)
    {
        if (type != TEST_PATH_RUN && type != TEST_PATH_RUN_FORCE)
        {
            test_path_item *item;

            for (item = path->head.tqh_first;
                 item != NULL;
                 item = item->links.tqe_next)
            {
                if (item->iterate != 1)
                {
                    WARN("Ignore iterators in neither --run nor "
                         "--run-force options value.\n"
                         "Path: %s", path_str);
                    item->iterate = 1;
                }
                if (item->step != 0 &&
                    (type == TEST_PATH_RUN_FROM ||
                     type == TEST_PATH_RUN_TO))
                {
                    WARN("Ignore step in --run-%s=%s",
                         (type == TEST_PATH_RUN_FROM) ? "from" : "to",
                         path_str);
                    item->step = 0;
                }
            }
        }
    }

    return rc;
}


/* See the description in tester_defs.h */
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

/**
 * Free resources allocated for test path argument.
 *
 * @param arg           Path argument to be freed
 */
static void
test_path_arg_free(test_path_arg *arg)
{
    tq_strings_free(&arg->values, free);
    free(arg->name);
    free(arg);
}

/**
 * Free resources allocated for test path item.
 *
 * @param item          Path item to be freed
 */
static void
test_path_item_free(test_path_item *item)
{
    test_path_arg  *p;

    while ((p = item->args.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&item->args, p, links);
        test_path_arg_free(p);
    }
    free(item->name);
    free(item);
}

/**
 * Free resources allocated for test path.
 *
 * @param path          Path to be freed
 */
static void
test_path_free(test_path *path)
{
    test_path_item *p;

    while ((p = path->head.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&path->head, p, links);
        test_path_item_free(p);
    }
    free(path->str);
    free(path);
}

/* See the description in test_path.h */
void
test_paths_free(test_paths *paths)
{
    test_path *p;

    while ((p = paths->tqh_first) != NULL)
    {
        TAILQ_REMOVE(paths, p, links);
        test_path_free(p);
    }
}


/**
 * Allocate a bit mask of specified length.
 *
 * @param num           Required number of bits in bitmask
 * @param set           Is default value of all bits set or clear?
 *
 * @return Pointer to allocated memory or NULL.
 */
static uint8_t *
bit_mask_alloc(unsigned int num, te_bool set)
{
    unsigned int    bytes;
    uint8_t        *mem;

    assert(num > 0);
    bytes = ((num - 1) >> 3) + 1;
    mem = malloc(bytes);
    if (mem == NULL)
        return NULL;

    if (set)
        memset(mem, 0xff, bytes);
    else
        memset(mem, 0, bytes);

    return mem;
}

/**
 * Update bit mask to keep start+step*N set bits only.
 *
 * @param bm            Bit mask
 * @param bm_len        Length of the bit mask
 * @param start         First (starting from 1) set bit to be kept as set
 * @param step          Step to keep bits set after the first
 *
 * @return Is resulting bit mask empty or not?
 */
static te_bool
bit_mask_start_step(uint8_t *bm, unsigned int bm_len,
                    unsigned int start, unsigned int step)
{
    te_bool         empty = TRUE;
    unsigned int    i;
    unsigned int    j;
    unsigned int    period = start;

    for (empty = TRUE, j = 0, i = 0; i < bm_len; ++i)
    {
        if (bit_mask_is_set(bm, i))
        {
            j++;
            if (j == period)
            {
                empty = FALSE;
                period = step;
                j = 0;
            }
            else
            {
                bit_mask_clear(bm, i);
            }
        }
    }
    return empty;
}

/**
 * Do logical AND operation for bitmasks. Result is stored in
 * left-hand value. Right-hand value is used specified number of
 * times and expanded (every bit is considered as few bits) to
 * match left-hand value bitmask length.
 *
 * For example,
 *  lhv = 1 0 1 0 1 0 1 0
 *  rhv = 0 1, times 2 -> 0 0 1 1 0 0 1 1
 *  result = 0 0 1 0 0 0 1 0
 *
 * @param lhv           Left-hand value and result
 * @param lhv_len       Length of the left-hand value bitmask
 * @param rhv           Right-hand value
 * @param rhv_len       Length of the right-hand value
 * @param times         How many times right-hand value should be
 *                      repeated
 */
static void
bit_mask_and_expanded(uint8_t *lhv, unsigned int lhv_len,
                      const uint8_t *rhv, unsigned int rhv_len,
                      unsigned int times)
{
    unsigned int weight = lhv_len / (rhv_len * times);
    unsigned int i;

    assert(lhv_len % (rhv_len * times) == 0);
    for (i = 0; i < lhv_len; ++i)
    {
        if (bit_mask_is_set(lhv, i) &&
            !bit_mask_is_set(rhv, (i / weight) % rhv_len))
        {
            bit_mask_clear(lhv, i);
        }
    }
}


/**
 * Data to be passed as opaque to test_path_arg_value_cb() function.
 */
typedef struct test_path_arg_value_cb_data {
    uint8_t        *bm;     /**< Argument bit mask */
    const char     *value;  /**< Value specified by user */
    unsigned int    index;  /**< Index of the current argument value */
    te_bool         found;  /**< Is at least one matching value found? */
    unsigned int    pref_i; /**< Index of the preferred value */

    const test_entity_value *pref_value;    /**< Pointer to the
                                                 preferred value */
} test_path_arg_value_cb_data;

/**
 * Function to be called for each singleton value of the run item
 * argument (explicit or inherited) to create bit mask of iterations
 * in accordace with value specified by user.
 *
 * The function complies with test_entity_value_enum_cb prototype.
 */
static te_errno
test_path_arg_value_cb(const test_entity_value *value, void *opaque)
{
    test_path_arg_value_cb_data *data = opaque;

    ENTRY("value=%s|%s search=%s index=%u found=%u",
          value->plain, value->ext, data->value, data->index, data->found);

    if (data->pref_value == value)
        data->pref_i = data->index;

    if (value->plain != NULL)
    {
        if (strcmp(data->value, value->plain) == 0)
        {
            data->found = TRUE;
            bit_mask_set(data->bm, data->index);
            /* Continue, since equal values are possible */
        }
    }
    else
    {
        assert(value->ext != NULL);
        ERROR("FIXME %s:%d - not supported", __FILE__, __LINE__);
        return TE_EOPNOTSUPP;
    }

    data->index++;

    EXIT();
    return 0;
}

/**
 * Data to be passed as opaque to test_path_arg_cb() function.
 */
typedef struct test_path_arg_cb_data {
    const run_item     *ri;         /**< Run item context */
    test_path_arg      *path_arg;   /**< Current test path argument */
    uint8_t            *bm;         /**< Matching argument values bit
                                         mask */
    unsigned int        n_iters;    /**< Number of outer iterations of
                                         the argument */
    const test_var_arg *va;         /**< Matching argument */
    unsigned int        n_values;   /**< Number of values */
} test_path_arg_cb_data;

static te_errno
test_path_arg_cb(const test_var_arg *va, void *opaque)
{
    test_path_arg_cb_data       *data = opaque;
    tqe_string                  *path_arg_value;
    test_path_arg_value_cb_data  value_data;
    te_bool                      empty = TRUE;
    te_errno                     rc;

    ENTRY("va=%s search=%s n_iters=%u",
          va->name, data->path_arg->name, data->n_iters);

    if (va->list != NULL)
    {
        const test_var_arg_list *list;

        for (list = data->ri->lists.lh_first;
             list != NULL && strcmp(list->name, va->list) != 0;
             list = list->links.le_next);

        assert(list != NULL);
        data->n_values = list->len;
    }
    else
    {
        data->n_values = test_var_arg_values(va)->num;
    }

    if (strcmp(data->path_arg->name, va->name) != 0)
    {
        data->n_iters *= data->n_values;
        data->n_values = 0;
        EXIT("no match");
        return 0;
    }

    data->va = va;

    data->bm = bit_mask_alloc(data->n_values, FALSE);
    if (data->bm == NULL)
        return TE_ENOMEM;

    value_data.pref_i = 0;
    value_data.pref_value = va->preferred;
    value_data.bm = data->bm;
    for (path_arg_value = data->path_arg->values.tqh_first;
         path_arg_value != NULL;
         path_arg_value = path_arg_value->links.tqe_next)
    {
        value_data.value = path_arg_value->v;
        value_data.index = 0;
        value_data.found = FALSE;

        rc = test_var_arg_enum_values(data->ri, va,
                                      test_path_arg_value_cb, &value_data,
                                      NULL, NULL);
        if (rc != 0)
        {
            ERROR("Failed to enumerate values of argument '%s' of "
                  "the test '%s' to find '%s': %r", va->name,
                  test_get_name(data->ri), path_arg_value->v, rc);
            EXIT("%r", rc);
            return rc;
        }
        if (!value_data.found)
        {
            /* May be this value is used in other call of the test */
            INFO("Argument '%s' of the test '%s' does not have value "
                 "'%s'", va->name, test_get_name(data->ri),
                 path_arg_value->v);
        }
        else
        {
            empty = FALSE;
        }
    }

    if (empty)
    {
        /* May be these values are used in other call of the test */
        INFO("Empty set of values is specified for argument '%s' "
             "of the test '%s'", va->name, test_get_name(data->ri));
        EXIT("0 - argument values do not match");
        return 0;
    }

    if (test_var_arg_values(va)->num < data->n_values)
    {
        unsigned int i;

        if (bit_mask_is_set(data->bm, value_data.pref_i))
        {
            for (i = test_var_arg_values(va)->num; i < data->n_values; ++i)
            {
                bit_mask_set(data->bm, i);
            }
        }
    }

    EXIT("EEXIST - found");
    return TE_EEXIST;
}
                      

/** Test path processing context */
typedef struct test_path_proc_ctx {
    LIST_ENTRY(test_path_proc_ctx)  links;  /**< List links */

    const test_path_item   *item;   /**< Current test path item */

    testing_scenario   *scenario;   /**< Testing scenario */
    testing_scenario    ts_local;   /**< Local testing scenario storage */

    const run_item *ri;     /**< Run item context when this instance
                                 was created */
    uint8_t        *bm;     /**< Bit mask with iterations to be run */
    unsigned int    iter;   /**< Current iteration of the run item */

} test_path_proc_ctx;

/**
 * Opaque data for all configuration traverse callbacks.
 */
typedef struct test_path_proc_data {

    LIST_HEAD(, test_path_proc_ctx) ctxs;   /**< Stack of contexts */

    testing_scenario   *scenario;   /**< Resulting testing scenario */
    te_errno            rc;         /**< Status code */

} test_path_proc_data;


/**
 * Clone the most recent (current) test path processing context.
 *
 * @param gctx          Test path processing global context
 * @param path_item     Test path item to be processed
 *
 * @return Allocated context.
 */
static test_path_proc_ctx *
test_path_proc_new_ctx(test_path_proc_data  *gctx,
                       const test_path_item *path_item)
{
    test_path_proc_ctx *new_ctx;

    assert(gctx != NULL);
    assert(path_item != NULL);

    new_ctx = TE_ALLOC(sizeof(*new_ctx));
    if (new_ctx == NULL)
    {
        gctx->rc = TE_RC(TE_TESTER, TE_ENOMEM);
        return NULL;
    }

    new_ctx->item = path_item;
    
    TAILQ_INIT(&new_ctx->ts_local);
    /* By default, testing scenario points to the local storage */
    new_ctx->scenario = &new_ctx->ts_local;
    
    LIST_INSERT_HEAD(&gctx->ctxs, new_ctx, links);

    VERB("%s(): New context %p path_item=%s", __FUNCTION__, new_ctx,
         path_item->name);

    return new_ctx;
}

/**
 * Destroy the most recent (current) test path processing context.
 *
 * @param gctx          Test path processing global context
 *
 * @return Status code.
 */
static void
test_path_proc_destroy_ctx(test_path_proc_data *gctx)
{
    test_path_proc_ctx *ctx = gctx->ctxs.lh_first;

    assert(ctx != NULL);

    VERB("%s(): Destroy context %p", __FUNCTION__, ctx);

    LIST_REMOVE(ctx, links);

    scenario_free(&ctx->ts_local);
    free(ctx->bm);
    free(ctx);
}


static tester_cfg_walk_ctl
test_path_proc_test_start(run_item *run, unsigned int cfg_id_off,
                          unsigned int flags, void *opaque)
{
    test_path_proc_data    *gctx = opaque;
    test_path_proc_ctx     *ctx;
    const char             *name;
    uint8_t                *bm = NULL;

    /* Skip service entries */
    if (flags & TESTER_CFG_WALK_SERVICE)
        return TESTER_CFG_WALK_SKIP;

    assert(gctx != NULL);
    assert(gctx->rc == 0);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    ENTRY("path_item=%s offset=%u run=%s test=%s",
          ctx->item->name, cfg_id_off, run->name, test_get_name(run));

    /* Filter out too long path */
    if (run->type == RUN_ITEM_SCRIPT && ctx->item->links.tqe_next != NULL)
    {
        /* There is no chance to match - too long path */
        EXIT("SKIP - too long, no chance to match");
        return TESTER_CFG_WALK_SKIP;
    }

    /*
     * Match name of the run/test and path item.
     * If run item has no name or it does not match path, just ignore it.
     */
    if (((name = run->name) != NULL) &&
        (strcmp(name, ctx->item->name) == 0))
    {
        /* Name of run item is specified and it matches path item */
    }
    else if ((name = test_get_name(run)) == NULL)
    {
        assert(run->type == RUN_ITEM_SESSION);
        /* Session without name */
    }
    else if (strcmp(name, ctx->item->name) != 0)
    {
        EXIT("SKIP - no match");
        return TESTER_CFG_WALK_SKIP;
    }

    /* Allocate bit mask for all iterations */
    bm = bit_mask_alloc(run->n_iters, TRUE);
    if (bm == NULL)
    {
        gctx->rc = TE_ENOMEM;
        return TESTER_CFG_WALK_FAULT;
    }

    if (name != NULL)
    {
        /*
         * Match arguments
         */
        test_path_arg_cb_data   arg_cb_data;

        arg_cb_data.ri = run;
        for (arg_cb_data.path_arg = ctx->item->args.tqh_first;
             arg_cb_data.path_arg != NULL;
             arg_cb_data.path_arg = arg_cb_data.path_arg->links.tqe_next)
        {
            arg_cb_data.n_iters = 1;
            arg_cb_data.bm = NULL;

            gctx->rc = test_run_item_enum_args(run, test_path_arg_cb,
                                               &arg_cb_data);
            if (gctx->rc == 0)
            {
                INFO("Argument with name '%s' and specified values not "
                     "found", arg_cb_data.path_arg->name);
                free(bm);
                EXIT("SKIP - arg '%s' does not match",
                     arg_cb_data.path_arg->name);
                return TESTER_CFG_WALK_SKIP;
            }
            else if (TE_RC_GET_ERROR(gctx->rc) != TE_EEXIST)
            {
                ERROR("Enumeration of run item arguments failed "
                      "unexpectedly: %r", gctx->rc);
                free(bm);
                EXIT("FAULT - %r", gctx->rc);
                return TESTER_CFG_WALK_FAULT;
            }
            gctx->rc = 0;

            assert(arg_cb_data.va != NULL);
            bit_mask_and_expanded(bm, run->n_iters, arg_cb_data.bm,
                                  arg_cb_data.n_values,
                                  arg_cb_data.n_iters);

            free(arg_cb_data.bm);
        }

        /*
         * Process selector by iteration number and step
         */
        if (ctx->item->select > 0)
        {
            if (bit_mask_start_step(bm, run->n_iters, ctx->item->select,
                                    ctx->item->step))
            {
                INFO("There is no iteration with number %u",
                      ctx->item->select);
                /* 
                 * May other time when the same test is called such
                 * iteration will be found.
                 */
                free(bm);
                EXIT("SKIP - no requested iteration number");
                return TESTER_CFG_WALK_SKIP;
            }
        }

        /*
         * Check for end of test path specification
         */
        if (ctx->item->links.tqe_next == NULL)
        {
            /* End of path */
            assert(gctx->rc == 0);
            gctx->rc = scenario_by_bit_mask(ctx->scenario, cfg_id_off, bm,
                                            run->n_iters, run->weight);

            free(bm);

            if (gctx->rc != 0)
            {
                EXIT("FAULT - %r", gctx->rc);
                return TESTER_CFG_WALK_FAULT;
            }

            /* We don't want go into the depth */
            EXIT("SKIP - end of run path");
            return TESTER_CFG_WALK_SKIP;
        }
    }

    assert(run->type != RUN_ITEM_SCRIPT);
    ctx = test_path_proc_new_ctx(gctx, name == NULL ? ctx->item :
                                           ctx->item->links.tqe_next);
    if (ctx == NULL)
    {
        free(bm);
        EXIT("FAULT - %r", gctx->rc);
        return TESTER_CFG_WALK_FAULT;
    }
    ctx->ri = run;
    ctx->bm = bm;

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
test_path_proc_test_end(run_item *run, unsigned int cfg_id_off,
                        unsigned int flags, void *opaque)
{
    test_path_proc_data    *gctx = opaque;
    test_path_proc_ctx     *ctx;
    test_path_proc_ctx     *parent;

    if (flags & TESTER_CFG_WALK_SERVICE)
        return TESTER_CFG_WALK_CONT;

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    ENTRY("path_item=%s offset=%u run=%s test=%s",
          ctx->item->name, cfg_id_off, run->name, test_get_name(run));

    if (ctx->ri == run)
    {
        /*
         * The context was created by start callback for this run item,
         * destroy it.
         */
        test_path_proc_destroy_ctx(gctx);

        ctx = gctx->ctxs.lh_first;
        assert(ctx != NULL);
    }

    if (gctx->rc == 0)
    {
        parent = ctx->links.le_next;

        /*
         * Iterate and append sub-scenario to scenario
         */
        gctx->rc = scenario_append(parent != NULL ? parent->scenario :
                                                    gctx->scenario,
                                   ctx->scenario, ctx->item->iterate);
        if (gctx->rc != 0)
        {
            EXIT("FAULT - %r", gctx->rc);
            return TESTER_CFG_WALK_FAULT;
        }
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
test_path_proc_iter_start(run_item *ri, unsigned int cfg_id_off,
                          unsigned int flags, unsigned int iter,
                          void *opaque)
{
    test_path_proc_data    *gctx = opaque;
    test_path_proc_ctx     *ctx;

    UNUSED(cfg_id_off);

    ENTRY("iter=%u", iter);

    assert(~flags & TESTER_CFG_WALK_SERVICE);

    if (ri->type == RUN_ITEM_SCRIPT)
    {
        EXIT("CONT - script");
        return TESTER_CFG_WALK_CONT;
    }

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    if (bit_mask_is_set(ctx->bm, iter))
    {
        ctx->iter = iter;
        EXIT("CONT");
        return TESTER_CFG_WALK_CONT;
    }
    else
    {
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }
}


/**
 * Process requested test path and generate testing scenario.
 *
 * @param cfgs          Configurations
 * @param total_iters   Total number of iterations
 * @param path          Path to be processed
 *
 * @return Status code.
 */
static te_errno
process_test_path(const tester_cfgs *cfgs, const unsigned int total_iters,
                  test_path *path)
{
    te_errno                rc;
    tester_cfg_walk_ctl     ctl;
    test_path_proc_data     gctx;
    const tester_cfg_walk   cbs = {
        NULL, /* cfg_start */
        NULL, /* cfg_end */
        NULL, /* pkg_start */
        NULL, /* pkg_end */
        NULL, /* session_start */
        NULL, /* session_end */
        NULL, /* prologue_start */
        NULL, /* prologue_end */
        NULL, /* epilogue_start */
        NULL, /* epilogue_end */
        NULL, /* keepalive_start */
        NULL, /* keepalive_end */
        NULL, /* exception_start */
        NULL, /* exception_end */
        test_path_proc_test_start,
        test_path_proc_test_end,
        test_path_proc_iter_start,
        NULL, /* iter_end */
        NULL, /* repeat_start */
        NULL, /* repeat_end */
        NULL, /* script */
    };

    ENTRY("path=%s type=%d", path->str, path->type);

    if (path->head.tqh_first == NULL)
    {
        rc = scenario_add_act(&path->scen, 0, total_iters - 1, 0);
        EXIT("%r", rc);
        return rc;
    }

    /* Initialize global context */
    LIST_INIT(&gctx.ctxs);
    gctx.scenario = &path->scen;
    gctx.rc = 0;

    /* Create the first test path processing context */
    if (test_path_proc_new_ctx(&gctx, path->head.tqh_first) == NULL)
    {
        EXIT("%r", gctx.rc);
        return gctx.rc;
    }

    /* Walk configurations */
    ctl = tester_configs_walk(cfgs, &cbs, 0, &gctx);
    if (ctl != TESTER_CFG_WALK_CONT)
    {
        ERROR("Walk of Tester configurations failed: %r", gctx.rc);
    }
    else 
    {
        assert(gctx.rc == 0);
        if (path->scen.tqh_first == NULL)
        {
            gctx.rc = TE_RC(TE_TESTER, TE_ENOENT);
        }
    }

    /* Destroy the first test path processing context */
    test_path_proc_destroy_ctx(&gctx);
    
    EXIT("%r", gctx.rc);
    return gctx.rc;
}


/**
 * Merge scenarios created for all test paths taking into account
 * types of paths.
 *
 * @param paths         List of paths specified by user
 * @param total_iters   Total number of test items (iterations)
 * @param scenario      Location for testing scenario
 */
static te_errno
merge_test_paths(test_paths *paths, const unsigned int total_iters,
                 testing_scenario *scenario)
{
    testing_scenario    vg;
    testing_scenario    gdb;
    testing_scenario    mix;
    te_bool             run_scen;
    te_bool             run_spec;
    test_path          *path;
    te_errno            rc;

    TAILQ_INIT(&vg);
    TAILQ_INIT(&gdb);
    TAILQ_INIT(&mix);

    for (run_spec = FALSE, path = paths->tqh_first;
         path != NULL;
         path = path->links.tqe_next)
    {
        rc = 0;
        run_scen = FALSE;
        switch (path->type)
        {
            case TEST_PATH_RUN:
                run_scen = TRUE;
                break;

            case TEST_PATH_RUN_FORCE:
                run_scen = TRUE;
                break;

            case TEST_PATH_RUN_FROM:
                run_scen = TRUE;
                if (path->links.tqe_next != NULL &&
                    path->links.tqe_next->type == TEST_PATH_RUN_TO)
                {
                    scenario_apply_to(&path->links.tqe_next->scen,
                                      (path->scen.tqh_first != NULL) ?
                                        path->scen.tqh_first->first : 0);
                    path = path->links.tqe_next;
                }
                else
                {
                    scenario_apply_from(&path->scen, total_iters - 1);
                }
                break;

            case TEST_PATH_RUN_TO:
                run_scen = TRUE;
                scenario_apply_to(&path->scen, 0);
                break;

            case TEST_PATH_VG:
                rc = scenario_merge(&vg, &path->scen, TESTER_VALGRIND);
                break;

            case TEST_PATH_GDB:
                rc = scenario_merge(&gdb, &path->scen, TESTER_GDB);
                break;

            case TEST_PATH_MIX:
                rc = scenario_merge(&mix, &path->scen,
                                    TESTER_MIX_VALUES |
                                    TESTER_MIX_ARGS |
                                    TESTER_MIX_TESTS |
                                    TESTER_MIX_ITERS);
                break;

            case TEST_PATH_MIX_VALUES:
                rc = scenario_merge(&mix, &path->scen, TESTER_MIX_VALUES);
                break;

            case TEST_PATH_MIX_ARGS:
                rc = scenario_merge(&mix, &path->scen, TESTER_MIX_ARGS);
                break;

            case TEST_PATH_MIX_TESTS:
                rc = scenario_merge(&mix, &path->scen, TESTER_MIX_TESTS);
                break;

            case TEST_PATH_MIX_ITERS:
                rc = scenario_merge(&mix, &path->scen, TESTER_MIX_ITERS);
                break;

            case TEST_PATH_MIX_SESSIONS:
                rc = scenario_merge(&mix, &path->scen, TESTER_MIX_SESSIONS);
                break;

            case TEST_PATH_NO_MIX:
                scenario_exclude(&mix, &path->scen);
                break;

            case TEST_PATH_FAKE:
                rc = scenario_merge(&gdb, &path->scen, TESTER_FAKE);
                break;

            default:
                assert(FALSE);
        }
        if (run_scen)
        {
            run_spec = TRUE;
            rc = scenario_apply_flags(&path->scen, &vg);
            rc = scenario_apply_flags(&path->scen, &gdb);
            rc = scenario_apply_flags(&path->scen, &mix);

            /* Append results test path scenario to whole scenario */
            scenario_append(scenario, &path->scen, 1);
        }
    }

    if (!run_spec)
    {
        /* No test paths to run are specified, scenarion is still empty */
        assert(scenario->tqh_first == NULL);
        /* Add act with all items and apply flags */
        rc = scenario_add_act(scenario, 0, total_iters - 1, 0);
        rc = scenario_apply_flags(scenario, &vg);
        rc = scenario_apply_flags(scenario, &gdb);
        rc = scenario_apply_flags(scenario, &mix);
    }

    return rc;
}

/* See the description in test_path.h */
te_errno
tester_process_test_paths(const tester_cfgs  *cfgs,
                          const unsigned int  total_iters,
                          test_paths         *paths,
                          testing_scenario   *scenario)
{
    te_errno    rc;
    test_path  *path;

    assert(cfgs != NULL);
    assert(paths != NULL);
    assert(scenario != NULL);
    
    ENTRY();

    for (path = paths->tqh_first, rc = 0;
         path != NULL && rc == 0;
         path = path->links.tqe_next)
    {
        rc = process_test_path(cfgs, total_iters, path);
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            ERROR("Test path requested by user not found.\nPath: %s",
                  path->str);
            /* Continue with the rest paths */
            rc = 0;
        }
        else if (rc != 0)
        {
            ERROR("Processing of test path failed: %r\nPath: %s",
                  rc, path->str);
            return rc;
        }
    }

    rc = merge_test_paths(paths, total_iters, scenario);

    if (rc == 0)
        INFO("Scenario is %s", scenario_to_str(scenario));

    EXIT("%r", rc);
    return rc;
}
