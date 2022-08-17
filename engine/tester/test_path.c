/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Run path processing code.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#if HAVE_FNMATCH_H
#include <fnmatch.h>
#endif

#include "te_alloc.h"
#include "te_param.h"
#include "logger_api.h"
#include "tq_string.h"

#include "tester_flags.h"
#include "tester_conf.h"
#include "test_path.h"

#if 0
#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL (TE_LL_WARN | TE_LL_ERROR | \
                      TE_LL_VERB | TE_LL_ENTRY_EXIT | TE_LL_RING)
#endif

/** Test path processing context */
typedef struct test_path_proc_ctx {
    SLIST_ENTRY(test_path_proc_ctx) links;  /**< List links */

    const test_path_item   *item;   /**< Current test path item */

    testing_scenario   *scenario;   /**< Testing scenario */
    testing_scenario    ts_local;   /**< Local testing scenario storage */

    const run_item *ri;     /**< Run item context when this instance
                                 was created */
    uint8_t        *bm;     /**< Bit mask with iterations to be run */
    unsigned int    iter;   /**< Current iteration of the run item */

    te_bool transparent;    /**< Is the run item transparent on the path? */

} test_path_proc_ctx;

/**
 * Opaque data for all configuration traverse callbacks.
 */
typedef struct test_path_proc_data {

    SLIST_HEAD(, test_path_proc_ctx) ctxs;   /**< Stack of contexts */

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

    SLIST_INSERT_HEAD(&gctx->ctxs, new_ctx, links);

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
    test_path_proc_ctx *ctx = SLIST_FIRST(&gctx->ctxs);

    assert(ctx != NULL);

    VERB("%s(): Destroy context %p", __FUNCTION__, ctx);

    SLIST_REMOVE(&gctx->ctxs, ctx, test_path_proc_ctx, links);

    scenario_free(&ctx->ts_local);
    free(ctx->bm);
    free(ctx);
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
 * Calculate index of the argument value.
 *
 * @param iter          Index of the iteration
 * @param total_iters   Total number of iterations
 * @param outer_iters   Number of outer iterations of this argument
 * @param n_values      Number of values for this argument
 */
static inline unsigned int
test_var_arg_value_index(unsigned int iter,
                         unsigned int total_iters,
                         unsigned int outer_iters,
                         unsigned int n_values)
{
    assert(iter < total_iters);
    assert(total_iters % (outer_iters * n_values) == 0);

    return (iter % (total_iters / outer_iters)) /
           (total_iters / (outer_iters * n_values));
}

/**
 * Get value of the argument for specified iteration of the run item.
 *
 * @param ctx           Context with run item and iteration specified
 *                      plus context to resolve further external
 *                      referencies
 * @param name          Name of the argument to find
 * @param value         Location to found value pointer
 *
 * @return Status code.
 */
static te_errno
get_iter_arg_value(const test_path_proc_ctx *ctx, const char *name,
                   const char **value)
{
    const test_var_arg         *va;
    unsigned int                n_values;
    unsigned int                outer_iters;
    const test_entity_value    *va_value;
    te_errno                    rc;

    assert(name != NULL);
    assert(value != NULL);

    ENTRY("name=%s", name);

    if (ctx == NULL)
    {
        ERROR("No context to get argument '%s' value", name);
        EXIT("EFAULT");
        return TE_RC(TE_TESTER, TE_EFAULT);
    }

    va = test_run_item_find_arg(ctx->ri, name, &n_values, &outer_iters);
    if (va == NULL)
    {
        ERROR("Argument '%s' not found in run item '%s' context",
              name, run_item_name(ctx->ri));
        EXIT("ESRCH");
        return TE_RC(TE_TESTER, TE_ESRCH);
    }

    rc = test_var_arg_get_value(ctx->ri, va,
                                test_var_arg_value_index(ctx->iter,
                                                         ctx->ri->n_iters,
                                                         outer_iters,
                                                         n_values),
                                NULL, NULL, &va_value);
    if (rc != 0)
    {
        EXIT("%r", rc);
        return rc;
    }

    if (va_value->plain != NULL)
    {
        *value = va_value->plain;
    }
    else
    {
        assert(va_value->ext != NULL);
        rc = get_iter_arg_value(SLIST_NEXT(ctx, links), va_value->ext,
                                value);
        if (rc != 0)
        {
            ERROR("Failed to resolve external reference '%s': %r",
                  va_value->ext, rc);
        }
    }

    EXIT("%r", rc);
    return rc;
}


/**
 * Data to be passed as opaque to test_path_arg_value_cb() function.
 */
typedef struct test_path_arg_value_cb_data {
    const test_path_proc_ctx   *ctx;    /**< Current test path
                                             processing context */

    test_path_arg  *path_arg;   /**< Test path argument with match
                                     style and values */

    uint8_t        *bm;     /**< Argument bit mask */
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
    const char                  *plain;
    tqe_string                  *path_arg_value;
    te_bool                      match;

    ENTRY("value=%s|%s ref=%p index=%u found=%u",
          value->plain, value->ext, value->ref,
          data->index, data->found);

    if (data->pref_value == value)
        data->pref_i = data->index;

    if (value->plain != NULL)
    {
        plain = value->plain;
    }
    else
    {
        te_errno    rc;

        assert(value->ext != NULL);
        rc = get_iter_arg_value(data->ctx, value->ext, &plain);
        if (rc != 0)
        {
            ERROR("Failed to resolve external reference '%s': %r",
                  value->ext, rc);
            EXIT("%r", rc);
            return rc;
        }
    }

    TAILQ_FOREACH(path_arg_value, &data->path_arg->values, links)
    {
        switch (data->path_arg->match)
        {
            case TEST_PATH_EXACT:
            {
                char *m = path_arg_value->v;

                if (strncmp(path_arg_value->v,
                            TEST_ARG_VAR_PREFIX,
                            strlen(TEST_ARG_VAR_PREFIX)) == 0)
                {
                    char env_name[128];

                    /* match glob value with plain */
                    /* fixme kostik: to read the environment is
                     * very very bad*/
                    te_var_name2env(path_arg_value->v,
                                    env_name, sizeof(env_name));
                    m = getenv(env_name);
                    if (m == NULL)
                    {
                        match = FALSE;
                        break;
                    }
                }

                match = (strcmp(m, plain) == 0);
                break;
            }

            case TEST_PATH_GLOB:
#if HAVE_FNMATCH_H
                match = (fnmatch(path_arg_value->v, plain,
                                 FNM_NOESCAPE) == 0);
#else
                WARN("Glob-style matching is not supported");
                match = FALSE;
#endif
                break;

            default:
                assert(FALSE);
                match = FALSE;
        }
        if (match)
        {
            data->found = TRUE;
            bit_mask_set(data->bm, data->index);
            /* Continue, since equal values are possible */
        }
    }

    data->index++;

    EXIT();
    return 0;
}


static tester_cfg_walk_ctl
test_path_proc_test_start(run_item *run, unsigned int cfg_id_off,
                          unsigned int flags, void *opaque)
{
    test_path_proc_data    *gctx = opaque;
    test_path_proc_ctx     *ctx;
    const char             *name;
    uint8_t                *bm = NULL;
    te_errno                rc;

    /* Skip service entries */
    if (flags & TESTER_CFG_WALK_SERVICE)
        return TESTER_CFG_WALK_SKIP;

    assert(gctx != NULL);
    assert(gctx->rc == 0);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    ENTRY("run=%p path_item=%s offset=%u run-name=%s test=%s", run,
          ctx->item->name, cfg_id_off, run->name, test_get_name(run));

    /* Filter out too long path */
    if (run->type == RUN_ITEM_SCRIPT &&
        TAILQ_NEXT(ctx->item, links) != NULL)
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
        VERB("%s(): Match run item name '%s'", __FUNCTION__, name);
    }
    else if ((name = test_get_name(run)) == NULL)
    {
        assert(run->type == RUN_ITEM_SESSION);
        /* Session without name */
    }
    else if (strcmp(name, ctx->item->name) != 0 &&
             strcmp("", ctx->item->name) != 0)
    {
        EXIT("SKIP - no match");
        return TESTER_CFG_WALK_SKIP;
    }

    /* Allocate bit mask for all iterations */
    bm = bit_mask_alloc(run->n_iters, TRUE);
    if (bm == NULL)
    {
        gctx->rc = TE_ENOMEM;
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    if (name != NULL)
    {
        /*
         * Match arguments
         */
        test_path_arg  *path_arg;

        TAILQ_FOREACH(path_arg, &ctx->item->args, links)
        {
            const test_var_arg             *va;
            unsigned int                    n_values;
            unsigned int                    outer_iters;
            uint8_t                        *arg_bm;
            test_path_arg_value_cb_data     value_data;

            VERB("%s: path_arg name=%s", __FUNCTION__, path_arg->name);
            va = test_run_item_find_arg(run, path_arg->name,
                                        &n_values, &outer_iters);
            if (va == NULL)
            {
                INFO("Argument with name '%s' and specified values not "
                     "found", path_arg->name);
                free(bm);
                EXIT("SKIP - arg '%s' does not match", path_arg->name);
                return TESTER_CFG_WALK_SKIP;
            }

            arg_bm = bit_mask_alloc(n_values, FALSE);
            if (arg_bm == NULL)
            {
                free(bm);
                return TE_ENOMEM;
            }

            value_data.ctx = ctx;
            value_data.path_arg = path_arg;
            value_data.bm = arg_bm;
            value_data.index = 0;
            value_data.found = FALSE;
            value_data.pref_i = 0;
            value_data.pref_value = va->preferred;
            rc = test_var_arg_enum_values(run, va,
                                          test_path_arg_value_cb,
                                          &value_data,
                                          NULL, NULL);
            if (rc != 0)
            {
                ERROR("Failed to enumerate values of argument '%s' of "
                      "the run item '%s': %r", va->name,
                      run_item_name(run), rc);
                free(arg_bm);
                free(bm);
                EXIT("%r", rc);
                return rc;
            }
            if (!value_data.found)
            {
                /* May be these values are used in other call of the test */
                INFO("Empty set of values is specified for argument '%s' "
                     "of the run item '%s'", va->name, run_item_name(run));
                free(arg_bm);
                free(bm);
                EXIT("0 - argument values do not match");
                return 0;
            }

            if (test_var_arg_values(va)->num < n_values)
            {
                unsigned int i;

                if (bit_mask_is_set(arg_bm, value_data.pref_i))
                {
                    for (i = test_var_arg_values(va)->num;
                         i < n_values;
                         ++i)
                    {
                        bit_mask_set(arg_bm, i);
                    }
                }
            }

            bit_mask_and_expanded(bm, run->n_iters, arg_bm,
                                  n_values, outer_iters);

            free(arg_bm);
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
        if (TAILQ_NEXT(ctx->item, links) == NULL)
        {
            /* End of path */
            assert(gctx->rc == 0);
            gctx->rc = scenario_by_bit_mask(ctx->scenario, cfg_id_off, bm,
                                            run->n_iters, run->weight,
                                            ctx->item->hash);

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
                                           TAILQ_NEXT(ctx->item, links));
    if (ctx == NULL)
    {
        free(bm);
        EXIT("FAULT - %r", gctx->rc);
        return TESTER_CFG_WALK_FAULT;
    }
    ctx->ri = run;
    ctx->bm = bm;
    ctx->transparent = (name == NULL);

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
    ctx = SLIST_FIRST(&gctx->ctxs);
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

        ctx = SLIST_FIRST(&gctx->ctxs);
        assert(ctx != NULL);
    }

    if (gctx->rc == 0)
    {
        parent = SLIST_NEXT(ctx, links);

        /*
         * Iterate and append sub-scenario to scenario
         */
        gctx->rc = scenario_append(parent != NULL ? parent->scenario :
                                                    gctx->scenario,
                                   ctx->scenario,
                                   ctx->transparent ? 1 :
                                       ctx->item->iterate);
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
    ctx = SLIST_FIRST(&gctx->ctxs);
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
 * @param path          Path to be processed
 *
 * @return Status code.
 */
static te_errno
process_test_path(const tester_cfgs *cfgs, test_path *path)
{
    te_errno                rc;
    test_path_item         *item;
    unsigned long           sn;
    char                   *end;
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
        NULL, /* skip_start */
        NULL, /* skip_end */
    };

    ENTRY("path=%s type=%d", path->str, path->type);

    if (TAILQ_EMPTY(&path->head))
    {
        rc = scenario_add_act(&path->scen, 0, cfgs->total_iters - 1, 0,
                              NULL);
        EXIT("%r", rc);
        return rc;
    }
    /*
     * If there is only one path item with no select, step and arguments
     * and its name is a number less than total number of tests,
     * it is scenario number.
     */
    else if (((item = TAILQ_FIRST(&path->head)) ==
                      TAILQ_LAST(&path->head, test_path_items)) &&
             (item->select == 0) && (item->step == 0) &&
             (TAILQ_EMPTY(&item->args)) &&
             ((sn = strtoul(item->name, &end, 0)) != ULONG_MAX) &&
             (end != item->name) && (*end == '\0') &&
             (sn < cfgs->total_iters))
    {
        unsigned int    i;

        rc = scenario_add_act(&path->scen, sn, sn, 0, NULL);

        assert(item->iterate > 0);
        for (i = 0; rc == 0 && i < item->iterate - 1; ++i)
            rc = scenario_act_copy(&path->scen, TAILQ_FIRST(&path->scen));

        EXIT("%r", rc);
        return rc;
    }

    /* Initialize global context */
    SLIST_INIT(&gctx.ctxs);
    gctx.scenario = &path->scen;
    gctx.rc = 0;

    /* Create the first test path processing context */
    if (test_path_proc_new_ctx(&gctx, TAILQ_FIRST(&path->head)) == NULL)
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
        if (TAILQ_EMPTY(&path->scen))
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
 * @param paths             List of paths specified by user
 * @param total_iters       Total number of test items (iterations)
 * @param scenario          Location for testing scenario
 * @param all_by_default    If tests to run are not specified, run
 *                          all by default
 *
 * @return Status code.
 */
static te_errno
merge_test_paths(test_paths *paths, const unsigned int total_iters,
                 testing_scenario *scenario, te_bool all_by_default)
{
    testing_scenario    flags;
    testing_scenario    exclude;
    te_bool             run_scen;
    te_bool             run_spec;
    test_path          *path;
    te_errno            rc;

    TAILQ_INIT(&flags);
    TAILQ_INIT(&exclude);

    run_spec = FALSE;
    TAILQ_FOREACH(path, paths, links)
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
                if (TAILQ_NEXT(path, links) != NULL &&
                    TAILQ_NEXT(path, links)->type == TEST_PATH_RUN_TO)
                {
                    scenario_apply_to(&TAILQ_NEXT(path, links)->scen,
                                      !TAILQ_EMPTY(&path->scen) ?
                                          TAILQ_FIRST(&path->scen)->first :
                                          0);
                    path = TAILQ_NEXT(path, links);
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

            case TEST_PATH_RUN_EXCLUDE:
                rc = scenario_merge(&exclude, &path->scen, TESTER_SHUTDOWN);
                break;

            case TEST_PATH_FAKE:
                rc = scenario_merge(&flags, &path->scen, TESTER_FAKE);
                break;

            case TEST_PATH_VG:
                rc = scenario_merge(&flags, &path->scen, TESTER_VALGRIND);
                break;

            case TEST_PATH_GDB:
                rc = scenario_merge(&flags, &path->scen, TESTER_GDB);
                break;

            case TEST_PATH_MIX:
                rc = scenario_merge(&flags, &path->scen,
                                    TESTER_MIX_VALUES |
                                    TESTER_MIX_ARGS |
                                    TESTER_MIX_TESTS |
                                    TESTER_MIX_ITERS);
                break;

            case TEST_PATH_MIX_VALUES:
                rc = scenario_merge(&flags, &path->scen, TESTER_MIX_VALUES);
                break;

            case TEST_PATH_MIX_ARGS:
                rc = scenario_merge(&flags, &path->scen, TESTER_MIX_ARGS);
                break;

            case TEST_PATH_MIX_TESTS:
                rc = scenario_merge(&flags, &path->scen, TESTER_MIX_TESTS);
                break;

            case TEST_PATH_MIX_ITERS:
                rc = scenario_merge(&flags, &path->scen, TESTER_MIX_ITERS);
                break;

            case TEST_PATH_MIX_SESSIONS:
                rc = scenario_merge(&flags, &path->scen,
                                    TESTER_MIX_SESSIONS);
                break;

            case TEST_PATH_NO_MIX:
                rc = scenario_exclude(&flags, &path->scen,
                                      TESTER_MIX_VALUES |
                                      TESTER_MIX_ARGS |
                                      TESTER_MIX_TESTS |
                                      TESTER_MIX_ITERS |
                                      TESTER_MIX_SESSIONS);
                break;

            default:
                assert(FALSE);
        }
        if (rc != 0)
            goto exit;

        if (run_scen)
        {
            run_spec = TRUE;
            rc = scenario_apply_flags(&path->scen, &flags);
            if (rc != 0)
                goto exit;

            /* Append results test path scenario to whole scenario */
            rc = scenario_append(scenario, &path->scen, 1);
            /* Append can't fail with @a iterate == 1 */
            assert(rc == 0);
        }
    }

    if (!run_spec && all_by_default)
    {
        /* No test paths to run are specified, scenarion is still empty */
        assert(TAILQ_EMPTY(scenario));
        /* Add act with all items */
        rc = scenario_add_act(scenario, 0, total_iters - 1, 0, NULL);
        if (rc != 0)
            goto exit;
        /* Apply collected flags */
        rc = scenario_apply_flags(scenario, &flags);
        if (rc != 0)
            goto exit;
    }

    /* Take excludes into account */
    if (!TAILQ_EMPTY(&exclude))
    {
        rc = scenario_apply_flags(scenario, &exclude);
        if (rc != 0)
            goto exit;

        /* Remove all acts with SHUTDOWN flag set */
        scenario_del_acts_by_flags(scenario, TESTER_SHUTDOWN);
    }

exit:
    /* TODO: Release resources */

    return rc;
}

/* See the description in test_path.h */
te_errno
tester_process_test_paths(const tester_cfgs *cfgs,
                          test_paths        *paths,
                          testing_scenario  *scenario,
                          te_bool            all_by_default)
{
    te_errno    rc;
    test_path  *path;

    assert(cfgs != NULL);
    assert(paths != NULL);
    assert(scenario != NULL);

    ENTRY();

    for (path = TAILQ_FIRST(paths), rc = 0;
         path != NULL && rc == 0;
         path = TAILQ_NEXT(path, links))
    {
        rc = process_test_path(cfgs, path);
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

    rc = merge_test_paths(paths, cfgs->total_iters, scenario,
                          all_by_default);

    if (rc == 0)
        INFO("Scenario is %s", scenario_to_str(scenario));

    EXIT("%r", rc);
    return rc;
}



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

            TAILQ_FOREACH(item, &path->head, links)
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

    while ((p = TAILQ_FIRST(&item->args)) != NULL)
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

    while ((p = TAILQ_FIRST(&path->head)) != NULL)
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

    while ((p = TAILQ_FIRST(paths)) != NULL)
    {
        TAILQ_REMOVE(paths, p, links);
        test_path_free(p);
    }
    assert(TAILQ_EMPTY(paths));
}
