/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Prepare configuration to be run.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** Logging user name to be used here */
#define TE_LGR_USER "Config Prepare"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "logger_api.h"

#include "tester_conf.h"
#include "type_lib.h"


/** Tester context */
typedef struct config_prepare_ctx {
    SLIST_ENTRY(config_prepare_ctx) links;  /**< List links */

    unsigned int        total_iters;

    unsigned int        inherit_flags;  /**< Inheritance flags */
/** Inherit exception handler for all descendant sessions */
#define PREPARE_INHERIT_EXCEPTION_ALL   (1 << 0)
/** Inherit keep-alive handler for all descendant sessions */
#define PREPARE_INHERIT_KEEPALIVE_ALL   (1 << 1)
/** Inherit 'track_conf' attribute */
#define PREPARE_INHERIT_TRACK_CONF      (1 << 2)
/** Inherit 'track_conf' attribute for all descendants */
#define PREPARE_INHERIT_TRACK_CONF_ALL  (1 << 3)

    run_item           *exception;  /**< Current exception handler */
    run_item           *keepalive;  /**< Current keep-alive handler */
    unsigned int        track_conf; /**< Current track_conf attribute */

} config_prepare_ctx;

/**
 * Opaque data for all configuration traverse callbacks.
 */
typedef struct config_prepare_data {

    SLIST_HEAD(, config_prepare_ctx) ctxs;   /**< Stack of contexts */

    te_errno                        rc;     /**< Status code */

} config_prepare_data;


/**
 * Clone the most recent (current) Tester context.
 *
 * @param gctx          Configuration prepare global context
 *
 * @return Allocated context.
 */
static config_prepare_ctx *
config_prepare_new_ctx(config_prepare_data *gctx)
{
    config_prepare_ctx *cur_ctx = SLIST_FIRST(&gctx->ctxs);
    config_prepare_ctx *new_ctx;

    new_ctx = calloc(1, sizeof(*new_ctx));
    if (new_ctx == NULL)
    {
        gctx->rc = TE_RC(TE_TESTER, TE_ENOMEM);
        return NULL;
    }

    if (cur_ctx != NULL)
    {
        new_ctx->inherit_flags = cur_ctx->inherit_flags;
        new_ctx->exception = cur_ctx->exception;
        new_ctx->keepalive = cur_ctx->keepalive;
        new_ctx->track_conf = cur_ctx->track_conf;
    }
    else
    {
        /* new_ctx->inherit_flags = 0; */
        /* new_ctx->exception = NULL; */
        /* new_ctx->keepalive = NULL; */
        new_ctx->track_conf = TESTER_TRACK_CONF_UNSPEC;
    }

    SLIST_INSERT_HEAD(&gctx->ctxs, new_ctx, links);

    return new_ctx;
}

/**
 * Destroy the most recent (current) context.
 *
 * @param gctx          Configuration prepare global context
 *
 * @return Status code.
 */
static void
config_prepare_destroy_ctx(config_prepare_data *gctx)
{
    config_prepare_ctx *curr = SLIST_FIRST(&gctx->ctxs);
    config_prepare_ctx *prev;

    assert(curr != NULL);
    prev = SLIST_NEXT(curr, links);
    if (prev != NULL)
        prev->total_iters += curr->total_iters;

    SLIST_REMOVE(&gctx->ctxs, curr, config_prepare_ctx, links);
    free(curr);
}


/**
 * Inherit executable and update inheritance settings for the next
 * descendanct.
 *
 * @param child_exec    Location of the child executable
 * @param child_flags   Child flags to mark that executable is inherited
 * @param inherit_done  Flag to set in child flags when executable is
 *                      inherited
 * @param inherit_exec  Location of the executable to be inherited
 * @param inherit_flags Location of inheritance flags
 * @param inherit_do    Flag which requests executable inheritance
 */
static void
inherit_executable(run_item **child_exec, unsigned int *child_flags,
                   unsigned int inherit_done,
                   run_item **inherit_exec, unsigned int *inherit_flags,
                   unsigned int inherit_do)
{
    assert(child_exec != NULL);
    assert(child_flags != NULL);
    assert(inherit_exec != NULL);
    assert(inherit_flags != NULL);

    if (*child_exec != NULL)
    {
        /*
         * If the current session (referred to as "child" here) already has
         * a given executable set, we should update executable and its
         * inheritance flags in the current processing context. It will
         * allow descendant sessions to inherit it later from descendant
         * contexts (see also config_prepare_new_ctx()), if handdown
         * property of executable says that it should be inherited.
         */

        *inherit_flags &= ~inherit_do;
        *inherit_exec = NULL;
        switch ((*child_exec)->handdown)
        {
            case TESTER_HANDDOWN_NONE:
                break;

            case TESTER_HANDDOWN_DESCENDANTS:
                *inherit_flags |= inherit_do;
                /* FALLTHROUGH */

            case TESTER_HANDDOWN_CHILDREN:
                *inherit_exec = *child_exec;
                break;

            default:
                assert(FALSE);
        }
    }
    else
    {
        /*
         * If the current session does not have a given executable set,
         * we should set it to a value stored in the current processing
         * context (which may have got it via the previous contexts from
         * some higher level session, see comments above).
         */

        /* Set flag even in the case of NULL executable - it is harmless */
        *child_flags |= inherit_done;
        *child_exec = *inherit_exec;

        /*
         * If context flags do not say that all descendants (not just
         * direct children) should inherit this executable, set it to
         * NULL in the current processing context, so that nothing will
         * be inherited from it by descendant contexts and therefore by
         * descendant sessions.
         */
        if (~(*inherit_flags) & inherit_do)
            *inherit_exec = NULL;
    }
}


/**
 * Function to be called for each singleton value of the run item
 * argument (explicit or inherited) to calculate total number of values.
 *
 * The function complies with test_entity_value_enum_cb prototype.
 */
static te_errno
prepare_arg_value_cb(const test_entity_value *value, void *opaque)
{
    unsigned int *num = opaque;

    UNUSED(value);

    *num += 1;

    return 0;
}

/** Data to be passed as opaque to prepare_arg_cb() function. */
typedef struct prepare_arg_cb_data {
    run_item       *ri;         /**< Run item context */
    unsigned int    n_args;     /**< Number of arguments */
    unsigned int    n_iters;    /**< Total number of iterations without
                                     lists */
} prepare_arg_cb_data;

/**
 * Function to be called for each argument (explicit or inherited) to
 * calculate total number of iterations.
 *
 * The function complies with test_var_arg_enum_cb prototype.
 */
static te_errno
prepare_arg_cb(const test_var_arg *va, void *opaque)
{
    prepare_arg_cb_data    *data = opaque;
    unsigned int            n_values = 0;
    te_errno                rc;

    data->n_args++;

    rc = test_var_arg_enum_values(data->ri, va, prepare_arg_value_cb,
                                  &n_values, NULL, NULL);
    if (rc != 0)
    {
        ERROR("Enumeration of values of argument '%s' of the run item "
              "'%s' failed: %r", va->name, run_item_name(data->ri), rc);
        return rc;
    }

    if (va->list == NULL)
    {
        data->n_iters *= n_values;
        VERB("%s(): arg=%s: n_values=%u -> n_iters =%u",
             __FUNCTION__, va->name, n_values, data->n_iters);
    }
    else
    {
        test_var_arg_list  *p;

        for (p = SLIST_FIRST(&data->ri->lists);
             p != NULL && strcmp(p->name, va->list) != 0;
             p = SLIST_NEXT(p, links));

        if (p != NULL)
        {
            VERB("%s(): arg=%s: found list=%s len=%u n_values=%u",
                 __FUNCTION__, va->name, p->name, p->len, n_values);
            assert(data->n_iters % p->len == 0);
            data->n_iters /= p->len;
            p->len = MAX(p->len, n_values);
            data->n_iters *= p->len;
        }
        else
        {
            p = malloc(sizeof(*p));
            if (p == NULL)
                return TE_RC(TE_TESTER, TE_ENOMEM);

            p->name = va->list;
            p->len = n_values;
            p->n_iters = data->n_iters;
            SLIST_INSERT_HEAD(&data->ri->lists, p, links);

            data->n_iters *= n_values;

            VERB("%s(): arg=%s: new list=%s len=%u -> n_iters=%u",
                 __FUNCTION__, va->name, p->name, p->len, data->n_iters);
        }
    }

    return 0;
}

/**
 * Calculate number of iterations for specified run item.
 *
 * @param ri            Run item
 *
 * @return Number of iterations.
 */
static te_errno
prepare_calc_iters(run_item *ri)
{
    prepare_arg_cb_data     data;
    te_errno                rc;

    data.ri = ri;
    data.n_args = 0;
    data.n_iters = 1;

    rc = test_run_item_enum_args(ri, prepare_arg_cb, TRUE, &data);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
        return rc;

    ri->n_args = data.n_args;
    ri->n_iters = data.n_iters;

    return 0;
}


static tester_cfg_walk_ctl
prepare_cfg_start(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    config_prepare_data    *gctx = opaque;
    config_prepare_ctx     *ctx;

    UNUSED(cfg);
    UNUSED(cfg_id_off);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    if (config_prepare_new_ctx(gctx) == NULL)
        return TESTER_CFG_WALK_FAULT;

    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
prepare_cfg_end(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    config_prepare_data    *gctx = opaque;
    config_prepare_ctx     *ctx;

    UNUSED(cfg_id_off);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    cfg->total_iters = ctx->total_iters;

    config_prepare_destroy_ctx(gctx);

    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
prepare_session_start(run_item *ri, test_session *session,
                      unsigned int cfg_id_off, void *opaque)
{
    config_prepare_data    *gctx = opaque;
    config_prepare_ctx     *ctx;

    UNUSED(ri);
    UNUSED(cfg_id_off);

    assert(gctx != NULL);

    if ((ctx =  config_prepare_new_ctx(gctx)) == NULL)
        return TESTER_CFG_WALK_FAULT;

    /* Service executables inheritance */
    inherit_executable(&session->exception, &session->flags,
                       TEST_INHERITED_EXCEPTION, &ctx->exception,
                       &ctx->inherit_flags, PREPARE_INHERIT_EXCEPTION_ALL);
    inherit_executable(&session->keepalive, &session->flags,
                       TEST_INHERITED_KEEPALIVE, &ctx->keepalive,
                       &ctx->inherit_flags, PREPARE_INHERIT_KEEPALIVE_ALL);

    /* 'track_conf' attribute inheritance */

    if (session->attrs.track_conf != TESTER_TRACK_CONF_UNSPEC)
    {
        /*
         * If track_conf attribute was specified for the current
         * session, reset inheritance settings.
         */
        ctx->track_conf = session->attrs.track_conf;
        if (session->attrs.track_conf_hd != TESTER_HANDDOWN_NONE)
        {
            ctx->inherit_flags |= PREPARE_INHERIT_TRACK_CONF;
            if (session->attrs.track_conf_hd == TESTER_HANDDOWN_DESCENDANTS)
                ctx->inherit_flags |= PREPARE_INHERIT_TRACK_CONF_ALL;
            else
                ctx->inherit_flags &= ~PREPARE_INHERIT_TRACK_CONF_ALL;
        }
        else
        {
            ctx->inherit_flags &= ~PREPARE_INHERIT_TRACK_CONF;
            ctx->inherit_flags &= ~PREPARE_INHERIT_TRACK_CONF_ALL;
        }
    }
    else
    {
        /*
         * Inherit track_conf attribute from parent if it is not specified
         * in the current session and if inheritance settings allow.
         */

        if (ctx->inherit_flags & PREPARE_INHERIT_TRACK_CONF)
            session->attrs.track_conf = ctx->track_conf;
        else
            session->attrs.track_conf = TESTER_TRACK_CONF_DEF;

        /*
         * If track_conf_handdown was not set to "descendants" when
         * setting currently inherited track_conf value, disable
         * passing inherited value to children.
         */
        if (~ctx->inherit_flags & PREPARE_INHERIT_TRACK_CONF_ALL)
            ctx->inherit_flags &= ~PREPARE_INHERIT_TRACK_CONF;
    }

    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
prepare_session_end(run_item *ri, test_session *session,
                    unsigned int cfg_id_off, void *opaque)
{
    config_prepare_data    *gctx = opaque;
    config_prepare_ctx     *ctx;

    UNUSED(session);
    UNUSED(cfg_id_off);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    ri->weight = ctx->total_iters;
    ctx->total_iters = 0;

    config_prepare_destroy_ctx(gctx);

    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
prepare_test_start(run_item *ri, unsigned int cfg_id_off,
                   unsigned int flags, void *opaque)
{
    config_prepare_data    *gctx = opaque;
    config_prepare_ctx     *ctx;
    test_attrs             *attrs = test_get_attrs(ri);

    UNUSED(cfg_id_off);
    UNUSED(flags);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    /*
     * 'track_conf' attribute inheritance.
     * Note: this handler is actually run_start(), so it is called for
     * every <run>, including <run> enclosing <session>. However <run>
     * does not have its own attributes - attrs will point to those
     * of <session> in such case. As session attributes inheritance is
     * handled in prepare_session_start(), here we should only process
     * scripts (for a package attrs would point to main <session>).
     */

    if (ri->type == RUN_ITEM_SCRIPT &&
        attrs->track_conf == TESTER_TRACK_CONF_UNSPEC)
    {
        if (ctx->inherit_flags & PREPARE_INHERIT_TRACK_CONF)
            attrs->track_conf = ctx->track_conf;
        else
            attrs->track_conf = TESTER_TRACK_CONF_DEF;
    }

    gctx->rc = prepare_calc_iters(ri);
    if (gctx->rc != 0)
        return TESTER_CFG_WALK_FAULT;

    VERB("%s(): run-item=%s n_iters=%u", __FUNCTION__,
         run_item_name(ri), ri->n_iters);

    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
prepare_test_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
                 void *opaque)
{
    config_prepare_data    *gctx = opaque;
    config_prepare_ctx     *ctx;

    UNUSED(cfg_id_off);

    assert(gctx != NULL);
    ctx = SLIST_FIRST(&gctx->ctxs);
    assert(ctx != NULL);

    VERB("%s(): run-item=%s n_iters=%u weight=%u", __FUNCTION__,
         run_item_name(ri), ri->n_iters, ri->weight);

    if (gctx->rc == 0)
    {
        assert(ri->n_iters > 0);
        /* Empty package/session may have zero weight */
        assert(ri->weight > 0 || ri->type != RUN_ITEM_SCRIPT);
        if (~flags & TESTER_CFG_WALK_SERVICE)
            ctx->total_iters += ri->n_iters * ri->weight;
    }

    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
prepare_iter_start(run_item *ri, unsigned int cfg_id_off,
                   unsigned int flags, unsigned int iter, void *opaque)
{
    UNUSED(ri);
    UNUSED(cfg_id_off);
    UNUSED(flags);
    UNUSED(opaque);

    /*
     * All iterations are equal from prepare point of view.
     * Moreover, it is assumed that each run item processed only once.
     */
    if (iter == 0)
        return TESTER_CFG_WALK_CONT;
    else
        return TESTER_CFG_WALK_SKIP;
}

static tester_cfg_walk_ctl
prepare_script(run_item *ri, test_script *script,
               unsigned int cfg_id_off, void *opaque)
{
    UNUSED(script);
    UNUSED(cfg_id_off);
    UNUSED(opaque);

    ri->weight = 1;

    return TESTER_CFG_WALK_CONT;
}


/* See the description in tester_conf.h */
te_errno
tester_prepare_configs(tester_cfgs *cfgs)
{
    config_prepare_data     gctx;
    const tester_cfg_walk   cbs = {
        prepare_cfg_start,
        prepare_cfg_end,
        NULL, /* pkg_start */
        NULL, /* pkg_end */
        prepare_session_start,
        prepare_session_end,
        NULL, /* prologue_start */
        NULL, /* prologue_end */
        NULL, /* epilogue_start */
        NULL, /* epilogue_end */
        NULL, /* keepalive_start */
        NULL, /* keepalive_end */
        NULL, /* exception_start */
        NULL, /* exception_end */
        prepare_test_start,
        prepare_test_end,
        prepare_iter_start,
        NULL, /* iter_end */
        NULL, /* repeat_start */
        NULL, /* repeat_end */
        prepare_script,
        NULL, /* skip_start */
        NULL, /* skip_end */
    };

    ENTRY();

    gctx.rc = 0;
    SLIST_INIT(&gctx.ctxs);
    if (config_prepare_new_ctx(&gctx) == NULL)
    {
        EXIT("ENOMEM");
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }

    if (tester_configs_walk(cfgs, &cbs, TESTER_CFG_WALK_FORCE_EXCEPTION,
                            &gctx) == TESTER_CFG_WALK_CONT)
    {
        assert(!SLIST_EMPTY(&gctx.ctxs));

        cfgs->total_iters = SLIST_FIRST(&gctx.ctxs)->total_iters;

        config_prepare_destroy_ctx(&gctx);
        assert(SLIST_EMPTY(&gctx.ctxs));

        EXIT("0 - total_iters=%u", cfgs->total_iters);
        return 0;
    }
    else
    {
        te_errno    rc = gctx.rc;

        while (!SLIST_EMPTY(&gctx.ctxs))
            config_prepare_destroy_ctx(&gctx);

        return rc;
    }
}
