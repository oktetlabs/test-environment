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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
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
#include "tq_string.h"
#include "trc_tags.h"
#include "logger_api.h"

#include "trc_report.h"

/* See the description in trc_report.h */
te_errno
trc_tools_filter_db(te_trc_db *db,
                    unsigned int *db_uids,
                    int db_uids_size,
                    tqh_strings *tests_include,
                    tqh_strings *tests_exclude)
{
    te_errno                rc = 0;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    mv;
    te_bool                 is_iter = TRUE;

    walker = trc_db_new_walker(db);
    if (walker == NULL)
        return TE_ENOMEM;

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                is_iter = !is_iter;
                /*@fallthrou@*/

            case TRC_DB_WALKER_BROTHER:
                if (is_iter)
                {
                    tqe_string     *test_path;
                    trc_test       *test = trc_db_walker_get_test(walker);
                    te_bool         do_include = FALSE;
                    te_bool         do_exclude = FALSE;

                    TAILQ_FOREACH(test_path, tests_include, links)
                    {
                        if (strstr(test->path, test_path->v) != NULL)
                        {
                            do_include = TRUE;
                            break;
                        }
                    }

                    TAILQ_FOREACH(test_path, tests_exclude, links)
                    {
                        if (strstr(test->path, test_path->v) != NULL)
                        {
                            do_exclude = TRUE;
                            break;
                        }
                    }

                    if (((do_include == FALSE) &&
                         (!TAILQ_EMPTY(tests_include))) ||
                        (do_exclude == TRUE))
                    {
                        int i;
                        for (i = 0; i < db_uids_size; i++)
                        {
                            unsigned int                db_uid = db_uids[i];
                            trc_report_test_iter_data  *iter_data =
                                trc_db_walker_get_user_data(walker, db_uid);

                            if (iter_data != NULL)
                            {
                                trc_report_free_test_iter_data(iter_data);
                                trc_db_walker_set_user_data(walker, db_uid,
                                                            NULL);
                            }
                        }
                    }
                }

                break;

            case TRC_DB_WALKER_FATHER:
                is_iter = !is_iter;
                break;

            default:
                assert(FALSE);
                break;
        }
    }

    trc_db_free_walker(walker);

    return rc;
}


/* See the description in trc_report.h */
te_errno
trc_tools_cut_db(te_trc_db *db, unsigned int db_uid,
                 const char *path_pattern, te_bool inverse)
{
    te_errno                rc = 0;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    mv;
    te_bool                 is_iter = TRUE;
    int                     removed = 0;
    int                     removed_total = 0;

    printf("\nRemove tests by %s path\n", path_pattern);

    walker = trc_db_new_walker(db);
    if (walker == NULL)
        return TE_ENOMEM;

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                is_iter = !is_iter;
                /*@fallthrou@*/

            case TRC_DB_WALKER_BROTHER:
                if (is_iter)
                {
                    if ((strstr(trc_db_walker_get_test(walker)->path,
                                path_pattern) != NULL) != inverse)
                    {
                        trc_report_test_iter_data *iter_data =
                            trc_db_walker_get_user_data(walker, db_uid);
                        if (iter_data != NULL)
                        {
                            trc_report_test_iter_entry *p;
                            TAILQ_FOREACH(p, &iter_data->runs, links)
                            {
                                removed++;
                            }

                            trc_report_free_test_iter_data(iter_data);
                            trc_db_walker_set_user_data(walker, db_uid,
                                                        NULL);
                        }
                    }
                }

                break;

            case TRC_DB_WALKER_FATHER:
                is_iter = !is_iter;
                if (!is_iter)
                {
                    if (removed > 0)
                    {
                        printf("  Remove %s: %d iters\n",
                               trc_db_walker_get_test(walker)->path,
                               removed);
                        removed_total += removed;
                        removed = 0;
                    }
                }
                break;

            default:
                assert(FALSE);
                break;
        }
    }

    trc_db_free_walker(walker);

    printf("Total removed %d iterations for %s path\n",
           removed_total, path_pattern);

    return rc;
}


/* See the description in trc_report.h */
te_errno
trc_tools_merge_db(te_trc_db *db, int dst_uid, int src_uid1, int src_uid2)
{
    te_errno                rc = 0;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    mv;
    te_bool                 is_iter = TRUE;
    te_bool                 overwritten = FALSE;
    int                     new_iters = 0;
    int                     replaced_iters = 0;
    int                     new_total = 0;
    int                     replaced_total = 0;

    printf("Merge results:\n");

    walker = trc_db_new_walker(db);
    if (walker == NULL)
        return TE_ENOMEM;

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                is_iter = !is_iter;
                /*@fallthrou@*/

            case TRC_DB_WALKER_BROTHER:
                /*
                 * Same actions are performed for iters and tests,
                 * but we may need more complex update for iterations.
                 */
                if (is_iter)
                {
                    trc_report_test_iter_data *iter_data = NULL;
                    trc_report_test_iter_data *iter_data1 = NULL;
                    trc_report_test_iter_data *iter_data2 = NULL;

                    iter_data1 = trc_db_walker_get_user_data(walker,
                                                             src_uid1);

                    iter_data2 = trc_db_walker_get_user_data(walker,
                                                             src_uid2);

                    iter_data = (iter_data2 != NULL) ? iter_data2 : iter_data1;

                    if (iter_data != NULL)
                        trc_db_walker_set_user_data(walker, dst_uid,
                                                    iter_data);
                    if (iter_data != iter_data1)
                    {
                        overwritten = TRUE;
                        if (iter_data1 == NULL)
                            new_iters++;
                        else
                            replaced_iters++;
                    }

                }
                else
                {
                    trc_report_test_data *test_data = NULL;
                    trc_report_test_data *test_data1 = NULL;
                    trc_report_test_data *test_data2 = NULL;

                    test_data1 =
                        trc_db_walker_get_user_data(walker, src_uid1);

                    test_data2 =
                        trc_db_walker_get_user_data(walker, src_uid2);

                    test_data =
                        (test_data2 != NULL) ? test_data2 : test_data1;

                    if (test_data != NULL)
                        trc_db_walker_set_user_data(walker, dst_uid,
                                                    test_data);
                    if (test_data != test_data1)
                    {
                        //overwritten = TRUE;
                    }
                }
                break;

            case TRC_DB_WALKER_FATHER:
                is_iter = !is_iter;
                if (!is_iter)
                {
                    if (overwritten)
                    {
                        printf("  Merge %s: %d replaced + %d new\n",
                               trc_db_walker_get_test(walker)->path,
                               replaced_iters, new_iters);
                        overwritten = FALSE;
                        new_total += new_iters;
                        new_iters = 0;
                        replaced_total += replaced_iters;
                        replaced_iters = 0;
                    }
                }
                break;

            default:
                assert(FALSE);
                break;
        }
    }

    printf("Total %d iterations replaced + %d new\n",
           replaced_total, new_total);

    trc_db_free_walker(walker);

    return rc;
}

/* See the description in trc_report.h */
te_errno
trc_report_merge(trc_report_ctx *ctx, const char *filename)
{
    te_errno rc = 0;
    trc_report_ctx aux_ctx;
    unsigned int merge_uid;

    /* Initialise aux context */
    trc_report_init_ctx(&aux_ctx);

    /* Allocate aux TRC database user ID */
    aux_ctx.db = ctx->db;

    /* Allocate TRC database user ID */
    aux_ctx.db_uid = trc_db_new_user(ctx->db);

    /* Process log */
    if ((rc = trc_report_process_log(&aux_ctx, filename)) != 0)
    {
        ERROR("Failed to process XML log");
        return rc;
    }

    /* Allocate new user ID to store merge result */
    merge_uid = trc_db_new_user(ctx->db);

    if ((rc = trc_tools_merge_db(ctx->db, merge_uid,
                                 ctx->db_uid, aux_ctx.db_uid)) != 0)
    {
        ERROR("Failed to merge with %s", filename);
        return rc;
    }

    /* Replace default user UID */
    ctx->db_uid = merge_uid;

    return 0;
}

/**
 * Copy all content of one file to another.
 *
 * @param dst   Destination file
 * @param src   Source file
 *
 * @return      Status code.
 */
int
trc_tools_file_to_file(FILE *dst, FILE *src)
{
    char    buf[4096];
    size_t  r;

    rewind(src);
    do {
        r = fread(buf, 1, sizeof(buf), src);
        if (r > 0)
           if (fwrite(buf, r, 1, dst) != 1)
               return errno;
    } while (r == sizeof(buf));

    return 0;
}
