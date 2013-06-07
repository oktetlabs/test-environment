/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Auxiluary routines for tool which generates TRC report by obtained
 * result.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#include "te_alloc.h"
#include "te_queue.h"
#include "trc_report.h"


/** Single not run iteration statistics */
static trc_report_stats not_run = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };


/* See the description in trc_report.h */
void
trc_report_init_ctx(trc_report_ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    TAILQ_INIT(&ctx->tags);
    TAILQ_INIT(&ctx->merge_fns);
    TAILQ_INIT(&ctx->cut_paths);
}


/**
 * Add one statistics to another.
 *
 * @param stats     Total statistics
 * @param add       Statistics to add
 */
static void
trc_report_stats_add(trc_report_stats *stats, const trc_report_stats *add)
{
    assert(stats != NULL);
    assert(add != NULL);

    stats->pass_exp += add->pass_exp;
    stats->pass_une += add->pass_une;
    stats->fail_exp += add->fail_exp;
    stats->fail_une += add->fail_une;
    stats->aborted += add->aborted;
    stats->new_run += add->new_run;

    stats->not_run += add->not_run;
    stats->skip_exp += add->skip_exp;
    stats->skip_une += add->skip_une;
    stats->new_not_run += add->new_not_run;
}

/* See the description in trc_report.h */
void
trc_report_free_test_iter_data(trc_report_test_iter_data *data)
{
    trc_report_test_iter_entry *p;

    if (data == NULL)
        return;

    while ((p = TAILQ_FIRST(&data->runs)) != NULL)
    {
        TAILQ_REMOVE(&data->runs, p, links);
        te_test_result_free_verdicts(&p->result);
        for (p->args_n = 0; p->args_n < p->args_max; p->args_n++)
        {
            free(p->args[p->args_n].name);
            free(p->args[p->args_n].value);
        }
        free(p->args);
        free(p);
    }
    free(data);
}

/* See the description in trc_report.h */
te_errno
trc_report_collect_stats(trc_report_ctx *ctx)
{
    te_errno                rc = 0;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    mv;
    te_bool                 is_iter = TRUE;
    trc_report_stats       *sum = NULL;
    trc_report_stats       *add = NULL;

    walker = trc_db_new_walker(ctx->db);
    if (walker == NULL)
        return TE_ENOMEM;

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        if (mv != TRC_DB_WALKER_SON)
        {
            /* 
             * Brother and father movements mean end-of-branch.
             * Therefore, add statistics of the branch to its parent
             * statistics.
             */
            trc_report_stats_add(sum, add);
            add = NULL; /* DEBUG */
        }

        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                is_iter = !is_iter;
                sum = add;
                /*@fallthrou@*/

            case TRC_DB_WALKER_BROTHER:
                if (is_iter)
                {
                    trc_report_test_iter_data *iter_data;

                    iter_data = trc_db_walker_get_user_data(walker,
                                                            ctx->db_uid);
                    if (iter_data == NULL)
                    {
                        const trc_test *test =
                            trc_db_walker_get_test(walker);

                        if (test->type != TRC_TEST_SCRIPT)
                        {
                            iter_data = TE_ALLOC(sizeof(*iter_data));
                            if (iter_data == NULL)
                            {
                                rc = TE_ENOMEM;
                                break;
                            }
                            TAILQ_INIT(&iter_data->runs);
                            rc = trc_db_walker_set_user_data(walker,
                                                             ctx->db_uid,
                                                             iter_data);
                            if (rc != 0)
                                break;
                            add = &iter_data->stats;
                        }
                        else
                        {
                            /* It is a script not run iteration */
                            add = &not_run;
                        }
                    }
                    else
                    {
                        add = &iter_data->stats;
                    }
                }
                else
                {
                    trc_report_test_data *test_data;

                    test_data = trc_db_walker_get_user_data(walker,
                                                            ctx->db_uid);
                    if (test_data == NULL)
                    {
                        test_data = TE_ALLOC(sizeof(*test_data));
                        if (test_data == NULL)
                        {
                            rc = TE_ENOMEM;
                            break;
                        }
                        rc = trc_db_walker_set_user_data(walker,
                                                         ctx->db_uid,
                                                         test_data);
                        if (rc != 0)
                            break;
                    }
                    add = &test_data->stats;
                }
                break;

            case TRC_DB_WALKER_FATHER:
                is_iter = !is_iter;
                add = sum;
                if (is_iter)
                {
                    trc_report_test_iter_data *iter_data;
                    trc_report_test_data      *test_data;

                    /* Get the iteration data */
                    iter_data = trc_db_walker_get_user_data(walker,
                                                            ctx->db_uid);
                    assert(iter_data != NULL);
                    if (!TAILQ_EMPTY(&iter_data->runs))
                    {
                        /* Set 'is_exp' flag using collected statistics */
                        /* FIXME: What to do with more iteration? */
                        TAILQ_FIRST(&iter_data->runs)->is_exp =
                            (TRC_STATS_RUN_UNEXP(&iter_data->stats) == 0);
                    }

                    test_data = trc_db_test_get_user_data(
                                    trc_db_walker_get_test(walker),
                                    ctx->db_uid);
                    assert(test_data != NULL);
                    sum = &test_data->stats;
                }
                else
                {
                    trc_test_iter             *iter;
                    trc_report_test_iter_data *iter_data;

                    iter = trc_db_walker_get_iter(walker);
                    if (iter != NULL)
                    {
                        iter_data = trc_db_iter_get_user_data(iter,
                                                              ctx->db_uid);
                        assert(iter_data != NULL);
                        sum = &iter_data->stats;
                    }
                    else
                    {
                        sum = &ctx->stats;
                    }
                }
                break;

            default:
                assert(FALSE);
                break;
        }
    }
    if (rc == 0 && sum != NULL && add != NULL)
    {
        trc_report_stats_add(sum, add);
    }

    trc_db_free_walker(walker);

    return rc;
}
