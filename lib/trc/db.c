/** @file
 * @brief Testing Results Comparator
 *
 * Implementation of auxiliary routines to work with TRC database.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "te_queue.h"
#include "logger_api.h"
#include "logic_expr.h"

#include "te_trc.h"
#include "trc_db.h"


static void trc_free_trc_tests(trc_tests *tests);

/**
 * Free resources allocated for the list of test arguments.
 *
 * @param args      List of test arguments to be freed
 */
static void
trc_free_test_args(trc_test_iter_args *args)
{
    trc_test_iter_arg   *p;

    while ((p = TAILQ_FIRST(&args->head)) != NULL)
    {
        TAILQ_REMOVE(&args->head, p, links);
        free(p->name);
        free(p->value);
        free(p);
    }
}

/**
 * Free resources allocated for expected result.
 *
 * @param result        Result to be freed
 */
static void
trc_free_exp_result(trc_exp_result *result)
{
    trc_exp_result_entry   *p;

    free(result->tags_str);
    logic_expr_free(result->tags_expr);

    while ((p = TAILQ_FIRST(&result->results)) != NULL)
    {
        TAILQ_REMOVE(&result->results, p, links);
        te_test_result_free_verdicts(&p->result);
        free(p->key);
        free(p->notes);
        free(p);
    }

    free(result->key);
    free(result->notes);
}

/**
 * Free resources allocated for the list of expected results.
 *
 * @param iters     List of expected results to be freed
 */
static void
trc_free_exp_results(trc_exp_results *results)
{
    trc_exp_result *p;

    while ((p = SLIST_FIRST(results)) != NULL)
    {
        SLIST_REMOVE(results, p, trc_exp_result, links);
        trc_free_exp_result(p);
        free(p);
    }
}

/**
 * Free resources allocated for the list of test iterations.
 *
 * @param iters     List of test iterations to be freed
 */
static void
trc_free_test_iters(trc_test_iters *iters)
{
    trc_test_iter  *p;

    while ((p = TAILQ_FIRST(&iters->head)) != NULL)
    {
        TAILQ_REMOVE(&iters->head, p, links);
        trc_free_test_args(&p->args);
        free(p->notes);
        trc_free_exp_results(&p->exp_results);
        trc_free_trc_tests(&p->tests);
        free(p);
    }
}

/**
 * Free resources allocated for the list of tests.
 *
 * @param tests     List of tests to be freed
 */
static void
trc_free_trc_tests(trc_tests *tests)
{
    trc_test   *p;

    while ((p = TAILQ_FIRST(&tests->head)) != NULL)
    {
        TAILQ_REMOVE(&tests->head, p, links);
        free(p->name);
        free(p->notes);
        free(p->objective);
        trc_free_test_iters(&p->iters);
        free(p);
    }
}

/* See description in trc_db.h */
void
trc_db_close(te_trc_db *db)
{
    if (db == NULL)
        return;

    free(db->filename);
    xmlFreeDoc(db->xml_doc);
    free(db->version);
    trc_free_trc_tests(&db->tests);
    free(db);
}


/* See the description in te_trc.h */
te_errno
trc_db_init(te_trc_db **db)
{
    *db = TE_ALLOC(sizeof(**db));
    if (*db == NULL)
        return TE_RC(TE_TRC, TE_ENOMEM);
    TAILQ_INIT(&(*db)->tests.head);
    TAILQ_INIT(&(*db)->globals.head);

    /* store current db pointer */
    current_db = (*db);

    return 0;
}

void
trc_db_test_update_path(trc_test *test)
{
    free(test->path);

    test->path = te_sprintf("%s/%s",
                            ((test->parent != NULL) &&
                             (test->parent->parent->name != NULL)) ?
                            test->parent->parent->path : "", test->name);
}

/* See the description in trc_db.h */
trc_test *
trc_db_new_test(trc_tests *tests, trc_test_iter *parent, const char *name)
{
    trc_test   *p;

    p = TE_ALLOC(sizeof(*p));
    if (p != NULL)
    {
        TAILQ_INIT(&p->iters.head);
        LIST_INIT(&p->users);
        p->parent = parent;
        p->path = NULL;
        if (name != NULL)
        {
            p->name = strdup(name);
            trc_db_test_update_path(p);

            if (p->name == NULL)
            {
                ERROR("%s(): strdup(%s) failed", __FUNCTION__, name);
                free(p);
                return NULL;
            }
        }
        TAILQ_INSERT_TAIL(&tests->head, p, links);
    }

    return p;
}

/**
 * Create list of arguments.
 *
 * @param args          Empty list of arguments
 * @param n_args        Number of arguments to add into the list
 * @param in_args       Added arguments
 *
 * @return Status code.
 */
static te_errno
trc_db_test_iter_args(trc_test_iter_args *args, unsigned int n_args,
                      trc_report_argument *add_args)
{
    unsigned int        i;

    assert(TAILQ_EMPTY(&args->head));

    for (i = 0; i < n_args; ++i)
    {
        trc_test_iter_arg  *arg;
        trc_test_iter_arg  *insert_after = NULL;
        
        arg = TE_ALLOC(sizeof(*arg));
        if (arg == NULL)
        {
            while ((arg = TAILQ_FIRST(&args->head)) != NULL)
            {
                TAILQ_REMOVE(&args->head, arg, links);
                /* Do not free name and value */
                free(arg);
            }
            return TE_RC(TE_TRC, TE_ENOMEM);
        }

        arg->name = add_args[i].name;
        arg->value = add_args[i].value;

        TAILQ_FOREACH_REVERSE(insert_after, &args->head, 
                              trc_test_iter_args_head, 
                              links)
        {
            if (strcmp(insert_after->name, arg->name) < 0)
                break;
        }

        if (insert_after == NULL)
            TAILQ_INSERT_HEAD(&args->head, arg, links);
        else
            TAILQ_INSERT_AFTER(&args->head, insert_after, arg, links);
    }

    /* Success, names and values are own */
    for (i = 0; i < n_args; ++i)
        add_args[i].name = add_args[i].value = NULL;

    return 0;
}

/* See the description in trc_db.h */
trc_test_iter *
trc_db_new_test_iter(trc_test *test, unsigned int n_args,
                     trc_report_argument *args)
{
    trc_test_iter *p;
    te_errno       rc;

    p = TE_ALLOC(sizeof(*p));
    if (p != NULL)
    {
        TAILQ_INIT(&p->args.head);
        SLIST_INIT(&p->exp_results);
        TAILQ_INIT(&p->tests.head);
        LIST_INIT(&p->users);
        p->parent = test;
        rc = trc_db_test_iter_args(&p->args, n_args, args);
        if (rc != 0)
        {
            free(p);
            return NULL;
        }
        TAILQ_INSERT_TAIL(&test->iters.head, p, links);
    }

    return p;
}

/* See the description in te_trc.h */
unsigned int
trc_db_new_user(te_trc_db *db)
{
    assert(db != NULL);
    return db->user_id++;
}

/* See the description in te_trc.h */
void
trc_db_free_user(te_trc_db *db, unsigned int user_id)
{
    UNUSED(db);
    UNUSED(user_id);
}

/**
 * Find user data attached to the current element of the TRC database.
 *
 * @param users_data    Users data for the current element
 * @param user_id       User ID
 *
 * @return Pointer to internal list element or NULL.
 */
static trc_user_data *
trc_db_find_user_data(const trc_users_data *users_data,
                      unsigned int          user_id)
{
    const trc_user_data *p;

    assert(users_data != NULL);
    for (p = LIST_FIRST(users_data);
         p != NULL && p->user_id != user_id;
         p = LIST_NEXT(p, links));

    /* 
     * Discard 'const' qualifier inherited from the list, since it
     * means that the function does not modify the list.
     */
    return (trc_user_data *)p;
}

/**
 * Find user data attached to the current element of the TRC database.
 *
 * @param walker        TRC database walker
 * @param user_id       User ID
 *
 * @return Pointer to internal list element or NULL.
 */
static trc_user_data *
trc_db_walker_find_user_data(const te_trc_db_walker *walker,
                             unsigned int            user_id)
{
    return trc_db_find_user_data(trc_db_walker_users_data(walker),
                                 user_id);
}

/* See the description in te_trc.h */
void *
trc_db_walker_get_user_data(const te_trc_db_walker *walker,
                            unsigned int            user_id)
{
    trc_user_data *ud = trc_db_walker_find_user_data(walker, user_id);

    return (ud == NULL) ? NULL : ud->data;
}

/* See the description in trc_db.h */
void *
trc_db_test_get_user_data(const trc_test *test, unsigned int user_id)
{
    trc_user_data *ud = trc_db_find_user_data(&test->users, user_id);

    return (ud == NULL) ? NULL : ud->data;
}

/* See the description in trc_db.h */
void *
trc_db_iter_get_user_data(const trc_test_iter *iter, unsigned int user_id)
{
    trc_user_data *ud = trc_db_find_user_data(&iter->users, user_id);

    return (ud == NULL) ? NULL : ud->data;
}

/* See the description in te_trc.h */
te_errno
trc_db_walker_set_user_data(const te_trc_db_walker *walker,
                            unsigned int user_id, void *user_data)
{
    trc_user_data *ud = trc_db_walker_find_user_data(walker, user_id);

    if (ud == NULL)
    {
        trc_users_data *list = trc_db_walker_users_data(walker);

        ud = TE_ALLOC(sizeof(*ud));
        if (ud == NULL)
            return TE_ENOMEM;
        ud->user_id = user_id;
        LIST_INSERT_HEAD(list, ud, links);
    }

    ud->data = user_data;

    return 0;
}

/* See the description in te_trc.h */
void
trc_db_walker_free_user_data(te_trc_db_walker *walker,
                             unsigned int user_id,
                             void (*user_free)(void *))
{
    trc_user_data *ud = trc_db_walker_find_user_data(walker, user_id);

    if (ud != NULL)
    {
        LIST_REMOVE(ud, links);
        if (user_free != NULL)
            user_free(ud->data);
        free(ud);
    }
}

/* See the description in te_trc.h */
te_errno
trc_db_free_user_data(te_trc_db *db, unsigned int user_id,
                      void (*test_free)(void *),
                      void (*iter_free)(void *))
{
    te_trc_db_walker       *walker;
    trc_db_walker_motion    mv;
    
    walker = trc_db_new_walker(db);
    if (walker == NULL)
        return TE_ENOMEM;

    while ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT)
    {
        if (mv != TRC_DB_WALKER_FATHER)
            trc_db_walker_free_user_data(walker, user_id,
                trc_db_walker_is_iter(walker) ? iter_free : test_free);
    }

    trc_db_free_walker(walker);

    return 0;
}
