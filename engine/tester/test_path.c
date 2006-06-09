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

    path = calloc(1, sizeof(*path));
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



static te_errno process_run_items(const test_path_item *item,
                                  const run_items      *runs,
                                  unsigned int          offset,
                                  testing_scenario     *scenario);

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
        ERROR("%s:%d - not supported", __FILE__, __LINE__);
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
            ERROR("Argument '%s' of the test '%s' does not have value "
                  "'%s'", va->name, test_get_name(data->ri),
                  path_arg_value->v);
            EXIT("%r", TE_ENOENT);
            return TE_ENOENT;
        }
        empty = FALSE;
    }

    if (empty)
    {
        ERROR("Empty set of values is specified for argument '%s' "
              "of the test '%s'", va->name, test_get_name(data->ri));
        EXIT("%r", TE_ENOENT);
        return TE_ENOENT;
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

    EXIT("found");
    return TE_EEXIST;
}

void
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
                      

static te_errno
process_run_item(const test_path_item *item, const run_item *run,
                 unsigned int offset, testing_scenario *scenario)
{
    te_errno            rc;
    uint8_t            *bm = NULL;
    unsigned int        i;
    testing_scenario    subscenario;

    ENTRY("path_item=%s offset=%u run=%s test=%s",
          item->name, offset, run->name, test_get_name(run));

    assert(item != NULL);
    assert(run != NULL);
    assert(scenario != NULL);

    /* Filter out too long path */
    if (run->type == RUN_ITEM_SCRIPT && item->links.tqe_next != NULL)
    {
        /* There is no chance to match - too long path */
        return TE_ENOENT;
    }

    /*
     * Match name of the run/test and path item.
     */
    if (run->name != NULL && strcmp(run->name, item->name) == 0)
    {
        /* Named run item match name of the path item */
    }
    else
    {
        /* 
         * If run item has no name or it does not match path,
         * just ignore it.
         */
        const char *name = test_get_name(run);

        if (name != NULL)
        {
            if (strcmp(name, item->name) != 0)
            {
                EXIT("ENOENT");
                return TE_ENOENT;
            }
        }
        else
        {
            assert(run->type == RUN_ITEM_SESSION);
            /* Transparently pass session without name */
            rc = process_run_items(item, &run->u.session.run_items,
                                   offset, scenario);
            if (rc != 0)
            {
                EXIT("%r", rc);
                return rc;
            }
        }
    }

    /* Allocate bit mask for all iterations */
    bm = bit_mask_alloc(run->n_iters, TRUE);
    if (bm == NULL)
    {
        rc = TE_ENOMEM;
        goto exit;
    }

    /*
     * Match arguments
     */
    {
        test_path_arg_cb_data   arg_cb_data;

        arg_cb_data.ri = run;
        for (arg_cb_data.path_arg = item->args.tqh_first;
             arg_cb_data.path_arg != NULL;
             arg_cb_data.path_arg = arg_cb_data.path_arg->links.tqe_next)
        {
            arg_cb_data.n_iters = 1;
            arg_cb_data.bm = NULL;

            rc = test_run_item_enum_args(run, test_path_arg_cb,
                                         &arg_cb_data);
            if (rc == 0)
            {
                ERROR("Argument with name '%s' not found",
                      arg_cb_data.path_arg->name);
                rc = TE_ENOENT;
                goto exit;
            }
            else if (rc == TE_ENOENT)
            {
                /* Error has already been logged */
                goto exit;
            }
            else if (rc != TE_EEXIST)
            {
                ERROR("Enumeration of run item arguments failed "
                      "unexpectedly: %r", rc);
                goto exit;
            }
            rc = 0;

            assert(arg_cb_data.va != NULL);
            bit_mask_and_expanded(bm, run->n_iters, arg_cb_data.bm,
                                  arg_cb_data.n_values,
                                  arg_cb_data.n_iters);

            free(arg_cb_data.bm);
        }
    }

    /*
     * Process selector by iteration number and step
     */
    if (item->select > 0)
    {
        if (bit_mask_start_step(bm, run->n_iters, item->select, item->step))
        {
            ERROR("There is not iteration with number %u", item->select);
            rc = TE_ENOENT;
            goto exit;
        }
    }

    TAILQ_INIT(&subscenario);

    if (item->links.tqe_next == NULL)
    {
        /* End of path */
        rc = scenario_by_bit_mask(&subscenario, offset, bm,
                                  run->n_iters, run->weight);
        if (rc != 0)
            goto exit;
    }
    else
    {
        const run_items    *children;

        assert(run->type != RUN_ITEM_SCRIPT);
        children = (run->type == RUN_ITEM_PACKAGE) ?
                       &run->u.package->session.run_items :
                       &run->u.session.run_items;
        /* 
         * We should process all iterations separately, since child
         * parameters may depend on iteration number.
         */
        for (i = 0; i < run->n_iters; ++i)
        {
            if (bit_mask_is_set(bm, i))
            {
                rc = process_run_items(item->links.tqe_next, children,
                                       offset + i * run->weight,
                                       &subscenario);
                if (rc != 0)
                    goto exit;
            }
        }
    }

    /*
     * Iterate and append sub-scenario to scenario
     */
    rc = scenario_append(scenario, &subscenario, item->iterate);
    if (rc != 0)
        goto exit;

exit:
    free(bm);
    EXIT("%r", rc);
    return rc;
}

static te_errno
process_run_items(const test_path_item *item, const run_items *runs,
                  unsigned int offset, testing_scenario *scenario)
{
    te_errno        result = TE_ENOENT;
    te_errno        rc;
    const run_item *run;

    ENTRY("path_item=%s offset=%u", item->name, offset);

    for (run = runs->tqh_first;
         run != NULL;
         offset += run->n_iters * run->weight,
         run = run->links.tqe_next)
    {
        rc = process_run_item(item, run, offset, scenario);
        if (rc == 0)
        {
            /* At least one run item match */
            result = 0;
        }
        else if (rc != TE_ENOENT)
        {
            /* Unexpected error */
            result = rc;
            break;
        }
    }

    EXIT("%r - offset=%u", result, offset);
    return result;
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
    te_errno                result = TE_ENOENT;
    te_errno                rc;
    unsigned int            offset = 0;
    const tester_cfg       *cfg;

    ENTRY("path=%s type=%d", path->str, path->type);

    if (path->head.tqh_first == NULL)
    {
        rc = scenario_add_act(&path->scen, 0, total_iters - 1, 0);
        EXIT("%r", rc);
        return rc;
    }

    for (cfg = cfgs->tqh_first;
         cfg != NULL;
         offset += cfg->total_iters, cfg = cfg->links.tqe_next)
    {
        rc = process_run_items(path->head.tqh_first, &cfg->runs, offset,
                               &path->scen);
        if (rc == 0)
        {
            /* At least one run item match */
            result = 0;
        }
        else if (rc != TE_ENOENT)
        {
            /* Unexpected error */
            result = rc;
            break;
        }
    }

    EXIT("%r - offset=%u", result, offset);
    return result;
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
        if (rc == TE_ENOENT)
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
