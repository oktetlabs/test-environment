/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with running of the requested configuration.
 */

#define TE_LGR_USER "Run"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <fcntl.h>
#include <poll.h>

#include <openssl/evp.h>

#include <jansson.h>

#include "te_alloc.h"
#include "tq_string.h"
#include "te_str.h"
#include "te_compound.h"
#include "conf_api.h"
#include "log_bufs.h"
#include "te_trc.h"

#include "tester_conf.h"
#include "tester_term.h"
#include "tester_run.h"
#include "tester_result.h"
#include "tester_interactive.h"
#include "tester_flags.h"
#include "tester_serial_thread.h"
#include "te_shell_cmd.h"
#include "tester.h"
#include "tester_msg.h"

/** Format string for Valgrind output filename */
#define TESTER_VG_FILENAME_FMT  "vg.test.%d"

/**
 * Log entry in a Tester configuration walking handler.
 *
 * @param _cfg_id_off         Configuration offset.
 * @param _gctx               Pointer to global context.
 */
#define LOG_WALK_ENTRY(_cfg_id_off, _gctx) \
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%" TE_PRINTF_TESTER_FLAGS "x) "   \
          "act_id=%u", _cfg_id_off,                                     \
          _gctx->act != NULL ? (int)_gctx->act->first : -1,             \
          _gctx->act != NULL ? (int)_gctx->act->last : -1,              \
          _gctx->act != NULL ? _gctx->act->flags : 0,                   \
          _gctx->act_id)

/** Size of the Tester shell command buffer */
#define TESTER_CMD_BUF_SZ           32768

/** Size of the bulk used to allocate space for a string */
#define TESTER_STR_BULK 64

/** Print string which may be NULL. */
#define PRINT_STRING(_str)  ((_str) ? : "")

/** ID assigned by the Tester to the test instance */
extern unsigned int te_test_id;

#if 0
#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL (TE_LL_WARN | TE_LL_ERROR | \
                      TE_LL_VERB | TE_LL_ENTRY_EXIT | TE_LL_RING)
#endif

/** A wrapper around json_t for SLIST */
typedef struct json_stack_entry {
    SLIST_ENTRY(json_stack_entry) links; /**< List links */

    json_t  *json;           /**< Pointer to JSON object */
    bool ka_encountered; /**< Whether the keepalive item was already
                                  encountered within this item */
} json_stack_entry;

/** A stack of JSON objects to track currently running packages and sessions */
typedef SLIST_HEAD(json_stack, json_stack_entry) json_stack;

/** Data structure to represent and assemble the execution plan */
typedef struct tester_plan {
    json_t        *root;    /**< Root plan object */
    json_stack     stack;   /**< Current package/session path */
    bool pending; /**< Whether some run item is pending */
    run_item_role  role;    /**< Role of run items to be added */
    const char    *test;    /**< Pending test name */
    int            iters;   /**< Pending test iterations */
    int            skipped; /**< Pending number of skipped items */
    int            ignore;  /**< How deep we are in the subtree
                                 that must be ignored */
} tester_plan;

/** Tester context */
typedef struct tester_ctx {
    SLIST_ENTRY(tester_ctx) links;  /**< List links */

    tester_flags        flags;          /**< Flags */

    tester_test_result  group_result;   /**< Result for the group of test
                                             executed in this context */
    tester_test_result  current_result; /**< Result of the current test
                                             in this context */

    bool group_step;     /**< Is group step should be done
                                             or group items have been
                                             enumerated one by one */

    const logic_expr   *targets;        /**< Target requirements
                                             expression */
    bool targets_free;   /**< Should target requirements
                                             be freed? */
    logic_expr         *dyn_targets;    /**< Dynamic target requirements
                                             expression */

    test_requirements   reqs;           /**< List of collected sticky
                                             requirements */

    char               *backup;         /**< Configuration backup name */
    bool backup_ok;      /**< Optimization to avoid
                                             duplicate (subsequent)
                                             verifications */

    test_iter_arg      *args;           /**< Test iteration arguments */
    unsigned int        n_args;         /**< Number of arguments */

    struct tester_ctx  *keepalive_ctx;  /**< Keep-alive context */

#if WITH_TRC
    te_trc_db_walker   *trc_walker;     /**< Current position in TRC
                                             database */
    te_trc_db_walker   *keepalive_walker; /**< Position in TRC database
                                               from which to look for
                                               keepalive test */
    bool do_trc_walker;  /**< Move TRC walker or not? */
#endif
} tester_ctx;

/**
 * Opaque data for all configuration traverse callbacks.
 */
typedef struct tester_run_data {
    tester_flags                flags;      /**< Flags */
    const char                 *verdict;    /**< Verdict to stop tester run
                                                 on */
    const tester_cfgs          *cfgs;       /**< Tester configurations */
    test_paths                 *paths;      /**< Testing paths */
    testing_scenario           *scenario;   /**< Testing scenario */
    testing_scenario            fixed_scen; /**< Testing scenario created by
                                                 a preparatory walk */
    const logic_expr           *targets;    /**< Target requirements
                                                 expression specified
                                                 in command line */

    const testing_act          *act;        /**< Current testing act */
    unsigned int                act_id;     /**< Configuration ID of
                                                 the current test to run */
    testing_direction           direction;  /**< Current direction of
                                                 tree walk (last result
                                                 returned from
                                                 run_this_item() */

    tester_test_results         results;    /**< Global storage of
                                                 results for tests
                                                 which are in progress */
    tester_test_msg_listener   *vl;         /**< Test messages listener
                                                 control data */

    tester_plan                 plan;       /**< Execution plan */
    int                         force_skip; /**< Current skip nesting level
                                                 due to prologue failure */
    int                         exception;  /**< Current exception handling
                                                 nesting level */
    int                         plan_id;    /**< ID of the next run item in
                                                 the plan */

#if WITH_TRC
    const te_trc_db            *trc_db;     /**< TRC database handle */
    tqh_strings                 trc_tags;   /**< TRC tags */
#endif

    SLIST_HEAD(, tester_ctx)    ctxs;       /**< Stack of contexts */

} tester_run_data;

/**
 * Callback data to process compound values.
 */
typedef struct compound_arg_data {
    const test_iter_arg *arg;
    union {
        te_string *dest;
        json_t *json_result;
        te_vec *params;
    };
} compound_arg_data;

static enum interactive_mode_opts tester_run_interactive(
                                      tester_run_data *gctx);

/* Forward declarations */
static json_t *persons_info_to_json(const persons_info *persons);

/* Check whether run item has keepalive handler */
static bool
run_item_has_keepalive(run_item *ri)
{
    return (ri->type == RUN_ITEM_SESSION &&
            ri->u.session.keepalive != NULL) ||
           (ri->type == RUN_ITEM_PACKAGE &&
            ri->u.package->session.keepalive != NULL);
}

/**
 * Output Tester control log message (i.e. message about
 * package/session/test start/end).
 *
 * @param json        JSON object that describes the message.
 * @param mi_type     MI message type.
 * @param mi_version  MI message version.
 */
static void
tester_control_log(json_t *json, const char *mi_type,
                   unsigned int mi_version)
{
    char   *text;
    json_t *mi;

    if (json == NULL)
    {
        ERROR("Tester control log failed: json was null");
        return;
    }

    mi = json_pack("{s:s, s:i, s:O}",
                   "type", mi_type,
                   "version", mi_version,
                   "msg", json);
    if (mi == NULL)
    {
        ERROR("Failed to construct MI message");
        return;
    }

    text = json_dumps(mi, JSON_COMPACT);
    if (text != NULL)
    {
        LGR_MESSAGE(TE_LL_MI | TE_LL_CONTROL,
                    TE_LOG_CMSG_USER,
                    "%s", text);
        free(text);
    }
    else
    {
        ERROR("Tester control log failed: json_dumps failure");
    }

    json_decref(mi);
}

/**
 * Add a new JSON object to the execution plan.
 *
 * @param plan      execution plan
 * @param ri        run item in JSON format
 *
 * @returns Status code
 */
static te_errno
tester_plan_add_child(tester_plan *plan, json_t *ri, run_item_role role)
{
    json_stack_entry *e;
    json_t           *parent = NULL;

    if (plan == NULL || ri == NULL)
        return TE_EINVAL;

    if (plan->root == NULL)
    {
        plan->root = ri;
        parent = NULL;
    }
    else
    {
        e = SLIST_FIRST(&plan->stack);
        assert(e != NULL);
        parent = e->json;
    }

    if (parent != NULL)
    {
        if (role == RI_ROLE_NORMAL)
        {
            json_t     *children;

            children = json_object_get(parent, "children");
            if (children == NULL)
            {
                children = json_array();
                if (children == NULL)
                    return TE_ENOMEM;
                if (json_object_set_new(parent, "children", children) != 0)
                {
                    json_decref(children);
                    return TE_EFAIL;
                }
            }
            if (json_array_append_new(children, ri) != 0)
                return TE_EFAIL;
        }
        else
        {
            if (json_object_set_new(parent, ri_role2str(role), ri) != 0)
                return TE_EFAIL;
        }
    }

    return 0;
}

/**
 * Add a pending test to the execution plan.
 *
 * @param plan      execution plan
 *
 * @returns Status code
 */
static te_errno
tester_plan_add_pending_test(tester_plan *plan)
{
    json_t   *json;
    te_errno  rc;

    if (plan == NULL)
        return TE_EINVAL;

    if (plan->iters > 1)
        json = json_pack("{s:s, s:s, s:i}",
                         "type", "test",
                         "name", plan->test,
                         "iterations", plan->iters);
    else
        json = json_pack("{s:s, s:s}",
                         "type", "test",
                         "name", plan->test);
    if (json == NULL)
    {
        ERROR("Failed to pack test object for execution plan");
        return TE_EFAIL;
    }
    rc = tester_plan_add_child(plan, json, plan->role);
    if (rc != 0)
    {
        json_decref(json);
        return rc;
    }

    plan->pending = false;
    plan->test = NULL;
    plan->role = RI_ROLE_NORMAL;
    plan->iters = 0;

    return 0;
}

/**
 * Add pending skipped items to the execution plan.
 *
 * @param plan      execution plan
 *
 * @returns Status code
 */
static te_errno
tester_plan_add_pending_skipped(tester_plan *plan)
{
    json_t   *json;
    te_errno  rc;

    if (plan == NULL)
        return TE_EINVAL;

    if (plan->skipped == 0)
        return 0;

    if (plan->skipped > 1)
        json = json_pack("{s:s, s:i}",
                         "type", "skipped",
                         "iterations", plan->skipped);
    else
        json = json_pack("{s:s}",
                         "type", "skipped");
    if (json == NULL)
    {
        ERROR("Failed to pack JSON object for skipped run items");
        return TE_EFAIL;
    }

    rc = tester_plan_add_child(plan, json, plan->role);
    if (rc != 0)
    {
        json_decref(json);
        return rc;
    }

    plan->pending = false;
    plan->skipped = 0;

    return 0;
}

/**
 * Add a "skipped" plan item.
 *
 * This item is silently skipped during execution. Its purpose in the plan is
 * to allow correct prediction of keepalive execution.
 *
 * @param plan          execution plan
 *
 * @returns Status code
 */
static te_errno
tester_plan_add_skipped(tester_plan *plan)
{
    json_stack_entry *e;

    e = SLIST_FIRST(&plan->stack);
    if (e == NULL)
        return 0;

    /* If current item doesn't have a keepalive section, do nothing */
    if (!e->ka_encountered)
        return 0;

    if (plan->test != NULL)
        tester_plan_add_pending_test(plan);

    plan->pending = true;
    plan->skipped++;
    return 0;
}

/**
 * Add pending item to the plan.
 *
 * @param plan          execution plan
 *
 * @returns Status code
 */
static te_errno
tester_plan_add_pending(tester_plan *plan)
{
    if (plan->test != NULL)
        return tester_plan_add_pending_test(plan);

    if (plan->skipped > 0)
        return tester_plan_add_pending_skipped(plan);

    return 0;
}

/**
 * Add an "ignored" package.
 *
 * This package and its children will not be present in the final plan.
 *
 * @param plan      execution plan
 *
 * @returns Status code
 */
static te_errno
tester_plan_add_ignore(tester_plan *plan)
{
    if (plan == NULL)
        return TE_EINVAL;

    plan->ignore++;
    return 0;
}

/**
 * Add a test iteration to the execution plan.
 *
 * @param plan              execution plan
 * @param test_name         run item in JSON format
 *
 * @returns Status code
 */
static te_errno
tester_plan_register_test(tester_plan *plan, const char *test_name,
                          run_item_role role)
{
    te_errno          rc;
    json_stack_entry *e;

    if (plan->ignore > 0)
        return 0;

    /* Only process the first keepalive item occurrence */
    if (role == RI_ROLE_KEEPALIVE)
    {
        e = SLIST_FIRST(&plan->stack);
        assert(e != NULL);
        if (e->ka_encountered)
            return 0;
        e->ka_encountered = true;
    }

    if (test_name != NULL && plan->test != NULL &&
        strcmp(test_name, plan->test) == 0 &&
        plan->role == role && role == RI_ROLE_NORMAL)
    {
        plan->iters++;
        return 0;
    }

    if (plan->pending)
    {
        rc = tester_plan_add_pending(plan);
        if (rc != 0)
            return rc;
    }

    if (test_name != NULL)
    {
        plan->pending = true;
        plan->test = test_name;
        plan->role = role;
        plan->iters = 1;
    }

    return 0;
}

/**
 * Add a run item in JSON format to the execution plan.
 *
 * This function changes the current active path in the execution tree.
 *
 * @param plan      execution plan
 * @param ri        run item in JSON format
 *
 * @returns Status code
 */
static te_errno
tester_plan_register(tester_plan *plan, json_t *ri, run_item_role role)
{
    te_errno          rc;
    json_stack_entry *e;

    if (plan == NULL || ri == NULL)
        return TE_EINVAL;

    if (plan->pending)
    {
        rc = tester_plan_add_pending(plan);
        if (rc != 0)
            return rc;
    }

    rc = tester_plan_add_child(plan, ri, role);
    if (rc != 0)
        return rc;

    e = TE_ALLOC(sizeof(*e));

    e->json = ri;
    SLIST_INSERT_HEAD(&plan->stack, e, links);

    plan->test = NULL;
    plan->role = RI_ROLE_NORMAL;
    plan->iters = 0;

    return 0;
}

/**
 * Add a run item to the execution plan.
 *
 * @param plan      execution plan
 * @param ri        run item in JSON format
 * @param ctx       Tester context
 *
 * @returns Status code
 */
static te_errno
tester_plan_register_run_item(tester_plan *plan, run_item *ri, tester_ctx *ctx)
{
    te_errno  rc;
    json_t   *obj;
    json_t   *authors;

    const char *name =
        (ctx->flags & TESTER_LOG_IGNORE_RUN_NAME) ? test_get_name(ri)
                                                  : run_item_name(ri);

    if (plan == NULL || ri == NULL)
        return TE_EINVAL;

    switch (ri->type)
    {
        case RUN_ITEM_SCRIPT:
            tester_plan_register_test(plan, name, ri->role);
            return 0;
        case RUN_ITEM_SESSION:
            /*
             * "*" instead of "?" would be better here, but it's not supported in
             * jansson-2.10, which is currently the newest version available on
             * CentOS/RHEL-7.x
             */
            obj = json_pack("{s:s, s:s?}",
                            "type", "session",
                            "name", name != NULL ? name : "session");
            if (obj == NULL)
            {
                ERROR("Failed to pack session info into JSON");
                return TE_EFAIL;
            }

            rc = tester_plan_register(plan, obj, ri->role);
            if (rc != 0)
            {
                ERROR("Failed to register session \"%s\": %r",
                      test_get_name(ri), rc);
                json_decref(obj);
                return TE_EFAIL;
            }
            break;
        case RUN_ITEM_PACKAGE:
            authors = persons_info_to_json(&ri->u.package->authors);
            if (authors == NULL)
            {
                ERROR("Failed to transform package authors into JSON");
                return TE_EFAIL;
            }

            /*
             * "*" instead of "?" would be better here, but it's not supported in
             * jansson-2.10, which is currently the newest version available on
             * CentOS/RHEL-7.x
             */
            obj = json_pack("{s:s, s:s, s:s?, s:o}",
                            "type", "pkg",
                            "name", name,
                            "objective", ri->u.package->objective,
                            "authors", authors);
            if (obj == NULL)
            {
                ERROR("Failed to pack package info into JSON");
                json_decref(authors);
                return TE_EFAIL;
            }

            rc = tester_plan_register(plan, obj, ri->role);
            if (rc != 0)
            {
                ERROR("Failed to register package \"%s\": %r",
                      name, rc);
                json_decref(obj);
                return TE_EFAIL;
            }
            break;
        default:
            return 0;
    }
    return 0;
}

/**
 * Move up one level in the execution tree.
 *
 * @param plan              execution plan
 *
 * @returns Status code
 */
static te_errno
tester_plan_pop(tester_plan *plan)
{
    json_stack_entry *e;

    if (plan == NULL)
        return TE_EINVAL;

    if (plan->ignore > 0)
    {
        plan->ignore--;
        return 0;
    }

    e = SLIST_FIRST(&plan->stack);
    if (e == NULL)
    {
        ERROR("Popping an empty path stack");
        return TE_EINVAL;
    }

    tester_plan_register_test(plan, NULL, RI_ROLE_NORMAL);
    plan->test = NULL;
    plan->role = RI_ROLE_NORMAL;
    plan->iters = 0;

    SLIST_REMOVE_HEAD(&plan->stack, links);
    free(e);
    return 0;
}

/**
 * Get unique test ID.
 */
static test_id
tester_get_id(void)
{
    static test_id id = 0;

    return id++;
}

/**
 * Free Tester context.
 *
 * @param ctx       Tester context to be freed
 */
static void
tester_run_free_ctx(tester_ctx *ctx)
{
    if (ctx == NULL)
        return;

    if (ctx->targets_free)
    {
        /*
         * It is OK to discard 'const' qualifier here, since it exactly
         * specified that it should be freed.
         */
        logic_expr_free_nr((logic_expr *)ctx->targets);
    }
    logic_expr_free(ctx->dyn_targets);
    test_requirements_free(&ctx->reqs);
    tester_run_free_ctx(ctx->keepalive_ctx);
    free(ctx);
}

/**
 * Allocate a new Tester context.
 *
 * @param flags         Initial flags
 * @param targets       Initial target requirements expression
 *
 * @return Allocated context (cannot return @c NULL).
 */
static tester_ctx *
tester_run_new_ctx(tester_flags flags, const logic_expr *targets)
{
    tester_ctx *new_ctx = TE_ALLOC(sizeof(*new_ctx));

    new_ctx->flags = flags;

    /* new_ctx->current_result.id = 0; */
    te_test_result_init(&new_ctx->group_result.result);
    new_ctx->group_result.status = TESTER_TEST_EMPTY;
#if WITH_TRC
    new_ctx->group_result.exp_result = NULL;
    new_ctx->group_result.exp_status = TRC_VERDICT_UNKNOWN;
#endif

    /* new_ctx->current_result.id = 0; */
    te_test_result_init(&new_ctx->current_result.result);
    new_ctx->current_result.status = TESTER_TEST_INCOMPLETE;
#if WITH_TRC
    new_ctx->current_result.exp_result = NULL;
    new_ctx->current_result.exp_status = TRC_VERDICT_UNKNOWN;
#endif

    new_ctx->targets = targets;
    new_ctx->targets_free = false;
    new_ctx->dyn_targets = NULL;
    TAILQ_INIT(&new_ctx->reqs);

    new_ctx->backup = NULL;
    new_ctx->backup_ok = false;
    new_ctx->args = NULL;
    /* new_ctx->n_args = 0; */

    new_ctx->keepalive_ctx = NULL;

#if WITH_TRC
    new_ctx->trc_walker = NULL;
    new_ctx->keepalive_walker = NULL;
    new_ctx->do_trc_walker = false;
#endif

    return new_ctx;
}

/**
 * Allocate a new Tester context.
 *
 * @param ctx           Tester context to be cloned.
 * @param new_group     Clone context for a new group
 *
 * @return Pointer to new Tester context (cannot return @c NULL).
 */
static tester_ctx *
tester_run_clone_ctx(const tester_ctx *ctx, bool new_group)
{
    tester_ctx *new_ctx = tester_run_new_ctx(ctx->flags, ctx->targets);

    if (new_group)
    {
        new_ctx->group_result.id = ctx->current_result.id;
#if WITH_TRC
        new_ctx->group_result.exp_result = ctx->current_result.exp_result;
        new_ctx->group_result.exp_status = ctx->current_result.exp_status;
#endif
    }
    else
    {
        new_ctx->group_result.id = ctx->group_result.id;
        new_ctx->group_result.status = ctx->group_result.status;
#if WITH_TRC
        new_ctx->group_result.exp_result = ctx->group_result.exp_result;
        new_ctx->group_result.exp_status = ctx->group_result.exp_status;
#endif
    }

    test_requirements_clone(&ctx->reqs, &new_ctx->reqs);

#if WITH_TRC
    new_ctx->trc_walker = ctx->trc_walker;
    new_ctx->keepalive_walker = ctx->keepalive_walker;
    new_ctx->do_trc_walker = ctx->do_trc_walker;
#endif

    return new_ctx;
}

/**
 * Destroy the most recent (current) Tester context.
 *
 * @param data          Global Tester context data
 *
 * @return Status code.
 */
static void
tester_run_destroy_ctx(tester_run_data *data)
{
    tester_ctx *curr = SLIST_FIRST(&data->ctxs);
    tester_ctx *prev;

    /* Test list is empty, nothing to do */
    if (curr == NULL)
        return;

    prev = SLIST_NEXT(curr, links);
    if (prev != NULL)
    {
        if (prev->group_result.id == curr->group_result.id)
        {
            prev->group_result.status = curr->group_result.status;
#if WITH_TRC
            prev->group_result.exp_status =
                curr->group_result.exp_status;
#endif
        }
        else
        {
            prev->current_result.status = curr->group_result.status;
#if WITH_TRC
            prev->current_result.exp_status =
                curr->group_result.exp_status;
#endif
        }
    }

    VERB("Tester context %p deleted: flags=0x%" TE_PRINTF_TESTER_FLAGS "x "
         "parent_id=%u child_id=%u status=%r",
         curr, curr->flags, curr->group_result.id, curr->current_result.id,
         curr->current_result.status);

    SLIST_REMOVE(&data->ctxs, curr, tester_ctx, links);

#if WITH_TRC
    if (SLIST_EMPTY(&data->ctxs))
        trc_db_free_walker(curr->trc_walker);
#endif

    tester_run_free_ctx(curr);
}

/**
 * Allocate a new (the first) tester context.
 *
 * @param data          Global Tester context data
 *
 * @return Allocated context (cannot return @c NULL).
 */
static tester_ctx *
tester_run_first_ctx(tester_run_data *data)
{
    tester_ctx *new_ctx = tester_run_new_ctx(data->flags, data->targets);

    if (!(data->flags & (TESTER_PRERUN | TESTER_ASSEMBLE_PLAN)))
        new_ctx->group_result.id = tester_get_id();

#if WITH_TRC
    if (~new_ctx->flags & TESTER_NO_TRC)
    {
        /* FIXME: Avoid type cast here */
        new_ctx->trc_walker =
            trc_db_new_walker((te_trc_db *)data->trc_db);
        if (new_ctx->trc_walker == NULL)
        {
            tester_run_free_ctx(new_ctx);
            return NULL;
        }
        new_ctx->do_trc_walker = false;
    }
#endif

    assert(SLIST_EMPTY(&data->ctxs));
    SLIST_INSERT_HEAD(&data->ctxs, new_ctx, links);

    VERB("Initial context: flags=0x%" TE_PRINTF_TESTER_FLAGS "x "
         "group_id=%u", new_ctx->flags, new_ctx->group_result.id);

    return new_ctx;
}

/**
 * Clone the most recent (current) Tester context.
 *
 * @param data          Global Tester context data
 * @param new_group     Clone context for a new group
 *
 * @return Allocated context (cannot return @c NULL).
 */
static tester_ctx *
tester_run_more_ctx(tester_run_data *data, bool new_group)
{
    tester_ctx *new_ctx;

    assert(!SLIST_EMPTY(&data->ctxs));
    new_ctx = tester_run_clone_ctx(SLIST_FIRST(&data->ctxs), new_group);

    SLIST_INSERT_HEAD(&data->ctxs, new_ctx, links);

    VERB("Tester context %p cloned %p: flags=0x%" TE_PRINTF_TESTER_FLAGS
         "x group_id=%u current_id=%u", SLIST_NEXT(new_ctx, links), new_ctx,
         new_ctx->flags, new_ctx->group_result.id,
         new_ctx->current_result.id);

    return new_ctx;
}

/**
 * Assemble and log the test execution plan.
 *
 * @param data          Tester run data
 * @param cbs           configuration walk callbacks
 * @param cfgs          Tester configurations
 *
 * @returns Status code
 */
static te_errno
tester_assemble_plan(tester_run_data *data, const tester_cfg_walk *cbs, const tester_cfgs *cfgs)
{
    char                *plan_text;
    tester_flags         orig_flags;
    const testing_act   *orig_act;
    unsigned int         orig_act_id;
    tester_cfg_walk_ctl  ctl;
    json_t              *json;
    json_error_t         err;

    orig_flags  = data->flags;
    data->flags |= TESTER_ASSEMBLE_PLAN | TESTER_NO_TRC | TESTER_NO_CS |
                   TESTER_NO_CFG_TRACK;
    orig_act    = data->act;
    orig_act_id = data->act_id;
    if (tester_run_first_ctx(data) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);

    ctl = tester_configs_walk(cfgs, cbs, 0, data);

    data->flags = orig_flags;
    data->act = orig_act;
    data->act_id = orig_act_id;
    data->direction = TESTING_FORWARD;
    tester_run_destroy_ctx(data);

    if (ctl != TESTER_CFG_WALK_FIN)
    {
        if (ctl == TESTER_CFG_WALK_CONT && data->plan.root == NULL)
        {
            WARN("The execution plan is empty");
            return 0;
        }
        ERROR("Plan-gathering tree walk returned unexpected result %u",
              ctl);
        LGR_MESSAGE(TE_LL_ERROR, TE_LOG_EXEC_PLAN_USER,
                    "Failed to assemble the execution plan");
        json_decref(data->plan.root);
        data->plan.root = NULL;
        return TE_RC(TE_TESTER, TE_EFAULT);
    }

    json = json_pack_ex(&err, 0, "{s:s, s:i, s:o}",
                        "type", "test_plan",
                        "version", 1,
                        "plan", data->plan.root);
    if (json == NULL)
    {
        ERROR("Failed to form execution plan MI message: %s", err.text);
        json_decref(data->plan.root);
        data->plan.root = NULL;
        return TE_RC(TE_TESTER, TE_EFAULT);
    }

    plan_text = json_dumps(json, JSON_COMPACT);
    json_decref(json);
    data->plan.root = NULL;
    if (plan_text == NULL)
    {
        LGR_MESSAGE(TE_LL_ERROR, TE_LOG_EXEC_PLAN_USER,
                    "Failed to dump the execution plan to string");
        return TE_RC(TE_TESTER, TE_EFAULT);
    }
    LGR_MESSAGE(TE_LL_MI | TE_LL_CONTROL, TE_LOG_EXEC_PLAN_USER,
                "%s", plan_text);
    free(plan_text);

    return 0;
}


/**
 * Update test group status.
 *
 * @param group_status  Group status
 * @param iter_status   Test iteration status
 *
 * @return Updated group status.
 */
static tester_test_status
tester_group_status(tester_test_status group_status,
                    tester_test_status iter_status)
{
    tester_test_status result;

    if (group_status < iter_status)
    {
        if (iter_status == TESTER_TEST_SEARCH)
            result = TESTER_TEST_FAILED;
        else
            result = iter_status;
    }
    else
    {
        result = group_status;
    }
    VERB("gs=%u is=%u -> %u", group_status, iter_status, result);
    return result;
}

/**
 * Update test group result.
 *
 * @param group_result  Group result to be updated
 * @param iter_result   Test iteration result
 */
static void
tester_group_result(tester_test_result *group_result,
                    const tester_test_result *iter_result)
{
    group_result->status = tester_group_status(group_result->status,
                                               iter_result->status);
#if WITH_TRC
    ENTRY("iter-status=%u group-exp-status=%u item-exp-status=%u",
          iter_result->status, group_result->exp_status,
          iter_result->exp_status);
    if (iter_result->status != TESTER_TEST_EMPTY)
    {
        /*
         * If group contains unknown or unexpected results, its
         * status is unexpected.
         */
        if (iter_result->exp_status == TRC_VERDICT_UNKNOWN &&
            group_result->exp_status == TRC_VERDICT_UNKNOWN)
            /* Do nothing */;
        else if (iter_result->exp_status != TRC_VERDICT_EXPECTED)
            group_result->exp_status = TRC_VERDICT_UNEXPECTED;
        else if (group_result->exp_status == TRC_VERDICT_UNKNOWN)
            group_result->exp_status = TRC_VERDICT_EXPECTED;
    }
    EXIT("%u", group_result->exp_status);
#endif
}

/**
 * Convert information about group of persons to string representation.
 *
 * @param persons   Information about persons
 *
 * @return Allocated string or NULL.
 */
static char *
persons_info_to_string(const persons_info *persons)
{
    size_t              total = TESTER_STR_BULK;
    char               *res = TE_ALLOC(total);
    char               *s = res;
    size_t              rest = total;
    const person_info  *p;
    int                 printed;
    bool again;

    s[0] = '\0';
    TAILQ_FOREACH(p, persons, links)
    {
        do  {
            printed = snprintf(s, rest, " %s%smailto:%s",
                               PRINT_STRING(p->name),
                               (p->name == NULL) ? "" : " ",
                               PRINT_STRING(p->mailto));
            assert(printed > 0);
            if ((size_t)printed >= rest)
            {
                /* Discard just printed */
                s[0] = '\0';
                /* We are going to extend buffer */
                rest += total;
                total <<= 1;
                res = realloc(res, total);
                if (res == NULL)
                {
                    ERROR("%s(): Memory allocation failure", __FUNCTION__);
                    return NULL;
                }
                /* Locate current pointer */
                s = res + strlen(res);
                /* Print the last item again */
                again = true;
            }
            else
            {
                rest -= printed;
                s += printed;
                again = false;
            }
        } while (again);
    }
    return res;
}

/**
 * Convert information about group of persons to JSON representation.
 *
 * @param persons   Information about persons
 *
 * @return JSON array or NULL.
 */
static json_t *
persons_info_to_json(const persons_info *persons)
{
    const person_info *p;
    json_t            *result;
    json_t            *item;

    if (TAILQ_EMPTY(persons))
        return NULL;

    result = json_array();
    if (result == NULL)
    {
        ERROR("%s: failed to allocate memory for array", __FUNCTION__);
        return NULL;
    }

    TAILQ_FOREACH(p, persons, links)
    {
        /*
         * "*" instead of "?" would be better here, but it's not supported in
         * jansson-2.10, which is currently the newest version available on
         * CentOS/RHEL-7.x
         */
        item = json_pack("{s:s? s:s?}",
                         "name",  p->name,
                         "email", p->mailto);
        if (item != NULL)
        {
            if (json_array_append_new(result, item) != 0)
            {
                ERROR("%s: failed to add item to the array", __FUNCTION__);
                json_decref(result);
                return NULL;
            }
        }
        else
        {
            ERROR("%s: failed to pack into JSON array", __FUNCTION__);
            json_decref(result);
            return NULL;
        }
    }

    return result;
}

static te_errno
param_to_string(char *key, size_t idx, char *value, bool has_more, void *user)
{
    compound_arg_data *data = user;


    VERB("%s(): parameter %s %s[%zu]=%s", __func__, data->arg->name,
         te_str_empty_if_null(key), idx, value);

    te_string_append(data->dest, " ");
    te_compound_build_name(data->dest, data->arg->name, key, idx);
    te_string_append(data->dest, "=");
    te_string_append_shell_arg_as_is(data->dest, value);

    UNUSED(has_more);
    return 0;
}

/**
 * Convert test parameters to string representation using te_string.
 * The first symbol is a space, if @p str is not NULL.
 *
 * @param str           Initial te_string to append parameters or NULL
 * @param n_args        Number of arguments
 * @param args          Current arguments context
 *
 * @note It is expected that @p str is allocated.
 *
 */
static void
test_params_to_te_string(te_string *str, const unsigned int n_args,
                         const test_iter_arg *args)
{
    unsigned int         i;
    const test_iter_arg *p;

    if (args == NULL || str == NULL)
        return;

    for (i = 0, p = args; i < n_args; ++i, ++p)
    {
        if (p->variable)
            continue;

        te_compound_iterate_str(p->value, param_to_string,
                                &(compound_arg_data){.arg = p, .dest = str});
    }

    VERB("%s(): %s", __FUNCTION__, str->ptr);
}

/**
 * Construct iteration objective from objective of the test/session/package
 * and objectives of parameter values.
 *
 * @param str         TE string for constructed value.
 * @param objective   The test/session/package objective.
 * @param n_args      Number of test iteration arguments.
 * @param args        Test iteration arguments.
 */
static void
collect_objectives(te_string *str, const char *objective,
                   const unsigned int n_args,
                   const test_iter_arg *args)
{
    unsigned int i;
    const test_iter_arg *p;
    bool colon = true;
    bool comma = false;

    if (objective != NULL)
    {
        te_string_append(str, "%s", objective);
    }
    else
    {
        colon = false;
    }

    for (i = 0, p = args; i < n_args; ++i, ++p)
    {
        if (p->variable)
            continue;

        if (p->objective != NULL)
        {
            if (colon)
            {
                te_string_chop(str, ".");
                te_string_append(str, ": ");
                colon = false;
            }

            if (comma)
                te_string_append(str, ", ");

            te_string_append(str, "%s", p->objective);
            comma = true;
        }
    }
}

static te_errno
param_to_json(char *key, size_t idx, char *value, bool has_more, void *user)
{
    te_string json_key = TE_STRING_INIT;
    compound_arg_data *data = user;
    json_t *item;

    te_compound_build_name(&json_key, data->arg->name, key, idx);
    item = json_pack("[ss]", json_key.ptr, value);
    te_string_free(&json_key);

    if (item == NULL)
    {
        ERROR("%s(): failed to pack into JSON array", __func__);
        return TE_EFAIL;
    }

    if (json_array_append_new(data->json_result, item) != 0)
    {
        ERROR("%s: failed to add item to the array", __func__);
        json_decref(item);
        return TE_EFAIL;
    }

    UNUSED(has_more);
    return 0;
}


/**
 * Convert test parameters to JSON representation.
 *
 * @param n_args        Number of arguments
 * @param args          Current arguments context
 *
 * @return JSON array or NULL
 */
static json_t *
test_params_to_json(const unsigned int n_args, const test_iter_arg *args)
{
    json_t       *result;
    unsigned int  i;

    if (n_args == 0)
        return NULL;

    result = json_array();
    if (result == NULL)
    {
        ERROR("%s: failed to allocate memory for array", __FUNCTION__);
        return NULL;
    }

    for (i = 0; i < n_args; i++)
    {
        if (args[i].variable)
            continue;

        if (te_compound_iterate_str(args[i].value, param_to_json,
                                    &(compound_arg_data){
                                        .arg = &args[i],
                                        .json_result = result
                                    }) == TE_EFAIL)
        {
            json_decref(result);
            return NULL;
        }
    }

    /* Can happen if all params are variables */
    if (json_array_size(result) == 0)
    {
        json_decref(result);
        return NULL;
    }

    return result;
}

static te_errno
param_stem_to_json(char *key, size_t idx, char *value, bool has_more, void *user)
{
    te_string json_key = TE_STRING_INIT;
    compound_arg_data *data = user;
    json_t *jstr;

    if (key == NULL && idx == 0)
    {
        te_string_free(&json_key);
        return 0;
    }
    te_compound_build_name(&json_key, data->arg->name, key, idx);
    jstr = json_string(data->arg->name);
    if (jstr == NULL)
    {
        ERROR("%s(): failed to create a JSON string", __func__);
        te_string_free(&json_key);
        return TE_EFAIL;
    }
    if (json_object_set_new(data->json_result, json_key.ptr, jstr) != 0)
    {
        ERROR("%s(): failed to add item to the array", __func__);
        json_decref(jstr);
        te_string_free(&json_key);
        return TE_EFAIL;
    }

    UNUSED(has_more);
    te_string_free(&json_key);
    return 0;
}

static te_errno
param_field_to_json(char *key, size_t idx, char *value, bool has_more, void *user)
{
    te_string json_key = TE_STRING_INIT;
    compound_arg_data *data = user;
    json_t *jstr;

    if (key == NULL)
    {
        te_string_free(&json_key);
        return 0;
    }
    te_compound_build_name(&json_key, data->arg->name, key, idx);
    jstr = json_string(key);
    if (jstr == NULL)
    {
        ERROR("%s(): failed to create a JSON string", __func__);
        te_string_free(&json_key);
        return TE_EFAIL;
    }
    if (json_object_set_new(data->json_result, json_key.ptr, jstr) != 0)
    {
        ERROR("%s(): failed to add item to the array", __func__);
        json_decref(jstr);
        te_string_free(&json_key);
        return TE_EFAIL;
    }

    UNUSED(has_more);
    UNUSED(idx);
    te_string_free(&json_key);
    return 0;
}


static json_t *
test_param_names_to_json(const unsigned int n_args, const test_iter_arg *args,
                         te_compound_iter_fn *callback)
{
    json_t       *result;
    unsigned int  i;

    if (n_args == 0)
        return NULL;

    result = json_object();
    if (result == NULL)
    {
        ERROR("%s(): failed to allocate memory for mapping", __func__);
        return NULL;
    }

    for (i = 0; i < n_args; i++)
    {
        if (args[i].variable)
            continue;

        if (te_compound_iterate_str(args[i].value, callback,
                                    &(compound_arg_data){
                                        .arg = &args[i],
                                        .json_result = result
                                    }) == TE_EFAIL)
        {
            json_decref(result);
            return NULL;
        }
    }

    if (json_object_size(result) == 0)
    {
        json_decref(result);
        return NULL;
    }

    return result;
}


/**
 * Add requirements to a strings queue ensuring that every requirement
 * is included only once.
 *
 * @param strs    Queue head.
 * @param reqs    Requirements.
 * @param n_args  Number of test arguments.
 * @param args    Test arguments (used when requirement has a
 *                "ref" attribute).
 */
static void
add_test_reqs_to_strings(tqh_strings *strs, const test_requirements *reqs,
                         unsigned int n_args, const test_iter_arg *args)
{
    test_requirement *p;

    TAILQ_FOREACH(p, reqs, links)
    {
        tq_strings_add_uniq_dup(strs, test_req_id(p, n_args, args));
    }
}

/**
 * Add strings from a queue to JSON array.
 *
 * @param array   JSON array.
 * @param strs    Queue head.
 *
 * @return 0 on success or -1 on failure.
 */
static int
add_strings_to_array(json_t *array, tqh_strings *strs)
{
    json_t *item;
    tqe_string *str;

    TAILQ_FOREACH(str, strs, links)
    {
        item = json_pack("s", str->v);
        if (item == NULL)
        {
            ERROR("%s(): json_pack() failed", __FUNCTION__);
            return -1;
        }

        if (json_array_append_new(array, item) != 0)
        {
            ERROR("%s(): failed to add item to the array", __FUNCTION__);
            return -1;
        }
    }

    return 0;
}

/**
 * Convert test requirements to JSON representation.
 *
 * @param sticky_reqs     Sticky requirements.
 * @param ri              Run item.
 * @param args            Iteration arguments.
 *
 * @return JSON array or NULL.
 */
static json_t *
test_reqs_to_json(const test_requirements *sticky_reqs,
                  const run_item *ri, const test_iter_arg *args)
{
    json_t *result;
    const test_requirements *reqs;
    int rc;
    unsigned int i;
    tqh_strings reqs_unique;

    TAILQ_INIT(&reqs_unique);

    add_test_reqs_to_strings(&reqs_unique, sticky_reqs,
                             ri->n_args, args);

    reqs = tester_get_ri_reqs(ri);
    if (reqs != NULL)
        add_test_reqs_to_strings(&reqs_unique, reqs, ri->n_args, args);

    for (i = 0; i < ri->n_args; i++)
    {
        if (args[i].variable)
            continue;

        add_test_reqs_to_strings(&reqs_unique, &args[i].reqs,
                                 ri->n_args, args);
    }

    result = json_array();
    if (result == NULL)
    {
        ERROR("%s(): failed to allocate memory for array", __FUNCTION__);
        tq_strings_free(&reqs_unique, free);
        return NULL;
    }

    rc = add_strings_to_array(result, &reqs_unique);
    tq_strings_free(&reqs_unique, free);
    if (rc < 0)
    {
        json_decref(result);
        return NULL;
    }

    if (json_array_size(result) == 0)
    {
        json_decref(result);
        return NULL;
    }

    return result;
}

/**
 * Normalize parameter value. Remove trailing spaces and newlines.
 *
 * @param param   Parameter value to normalize
 *
 * @return Allocated string (cannot return @c NULL).
 */
char *
test_params_normalise(const char *param)
{
    const char *p;
    char *q;
    char *str = NULL;
    bool skip_spaces = true;

    if (param == NULL)
        return NULL;

    str = TE_ALLOC(strlen(param) + 1);

    for (p = param, q = str; *p != '\0'; p++)
    {
        if (isspace(*p))
        {
            if (!skip_spaces)
            {
                *q++ = ' ';
                skip_spaces = true;
            }
        }
        else
        {
            *q++ = *p;
            skip_spaces = false;
        }
    }

    /* remove trailing space at the end, if any */
    if ((q > str) && (skip_spaces))
        q--;

    *q = '\0';

    return str;
}

/**
 * Calculate hash (MD5 checksum) for set of test arguments.
 *
 * @param args   Array of test parameters
 * @param n_args Number of of test parameters
 *
 * @return Allocated string (cannot return @c NULL).
 */
char *
test_params_hash(test_iter_arg *args, unsigned int n_args)
{
    EVP_MD_CTX *md5;
    unsigned int md5_len;

    unsigned int  i;
    unsigned int  j;
    unsigned int  k;
    unsigned char digest[EVP_MAX_MD_SIZE];
    char *hash_str = TE_ALLOC(sizeof(digest) * 2 + 1);
    int  *sorted = TE_ALLOC(n_args * sizeof(int));
    char  buf[8192] = {0, };
    int   len = 0;

    for (k = 0; k < n_args; k++)
        sorted[k] = k;

    /* Sort arguments first */
    if (n_args > 0)
    {
        for (i = 0; i < n_args - 1; i++)
        {
            for (j = i + 1; j < n_args; j++)
            {
                if (strcmp(args[sorted[i]].name, args[sorted[j]].name) > 0)
                {
                    k = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = k;
                }
            }
        }
    }

    md5 = EVP_MD_CTX_new();
    EVP_DigestInit_ex(md5, EVP_get_digestbyname("md5"), NULL);

    for (i = 0; i < n_args; i++)
    {
        const char *name = args[sorted[i]].name;
        char *value = test_params_normalise(args[sorted[i]].value);

        VERB("%s %s", name, value);
        len += snprintf(buf + len, sizeof(buf) - len,
                        "%s%s %s", (i != 0) ? " " : "", name, value);
        /*
         * In case buffer size is too short to dump the whole string
         * snprintf dumps only a part of string (that fits into the buffer)
         * and returns the number of bytes it would have been written
         * if buffer was bigger in size.
         * We may easily have buffer overflow here, so we should catch
         * such a condition and set current buffer length to total buffer
         * length value in order to have '0' length for the next
         * snprintf() call.
         * In any case this buffer is filled in for debugging purposes,
         * so there is no much need to care about it.
         */
        if (len >= (int)sizeof(buf))
            len = sizeof(buf);

        if (i != 0)
            EVP_DigestUpdate(md5, " ", 1);

        EVP_DigestUpdate(md5, name, strlen(name));
        EVP_DigestUpdate(md5, " ", 1);
        EVP_DigestUpdate(md5, value, strlen(value));

        free(value);
    }

    EVP_DigestFinal_ex(md5, digest, &md5_len);
    EVP_MD_CTX_free(md5);

    for (i = 0; i < md5_len; i++)
    {
        sprintf(hash_str + strlen(hash_str), "%02hhx", digest[i]);
    }

    free(sorted);

    VERB("\nHash: %s\n", hash_str);
    VERB("%s->%s", buf, hash_str);

    return hash_str;
}

/**
 * Log test (script, package, session) start.
 *
 * @param ctx           Tester context
 * @param ri            Run item
 * @param tin           Test identification number
 */
static void
log_test_start(unsigned int flags,
               const tester_ctx *ctx, const run_item *ri,
               unsigned int tin)
{
    test_id                 parent = ctx->group_result.id;
    test_id                 test = ctx->current_result.id;
    const char             *name =
        (ctx->flags & TESTER_LOG_IGNORE_RUN_NAME) ? test_get_name(ri)
                                                  : run_item_name(ri);
    json_t                 *result;
    json_t                 *authors;
    json_t                 *tmp;
    json_t                 *param_stems;
    json_t                 *param_fields;

    te_string   params_str  = TE_STRING_INIT;
    te_string   obj_str     = TE_STRING_INIT;
    char       *hash_str    = NULL;

    const char *objective = NULL;

#define SET_JSON_STRING(_target, _string) \
    do {                                                                 \
        _target = json_string(_string);                                  \
        if (_target == NULL)                                             \
        {                                                                \
            ERROR("%s: json_string failed for " #_string, __FUNCTION__); \
            json_decref(result);                                         \
            return;                                                      \
        }                                                                \
    } while(0)

#define SET_JSON_INT(_target, _integer) \
    do {                                                                   \
        _target = json_integer(_integer);                                  \
        if (_target == NULL)                                               \
        {                                                                  \
            ERROR("%s: json_integer failed for " #_integer, __FUNCTION__); \
            json_decref(result);                                           \
            return;                                                        \
        }                                                                  \
    } while(0)

#define SET_NEW_JSON(_json, _key, _val) \
    do {                                                      \
        int rc;                                               \
                                                              \
        rc = json_object_set_new(_json, _key, _val);          \
        if (rc != 0)                                          \
        {                                                     \
            ERROR("%s: failed to set field %s for object %s", \
                  __FUNCTION__, _key, #_json);                \
            json_decref(_val);                                \
            json_decref(result);                              \
            return;                                           \
        }                                                     \
    } while (0)

    /*
     * "*" instead of "?" would be better here, but it's not supported in
     * jansson-2.10, which is currently the newest version available on
     * CentOS/RHEL-7.x
     */
    result = json_pack("{s:i, s:i, s:i, s:o?, s:o?}",
                       "id", test,
                       "parent", parent,
                       "plan_id", ri->plan_id,
                       "params", test_params_to_json(ri->n_args, ctx->args),
                       "reqs", test_reqs_to_json(&ctx->reqs, ri, ctx->args));
    if (result == NULL)
    {
        ERROR("JSON object creation failed in %s", __FUNCTION__);
        return;
    }

    param_stems = test_param_names_to_json(ri->n_args, ctx->args,
                                           param_stem_to_json);
    if (param_stems != NULL)
        SET_NEW_JSON(result, "param_stems", param_stems);
    param_fields = test_param_names_to_json(ri->n_args, ctx->args,
                                             param_field_to_json);
    if (param_fields != NULL)
        SET_NEW_JSON(result, "param_fields", param_fields);

    if (name == NULL && ri->type == RUN_ITEM_SESSION)
        name = "session";
    if (name != NULL)
    {
        SET_JSON_STRING(tmp, name);
        SET_NEW_JSON(result, "name", tmp);
    }

    switch (ri->type)
    {
        case RUN_ITEM_SCRIPT:
        {
            const char *page_name =
                ri->page != NULL ? ri->page : ri->u.script.page;

            objective = ri->objective != NULL ?
                ri->objective : ri->u.script.objective;

            SET_JSON_STRING(tmp, "test");
            SET_NEW_JSON(result, "node_type", tmp);

            if (page_name != NULL)
            {
                SET_JSON_STRING(tmp, page_name);
                SET_NEW_JSON(result, "page", tmp);
            }

            if (tin != TE_TIN_INVALID)
            {
                SET_JSON_INT(tmp, tin);
                SET_NEW_JSON(result, "tin", tmp);
            }

            hash_str = test_params_hash(ctx->args, ri->n_args);
            tmp = json_string(hash_str);
            free(hash_str);
            if (tmp == NULL)
            {
                ERROR("%s: json_string failed for hash_str",
                      __FUNCTION__);
                json_decref(result);
                return;
            }
            SET_NEW_JSON(result, "hash", tmp);

            if (flags & TESTER_CFG_WALK_OUTPUT_PARAMS)
            {
                test_params_to_te_string(&params_str, ri->n_args, ctx->args);
                fprintf(stderr, "\n"
                        "                       "
                        "ARGs%s\n"
                        "                              \n",
                        PRINT_STRING(params_str.ptr));
            }
            break;
        }

        case RUN_ITEM_SESSION:
            assert(tin == TE_TIN_INVALID);
            SET_JSON_STRING(tmp, "session");
            SET_NEW_JSON(result, "node_type", tmp);

            objective = ri->u.session.objective;
            break;

        case RUN_ITEM_PACKAGE:
            assert(tin == TE_TIN_INVALID);

            SET_JSON_STRING(tmp, "pkg");
            SET_NEW_JSON(result, "node_type", tmp);

            authors = persons_info_to_json(&ri->u.package->authors);
            if (authors != NULL)
                SET_NEW_JSON(result, "authors", authors);

            objective = ri->u.package->objective;
            break;

        default:
            ERROR("Invalid run item type %d", ri->type);
    }

    collect_objectives(&obj_str, objective, ri->n_args,
                       ctx->args);
    objective = te_string_value(&obj_str);
    if (!te_str_is_null_or_empty(objective))
    {
        SET_JSON_STRING(tmp, objective);
        SET_NEW_JSON(result, "objective", tmp);
    }

    te_string_free(&params_str);
    te_string_free(&obj_str);
    tester_control_log(result, "test_start", 1);
    json_decref(result);

#undef SET_JSON_STRING
#undef SET_JSON_INT
#undef SET_NEW_JSON
}

#ifdef WITH_TRC
/* See description near definition */
static json_t *pack_test_result(const te_test_result *result);

/**
 * Add key and notes attributes to JSON object storing test result.
 *
 * @param json    JSON object.
 * @param key     Key (bug reference).
 * @param notes   Notes.
 *
 * @return Status code.
 */
static te_errno
add_key_notes(json_t *json, const char *key, const char *notes)
{
    json_t *tmp;

    if (key != NULL)
    {
        tmp = json_string(key);
        if (tmp == NULL)
        {
            ERROR("Failed to convert key to JSON");
            return TE_EFAIL;
        }
        if (json_object_set_new(json, "key", tmp) != 0)
        {
            ERROR("Failed to set the \"key\" field");
            json_decref(tmp);
            return TE_EFAIL;
        }
    }

    if (notes != NULL)
    {
        tmp = json_string(notes);
        if (tmp == NULL)
        {
            ERROR("Failed to convert notes to JSON");
            return TE_EFAIL;
        }
        if (json_object_set_new(json, "notes", tmp) != 0)
        {
            ERROR("Failed to set the \"notes\" field");
            json_decref(tmp);
            return TE_EFAIL;
        }
    }

    return 0;
}

/**
 * Pack an expected test result into a JSON object.
 *
 * @param result        test result
 *
 * @returns Pointer to json object or NULL
 */
static json_t *
pack_test_exp_result(const trc_exp_result_entry *result)
{
    json_t *json;
    te_errno rc;

    json = pack_test_result(&result->result);
    if (json == NULL)
        return NULL;

    rc = add_key_notes(json, result->key, result->notes);
    if (rc != 0)
    {
        json_decref(json);
        return NULL;
    }

    return json;
}
#endif

/**
 * Pack verdicts or artifacts into JSON object.
 *
 * @param packed    Verdicts or artifacts.
 *
 * @returns Pointer to JSON object or NULL.
 */
static json_t *
pack_verdicts_or_artifacts(const te_test_verdicts *packed)
{
    json_t *tmp;
    json_t *array;
    te_test_verdict *item;

    array = json_array();
    if (array == NULL)
    {
        ERROR("Failed to create JSON array");
        return NULL;
    }

    TAILQ_FOREACH(item, packed, links)
    {
        tmp = json_string(item->str);
        if (tmp == NULL)
        {
            ERROR("Failed to create JSON string for \"%s\"", item->str);
            json_decref(array);
            return NULL;
        }
        if (json_array_append_new(array, tmp) != 0)
        {
            ERROR("Failed to append JSON string to array");
            json_decref(tmp);
            json_decref(array);
            return NULL;
        }
    }

    return array;
}

/**
 * Pack a test result into a JSON object.
 *
 * @param result        test result
 *
 * @returns Pointer to json object or NULL
 */
static json_t *
pack_test_result(const te_test_result *result)
{
    json_t *json;
    json_t *verdicts = NULL;
    json_t *artifacts = NULL;

    /* Verdicts */
    if (!TAILQ_EMPTY(&result->verdicts))
    {
        verdicts = pack_verdicts_or_artifacts(&result->verdicts);
        if (verdicts == NULL)
            return NULL;
    }

    /* Artifacts */
    if (!TAILQ_EMPTY(&result->artifacts))
    {
        artifacts = pack_verdicts_or_artifacts(&result->artifacts);
        if (artifacts == NULL)
        {
            if (verdicts != NULL)
                json_decref(verdicts);

            return NULL;
        }
    }

    /*
     * "*" instead of "?" would be better here for verdicts, but
     * it must be ensured that Bublik can handle absence of verdicts
     * field.
     */
    json = json_pack("{s:s, s:o?, s:o*}",
                     "status", te_test_status_to_str(result->status),
                     "verdicts", verdicts, "artifacts", artifacts);

    if (json == NULL)
    {
        ERROR("Failed to pack the result object");
        json_decref(verdicts);
        json_decref(artifacts);
    }

    return json;
}

/**
 * Log test (script, package, session) result.
 *
 * @param parent        Parent ID
 * @param test          Test ID
 * @param status        Test status
 * @param error         Reason of failure
 */
static void
log_test_result(test_id parent, tester_test_result *result, int plan_id)
{
    json_t      *json;
    json_t      *obtained = NULL;
    json_t      *expected = NULL;
    const char  *tags = NULL;

#if WITH_TRC
    const trc_exp_result       *exp_result = NULL;
    const trc_exp_result_entry *exp_entry = NULL;
    const char *exp_key = NULL;
    const char *exp_notes = NULL;
    const char *exp_status = NULL;

    if (result->exp_result != NULL)
    {
        exp_result = result->exp_result;
        exp_entry = trc_is_result_expected(exp_result, &result->result);
        tags = exp_result->tags_str;
        exp_key = exp_result->key;
        exp_notes = exp_result->notes;
    }

    exp_status = trc_verdict2str(result->exp_status);

    /* If the result was unexpected, pack the expected results */
    if (exp_result != NULL && exp_entry == NULL)
    {
        trc_exp_result_entry *e;

        expected = json_array();
        if (expected == NULL)
        {
            ERROR("Failed to create \"expected\" object");
            return;
        }

        TAILQ_FOREACH(e, &exp_result->results, links)
        {
            json_t *item;

            item = pack_test_exp_result(e);
            if (item == NULL)
            {
                ERROR("Failed to pack expected result");
                json_decref(expected);
                return;
            }

            if (json_array_append_new(expected, item) != 0)
            {
                ERROR("Failed to add expected result to array");
                json_decref(expected);
                json_decref(item);
                return;
            }
        }
    }

    obtained = pack_test_result(&result->result);
    /*
     * If the obtained result matches one of the expected results,
     * add key and notes from the expected result.
     */
    if (obtained != NULL && exp_entry != NULL)
        add_key_notes(obtained, exp_entry->key, exp_entry->notes);
#else
    obtained = pack_test_result(&result->result);
#endif
    if (obtained == NULL)
    {
        ERROR("Failed to pack the obtained result");
        json_decref(expected);
        return;
    }

    /*
     * "*" instead of "?" would be better here, but it's not supported in
     * jansson-2.10, which is currently the newest version available on
     * CentOS/RHEL-7.x
     */
    json = json_pack("{s:i, s:i, s:i, s:s?, s:o?, s:s?, s:s?, "
                     "s:s?, s:s?, s:o?}",
                     "id", result->id,
                     "parent", parent,
                     "plan_id", plan_id,
                     "error", result->error,
                     "obtained", obtained,
                     "tags_expr", tags,
                     "exp_key", exp_key,
                     "exp_notes", exp_notes,
                     "exp_status", exp_status,
                     "expected", expected);

    if (json == NULL)
    {
        ERROR("Failed to pack test_end object");
        return;
    }

    tester_control_log(json, "test_end", TESTER_TEST_END_VERSION);
    json_decref(json);
}


/**
 * Map tester test status to TE test status plus error string.
 * If error string is not empty, it is added as verdict in test result.
 *
 * @param status        Tester internal test status
 * @param result        TE test result
 * @param error         Location for additional error string
 * @param id            Test ID
 */
static void
tester_test_status_to_te_test_result(tester_test_status status,
                                     te_test_result *result,
                                     const char **error,
                                     test_id id)
{
    static const char *test_status_descr[] = {
        [TESTER_TEST_FAILED] = NULL,
        [TESTER_TEST_DIRTY] = "Unexpected configuration changes",
        [TESTER_TEST_SEARCH] = "Executable not found",
        [TESTER_TEST_KILLED] = "Test application died",
        [TESTER_TEST_CORED] = "Test application core dumped",
        [TESTER_TEST_PROLOG] = "Session prologue failed",
        [TESTER_TEST_EPILOG] = "Session epilogue failed",
        [TESTER_TEST_KEEPALIVE] = "Keep-alive validation failed",
        [TESTER_TEST_EXCEPTION] = "Exception handler failed",
        [TESTER_TEST_INCOMPLETE] = "Internal error",
        [TESTER_TEST_ERROR] = "Internal error"
    };
    test_id saved_id;

    *error = NULL;

    switch (status)
    {
        case TESTER_TEST_PASSED:
            result->status = TE_TEST_PASSED;
            break;

        case TESTER_TEST_SKIPPED:
            result->status = TE_TEST_SKIPPED;
            break;

        case TESTER_TEST_STOPPED:
            result->status = TE_TEST_INCOMPLETE;
            break;

        case TESTER_TEST_FAKED:
            result->status = TE_TEST_FAKED;
            break;

        case TESTER_TEST_EMPTY:
            result->status = TE_TEST_EMPTY;
            break;

        default:
        {
            result->status = TE_TEST_FAILED;
            *error = test_status_descr[status];
            break;
        }
    }

    if (*error != NULL)
    {
        te_test_verdict *v;

        if (id >= 0)
        {
            /*
             * Put additional verdict into the log to have correct
             * results with off-line TRC tools if the result of
             * test script execution is considered.
             */
            saved_id = te_test_id;
            te_test_id = id;
            TE_LOG(TE_LL_ERROR | TE_LL_CONTROL, "Tester Verdict",
                   TE_LOG_VERDICT_USER, "%s", *error);
            te_test_id = saved_id;
        }

        v = TE_ALLOC(sizeof(*v));
        v->str = TE_STRDUP(*error);

        TAILQ_INSERT_TAIL(&result->verdicts, v, links);
    }
}

static te_errno
add_test_script_value(char *key, size_t idx, char *value,
                      bool has_more, void *user)
{
    te_string param_name = TE_STRING_INIT;
    compound_arg_data *data = user;

    te_compound_build_name(&param_name, data->arg->name, key, idx);
    VERB("%s(): parameter %s=%s", __func__, param_name.ptr, value);

    te_vec_append_str_fmt(data->params, "%s=%s",
                          param_name.ptr, value);
    te_string_free(&param_name);

    UNUSED(has_more);
    return 0;
}

static void
prepare_test_script_arguments(te_vec *params, tester_flags flags,
                              const test_script *script,
                              test_id exec_id,
                              const char *test_name,
                              int rand_seed,
                              unsigned int n_args,
                              const test_iter_arg args[n_args])
{
    unsigned int i;

    if (flags & TESTER_GDB)
    {
        static const char *gdb_args[] = {
            "gdb",
            "--args",
            NULL,
        };

        te_vec_append_strarray(params, gdb_args);
    }
    else if (flags & TESTER_VALGRIND)
    {
        static const char *valgrind_args[] = {
            "valgrind",
            "--tool=memcheck",
            "--show-reachable=yes",
            "--leak-check=yes",
            "--num-callers=16",
            NULL,
        };

        te_vec_append_strarray(params, valgrind_args);
        if (flags & TESTER_FAIL_ON_LEAK)
        {
            te_vec_append_str_fmt(params, "--error-exitcode=%d",
                                  EXIT_FAILURE);
        }
    }

    te_vec_append_str_fmt(params, "%s", script->execute);
    te_vec_append_str_fmt(params, "te_test_id=%u", (unsigned int )exec_id);
    te_vec_append_str_fmt(params, "te_test_name=%s", test_name);
    te_vec_append_str_fmt(params, "te_rand_seed=%d", rand_seed);

    for (i = 0; i < n_args; i++)
    {
        if (args[i].variable)
            continue;
        te_compound_iterate_str(args[i].value,
                                add_test_script_value,
                                &(compound_arg_data){
                                    .arg = &args[i],
                                    .params = params
                                });
    }
    te_vec_append(params, NULL);

}

static te_errno
execute_test_script(tester_flags flags, test_id exec_id, char **args, int *code)
{
    char vg_filename[PATH_MAX];
    int fderr = -1;
    pid_t pid;
    te_errno rc = 0;

    if (flags & TESTER_VALGRIND)
    {
        TE_SPRINTF(vg_filename, TESTER_VG_FILENAME_FMT, exec_id);
        fderr = open(vg_filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (fderr < 0)
        {
            rc = TE_OS_RC(TE_TESTER, errno);
            ERROR("Failed to open valgrind output file %s: %r",
                  vg_filename, rc);

            return rc;
        }
    }

    VERB("ID=%d execvp(%s, ...)", exec_id, args[0]);
    pid = fork();
    if (pid < 0)
    {
        rc = TE_OS_RC(TE_TESTER, errno);
        ERROR("Cannot fork: %r", rc);
        if (fderr >= 0)
            close(fderr);
        return rc;
    }

    if (pid == 0)
    {
        /* TODO: is it really safe to call TE logging in a child process? */
        if (fderr >= 0)
        {
            if (dup2(fderr, STDERR_FILENO) < 0)
            {
                ERROR("valgrind: failed to duplicate %s fd to stderr: %r",
                      vg_filename, TE_OS_RC(TE_TESTER, errno));
                _Exit(EXIT_FAILURE);
            }
            close(fderr);
        }
        if (execvp(args[0], args) < 0)
        {
            ERROR("ID=%d execvp(%s, ...) failed: %r", exec_id, args[0],
                  TE_OS_RC(TE_TESTER, errno));
            _Exit(EXIT_FAILURE);
        }
        /* unreachable */
        TE_FATAL_ERROR("Cannot happen");
    }
    if (fderr >= 0)
        close(fderr);

    tester_set_serial_pid(pid);
    pid = waitpid(pid, code, 0);
    if (pid < 0)
    {
        rc = TE_OS_RC(TE_TESTER, errno);
        ERROR("waitpid failed: %r", rc);
    }
    tester_release_serial_pid();

    if (flags & TESTER_VALGRIND)
    {
        TE_LOG(TE_LL_INFO, TE_LGR_ENTITY, TE_LGR_USER,
               "Standard error output of the script with ID=%u:\n"
               "%Tf", (unsigned int)exec_id, vg_filename);
    }

    return rc;
}

static tester_test_status
translate_script_exit_code(const char *script_name, test_id exec_id,
                           int code)
{
    if (tester_check_serial_stop())
        return TESTER_TEST_STOPPED;

#ifdef WCOREDUMP
    if (WCOREDUMP(code))
    {
        ERROR("ID=%u: executable '%s' dumped core", (unsigned int)exec_id,
              script_name);
        return TESTER_TEST_CORED;
    }
#endif
    if (WIFSIGNALED(code))
    {
        if (WTERMSIG(code) == SIGINT)
        {
            ERROR("ID=%u was interrupted by SIGINT, shut down",
                  (unsigned int)exec_id);
            return TESTER_TEST_STOPPED;
        }
        else
        {
            ERROR("ID=%u was killed by the signal %d : %s",
                  (unsigned int)exec_id,
                  WTERMSIG(code), strsignal(WTERMSIG(code)));
            return TESTER_TEST_KILLED;
        }
    }
    else if (!WIFEXITED(code))
    {
        ERROR("ID=%u was abnormally terminated", (unsigned int)exec_id);
        return TESTER_TEST_FAILED;
    }
    else
    {
        switch (WEXITSTATUS(code))
        {
            case EXIT_FAILURE:
                return TESTER_TEST_FAILED;

            case EXIT_SUCCESS:
                return TESTER_TEST_PASSED;

            case TE_EXIT_SIGUSR2:
            case TE_EXIT_SIGINT:
                ERROR("ID=%u was interrupted by %s, shut down",
                      exec_id,
                      WEXITSTATUS(code) == TE_EXIT_SIGINT ?
                      "SIGINT" : "SIGUSR2");
                return TESTER_TEST_STOPPED;

            case TE_EXIT_NOT_FOUND:
                ERROR("ID=%u was not run, executable not found",
                      (unsigned int)exec_id);
                return TESTER_TEST_SEARCH;

            case TE_EXIT_ERROR:
                ERROR("Serious error occurred during execution of "
                      "the test, shut down");
                return TESTER_TEST_STOPPED;

            case TE_EXIT_SKIP:
                return TESTER_TEST_SKIPPED;

            default:
                WARN("ID=%u: unknown test exit code %d, treating as failure",
                     (unsigned int)exec_id, code);
                return TESTER_TEST_FAILED;
        }
    }
}

/**
 * Run test script in provided context with specified parameters.
 *
 * @param script        Test script to run
 * @param run_name      Run item name or @c NULL
 * @param exec_id       Test execution ID
 * @param n_args        Number of arguments
 * @param args          Arguments to be passed
 * @param flags         Flags
 * @param status        Location for test status
 *
 * @return Status code.
 */
static te_errno
run_test_script(test_script *script, const char *run_name, test_id exec_id,
                const unsigned int n_args, const test_iter_arg *args,
                const tester_flags flags, tester_test_status *status)
{
    int code;
    te_vec params = TE_VEC_INIT_AUTOPTR(char *);
    const char *test_name = run_name != NULL ? run_name : script->name;
    int rand_seed = rand();
    te_errno rc = 0;

    assert(status != NULL);

    ENTRY("name=%s exec_id=%u n_args=%u arg=%p flags=0x%"
          TE_PRINTF_TESTER_FLAGS "x",
          script->name, exec_id, n_args, args, flags);

    if (flags & TESTER_FAKE)
    {
        *status = TESTER_TEST_FAKED;
        RING("Faked with te_test_id=%u te_test_name=\"%s\" te_rand_seed=%d",
             (unsigned int)exec_id, test_name, rand_seed);
        EXIT("%u", *status);

        te_vec_free(&params);
        return 0;
    }

    prepare_test_script_arguments(&params, flags, script, exec_id, test_name,
                                  rand_seed, n_args, args);

    *status = TESTER_TEST_INCOMPLETE;
    rc = execute_test_script(flags, exec_id, te_vec_get(&params, 0), &code);
    if (rc != 0)
    {
        te_vec_free(&params);
        return rc;
    }

    *status = translate_script_exit_code(script->execute, exec_id, code);

    EXIT("%u", *status);

    te_vec_free(&params);
    return 0;
}

#ifdef WITH_TRC
/**
 * This function reads TRC tags from Configurator tree and appends them to the
 * list.
 *
 * @param[out]  trc_tags  List of TRC tags to append to.
 *
 * @return      Status code.
 */
static te_errno
get_trc_tags(tqh_strings *trc_tags)
{
    te_errno      rc = 0;
    te_string     tag = TE_STRING_INIT;
    char         *tag_name = NULL;
    char         *tag_value = NULL;
    unsigned int  num;
    cfg_handle   *handles = NULL;
    cfg_val_type  type = CVT_STRING;
    int           i;

    if (trc_tags == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_find_pattern_fmt(&num, &handles, TE_CFG_TRC_TAGS_FMT, "*");
    if (rc != 0)
    {
        ERROR("%s(): Cannot get the list of TRC tags: %r", __FUNCTION__, rc);
        return rc;
    }

    for (i = 0; i < num; i++)
    {
        rc = cfg_get_inst_name(handles[i], &tag_name);
        if (rc != 0)
        {
            ERROR("%s(): Cannot get TRC tag name by its handle %d: %r",
                  __FUNCTION__, handles[i], rc);
            goto cleanup;
        }

        rc = cfg_get_instance(handles[i], &type, &tag_value);
        if (rc != 0)
        {
            ERROR("%s(): Cannot get TRC tag value by its handle %d: %r",
                  __FUNCTION__, handles[i], rc);
            free(tag_name);
            goto cleanup;
        }

        if (strlen(tag_value) > 0)
            te_string_append(&tag, "%s:%s", tag_name, tag_value);
        else
            te_string_append(&tag, "%s", tag_name);
        free(tag_value);
        free(tag_name);

        rc = trc_add_tag(trc_tags, te_string_value(&tag));
        te_string_reset(&tag);
        if (rc != 0)
        {
            ERROR("%s(): Failed to add TRC tag: %r", __FUNCTION__, rc);
            goto cleanup;
        }
    }

cleanup:
    free(handles);
    te_string_free(&tag);

    return rc;
}
#endif /* WITH_TRC */

static tester_cfg_walk_ctl
run_script(run_item *ri, test_script *script,
           unsigned int cfg_id_off, void *opaque)
{
    tester_run_data        *gctx = opaque;
    tester_ctx             *ctx;
    tester_cfg_walk_ctl     ctl;
    tester_flags            def_flags = (gctx->flags & TESTER_FAKE) ?
                                            TESTER_FAKE : 0;

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%" TE_PRINTF_TESTER_FLAGS "x) "
          "act_id=%u script=%s", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? (gctx->act->flags | def_flags) : def_flags,
          gctx->act_id, script->name);

    if (ctx->flags & (TESTER_PRERUN | TESTER_ASSEMBLE_PLAN))
    {
        EXIT("CONT");
        ctx->current_result.status = TESTER_TEST_PASSED;
        return TESTER_CFG_WALK_CONT;
    }

    if (ctx->flags & TESTER_FAIL_ON_LEAK)
        def_flags |= TESTER_FAIL_ON_LEAK;

    assert(ri != NULL);
    assert(ri->n_args == ctx->n_args);
    if (run_test_script(script, ri->name, ctx->current_result.id,
                        ctx->n_args, ctx->args,
                        gctx->act == NULL ? def_flags : /* FIXME */
                           (gctx->act->flags | def_flags),
                        &ctx->current_result.status) != 0)
    {
        ctx->current_result.status = TESTER_TEST_ERROR;
    }

    switch (ctx->current_result.status)
    {
        case TESTER_TEST_FAKED:
        case TESTER_TEST_PASSED:
        case TESTER_TEST_FAILED:
        case TESTER_TEST_SEARCH:
        case TESTER_TEST_SKIPPED:
            ctl = TESTER_CFG_WALK_CONT;
            break;

        case TESTER_TEST_KILLED:
        case TESTER_TEST_CORED:
            ctl = TESTER_CFG_WALK_EXC;
            break;

        case TESTER_TEST_STOPPED:
            ctl = TESTER_CFG_WALK_STOP;
            break;

        default:
            ctl = TESTER_CFG_WALK_FAULT;
            break;
    }

    EXIT("%u", ctl);
    return ctl;
}


/**
 * Create (if it is necessary) configuration backup in specified context.
 *
 * @param ctx           Context
 * @param track_conf    'track_conf' attribute of the run item
 *
 * @return Status code.
 *
 * @note Status code in context is updated as well.
 */
static te_errno
run_create_cfg_backup(tester_ctx *ctx, unsigned int track_conf)
{
    if ((~ctx->flags & TESTER_NO_CFG_TRACK) &&
        (track_conf & TESTER_TRACK_CONF_ENABLED))
    {
        te_errno rc;

        /* Create backup to be verified after each iteration */
        rc = cfg_create_backup(&ctx->backup);
        if (rc != 0)
        {
            ERROR("Cannot create configuration backup: %r", rc);
            ctx->group_result.status = TESTER_TEST_ERROR;
            EXIT("FAULT");
            return rc;
        }
        ctx->backup_ok = true;
    }

    return 0;
}

/**
 * Verify configuration backup in the specified context.
 *
 * @param ctx           Context
 * @param track_conf    'track_conf' attribute of the run item
 *
 * @note Status code in context is updated in the case of failure.
 */
static void
run_verify_cfg_backup(tester_ctx *ctx, unsigned int track_conf)
{
    te_errno    rc;

    if (!ctx->backup_ok && ctx->backup != NULL)
    {
        /*
         * Syncing is time-consuming, so it is not enabled by
         * default for all tests.
         */
        if (track_conf & TESTER_TRACK_CONF_SYNC)
            cfg_synchronize("/:", true);

        /* Check configuration backup */
        rc = cfg_verify_backup(ctx->backup);
        if (TE_RC_GET_ERROR(rc) == TE_EBACKUP ||
            TE_RC_GET_ERROR(rc) == TE_ETADEAD)
        {
            if (track_conf & TESTER_TRACK_CONF_MARK_DIRTY)
            {
                /* If backup is not OK, restore it */
                WARN("Current configuration differs from backup - "
                     "restore");
            }
            rc = (track_conf & TESTER_TRACK_CONF_ROLLBACK_HISTORY) ?
                  cfg_restore_backup(ctx->backup) :
                  cfg_restore_backup_nohistory(ctx->backup);
            if (rc != 0)
            {
                ERROR("Cannot restore configuration backup: %r", rc);
                ctx->current_result.status = TESTER_TEST_ERROR;
            }
            else if (track_conf & TESTER_TRACK_CONF_MARK_DIRTY)
            {
                RING("Configuration successfully restored using backup");
                if (ctx->current_result.status < TESTER_TEST_DIRTY)
                    ctx->current_result.status = TESTER_TEST_DIRTY;
            }
            else
            {
                ctx->backup_ok = true;
            }
        }
        else if (rc != 0)
        {
            ERROR("Cannot verify configuration backup: %r", rc);
            ctx->current_result.status = TESTER_TEST_ERROR;
        }
        else
        {
            ctx->backup_ok = true;
        }
    }
}

/**
 * Release configuration backup in the specified context.
 *
 * @param ctx           Context
 *
 * @return Status code.
 *
 * @note Status code in context is updated in the case of failure.
 */
static te_errno
run_release_cfg_backup(tester_ctx *ctx)
{
    te_errno    rc = 0;

    if (ctx->backup != NULL)
    {
        rc = cfg_release_backup(&ctx->backup);
        if (rc != 0)
        {
            ERROR("cfg_release_backup() failed: %r", rc);
            ctx->group_result.status = TESTER_TEST_ERROR;
            ctx->backup = NULL;
        }
        else
        {
            assert(ctx->backup == NULL);
        }
    }
    return rc;
}


static tester_cfg_walk_ctl
run_cfg_start(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    char               *maintainers;

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (gctx->act_id >= cfg_id_off + cfg->total_iters)
    {
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    if (!(ctx->flags & (TESTER_PRERUN | TESTER_ASSEMBLE_PLAN)))
    {
        maintainers = persons_info_to_string(&cfg->maintainers);
        RING("Running configuration:\n"
             "File:        %s\n"
             "Maintainers:%s\n"
             "Description: %s",
             cfg->filename, maintainers,
             cfg->descr != NULL ? cfg->descr : "(no description)");
        free(maintainers);
    }

    /*
     * Process options. Put options in appropriate context.
     */
    if (!TAILQ_EMPTY(&cfg->options))
        WARN("Options in Tester configuration files are ignored.");

    /* Clone Tester context */
    ctx = tester_run_more_ctx(gctx, false);

    if (cfg->targets != NULL)
    {
        if (ctx->targets != NULL)
        {
            /* Add configuration to context requirements */
            ctx->targets = logic_expr_binary(LOGIC_EXPR_AND,
                                             (logic_expr *)ctx->targets,
                                             cfg->targets);
            if (ctx->targets == NULL)
            {
                tester_run_destroy_ctx(gctx);
                ctx->current_result.status = TESTER_TEST_ERROR;
                return TESTER_CFG_WALK_FAULT;
            }
            ctx->targets_free = true;
        }
        else
        {
            ctx->targets = cfg->targets;
            ctx->targets_free = false;
        }
    }

    EXIT();
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_cfg_end(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;

    UNUSED(cfg);

    assert(gctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    tester_run_destroy_ctx(gctx);

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

/**
 * Should this run item be run?
 *
 * @param cfg_id_off    Current offset of the configuration ID
 * @param act_id        Action ID to be run
 * @param weight        Weight of single iterations
 * @param n_iters       Number of iterations
 *
 * @retval TESTING_STOP         This item should be run
 * @retval TESTING_BACKWARD     Something before this item should be run
 * @retval TESTING_FORWARD      Something after this item should be run
 */
static testing_direction
run_this_item(const unsigned int cfg_id_off, const unsigned int act_id,
              const unsigned int weight, const unsigned int n_iters)
{
    VERB("%s(): act_id=%u cfg_id_off=%u weight=%u n_iters=%u",
         __FUNCTION__, act_id, cfg_id_off, weight, n_iters);

    if (act_id < cfg_id_off)
        return TESTING_BACKWARD;
    else if (act_id >= cfg_id_off + n_iters * weight)
        return TESTING_FORWARD;
    else
        return TESTING_STOP;
}

static tester_cfg_walk_ctl
run_item_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
               void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

#if WITH_TRC
    ctx->do_trc_walker = false;
#endif

    if (~flags & TESTER_CFG_WALK_SERVICE)
    {
        if (tester_sigint_received)
        {
            ctx->current_result.status = TESTER_TEST_STOPPED;
            return TESTER_CFG_WALK_STOP;
        }

        gctx->direction = run_this_item(cfg_id_off, gctx->act_id,
                                        ri->weight, ri->n_iters);
        switch (gctx->direction)
        {
            case TESTING_FORWARD:
                EXIT("SKIP");
                return TESTER_CFG_WALK_SKIP;

            case TESTING_BACKWARD:
                EXIT("BACK");
                return TESTER_CFG_WALK_BACK;

            case TESTING_STOP:
                /* Run here */
                break;

            default:
                assert(false);
                ctx->current_result.status = TESTER_TEST_ERROR;
                return TESTER_CFG_WALK_FAULT;
        }

    }

    if (!(gctx->flags & (TESTER_FAKE | TESTER_PRERUN | TESTER_ASSEMBLE_PLAN)))
        start_cmd_monitors(&ri->cmd_monitors);

    if (~flags & TESTER_CFG_WALK_SERVICE)
    {
        if (ctx->backup == NULL)
            run_create_cfg_backup(ctx, test_get_attrs(ri)->track_conf);
    }

    assert(ctx->args == NULL);
    if (ri->n_args > 0)
    {
        unsigned int    i;

        ctx->args = TE_ALLOC(ri->n_args * sizeof(*ctx->args));
        ctx->n_args = ri->n_args;
        for (i = 0; i < ctx->n_args; ++i)
            TAILQ_INIT(&ctx->args[i].reqs);
    }

#if WITH_TRC
    if ((~ctx->flags & TESTER_NO_TRC) && (test_get_name(ri) != NULL))
    {
        trc_db_walker_step_test(ctx->trc_walker, test_get_name(ri), false);
        ctx->do_trc_walker = true;
    }
#endif

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_item_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
             void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);
    UNUSED(flags);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (!(gctx->flags & (TESTER_FAKE | TESTER_PRERUN | TESTER_ASSEMBLE_PLAN)))
        stop_cmd_monitors(&ri->cmd_monitors);

#if WITH_TRC
    if (ctx->do_trc_walker && test_get_name(ri) != NULL)
        trc_db_walker_step_back(ctx->trc_walker);
    else if (~ctx->flags & TESTER_NO_TRC)
        ctx->do_trc_walker = true;
#endif

    ctx->n_args = 0;
    free(ctx->args);
    ctx->args = NULL;

    if (run_release_cfg_backup(ctx) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_pkg_start(run_item *ri, test_package *pkg,
              unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);

    assert(gctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    ctx = tester_run_more_ctx(gctx, true);

    assert(~ctx->flags & TESTER_INLOGUE);

    tester_get_sticky_reqs(&ctx->reqs, &pkg->reqs);

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_session_start(run_item *ri, test_session *session,
                  unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);
    UNUSED(session);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    /*
     * If it is a standalone session (not a primary session of a test
     * package), create own context. Otherwise, context has been
     * created in run_pkg_start() routine.
     */
    if (ri->type == RUN_ITEM_SESSION)
        ctx = tester_run_more_ctx(gctx, true);

    tester_get_sticky_reqs(&ctx->reqs, &session->reqs);

#if WITH_TRC
    if (~ctx->flags & TESTER_NO_TRC)
    {
        if (run_item_has_keepalive(ri))
        {
            /*
             * Save current position in TRC DB to know later where to
             * look for keepalive test when it is run between other
             * tests' iterations.
             */
            ctx->keepalive_walker = trc_db_walker_copy(ctx->trc_walker);
            assert(ctx->keepalive_walker != NULL);
        }
    }
#endif

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_session_end(run_item *ri, test_session *session,
                unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);
    UNUSED(session);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

#if WITH_TRC
    if (~ctx->flags & TESTER_NO_TRC)
    {
        if (run_item_has_keepalive(ri))
        {
            trc_db_free_walker(ctx->keepalive_walker);
            ctx->keepalive_walker = NULL;
        }
    }
#endif

    tester_run_destroy_ctx(gctx);

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_prologue_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (ctx->flags & TESTER_NO_LOGUES)
    {
        WARN("Prologues are disabled globally");
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    ctx = tester_run_more_ctx(gctx, false);

    VERB("Running test session prologue...");
    ctx->flags |= TESTER_INLOGUE;

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_prologue_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            status;
    test_id             id;
    te_errno            rc;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    assert(ctx->flags & TESTER_INLOGUE);
    status = ctx->group_result.status;
    id = ctx->current_result.id;
    tester_run_destroy_ctx(gctx);

    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    if (status == TESTER_TEST_PASSED)
    {
        if (!(ctx->flags & (TESTER_PRERUN | TESTER_ASSEMBLE_PLAN)))
        {
            char *reqs = NULL;

            rc = cfg_get_instance_string_fmt(&reqs, "/local:/reqs:%u", id);

            if (rc == 0)
            {
                rc = logic_expr_parse(reqs, &ctx->dyn_targets);
                if (rc != 0)
                {
                    ERROR("Failed to parse target requirements expression "
                          "populated by test with ID=%u: %r", id, rc);
                    ctx->group_result.status = TESTER_TEST_PROLOG;
                    assert(SLIST_NEXT(ctx, links) != NULL);
                    SLIST_NEXT(ctx, links)->group_step = true;
                    EXIT("SKIP");
                    return TESTER_CFG_WALK_SKIP;
                }

                if (ctx->targets != NULL)
                {
                    ctx->targets = logic_expr_binary(LOGIC_EXPR_AND,
                                                     (logic_expr *)ctx->targets,
                                                     ctx->dyn_targets);
                    ctx->targets_free = true;
                }
                else
                {
                    ctx->targets = ctx->dyn_targets;
                    ctx->targets_free = false;
                }

                if (ctx->targets == NULL)
                {
                    tester_run_destroy_ctx(gctx);
                    ctx->group_result.status = TESTER_TEST_ERROR;
                    EXIT("FAULT");
                    return TESTER_CFG_WALK_FAULT;
                }
            }
            else if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
            {
                ERROR("Get of /local:/reqs:%u failed unexpectedly: %r",
                      id, rc);
                ctx->group_result.status = TESTER_TEST_ERROR;
                EXIT("FAULT");
                return TESTER_CFG_WALK_FAULT;
            }
        }
    }
    else if (status != TESTER_TEST_FAKED)
    {
        if (status == TESTER_TEST_SKIPPED)
            ctx->group_result.status = TESTER_TEST_SKIPPED;
        else if (status != TESTER_TEST_EMPTY)
            ctx->group_result.status = TESTER_TEST_PROLOG;
        assert(SLIST_NEXT(ctx, links) != NULL);
        SLIST_NEXT(ctx, links)->group_step = true;
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    if (!(ctx->flags & (TESTER_NO_CS | TESTER_NO_CFG_TRACK)))
    {
        rc = cfg_synchronize("/:", true);
        if (rc != 0)
            ERROR("%s(): cfg_synchronize() failed returning %r",
                  __FUNCTION__, rc);
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_epilogue_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (ctx->flags & TESTER_NO_LOGUES)
    {
        WARN("Epilogues are disabled globally");
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    if (ctx->flags & TESTER_PRERUN)
    {
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    ctx = tester_run_more_ctx(gctx, false);

    VERB("Running test session epilogue...");
    ctx->flags |= TESTER_INLOGUE;

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_epilogue_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            status;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (ctx->flags & TESTER_PRERUN)
    {
        EXIT("CONT");
        return TESTER_CFG_WALK_CONT;
    }

    assert(ctx->flags & TESTER_INLOGUE);
    status = ctx->current_result.status;
    tester_run_destroy_ctx(gctx);

    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    if ((status != TESTER_TEST_PASSED) && (status != TESTER_TEST_FAKED))
    {
        if (status == TESTER_TEST_SKIPPED)
            ctx->group_result.status = TESTER_TEST_SKIPPED;
        else if (status != TESTER_TEST_EMPTY)
            ctx->group_result.status = TESTER_TEST_EPILOG;
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_keepalive_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (ctx->flags & TESTER_PRERUN)
    {
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    if (ctx->keepalive_ctx == NULL)
        ctx->keepalive_ctx = tester_run_clone_ctx(ctx, false);

    ctx = ctx->keepalive_ctx;

    if ((~ctx->flags & TESTER_ASSEMBLE_PLAN) &&
        run_create_cfg_backup(ctx, test_get_attrs(ri)->track_conf) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    SLIST_INSERT_HEAD(&gctx->ctxs, ctx, links);

#if WITH_TRC
    if (~ctx->flags & TESTER_NO_TRC)
    {
        /*
         * keepalive handler is run before every iteration of any test
         * in a session. To match TRC DB for it correctly, we need
         * to go back to session level.
         */

        ctx->trc_walker = ctx->keepalive_walker;
        ctx->do_trc_walker = true;
    }
#endif

    VERB("Running test session keep-alive validation...");

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_keepalive_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    tester_test_status  status;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (ctx->flags & TESTER_PRERUN)
    {
        EXIT("CONT");
        return TESTER_CFG_WALK_CONT;
    }

    /* Release configuration backup, since it may differ the next time */
    if ((~ctx->flags & TESTER_ASSEMBLE_PLAN) &&
        run_release_cfg_backup(ctx) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    /* Remember status of the keep-alive validation */
    status = ctx->group_result.status;

    /* Remove keep-alive context (it is still stored in keepalive_ctx) */
    SLIST_REMOVE(&gctx->ctxs, ctx, tester_ctx, links);

    /* Get current context and update its group status */
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    if ((gctx->force_skip == 0) &&
        (status != TESTER_TEST_PASSED) &&
        (status != TESTER_TEST_FAKED))
    {
        ERROR("Keep-alive validation failed: %u", status);
        ctx->group_result.status =
            tester_group_status(ctx->group_result.status,
                                TESTER_TEST_KEEPALIVE);
        EXIT("INTR");
        return TESTER_CFG_WALK_INTR;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_exception_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    assert(gctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (gctx->flags & (TESTER_PRERUN | TESTER_ASSEMBLE_PLAN))
    {
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    /* Exception handler is always run in a new context */
    ctx = tester_run_more_ctx(gctx, false);

    /* Create configuration backup in a new context */
    if (run_create_cfg_backup(ctx, test_get_attrs(ri)->track_conf) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    VERB("Running test session exception handler...");
    gctx->exception++;

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_exception_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            status;

    UNUSED(ri);

    assert(gctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (gctx->flags & (TESTER_PRERUN | TESTER_ASSEMBLE_PLAN))
    {
        EXIT("CONT");
        return TESTER_CFG_WALK_CONT;
    }

    gctx->exception--;

    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    /* Release configuration backup, since it may differ the next time */
    if (run_release_cfg_backup(ctx) != 0)
    {
        tester_run_destroy_ctx(gctx);
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    /* Remember status of the exception handler */
    status = ctx->group_result.status;

    /* Destroy exception handler context */
    tester_run_destroy_ctx(gctx);

    /* Get current context and update its group status */
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    if ((status != TESTER_TEST_PASSED) && (status != TESTER_TEST_FAKED))
    {
        ERROR("Exception handler failed: %r", status);
        ctx->group_result.status =
            tester_group_status(ctx->group_result.status,
                                TESTER_TEST_EXCEPTION);
        EXIT("INTR");
        return TESTER_CFG_WALK_INTR;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}


/**
 * Get plain value of the aggregated argument value.
 *
 * @param value         Aggregated argument value
 * @param args          Context arguments
 * @param n_args        Number of arguments in the context
 * @param arg           Target argument
 */
static const char *
run_get_value(const test_entity_value *value,
              const test_iter_arg *args, const unsigned int n_args,
              test_iter_arg *arg)
{
    VERB("%s(): name=%s plain=%p ref=%p ext=%p global=%s",
         __FUNCTION__, value->name, value->plain, value->ref,
         value->ext, value->global ? "true" : "false");

    if (value->plain != NULL)
    {
        VERB("%s(): plain", __FUNCTION__);
        return (value->global && value->name) ?
            value->name : value->plain;
    }
    else if (value->ref != NULL)
    {
        VERB("%s(): ref to %p", __FUNCTION__, value->ref);
        return run_get_value(value->ref, args, n_args, arg);
    }
    else if (value->ext != NULL)
    {
        unsigned int i;

        VERB("%s(): ext to %s", __FUNCTION__, value->ext);

        for (i = 0;
             i < n_args && strcmp(value->ext, args[i].name) != 0;
             ++i);

        if (i < n_args)
        {
            test_requirements_clone(&args[i].reqs, &arg->reqs);

            if (args[i].objective != NULL)
                arg->objective = args[i].objective;

            return args[i].value;
        }
        else
        {
            ERROR("Failed to get argument value by external "
                  "reference '%s'", value->ext);
            return NULL;
        }
    }
    else
    {
        assert(false);
        return NULL;
    }
}


/**
 * Callback function to collect all requirements and objective associated
 * with argument value.
 *
 * The function complies with test_entity_value_enum_error_cb()
 * prototype. Always returns @c 0.
 */
static te_errno
run_prepare_arg_value_collect_reqs(const test_entity_value *value,
                                   te_errno status, void *opaque)
{
    test_iter_arg *arg = opaque;

    if (TE_RC_GET_ERROR(status) == TE_EEXIST)
    {
        test_requirements_clone(&value->reqs, &arg->reqs);

        /*
         * Do not set to NULL - it may hide objective set in enumeration
         * type when processing test arguments of that type.
         */
        if (value->objective != NULL)
            arg->objective = value->objective;
    }

    return 0;
}

/** Information about length of the list */
typedef struct run_prepare_arg_list_data {
    SLIST_ENTRY(run_prepare_arg_list_data)  links;  /**< List links */

    const char     *name;   /**< Name of the list */
    unsigned int    index;  /**< Index of the value */
} run_prepare_arg_list_data;

/** Data to be passed as opaque to run_prepare_arg_cb() function. */
typedef struct run_prepare_arg_cb_data {
    const test_iter_arg    *ctx_args;   /**< Context of arguments */
    unsigned int            ctx_n_args; /**< Number of arguments
                                             in the context */

    const run_item *ri;         /**< Run item context */
    unsigned int    n_iters;    /**< Remaining number of iterations */
    unsigned int    i_iter;     /**< Index of the required iteration */
    test_iter_arg  *arg;        /**< The next argument to be filled in */

    SLIST_HEAD(, run_prepare_arg_list_data) lists;  /**< Lists */

} run_prepare_arg_cb_data;

/**
 * Function to be called for each argument (explicit or inherited) to
 * calculate total number of iterations.
 *
 * The function complies with test_var_arg_enum_cb prototype.
 */
static te_errno
run_prepare_arg_cb(const test_var_arg *va, void *opaque)
{
    run_prepare_arg_cb_data        *data = opaque;
    test_var_arg_list              *ri_list = NULL;
    run_prepare_arg_list_data      *iter_list = NULL;
    unsigned int                    n_values;
    unsigned int                    i_value;
    const test_entity_value        *value;
    te_errno                        rc;

    data->arg->name = va->name;

    if (va->list != NULL)
    {
        for (ri_list = SLIST_FIRST(&data->ri->lists);
             ri_list != NULL && strcmp(ri_list->name, va->list) != 0;
             ri_list = SLIST_NEXT(ri_list, links));

        assert(ri_list != NULL);

        for (iter_list = SLIST_FIRST(&data->lists);
             iter_list != NULL && strcmp(iter_list->name, va->list) != 0;
             iter_list = SLIST_NEXT(iter_list, links));
    }

    if (iter_list != NULL)
    {
        i_value = iter_list->index;
        VERB("%s(): Index of the value of '%s' to get is '%u' because of "
             "the list '%s'", __FUNCTION__, va->name, i_value,
             iter_list->name);
    }
    else
    {
        n_values = (ri_list == NULL) ? test_var_arg_values(va)->num :
                                       ri_list->len;

        assert(data->n_iters % n_values == 0);
        data->n_iters /= n_values;

        i_value = data->i_iter / data->n_iters;
        data->i_iter %= data->n_iters;

        if (ri_list != NULL)
        {
            iter_list = TE_ALLOC(sizeof(*iter_list));

            iter_list->name = ri_list->name;
            iter_list->index = i_value;
            SLIST_INSERT_HEAD(&data->lists, iter_list, links);
        }
        VERB("%s(): Index of the value of '%s' to get is %u -> "
             "n_iters=%u i_iter=%u", __FUNCTION__, va->name, i_value,
             data->n_iters, data->i_iter);
    }

    rc = test_var_arg_get_value(data->ri, va, i_value,
                                run_prepare_arg_value_collect_reqs,
                                data->arg,
                                &value);
    if (rc != 0)
    {
        data->arg->value = "[FAILED TO GET ARGUMENT VALUE]";
        goto callback_return;
    }

    VERB("%s: value name=%s ref=%p ext=%p plain=%s", __FUNCTION__,
         value->name, value->ref, value->ext, value->plain);

    data->arg->variable = va->variable;
    data->arg->value = run_get_value(value, data->ctx_args,
                                     data->ctx_n_args, data->arg);
    if (data->arg->value == NULL)
    {
        ERROR("Unable to get value of the argument of the run item '%s'",
              run_item_name(data->ri));
        data->arg->value = "[FAILED TO GET ARGUMENT VALUE]";
        rc = TE_RC(TE_TESTER, TE_ESRCH);
        goto callback_return;
    }

    VERB("%s(): arg=%s run_get_value() -> %s reqs=%p", __FUNCTION__,
         data->arg->name, data->arg->value, TAILQ_FIRST(&data->arg->reqs));

    rc = 0;

callback_return:
    /* Move to the next argument */
    data->arg++;

    return rc;
}

/**
 * Prepare arguments to be passed in executable.
 *
 * @param ri            Run item
 * @param i_iter        Index of the iteration to prepare
 * @param args          Location for arguments
 *
 * @return Status code.
 */
static te_errno
run_prepare_args(const test_iter_arg *ctx_args,
                 const unsigned int ctx_n_args,
                 const run_item *ri, unsigned i_iter,
                 test_iter_arg *args)
{
    run_prepare_arg_cb_data     data;
    run_prepare_arg_list_data  *p;
    te_errno                    rc;

    data.ctx_args = ctx_args;
    data.ctx_n_args = ctx_n_args;
    data.ri = ri;
    data.n_iters = ri->n_iters;
    data.i_iter = i_iter;
    data.arg = args;
    SLIST_INIT(&data.lists);

    rc = test_run_item_enum_args(ri, run_prepare_arg_cb, false, &data);
    if (!(rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT))
        rc = 0;

    while ((p = SLIST_FIRST(&data.lists)) != NULL)
    {
        SLIST_REMOVE(&data.lists, p, run_prepare_arg_list_data, links);
        free(p);
    }

    return rc;
}

static tester_cfg_walk_ctl
run_iter_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
               unsigned int iter, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    tester_ctx         *parent_ctx;
    te_errno            rc;
    bool args_preparation_fail = false;

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    parent_ctx = SLIST_NEXT(ctx, links);
    if (flags & TESTER_CFG_WALK_SERVICE)
    {
        assert(parent_ctx != NULL);
        parent_ctx = SLIST_NEXT(parent_ctx, links);
    }
    LOG_WALK_ENTRY(cfg_id_off, gctx);

#if WITH_TRC
    ctx->do_trc_walker = false;
#endif

    if (~flags & TESTER_CFG_WALK_SERVICE)
    {
        if (tester_sigint_received)
        {
            ctx->current_result.status = TESTER_TEST_STOPPED;
            return TESTER_CFG_WALK_STOP;
        }

        gctx->direction = run_this_item(cfg_id_off, gctx->act_id,
                                        ri->weight, 1);
        switch (gctx->direction)
        {
            case TESTING_FORWARD:
                EXIT("SKIP");
                return TESTER_CFG_WALK_SKIP;

            case TESTING_BACKWARD:
                EXIT("BACK");
                return TESTER_CFG_WALK_BACK;

            case TESTING_STOP:
                /* Run here */
                break;

            default:
                assert(false);
                ctx->current_result.status = TESTER_TEST_ERROR;
                return TESTER_CFG_WALK_FAULT;
        }
    }

    /* Fill in arguments */
    assert(ri->n_args == ctx->n_args);
    assert((ctx->args == NULL) == (ctx->n_args == 0));
    if (ctx->n_args > 0)
    {
        rc = run_prepare_args(parent_ctx != NULL ? parent_ctx->args : NULL,
                              parent_ctx != NULL ? parent_ctx->n_args : 0,
                              ri, iter, ctx->args);
        if (rc != 0)
            args_preparation_fail = true;
    }

    /*
     * If some argument is not resolved it is set with an error message.
     * Preserve it when such arguments are resolved within subsessions.
     */
    if (flags & TESTER_CFG_WALK_SKIP_REPEAT)
        args_preparation_fail = true;

#if WITH_TRC
    if ((~ctx->flags & TESTER_NO_TRC) && (test_get_name(ri) != NULL))
    {
        trc_report_argument args[ctx->n_args];
        unsigned int    i;

        for (i = 0; i < ctx->n_args; ++i)
        {
            args[i].name = (char *)ctx->args[i].name;
            args[i].value = (char *)ctx->args[i].value;
            args[i].variable = ctx->args[i].variable;
        }
        /*
         * It is guaranteed that trc_db_walker_step_iter() does not
         * touch names and values with the last parameters equal to
         * false/0/NULL.
         */
        (void)trc_db_walker_step_iter(ctx->trc_walker,
                                      ctx->n_args,
                                      args, 0,
                                      0, NULL);
        ctx->do_trc_walker = true;
    }

    /* Initialize as unknown by default */
    ctx->current_result.exp_result = NULL;
    ctx->current_result.exp_status = TRC_VERDICT_UNKNOWN;
#endif

    if (args_preparation_fail)
    {
        ctx->current_result.status = TESTER_TEST_FAILED;
        EXIT("EARGS");
        return TESTER_CFG_WALK_EARGS;
    }
    else
    {
        EXIT("CONT");
        return TESTER_CFG_WALK_CONT;
    }
}

static tester_cfg_walk_ctl
run_iter_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
             unsigned int iter, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    unsigned int        i;

    UNUSED(flags);
    UNUSED(iter);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

#if WITH_TRC
    if (ctx->do_trc_walker && test_get_name(ri) != NULL)
        trc_db_walker_step_back(ctx->trc_walker);
    else if (~ctx->flags & TESTER_NO_TRC)
        ctx->do_trc_walker = true;
#endif

    /* TODO: Optimize arguments fill in */
    assert(ri->n_args == ctx->n_args);
    for (i = 0; i < ctx->n_args; ++i)
    {
        test_requirements_free(&ctx->args[i].reqs);
        ctx->args[i].objective = NULL;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}


static tester_cfg_walk_ctl
run_repeat_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
                 void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    unsigned int        tin;
    char               *hash_str;
    te_errno            rc;

    UNUSED(flags);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (ri->type == RUN_ITEM_SCRIPT && gctx->act != NULL &&
        gctx->act->hash != NULL && ~ctx->flags & TESTER_INLOGUE)
    {
        hash_str = test_params_hash(ctx->args, ri->n_args);

        if (strcmp(hash_str, gctx->act->hash) != 0)
        {
            ctx->current_result.status = TESTER_TEST_INCOMPLETE;
            ctx->group_step = true;
            free(hash_str);
            return TESTER_CFG_WALK_SKIP;
        }
        free(hash_str);
    }

    if (ctx->flags & TESTER_ASSEMBLE_PLAN)
    {
        bool quiet_skip;
        bool required;

        /* verb overwrites quiet */
        quiet_skip = (~ctx->flags & TESTER_VERB_SKIP) &&
                     (ctx->flags & TESTER_QUIET_SKIP);
        required = tester_is_run_required(ctx->targets, &ctx->reqs,
                                          ri, ctx->args, ctx->flags,
                                          true);
        ri->plan_id = -1;
        if (required || !quiet_skip)
        {
            rc = tester_plan_register_run_item(&gctx->plan, ri, ctx);
            if (rc != 0)
            {
                ERROR("Failed to register run item");
                EXIT("FAULT");
                return TESTER_CFG_WALK_FAULT;
            }
        }
        else if (!required)
        {
            if (quiet_skip)
            {
                rc = tester_plan_add_skipped(&gctx->plan);
                if (rc != 0)
                {
                    ERROR("Failed to add \"skipped\" package");
                    EXIT("FAULT");
                    return TESTER_CFG_WALK_FAULT;
                }
            }
            if (run_item_container(ri))
            {
                rc = tester_plan_add_ignore(&gctx->plan);
                if (rc != 0)
                {
                    ERROR("Failed to add \"ignore\" package");
                    EXIT("FAULT");
                    return TESTER_CFG_WALK_FAULT;
                }
            }
        }
        if (required)
        {
            EXIT("CONT");
            return TESTER_CFG_WALK_CONT;
        }
        else
        {
            ctx->current_result.status = TESTER_TEST_INCOMPLETE;
            ctx->group_step = true;
            EXIT("SKIP");
            return TESTER_CFG_WALK_SKIP;
        }
    }

    if (ctx->flags & TESTER_PRERUN)
    {
        if (!(ctx->flags & TESTER_ONLY_REQ_LOGUES) ||
            tester_is_run_required(ctx->targets, &ctx->reqs,
                                   ri, ctx->args, ctx->flags,
                                   true))
        {
            if (ri->type == RUN_ITEM_SCRIPT &&
                (~flags & TESTER_CFG_WALK_SERVICE))
            {
                scenario_add_act(&gctx->fixed_scen, cfg_id_off, cfg_id_off,
                                 gctx->act->flags, gctx->act->hash);
            }

            EXIT("CONT");
            return TESTER_CFG_WALK_CONT;
        }
        else
        {
            ctx->current_result.status = TESTER_TEST_INCOMPLETE;
            ctx->group_step = true;
            EXIT("SKIP");
            return TESTER_CFG_WALK_SKIP;
        }
    }

    /*
     * Increment current plan reference ID if not handling any exceptions
     * and current item is mentioned in the plan
     */
    if (gctx->exception == 0 &&
        ((ctx->flags & TESTER_VERB_SKIP) ||
         (~ctx->flags & TESTER_QUIET_SKIP) ||
         tester_is_run_required(ctx->targets, &ctx->reqs,
                                ri, ctx->args, ctx->flags,
                                true)))
        gctx->plan_id++;

    /* Go inside skipped packages and sessions */
    if (gctx->force_skip > 0 && run_item_container(ri) &&
        tester_is_run_required(ctx->targets, &ctx->reqs,
                                    ri, ctx->args, ctx->flags,
                                    true))
    {
        ctx->current_result.status = TESTER_TEST_INCOMPLETE;
        ctx->group_step = true;
        EXIT("CONT");
        return TESTER_CFG_WALK_CONT;
    }

    /* FIXME: Optimize */
    /* verb overwrites quiet */
    if ((gctx->force_skip > 0) ||
        ((~ctx->flags & TESTER_VERB_SKIP) &&
         (ctx->flags & TESTER_QUIET_SKIP) &&
         !tester_is_run_required(ctx->targets, &ctx->reqs,
                                 ri, ctx->args, ctx->flags, true)))
    {
        /* Silently skip without any logs */
        ctx->current_result.status = TESTER_TEST_INCOMPLETE;
        ctx->group_step = true;
        EXIT("SKIP - ENOENT");
        return TESTER_CFG_WALK_SKIP;
    }

    ctx->current_result.id = tester_get_id();

    ri->plan_id = gctx->exception == 0 ? gctx->plan_id - 1 : -1;

    tin = (ctx->flags & TESTER_INLOGUE || ri->type != RUN_ITEM_SCRIPT) ?
              TE_TIN_INVALID : cfg_id_off;
    /* Test is considered here as run, if such event is logged */
    tester_term_out_start(ctx->flags, ri->type, run_item_name(ri), tin,
                          ctx->group_result.id, ctx->current_result.id);
    log_test_start(flags, ctx, ri, tin);

    tester_test_result_add(&gctx->results, &ctx->current_result);

    /* FIXME: Optimize */
    if (((ctx->flags & TESTER_VERB_SKIP) ||
        (~ctx->flags & TESTER_QUIET_SKIP)) &&
        !tester_is_run_required(ctx->targets, &ctx->reqs,
                                ri, ctx->args, ctx->flags, false))
    {
        ctx->current_result.status = TESTER_TEST_SKIPPED;
        ctx->group_step = true;
        EXIT("SKIP - TESTER_TEST_SKIPPED");
        return TESTER_CFG_WALK_SKIP;
    }
    else
    {
        ctx->backup_ok = false;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

#if WITH_TRC
/**
 * Log TE test result to provided buffer.
 *
 * @param lb            Log buffer
 * @param result        Result to be appended
 */
static void
te_test_result_to_log_buf(te_log_buf *lb, const te_test_result *result)
{
    te_test_verdict *v;

    te_log_buf_append(lb, "%s%s\n",
                      te_test_status_to_str(result->status),
                      TAILQ_EMPTY(&result->verdicts) ? "" :
                          " with verdicts:");
    TAILQ_FOREACH(v, &result->verdicts, links)
    {
        te_log_buf_append(lb, "%s;\n", v->str);
    }

    if (!TAILQ_EMPTY(&result->artifacts))
    {
        te_log_buf_append(lb, "\nArtifacts:\n");
        TAILQ_FOREACH(v, &result->artifacts, links)
        {
            te_log_buf_append(lb, "%s;\n", v->str);
        }
    }
}

/**
 * Log TRC expected result to provided log buffer.
 *
 * @param lb            Log buffer
 * @param result        Expected result
 */
static void
trc_exp_result_to_log_buf(te_log_buf *lb, const trc_exp_result *result)
{
    const trc_exp_result_entry *p;

    te_log_buf_append(lb, "%s\n", result->tags_str != NULL ?
                                      result->tags_str : "default");
    if (result->key != NULL)
        te_log_buf_append(lb, "Key: %s\n", result->key);
    if (result->notes != NULL)
        te_log_buf_append(lb, "Notes: %s\n", result->notes);
    TAILQ_FOREACH(p, &result->results, links)
    {
        te_test_result_to_log_buf(lb, &p->result);
        if (p->key != NULL)
            te_log_buf_append(lb, "Key: %s\n", p->key);
        if (p->notes != NULL)
            te_log_buf_append(lb, "Notes: %s\n", p->notes);
        te_log_buf_append(lb, "\n");
    }
}
#endif

static bool
result_has_verdict(te_test_result *result, const char *verdict_str)
{
    te_test_verdict *verdict = NULL;

    TAILQ_FOREACH(verdict, &result->verdicts, links)
    {
        if (strcmp(verdict->str, verdict_str) == 0)
            return true;
    }

    return false;
}

static tester_cfg_walk_ctl
run_repeat_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
               void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    unsigned int        step;
    bool has_verdict = false;
    te_errno            rc;

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);
    LOG_WALK_ENTRY(cfg_id_off, gctx);

    if (gctx->force_skip > 0 ||
        ctx->current_result.status == TESTER_TEST_INCOMPLETE)
    {
        ctx->current_result.status = TESTER_TEST_EMPTY;
    }
    else if (!(ctx->flags & (TESTER_PRERUN | TESTER_ASSEMBLE_PLAN)))
    {
        unsigned int    tin;

        /* The last step in test execution - verification of backup */
        run_verify_cfg_backup(ctx, test_get_attrs(ri)->track_conf);

        /* Test execution has been finished */
        tester_test_result_del(&gctx->results, &ctx->current_result);

        /*
         * Convert internal test status to TE test status.
         * Do it before TRC processing, since it may cause verdicts
         * addition.
         */
        tester_test_status_to_te_test_result(ctx->current_result.status,
                                             &ctx->current_result.result,
                                             &ctx->current_result.error,
                                             /* Verdicts in case of test
                                              * fails like segfault
                                              * should be additionally
                                              * reported in log for test
                                              * scripts only */
                                             ri->type == RUN_ITEM_SCRIPT ?
                                               ctx->current_result.id : -1);

#if WITH_TRC
        if (~ctx->flags & TESTER_NO_TRC)
        {
            if (ctx->current_result.id == TE_TEST_ID_ROOT_PROLOGUE)
            {
                tqh_strings new_tags;

                TAILQ_INIT(&new_tags);

                rc = get_trc_tags(&new_tags);
                if (rc != 0)
                {
                    ERROR("Get new TRC tags failed: %r", rc);
                    EXIT("FAULT");
                    return TESTER_CFG_WALK_FAULT;
                }

                if (!TAILQ_EMPTY(&new_tags))
                {
                    tqe_string *cur_tag;

                    rc = tester_log_trc_tags(&new_tags);
                    if (rc != 0)
                    {
                        ERROR("Logging of TRC tags failed: %r", rc);
                        EXIT("FAULT");
                        tq_strings_free(&new_tags, free);
                        return TESTER_CFG_WALK_FAULT;
                    }

                    TAILQ_FOREACH(cur_tag, &new_tags, links)
                    {
                        rc = trc_add_tag(&gctx->trc_tags, cur_tag->v);
                        if (rc != 0)
                        {
                            ERROR("Update of TRC tags failed: %r", rc);
                            EXIT("FAULT");
                            tq_strings_free(&new_tags, free);
                            return TESTER_CFG_WALK_FAULT;
                        }
                    }
                }
                tq_strings_free(&new_tags, free);
            }

            if (ctx->do_trc_walker && test_get_name(ri) != NULL)
            {
                /*
                 * Expected result is obtained here to take into
                 * account tags which may be added just above.
                 */
                ctx->current_result.exp_result =
                    trc_db_walker_get_exp_result(ctx->trc_walker,
                                                 &gctx->trc_tags);
            }

            if (ctx->current_result.result.status == TE_TEST_EMPTY)
            {
                assert(run_item_container(ri));
                assert(ctx->current_result.exp_status ==
                       TRC_VERDICT_UNKNOWN);
                /*
                 * No tests have been run in this package/session,
                 * we don't want to scream that result is unexpected.
                 */
                ctx->current_result.exp_status = TRC_VERDICT_EXPECTED;
            }
            else if (ctx->current_result.exp_result == NULL &&
                (/* Any test with specified name w/o record in TRC DB */
                 test_get_name(ri) != NULL ||
                 /* Noname session with only unknown tests inside */
                 ctx->current_result.exp_status == TRC_VERDICT_UNKNOWN))
            {
                te_log_buf *lb = te_log_buf_alloc();

                te_log_buf_append(lb, "\nObtained result is:\n");
                te_test_result_to_log_buf(lb, &ctx->current_result.result);
                RING("%s", te_log_buf_get(lb));
                te_log_buf_free(lb);

                assert(ctx->current_result.exp_status ==
                           TRC_VERDICT_UNKNOWN);
                if (ctx->current_result.error == NULL)
                    ctx->current_result.error = "Unknown test/iteration";
            }
            else if (ri->type != RUN_ITEM_SCRIPT &&
                     ((test_get_name(ri) == NULL) ||
                      (ctx->current_result.result.status !=
                           TE_TEST_SKIPPED)))
            {
                /*
                 * Expectations status can be either unknown, if
                 * everything is skipped inside or only unknown
                 * tests are run, or known otherwise.
                 */

                if (ctx->current_result.exp_status ==
                        TRC_VERDICT_UNEXPECTED &&
                    ctx->current_result.error == NULL)
                {
                    ctx->current_result.error = "Unexpected test result(s)";
                }
            }
            /* assert(ctx->current_result.exp_result != NULL) */
            else
            {
                /* Even for expected test result, we want to see what we've
                 * got and what we expect */
                te_log_buf *lb = te_log_buf_alloc();

                te_log_buf_append(lb, "\nObtained result is:\n");
                te_test_result_to_log_buf(lb, &ctx->current_result.result);
                te_log_buf_append(lb, "\nExpected results are: ");
                trc_exp_result_to_log_buf(lb,
                                          ctx->current_result.exp_result);
                RING("%s", te_log_buf_get(lb));
                te_log_buf_free(lb);

                if (trc_is_result_expected(
                         ctx->current_result.exp_result,
                         &ctx->current_result.result) != NULL)
                {
                    ctx->current_result.exp_status = TRC_VERDICT_EXPECTED;
                }
                else
                {
                    ctx->current_result.exp_status = TRC_VERDICT_UNEXPECTED;
                    if (ctx->current_result.error == NULL)
                        ctx->current_result.error = "Unexpected test result";
                }
            }
        }
#endif

        tin = (ctx->flags & TESTER_INLOGUE || ri->type != RUN_ITEM_SCRIPT) ?
                  TE_TIN_INVALID : cfg_id_off;
        log_test_result(ctx->group_result.id, &ctx->current_result, ri->plan_id);

        tester_term_out_done(ctx->flags, ri->type, run_item_name(ri), tin,
                             ctx->group_result.id,
                             ctx->current_result.id,
                             ctx->current_result.status,
#if WITH_TRC
                             ctx->current_result.exp_status
#else
                             TRC_VERDICT_UNKNOWN
#endif
                             );

        if (gctx->verdict != NULL)
        {
            has_verdict = result_has_verdict(&ctx->current_result.result,
                                             gctx->verdict);
        }

        te_test_result_clean(&ctx->current_result.result);
    }

    if ((ctx->flags & TESTER_ASSEMBLE_PLAN) && run_item_container(ri))
    {
        rc = tester_plan_pop(&gctx->plan);
        if (rc != 0)
        {
            ERROR("Failed to pop path stack: %r", rc);
            EXIT("FAULT");
            return TESTER_CFG_WALK_FAULT;
        }
    }

    /* Update result of the group */
    tester_group_result(&ctx->group_result, &ctx->current_result);
    if (ctx->group_result.status == TESTER_TEST_ERROR)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    if (~flags & TESTER_CFG_WALK_SERVICE)
    {
        if (tester_sigint_received)
        {
            ctx->current_result.status = TESTER_TEST_STOPPED;
            return TESTER_CFG_WALK_STOP;
        }

        if (!(ctx->flags &
            (TESTER_INLOGUE | TESTER_PRERUN | TESTER_ASSEMBLE_PLAN)))
        {
            if ((ctx->flags & TESTER_RUN_WHILE_PASSED) &&
                (ctx->current_result.status != TESTER_TEST_PASSED))
            {
                return TESTER_CFG_WALK_FIN;
            }
            if ((ctx->flags & TESTER_RUN_WHILE_FAILED) &&
                (ctx->current_result.status != TESTER_TEST_FAILED))
            {
                return TESTER_CFG_WALK_FIN;
            }
#if WITH_TRC
            if ((ctx->flags & TESTER_RUN_WHILE_EXPECTED) &&
                (ctx->current_result.exp_status != TRC_VERDICT_EXPECTED))
            {
                return TESTER_CFG_WALK_FIN;
            }
            if ((ctx->flags & TESTER_RUN_WHILE_UNEXPECTED) &&
                (ctx->current_result.exp_status != TRC_VERDICT_UNEXPECTED &&
                 ctx->current_result.exp_status != TRC_VERDICT_UNKNOWN))
            {
                return TESTER_CFG_WALK_FIN;
            }
#endif
            if ((ctx->flags & TESTER_RUN_UNTIL_VERDICT) && has_verdict)
            {
                return TESTER_CFG_WALK_FIN;
            }
        }

        if (ri->type == RUN_ITEM_SCRIPT)
        {
            ctx->group_step = false;
            step = 1;
        }
        else if (ctx->group_step)
        {
            ctx->group_step = false;
            step = ri->weight;
        }
        else
        {
            step = 0;
        }

        if (gctx->direction != TESTING_BACKWARD)
        {
            bool skip_tests = false;

            /*
             * By this time, all the acts which are within the current
             * session should have been run until an act (partly)
             * outside of it was encountered. This is ensured when
             * calling run_repeat_end() for session's children which
             * can result in going back to earlier children if necessary.
             * If the next act(s) are still within the current session,
             * they should be skipped because it means that the session
             * was skipped due to requirements or some failure prevented
             * it from running normally (like failed prologue).
             * In case of script, there may be multiple acts with the
             * same ID in a row if it was requested to run the same
             * iteration multiple times. After each rerun this
             * run_repeat_end() is called and nothing should be skipped
             * in it.
             */
            if (ri->type != RUN_ITEM_SCRIPT)
                skip_tests = true;

            while (scenario_step(
                      &gctx->act, &gctx->act_id,
                      cfg_id_off, cfg_id_off + step,
                      skip_tests) == TESTING_STOP)
            {
                if (gctx->flags & TESTER_INTERACTIVE)
                {
                    switch (tester_run_interactive(gctx))
                    {
                        case TESTER_INTERACTIVE_RUN:
                            step = 0;
                            break;

                        case TESTER_INTERACTIVE_RESUME:
                        case TESTER_INTERACTIVE_STOP:
                            /* Just try to continue */
                            break;

                        default:
                            assert(false);
                            /*@fallthrough@*/

                        case TESTER_INTERACTIVE_ERROR:
                            EXIT("FAULT");
                            return TESTER_CFG_WALK_FAULT;
                    }
                }
                else
                {
                    /* End of testing scenario */
                    EXIT("FIN");
                    return TESTER_CFG_WALK_FIN;
                }
            }
        }

        gctx->direction = run_this_item(cfg_id_off, gctx->act_id,
                                        ri->weight, 1);
        switch (gctx->direction)
        {
            case TESTING_STOP:
                EXIT("CONT");
                return TESTER_CFG_WALK_CONT;

            case TESTING_FORWARD:
                EXIT("BREAK");
                return TESTER_CFG_WALK_BREAK;

            case TESTING_BACKWARD:
                EXIT("BACK");
                return TESTER_CFG_WALK_BACK;

            default:
                assert(false);
                return TESTER_CFG_WALK_FAULT;
        }
    }
    else
    {
        EXIT("BREAK");
        return TESTER_CFG_WALK_BREAK;
    }
}

static void
run_skip_start(void *opaque)
{
    tester_run_data *gctx = opaque;

    gctx->force_skip++;
}

static void
run_skip_end(void *opaque)
{
    tester_run_data *gctx = opaque;

    gctx->force_skip--;
}

/**
 * Check whether preparatory walk over testing configuration
 * tree can help to improve scenario.
 *
 * @param scenario            Testing scenario.
 * @param targets             Target requirements expression.
 * @param flags               Testing flags.
 *
 * @return @c true if preparatory walk is helpful, @c false otherwise.
 */
static bool
is_prerun_helpful(testing_scenario *scenario,
                  const logic_expr *targets,
                  tester_flags flags)
{
    testing_act *act = NULL;

    /*
     * If hash is specified for some scenario acts, preparatory walk
     * will exclude iterations which do not match that hash.
     */
    TAILQ_FOREACH(act, scenario, links)
    {
        if (act->hash != NULL)
            return true;
    }

    /*
     * If requirements are specified, preparatory walk will exclude
     * those iterations which do not match them.
     */
    if ((flags & TESTER_ONLY_REQ_LOGUES) && targets != NULL)
        return true;

    return false;
}

/* See the description in tester_run.h */
te_errno
tester_run(testing_scenario   *scenario,
           const logic_expr   *targets,
           const tester_cfgs  *cfgs,
           test_paths         *paths,
           const te_trc_db    *trc_db,
           const tqh_strings  *trc_tags,
           const tester_flags  flags,
           const char         *verdict)
{
    te_errno                rc, rc2;
    tester_run_data         data;
    tester_cfg_walk_ctl     ctl;
    const tester_cfg_walk   cbs = {
        run_cfg_start,
        run_cfg_end,
        run_pkg_start,
        NULL,
        run_session_start,
        run_session_end,
        run_prologue_start,
        run_prologue_end,
        run_epilogue_start,
        run_epilogue_end,
        run_keepalive_start,
        run_keepalive_end,
        run_exception_start,
        run_exception_end,
        run_item_start,
        run_item_end,
        run_iter_start,
        run_iter_end,
        run_repeat_start,
        run_repeat_end,
        run_script,
        run_skip_start,
        run_skip_end,
    };

    testing_act   *act;
    bool all_faked = true;
    tester_flags   orig_flags;

    TAILQ_FOREACH(act, scenario, links)
    {
        if (!(act->flags & TESTER_FAKE))
        {
            all_faked = false;
            break;
        }
    }

    memset(&data, 0, sizeof(data));
    data.flags = flags;
    if (all_faked == true)
        data.flags |= TESTER_FAKE;

    data.verdict = verdict;
    data.cfgs = cfgs;
    data.paths = paths;
    data.scenario = scenario;
    TAILQ_INIT(&data.fixed_scen);
    data.targets = targets;
    data.act = TAILQ_FIRST(scenario);
    data.act_id = (data.act != NULL) ? data.act->first : 0;
    data.direction = TESTING_FORWARD;
#if WITH_TRC
    data.trc_db = trc_db;
    TAILQ_INIT(&data.trc_tags);
    rc = tq_strings_copy(&data.trc_tags, trc_tags);
    if (rc != 0)
        TE_FATAL_ERROR("Failed to copy the list of trc_tags: %r", rc);
#else
    UNUSED(trc_db);
    UNUSED(trc_tags);
#endif
    rc = tester_test_results_init(&data.results);
    if (rc != 0)
        return rc;

    while (TAILQ_EMPTY(data.scenario) &&
           (data.flags & TESTER_INTERACTIVE))
    {
        switch (tester_run_interactive(&data))
        {
            case TESTER_INTERACTIVE_RUN:
            case TESTER_INTERACTIVE_RESUME:
            case TESTER_INTERACTIVE_STOP:
                break;

            default:
                assert(false);
                /*@fallthrough@*/

            case TESTER_INTERACTIVE_ERROR:
                return TE_RC(TE_TESTER, TE_EFAULT);
        }
    }

    if (TAILQ_EMPTY(data.scenario))
    {
        WARN("Testing scenario is empty");
        return TE_RC(TE_TESTER, TE_ENOENT);
    }

    if ((~flags & TESTER_INTERACTIVE) &&
        is_prerun_helpful(data.scenario, data.targets, data.flags))
    {
        /*
         * Do preparatory walk over testing configuration tree.
         * In this walk improved scenario is created which includes only
         * tests which are going to be run taking into account hashes and
         * (if --only-req-logues option was passed) requirements passed
         * in command line.
         * This allows to avoid running unnecessary prologues/epilogues
         * in case when test iterations are specified by hash or
         * requirements rather than by full path with parameter values.
         */

        orig_flags = data.flags;
        data.flags |= TESTER_PRERUN | TESTER_NO_TRC | TESTER_NO_CS |
                      TESTER_NO_CFG_TRACK;
        if (tester_run_first_ctx(&data) == NULL)
            return TE_RC(TE_TESTER, TE_ENOMEM);

        ctl = tester_configs_walk(cfgs, &cbs, 0, &data);
        if (ctl != TESTER_CFG_WALK_FIN)
        {
            ERROR("Preparatory tree walk returned unexpected result %u",
                  ctl);
            return TE_RC(TE_TESTER, TE_EFAULT);
        }

        data.flags = orig_flags;
        data.act = TAILQ_FIRST(&data.fixed_scen);
        data.act_id = (data.act != NULL) ? data.act->first : 0;
        data.direction = TESTING_FORWARD;
        tester_run_destroy_ctx(&data);

        if (TAILQ_EMPTY(&data.fixed_scen))
        {
            WARN("Testing scenario is empty");
            return TE_RC(TE_TESTER, TE_ENOENT);
        }
    }

    rc = tester_assemble_plan(&data, &cbs, cfgs);
    if (rc != 0)
        return rc;

    if (tester_run_first_ctx(&data) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);

    rc = tester_test_msg_listener_start(&data.vl, &data.results);
    if (rc != 0)
    {
        ERROR("Failed to start test messages listener: %r", rc);
        return rc;
    }

    ctl = tester_configs_walk(cfgs, &cbs,
                              (flags & TESTER_OUT_TEST_PARAMS) ?
                              TESTER_CFG_WALK_OUTPUT_PARAMS : 0,
                              &data);
    switch (ctl)
    {
        case TESTER_CFG_WALK_CONT:
            if (cfgs->total_iters == 0)
                rc = 0;
            else
            {
                ERROR("Unexpected 'continue' at the end of walk");
                rc = TE_RC(TE_TESTER, TE_EFAULT);
            }
            break;

        case TESTER_CFG_WALK_FIN:
            rc = 0;
            break;

        case TESTER_CFG_WALK_SKIP:
            ERROR("Unexpected 'skip' at the end of walk");
            rc = TE_RC(TE_TESTER, TE_EFAULT);
            break;

        case TESTER_CFG_WALK_INTR:
            ERROR("Execution of testing scenario interrupted");
            rc = TE_RC(TE_TESTER, TE_EFAULT);
            break;

        case TESTER_CFG_WALK_STOP:
            ERROR("Execution of testing scenario interrupted by user");
            rc = TE_RC(TE_TESTER, TE_EINTR);
            break;

        case TESTER_CFG_WALK_FAULT:
            rc = TE_RC(TE_TESTER, TE_EFAULT);
            break;

        default:
            assert(false);
            rc = TE_RC(TE_TESTER, TE_EFAULT);
    }

    tester_run_destroy_ctx(&data);
    scenario_free(&data.fixed_scen);
#if WITH_TRC
    tq_strings_free(&data.trc_tags, free);
#endif

    rc2 = tester_test_msg_listener_stop(&data.vl);
    if (rc2 != 0)
    {
        ERROR("Failed to stop test messages listener: %r", rc2);
        TE_RC_UPDATE(rc, rc2);
    }

    return rc;
}


/**
 * Run Tester interactive session.
 *
 * @param gctx          Run global context
 *
 * @return Tester interactive result.
 */
static enum interactive_mode_opts
tester_run_interactive(tester_run_data *gctx)
{
    enum interactive_mode_opts  result;
    test_paths                  paths;
    testing_scenario            scenario;

    result = tester_interactive_open_prompt(gctx->cfgs, &paths, &scenario);
    switch (result)
    {
        case TESTER_INTERACTIVE_RUN:
            gctx->act = TAILQ_FIRST(&scenario);
            gctx->act_id = gctx->act->first;
            TAILQ_CONCAT(gctx->paths, &paths, links);
            (void)scenario_append(gctx->scenario, &scenario, 1);
            break;

        case TESTER_INTERACTIVE_RESUME:
            /* Just try to continue */
            break;

        case TESTER_INTERACTIVE_STOP:
            /*
             * Remove interactive flag to avoid further
             * prompting.
             */
            gctx->flags &= ~TESTER_INTERACTIVE;
            break;

        default:
            assert(false);
            /*@fallthrough@*/

        case TESTER_INTERACTIVE_ERROR:
            break;
    }

    return result;
}
