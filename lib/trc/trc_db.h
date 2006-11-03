/** @file
 * @brief Testing Results Comparator
 *
 * Definition of database representation and functions.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TRC_DB_H__
#define __TE_TRC_DB_H__

#include <libxml/tree.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "logic_expr.h"

#include "te_trc.h"

#ifdef __cplusplus
extern "C" {
#endif


/** User data associated with TRC database element */
typedef struct trc_user_data {
    LIST_ENTRY(trc_user_data)   links;
    unsigned int                user_id;
    void                       *data;
} trc_user_data;

/** List with users' data associated with TRC database element */
typedef LIST_HEAD(, trc_user_data)  trc_users_data;


/** List of expected results */
typedef SLIST_HEAD(trc_exp_results, trc_exp_result) trc_exp_results;


/** Test iteration argument */
typedef struct trc_test_iter_arg {
    
    xmlNodePtr  node;   /**< XML node with this element */

    TAILQ_ENTRY(trc_test_iter_arg)   links;  /**< List links */

    char       *name;   /**< Argument name */
    char       *value;  /**< Argument value */

} trc_test_iter_arg;

/** Head of the list with test iteration arguments */
typedef struct trc_test_iter_args {

    xmlNodePtr  node;   /**< XML node with this element */

    TAILQ_HEAD(, trc_test_iter_arg) head;   /**< Head of the list */

} trc_test_iter_args;


/* Forward */
struct trc_test;

/** Head of the list with tests */
typedef struct trc_tests {
    
    xmlNodePtr  node;   /**< XML node with this element */
    
    TAILQ_HEAD(, trc_test)  head;   /**< Head of the list */

} trc_tests;


/** Test iteration */
typedef struct trc_test_iter {

    xmlNodePtr  node;   /**< XML node with this element */
    
    TAILQ_ENTRY(trc_test_iter)  links;  /**< List links */
    
    struct trc_test    *parent;     /**< Back reference */

    trc_test_iter_args  args;       /**< Iteration arguments */
    char               *notes;      /**< Common notes */

    const trc_exp_result   *exp_default;    /**< Default result */
    trc_exp_results         exp_results;    /**< The expected results */

    trc_tests           tests;      /**< Children tests of the session */

    trc_users_data      users;      /**< Users data */

} trc_test_iter;

/** Head of the list with test iterations */
typedef struct trc_test_iters {

    xmlNodePtr  node;   /**< XML node with this element */

    TAILQ_HEAD(, trc_test_iter) head;   /**< Head of the list */

} trc_test_iters;


/** Types of tests */
typedef enum trc_test_type {
    TRC_TEST_UNKNOWN = 0,   /**< Unknown (not initialized) test type */
    TRC_TEST_SCRIPT,        /**< Standalone script-executable */
    TRC_TEST_SESSION,       /**< Group of tests */
    TRC_TEST_PACKAGE,       /**< Group of tests described in a separate
                                 file */
} trc_test_type;

/** Test run */
typedef struct trc_test {

    xmlNodePtr  node;   /**< XML node with this element */

    TAILQ_ENTRY(trc_test)   links;  /**< List links */

    trc_test_iter  *parent;         /**< Back reference */

    trc_test_type   type;           /**< Type of the test */
    te_bool         aux;            /**< Is test auxiliary? */
    char           *name;           /**< Test name */
    char           *notes;          /**< Some notes */

    char           *objective;      /**< Test objective */
    xmlNodePtr      obj_node;       /**< XML node with objective */
    te_bool         obj_update;     /**< Whether objective of the test
                                         should be updated */

    trc_test_iters  iters;          /**< Iterations of the test */

    trc_users_data  users;          /**< Users data */

} trc_test;


/** Testing results comparison database */
struct te_trc_db {
    char           *filename;   /**< Location of the database file */
    xmlDocPtr       xml_doc;    /**< XML document */
    char           *version;    /**< Database version */
    trc_tests       tests;      /**< Tree of tests */
    unsigned int    user_id;    /**< ID of the next user */
};


/**
 * Allocate and initialize control structure for a new test entry.
 *
 * @param tests         List of tests to add a new
 * @param parent        Parent test iteration
 * @param name          Name of the test or NULL
 */
extern trc_test *trc_db_new_test(trc_tests *tests, trc_test_iter *parent,
                                 const char *name);

/**
 * Allocate and initialize control structure for a new test iteration.
 *
 * @param parent        Test a new iteration belongs to
 * @param n_args        Number of arguments
 * @param names         Array with argument names
 * @param values        Array with argument values
 *
 * @note If @a names[] and @a values[] are owned by the function,
 *       the pointers in @a names and @a values are set to @c NULL.
 *       It is assumed that this memory is allocated from heap and
 *       should be deallocated using @p free().
 */
extern trc_test_iter *trc_db_new_test_iter(trc_test      *test,
                                           unsigned int   n_args,
                                           char         **names,
                                           char         **values);


extern te_errno trc_db_save(te_trc_db *db, const char *filename);

extern void trc_db_free(te_trc_db *db);

extern trc_test *trc_db_walker_get_test(const te_trc_db_walker *walker);
extern trc_test_iter *trc_db_walker_get_iter(
                          const te_trc_db_walker *walker);

extern trc_users_data *trc_db_walker_users_data(
                           const te_trc_db_walker *walker);


extern void *trc_db_test_get_user_data(const trc_test *test,
                                       unsigned int    user_id);

extern void *trc_db_iter_get_user_data(const trc_test_iter *iter,
                                       unsigned int         user_id);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_DB_H__ */
