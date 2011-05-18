/** @file
 * @brief Tester Subsystem
 *
 * Implementation of configuration traverse routines.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "Config Walk"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tester_conf.h"
#include "tester.h"

#if 0
#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL (TE_LL_WARN | TE_LL_ERROR | \
                      TE_LL_VERB | TE_LL_ENTRY_EXIT | TE_LL_RING)
#endif

typedef  tester_cfg_walk_ctl (* tester_cfg_walk_run_item_cb)
                                 (run_item *, unsigned int, void *);


static tester_cfg_walk_ctl walk_run_item(
                               const tester_cfg_walk *walk,
                               const void            *opaque,
                               const unsigned int     id_off,
                               const unsigned int     flags,
                               const run_item        *run,
                               const run_item        *keepalive,
                               const run_item        *exception);

static tester_cfg_walk_ctl walk_run_items(const tester_cfg_walk *walk,
                                          const void            *opaque,
                                          const unsigned int     id_off,
                                          const unsigned int     flags,
                                          const run_items       *runs,
                                          const run_item        *keepalive,
                                          const run_item        *exception);


/**
 * Convert Tester configuration walk control to string.
 *
 * @param ctl           Control
 */
static const char *
tester_cfg_walk_ctl2str(tester_cfg_walk_ctl ctl)
{
    switch (ctl)
    {
#define TESTER_CFG_WALK_CTL2STR(_ctl) \
        case TESTER_CFG_WALK_##_ctl:    return #_ctl

        TESTER_CFG_WALK_CTL2STR(CONT);
        TESTER_CFG_WALK_CTL2STR(BACK);
        TESTER_CFG_WALK_CTL2STR(BREAK);
        TESTER_CFG_WALK_CTL2STR(SKIP);
        TESTER_CFG_WALK_CTL2STR(EXC);
        TESTER_CFG_WALK_CTL2STR(FIN);
        TESTER_CFG_WALK_CTL2STR(STOP);
        TESTER_CFG_WALK_CTL2STR(INTR);
        TESTER_CFG_WALK_CTL2STR(FAULT);

#undef TESTER_CFG_WALK_CTL2STR

        default:
            assert(FALSE);
            return "<UNKNOWN>";
    }
}

/**
 * Update current walk control when a new is received.
 *
 * @param cur           Current control
 * @param anew          A new control
 *
 * @return Result of the merge.
 */
static tester_cfg_walk_ctl
walk_ctl_merge(tester_cfg_walk_ctl cur, tester_cfg_walk_ctl anew)
{
    switch (cur)
    {
        case TESTER_CFG_WALK_FAULT:
        case TESTER_CFG_WALK_FIN:
        case TESTER_CFG_WALK_STOP:
        case TESTER_CFG_WALK_INTR:
            VERB("%s(): curr=%s anew=%s -> %s", __FUNCTION__,
                 tester_cfg_walk_ctl2str(cur),
                 tester_cfg_walk_ctl2str(anew),
                 tester_cfg_walk_ctl2str(cur));
            return cur;

        case TESTER_CFG_WALK_BREAK:
            if (anew == TESTER_CFG_WALK_CONT)
            {
                VERB("%s(): curr=%s anew=%s -> BREAK", __FUNCTION__,
                     tester_cfg_walk_ctl2str(cur),
                     tester_cfg_walk_ctl2str(anew));
                return TESTER_CFG_WALK_BREAK;
            }
            VERB("%s(): curr=%s anew=%s -> %s", __FUNCTION__,
                 tester_cfg_walk_ctl2str(cur),
                 tester_cfg_walk_ctl2str(anew),
                 tester_cfg_walk_ctl2str(anew));
            return anew;

        case TESTER_CFG_WALK_BACK:
            if (anew == TESTER_CFG_WALK_CONT)
            {
                VERB("%s(): curr=%s anew=%s -> BACK", __FUNCTION__,
                     tester_cfg_walk_ctl2str(cur),
                     tester_cfg_walk_ctl2str(anew));
                return TESTER_CFG_WALK_BACK;
            }
            VERB("%s(): curr=%s anew=%s -> %s", __FUNCTION__,
                 tester_cfg_walk_ctl2str(cur),
                 tester_cfg_walk_ctl2str(anew),
                 tester_cfg_walk_ctl2str(anew));
            return anew;

        default:
            VERB("%s(): curr=%s anew=%s -> %s", __FUNCTION__,
                 tester_cfg_walk_ctl2str(cur),
                 tester_cfg_walk_ctl2str(anew),
                 tester_cfg_walk_ctl2str(anew));
            return anew;
    }
}

/**
 * Walk service run item.
 *
 * @param walk          Walk callbacks
 * @param opaque        Opaque data to be passed in callbacks
 * @param id_off        Configuration ID of the next test
 * @param run           Run item (may be NULL)
 * @param start_cb      Function to be called before run item
 * @param end_cb        Function to be called after run item
 *
 * @return Walk control.
 */
static tester_cfg_walk_ctl
walk_service(const tester_cfg_walk *walk, const void *opaque,
             const unsigned int id_off, const run_item *run,
             tester_cfg_walk_run_item_cb start_cb,
             tester_cfg_walk_run_item_cb end_cb)
{
    tester_cfg_walk_ctl ctl;
    tester_cfg_walk_ctl ctl_tmp;

    ENTRY("run=%s id_off=%u start_cb=%p end_cb=%p",
          run_item_name(run), id_off, start_cb, end_cb);

    if (start_cb != NULL)
        ctl = start_cb((run_item *)run, id_off, (void *)opaque);
    else
        ctl = TESTER_CFG_WALK_CONT;

    assert(run != NULL);
    if (ctl == TESTER_CFG_WALK_CONT)
    {
        ctl = walk_run_item(walk, opaque, id_off, TESTER_CFG_WALK_SERVICE,
                            run, NULL, NULL);
    }

    if (end_cb != NULL)
    {
        ctl_tmp = end_cb((run_item *)run, id_off, (void *)opaque);
        ctl = walk_ctl_merge(ctl, ctl_tmp);
    }

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}

/**
 * Walk test session.
 *
 * @param walk          Walk callbacks
 * @param opaque        Opaque data to be passed in callbacks
 * @param id_off        Configuration ID of the next test
 * @param flags         Current flags
 * @param ri            Run item
 * @param session       Test session
 *
 * @return Walk control.
 */
static tester_cfg_walk_ctl
walk_test_session(const tester_cfg_walk *walk, const void *opaque,
                  const unsigned int id_off, const unsigned int flags,
                  const run_item *ri, const test_session *session)
{
    tester_cfg_walk_ctl ctl;
    tester_cfg_walk_ctl ctl_tmp;

    ENTRY("run=%s id_off=%u flags=%#x", run_item_name(ri), id_off, flags);

    if (walk->session_start != NULL)
        ctl = walk->session_start((run_item *)ri, (test_session *)session,
                                  id_off, (void *)opaque);
    else
        ctl = TESTER_CFG_WALK_CONT;

    if (ctl == TESTER_CFG_WALK_CONT)
    {
        if (session->prologue != NULL)
        {
            ctl = walk_service(walk, opaque, id_off, session->prologue,
                               walk->prologue_start, walk->prologue_end);
        }

        if (ctl == TESTER_CFG_WALK_CONT)
        {
            ctl = walk_run_items(walk, (void *)opaque, id_off, flags,
                                 &session->run_items,
                                 session->keepalive,
                                 session->exception);
        }

        /*
         * Global context flag TESTER_BREAK_SESION is 0 on default.
         * This makes tester behave in usual maner: session's
         * epilogue is always executed if exists.
         *
         * If TESTER_BREAK_SESSION is 1 tester executes session's
         * epilogue if epilogue exists and session was not killed
         * with Ctrl-C command. When session is killed with Ctrl-C
         * epilogue is skipped too. This feature is introduced for
         * debug purpose. Specify option --tester-break-session if
         * you need it.
         */
        if (session->epilogue != NULL &&
            (!(tester_global_context.flags & TESTER_BREAK_SESSION) ||
              (tester_global_context.flags & TESTER_BREAK_SESSION) &&
                                    ctl != TESTER_CFG_WALK_STOP))
        {
            ctl_tmp = walk_service(walk, opaque, id_off,
                                   session->epilogue,
                                   walk->epilogue_start,
                                   walk->epilogue_end);
            ctl = walk_ctl_merge(ctl, ctl_tmp);
        }
    }

    if (walk->session_end != NULL)
    {
        ctl_tmp = walk->session_end((run_item *)ri,
                                    (test_session *)session, id_off,
                                    (void *)opaque);
        ctl = walk_ctl_merge(ctl, ctl_tmp);
    }

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}

/**
 * Walk test package.
 * 
 * @param walk          Walk callbacks
 * @param opaque        Opaque data to be passed in callbacks
 * @param id_off        Configuration ID of the next test
 * @param flags         Current flags
 * @param ri            Run item
 * @param pkg           Test package
 *
 * @return Walk control.
 */
static tester_cfg_walk_ctl
walk_test_package(const tester_cfg_walk *walk, const void *opaque,
                  const unsigned int id_off, const unsigned int flags,
                  const run_item *ri, const test_package *pkg)
{
    tester_cfg_walk_ctl  ctl;
    tester_cfg_walk_ctl  ctl_tmp;

    ENTRY("run=%s id_off=%u flags=%#x", run_item_name(ri), id_off, flags);

    if (walk->pkg_start != NULL)
        ctl = walk->pkg_start((run_item *)ri, (test_package *)pkg,
                              id_off, (void *)opaque);
    else
        ctl = TESTER_CFG_WALK_CONT;

    if (ctl == TESTER_CFG_WALK_CONT)
    {
        ctl = walk_test_session(walk, opaque, id_off, flags, ri,
                                &pkg->session);
    }

    if (walk->pkg_end != NULL)
    {
        ctl_tmp = walk->pkg_end((run_item *)ri, (test_package *)pkg,
                                id_off, (void *)opaque);
        ctl = walk_ctl_merge(ctl, ctl_tmp);
    }

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}


/**
 * Repeat single run item.
 *
 * @param walk          Walk callbacks
 * @param opaque        Opaque data to be passed in callbacks
 * @param id_off        Configuration ID of the next test
 * @param flags         Current flags
 * @param run           Run item (may be NULL)
 * @param keepalive     Keep-alive validation to be called before and
 *                      after every iteration
 * @param exception     Exceptions handler to be called in the case of
 *                      exception (TESTER_CFG_WALK_EXC)
 *
 * @return Walk control.
 */
static tester_cfg_walk_ctl
walk_repeat(const tester_cfg_walk *walk, const void *opaque,
            const unsigned int id_off, const unsigned int flags,
            const run_item *run,
            const run_item *keepalive, const run_item *exception)
{
    tester_cfg_walk_ctl ctl;
    tester_cfg_walk_ctl ctl_tmp;
    te_bool             do_exception = FALSE;

    ENTRY("run=%s id_off=%u flags=%#x keepalive=%p exception=%p",
          run_item_name(run), id_off, flags, keepalive, exception);

    do {
        if (keepalive != NULL)
        {
            ctl = walk_service(walk, opaque, id_off, keepalive,
                               walk->keepalive_start,
                               walk->keepalive_end);
            if (ctl != TESTER_CFG_WALK_CONT)
                break;
        }

        if (walk->repeat_start != NULL)
            ctl = walk->repeat_start((run_item *)run, id_off, flags,
                                     (void *)opaque);
        else
            ctl = TESTER_CFG_WALK_CONT;

        if (ctl == TESTER_CFG_WALK_CONT && run != NULL)
        {
            switch (run->type)
            {
                case RUN_ITEM_SCRIPT:
                    if (walk->script != NULL)
                        ctl = walk->script((run_item *)run,
                                           (test_script *)&run->u.script,
                                           id_off, (void *)opaque);
                    break;

                case RUN_ITEM_SESSION:
                    ctl = walk_test_session(walk, opaque, id_off, flags,
                                            run, &run->u.session);
                    break;

                case RUN_ITEM_PACKAGE:
                    ctl = walk_test_package(walk, opaque, id_off, flags,
                                            run, run->u.package);
                    break;

                default:
                    assert(NULL);
                    ctl = TESTER_CFG_WALK_FAULT;
            }
        }

        if (ctl == TESTER_CFG_WALK_EXC)
        {
            do_exception = TRUE;
            ctl = TESTER_CFG_WALK_CONT;
        }

        if (walk->repeat_end != NULL)
        {
            ctl_tmp = walk->repeat_end((run_item *)run, id_off, flags,
                                       (void *)opaque);
            ctl = walk_ctl_merge(ctl, ctl_tmp);
        }
        else if (ctl == TESTER_CFG_WALK_CONT)
        {
            ctl = TESTER_CFG_WALK_BREAK;
        }

        if (do_exception || (flags & TESTER_CFG_WALK_FORCE_EXCEPTION))
        {
            do_exception = FALSE;
            if (exception != NULL)
            {
                ctl_tmp = walk_service(walk, opaque, id_off, exception,
                                       walk->exception_start,
                                       walk->exception_end);
                ctl = walk_ctl_merge(ctl, ctl_tmp);
            }
            else
            {
                /* Ignore exceptions by default */
            }
        }

    } while (ctl == TESTER_CFG_WALK_CONT);

    if (ctl == TESTER_CFG_WALK_BREAK)
        ctl = TESTER_CFG_WALK_CONT;

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}

/**
 * Iterate single run item.
 *
 * @param walk          Walk callbacks
 * @param opaque        Opaque data to be passed in callbacks
 * @param id_off        Configuration ID of the next test
 * @param flags         Current flags
 * @param run           Run item (may be NULL)
 * @param keepalive     Keep-alive validation to be called before and
 *                      after every iteration
 * @param exception     Exceptions handler to be called in the case of
 *                      exception (TESTER_CFG_WALK_EXC)
 *
 * @return Walk control.
 */
static tester_cfg_walk_ctl
walk_iterate(const tester_cfg_walk *walk, const void *opaque,
             const unsigned int id_off, const unsigned int flags,
             const run_item *run,
             const run_item *keepalive, const run_item *exception)
{
    tester_cfg_walk_ctl ctl = TESTER_CFG_WALK_CONT;
    tester_cfg_walk_ctl ctl_tmp;
    unsigned int        curr_id_off = id_off;
    unsigned int        i = 0;

    ENTRY("run=%s id_off=%u flags=%#x keepalive=%p exception=%p",
          run_item_name(run), id_off, flags, keepalive, exception);

    assert(run != NULL);

    while (i < run->n_iters && ctl == TESTER_CFG_WALK_CONT)
    {
        if (walk->iter_start != NULL)
            ctl = walk->iter_start((run_item *)run, curr_id_off, flags, i,
                                   (void *)opaque);
        else
            ctl = TESTER_CFG_WALK_CONT;

        if (ctl == TESTER_CFG_WALK_CONT)
            ctl = walk_repeat(walk, opaque, curr_id_off, flags, run,
                              keepalive, exception);
        else if (ctl == TESTER_CFG_WALK_SKIP)
            ctl = TESTER_CFG_WALK_CONT;

        if (walk->iter_end != NULL)
        {
            ctl_tmp = walk->iter_end((run_item *)run, curr_id_off, flags, i,
                                     (void *)opaque);
            ctl = walk_ctl_merge(ctl, ctl_tmp);
        }

        if (ctl == TESTER_CFG_WALK_BACK && curr_id_off != id_off)
        {
            VERB("%s(): Try the first iteration", __FUNCTION__);
            i = 0;
            curr_id_off = id_off;
            ctl = TESTER_CFG_WALK_CONT;
        }
        else
        {
             ++i;
             curr_id_off += run->weight;
        }
    }

    if (ctl == TESTER_CFG_WALK_BREAK)
        ctl = TESTER_CFG_WALK_CONT;

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}

/**
 * Walk single run item.
 *
 * @param walk          Walk callbacks
 * @param opaque        Opaque data to be passed in callbacks
 * @param id_off        Configuration ID of the next test
 * @param flags         Current flags
 * @param run           Run item (may be NULL)
 * @param keepalive     Keep-alive validation to be called before and
 *                      after every iteration
 * @param exception     Exceptions handler to be called in the case of
 *                      exception (TESTER_CFG_WALK_EXC)
 *
 * @return Walk control.
 */
static tester_cfg_walk_ctl
walk_run_item(const tester_cfg_walk *walk, const void *opaque,
              const unsigned int id_off, const unsigned int flags,
              const run_item *run,
              const run_item *keepalive, const run_item *exception)
{
    tester_cfg_walk_ctl ctl;
    tester_cfg_walk_ctl ctl_tmp;

    ENTRY("run=%s id_off=%u flags=%#x keepalive=%p exception=%p",
          run_item_name(run), id_off, flags, keepalive, exception);

    if (walk->run_start != NULL)
        ctl = walk->run_start((run_item *)run, id_off, flags,
                              (void *)opaque);
    else
        ctl = TESTER_CFG_WALK_CONT;

    if (ctl == TESTER_CFG_WALK_CONT && run != NULL)
    {
        ctl = walk_iterate(walk, opaque, id_off, flags, run,
                           keepalive, exception);
    }

    if (walk->run_end != NULL)
    {
        ctl_tmp = walk->run_end((run_item *)run, id_off, flags,
                                (void *)opaque);
        ctl = walk_ctl_merge(ctl, ctl_tmp);
    }

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}

/**
 * Walk list of run items.
 *
 * @param walk          Walk callbacks
 * @param opaque        Opaque data to be passed in callbacks
 * @param id_off        Configuration ID of the next test
 * @param flags         Current flags
 * @param runs          List of run items
 * @param keepalive     Keep-alive validation to be called before and
 *                      after every run item
 * @param exception     Exceptions handler to be called in the case of
 *                      exception (TESTER_CFG_WALK_EXC)
 *
 * @return Walk control.
 */
static tester_cfg_walk_ctl
walk_run_items(const tester_cfg_walk *walk, const void *opaque,
               const unsigned int id_off, const unsigned int flags,
               const run_items *runs,
               const run_item *keepalive, const run_item *exception)
{
    tester_cfg_walk_ctl     ctl = TESTER_CFG_WALK_CONT;
    unsigned int            curr_id_off = id_off;
    const run_item         *ri;
    const run_item         *ri_next;

    ENTRY("id_off=%u flags=%#x keepalive=%p exception=%p",
          id_off, flags, keepalive, exception);

    for (ri = TAILQ_FIRST(runs);
         ri != NULL && ctl == TESTER_CFG_WALK_CONT;
         ri = ri_next)
    {
        ctl = walk_run_item(walk, (void *)opaque, curr_id_off, flags,
                            ri, keepalive, exception);
        if (ctl == TESTER_CFG_WALK_BACK && ri != TAILQ_FIRST(runs))
        {
            VERB("%s(): Try the first run item", __FUNCTION__);
            ri_next = TAILQ_FIRST(runs);
            curr_id_off = id_off;
            ctl = TESTER_CFG_WALK_CONT;
        }
        else
        {
            curr_id_off += ri->n_iters * ri->weight;
            ri_next = TAILQ_NEXT(ri, links);
        }
    }

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}

/* See the description in tester_conf.h */
tester_cfg_walk_ctl
tester_configs_walk(const tester_cfgs     *cfgs,
                    const tester_cfg_walk *walk_cbs,
                    const unsigned int     walk_flags,
                    const void            *opaque)
{
    tester_cfg_walk_ctl     ctl = TESTER_CFG_WALK_CONT;
    tester_cfg_walk_ctl     ctl_tmp;
    const tester_cfg       *cfg;
    unsigned int            id_off;

    ENTRY("flags=%#x", walk_flags);

    for (id_off = 0, cfg = TAILQ_FIRST(&cfgs->head);
         cfg != NULL && ctl == TESTER_CFG_WALK_CONT;
         id_off += cfg->total_iters, cfg = TAILQ_NEXT(cfg, links))
    {
        if (walk_cbs->cfg_start != NULL)
            ctl = walk_cbs->cfg_start((tester_cfg *)cfg, id_off,
                                      (void *)opaque);

        if (ctl == TESTER_CFG_WALK_CONT)
            ctl = walk_run_items(walk_cbs, opaque, id_off, walk_flags,
                                 &cfg->runs, NULL, NULL);

        if (walk_cbs->cfg_end != NULL)
        {
            ctl_tmp = walk_cbs->cfg_end((tester_cfg *)cfg, id_off,
                                        (void *)opaque);
            ctl = walk_ctl_merge(ctl, ctl_tmp);
        }

        if (ctl == TESTER_CFG_WALK_SKIP)
            ctl = TESTER_CFG_WALK_CONT;
    }

    EXIT("ctl=%s", tester_cfg_walk_ctl2str(ctl));
    return ctl;
}
