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

/** Pointer to DB we're currently working with - last DB
 * opened by trc_db_open */
extern te_trc_db *current_db;

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

/** Item of queue of included files */
typedef struct trc_file {
    TAILQ_ENTRY(trc_file)    links;     /**< Queue links */
    char                    *filename;  /**< File name */
} trc_file;

/** Queue of included files */
typedef TAILQ_HEAD(trc_files, trc_file) trc_files;

/* Forward */
struct trc_test;

/** Head of the list with tests */
typedef struct trc_tests {
    
    xmlNodePtr  node;   /**< XML node with this element */
    
    TAILQ_HEAD(, trc_test)  head;   /**< Head of the list */

} trc_tests;

/* Forward */
struct trc_global;

typedef struct trc_globals {
    xmlNodePtr  node;

    TAILQ_HEAD(, trc_global)  head;   /**< Head of the list */
} trc_globals;

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

    char               *filename;   /**< File in which this iteration
                                         is described in TRC */
    int                 file_pos;   /**< Number of the iteration in
                                         the list of all its siblings
                                         belonging to the same file */

    te_bool         newly_created;  /**< Whether this iteration was
                                         created during log processing
                                         or not */
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
    char           *path;           /**< Test path */

    char           *notes;          /**< Some notes */
    char           *objective;      /**< Test objective */

    trc_test_iters  iters;          /**< Iterations of the test */

    trc_users_data  users;          /**< Users data */

    char               *filename;   /**< File in which this test
                                         is described in TRC */
    int                 file_pos;   /**< Number of the test in
                                         the list of all its siblings
                                         belonging to the same file */
} trc_test;

typedef struct trc_global {
    xmlNodePtr node;

    TAILQ_ENTRY(trc_global)    links;

    char *name;
    char *value;
} trc_global;

/** Testing results comparison database */
struct te_trc_db {
    char           *filename;   /**< Location of the database file */
    xmlDocPtr       xml_doc;    /**< XML document */
    char           *version;    /**< Database version */
    trc_tests       tests;      /**< Tree of tests */
    unsigned int    user_id;    /**< ID of the next user */
    trc_globals     globals;
};

/** Kinds of matching of iteration TRC with iteration from XML log */
enum {
    ITER_NO_MATCH = 0,  /**< No matching */
    ITER_EXACT_MATCH,   /**< Exact matching of all arguments values */
    ITER_WILD_MATCH,    /**<  Matching to a wildcard record in TRC */
};

/**
 * Update test path.
 *
 * @param test  Test
 */
extern void trc_db_test_update_path(trc_test *test);

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
 * @param args          Array with arguments
 *
 * @note If @a names[] and @a values[] are owned by the function,
 *       the pointers in @a names and @a values are set to @c NULL.
 *       It is assumed that this memory is allocated from heap and
 *       should be deallocated using @p free().
 */
extern trc_test_iter *trc_db_new_test_iter(trc_test      *test,
                                           unsigned int   n_args,
                                           trc_report_argument *args);

extern void *trc_db_get_test_by_path(te_trc_db *db,
                                     char *path);

/**
 * Duplicate expected result entry.
 *
 * @param rentry        Result entry to be duplicated
 *
 * @return
 *      Duplicate of result
 */
extern trc_exp_result_entry *trc_exp_result_entry_dup(
                                    trc_exp_result_entry *rentry);

/**
 * Duplicate expected result.
 *
 * @param result        Result to be duplicated
 *
 * @return
 *      Duplicate of result
 */
extern trc_exp_result *trc_exp_result_dup(trc_exp_result *result);

/**
 * Get expected results from set of widely used singleton results
 * without verdicts.
 *
 * @param status        Expected test status
 *
 * @return Pointer to expected result with signle entry with specified
 *         status and no verdicts.
 */
extern const trc_exp_result *exp_defaults_get(te_test_status status);

/**
 * Duplicate expected results.
 *
 * @param result        Results to be duplicated.
 *
 * @return
 *      Duplicate results.
 */
extern trc_exp_results *trc_exp_results_dup(trc_exp_results *results);

/**
 * Copy expected results from one list at the beginning
 * of another list.
 *
 * @param dest          Destination expected results list
 * @param src           Source expected results list
 */
extern void trc_exp_results_cpy(trc_exp_results *dest,
                                trc_exp_results *src);

/**
 * Copy notes, default and expected results from
 * one test iteration to another.
 *
 * @param dest          Destination test iteration
 * @param src           Source test iteration
 */
extern void trc_db_test_iter_res_cpy(trc_test_iter *dest,
                                     trc_test_iter *src);

/**
 * Split every result having tag logical expression with more than
 * one disjunct in DNF in several results, each with only one
 * disjunct in DNF of logical expression.
 *
 * @param itr   Test iteration
 */
void trc_db_test_iter_res_split(trc_test_iter *itr);

/** TRC DB saving options */
typedef enum {
    TRC_SAVE_REMOVE_OLD    = 0x1,   /**< Remove XML representation and
                                         generate it from scratch */
    TRC_SAVE_RESULTS       = 0x2,   /**< Save expected results of
                                         iterations */
    TRC_SAVE_GLOBALS       = 0x4,   /**< Save global variables */
    TRC_SAVE_UPDATE_OLD    = 0x8,   /**< Update existing nodes */
    TRC_SAVE_DEL_XINCL     = 0x10,  /**< Delete XInclude elements */
    TRC_SAVE_NO_VOID_XINCL = 0x20,  /**< Do not mark XInclude elements
                                         having no included content
                                         between them */
    TRC_SAVE_POS_ATTR      = 0x40,  /**< Save "pos" attribute for
                                         tests and iterations
                                         which value is a number
                                         of a given element
                                         in list of its siblings
                                         included from the same
                                         file */
} trc_save_flags;

/**
 * Get XML representation of TRC test verdict.
 *
 * @param v             Verdict text
 * @param result_node   XML node where to insert XML representation
 *                      of verdict
 *
 * @return
 *      0 on success
 */
extern te_errno trc_verdict_to_xml(char *v, xmlNodePtr result_node);

/**
 * Get XML representation of TRC expected result entry.
 *
 * @param res_entry     TRC expected result
 * @param results_node  XML node where to insert XML representation
 *                      of result entry
 *
 * @return
 *      0 on success
 */
extern te_errno trc_exp_result_entry_to_xml(
                                    trc_exp_result_entry *res_entry,
                                    xmlNodePtr results_node);
/**
 * Get XML representation of TRC expected result.
 *
 * @param exp_result    TRC expected result
 * @param result_node   XML node pointer where to insert XML
 *                      representation of TRC expected result
 * @param is_default    Whether expected result is default or not
 *
 * @return
 *      0 on success
 */
extern te_errno trc_exp_result_to_xml(trc_exp_result *exp_result,
                                      xmlNodePtr result_node,
                                      te_bool is_default);

/**
 * Get XML representation of TRC expected results.
 *
 * @param exp_results   TRC expected results
 * @param node          XML node pointer
 * @param insert_after  Whether to insert <results> nodes
 *                      after @p node or into it.
 *
 * @return
 *      0 on success
 */
extern te_errno trc_exp_results_to_xml(trc_exp_results *exp_results,
                                       xmlNodePtr node,
                                       te_bool insert_after);

/**
 * Get expected verdict from XML.
 *
 * @param node          Location of the node of the test result
 * @param verdict       Location for the expected verdict data
 *
 * @return Status code.
 */
extern te_errno get_expected_verdict(xmlNodePtr node,
                                     char **verdict);

/**
 * Get expected result entry from XML.
 *
 * @param node          Location of the node of the test result entry
 * @param rentry        Location for the expected result entry data
 *
 * @return Status code.
 */
extern te_errno get_expected_rentry(xmlNodePtr node,
                                    trc_exp_result_entry *rentry);

/**
 * Get expected results from XML.
 *
 * @param node          Location of the node of the test result
 * @param results       Location for the expected result data
 * @param tags_tolerate Whether to allow absence of tags attribute or
 *                      void string as its value or not
 *
 * @return Status code.
 */
extern te_errno get_expected_result(xmlNodePtr node,
                                    trc_exp_result *result,
                                    te_bool tags_tolerate);

/**
 * Get expected results from XML.
 *
 * @param node      Location of the first non-arg node of the test 
 *                  iteration (IN), of the first non-result node of the
 *                  test iteration (OUT)
 * @param results   Location for the expected results data
 *
 * @return Status code.
 */
extern te_errno get_expected_results(xmlNodePtr *node,
                                     trc_exp_results *results);

/**
 * Get test iteration arguments.
 *
 * @param node      XML node
 * @param args      List of arguments to be filled in
 *
 * @return Status code
 */
extern te_errno get_test_args(xmlNodePtr *node, trc_test_iter_args *args);

/**
 * Get text content of the node.
 *
 * @param node      Location of the XML node pointer
 * @param content   Location for the result
 *
 * @return Status code
 */
extern te_errno trc_db_get_text_content(xmlNodePtr node, char **content);

/**
 * Save TRC database to file
 *
 * @param db            TRC database
 * @param filename      Name of file
 * @param flags         Flags
 * @param uid           TRC DB User ID
 * @param to_save       Function to determine whether to
 *                      save a given test or iteration
 *                      (takes user data and boolean which
 *                      is TRUE if iteration is considered as
 *                      parameters)
 * @param set_user_attr Function determining which value
 *                      should be assigned to "user_attr"
 *                      attribute of test or iteration
 * @param cmd           Command used to generate TRC DB
 *
 * @return
 *      0 on success or error code
 */
extern te_errno trc_db_save(te_trc_db *db, const char *filename,
                            int flags, int uid,
                            te_bool (*to_save)(void *, te_bool),
                            char *(*set_user_attr)(void *, te_bool),
                            char *cmd);

extern void trc_db_free(te_trc_db *db);

/**
 * Remove all expected results from TRC DB,
 * unlink and free related XML nodes.
 *
 * @param db      TRC database
 */
extern void trc_remove_exp_results(te_trc_db *db);

/**
 * Free resources allocated for the list of tests.
 *
 * @param tests     List of tests to be freed
 */
extern void trc_free_trc_tests(trc_tests *tests);

/**
 * Free resources allocated for the test iteration.
 *
 * @param iter     Test iteration to be freed
 */
extern void trc_free_test_iter(trc_test_iter *iter);

/**
 * Free resources allocated for the list of test arguments.
 *
 * @param head      Head of list of test arguments to be freed
 */
extern void trc_free_test_iter_args_head(trc_test_iter_args_head *head);

/**
 * Free resources allocated for the list of test arguments.
 *
 * @param args      Structure containing head of
 *                  list of test arguments to be freed
 */
extern void trc_free_test_iter_args(trc_test_iter_args *args);

/**
 * Duplicate list of test arguments.
 *
 * @param args      Structure containing head of
 *                  list of test arguments to be freed
 */
extern trc_test_iter_args *trc_test_iter_args_dup(
                                        trc_test_iter_args *args);

/**
 * Free resources allocated for expected result entry.
 *
 * @param rentry        Result entry to be freed
 */
extern void trc_exp_result_entry_free(trc_exp_result_entry *rentry);

/**
 * Free resources allocated for expected result.
 *
 * @param result        Result to be freed
 */
extern void trc_exp_result_free(trc_exp_result *result);

/**
 * Free resources allocated for the list of expected results.
 *
 * @param iters     List of expected results to be freed
 */
extern void trc_exp_results_free(trc_exp_results *results);

/**
 * Delete all wildcard iterations from a given test
 *
 * @param test        Test
 */
extern void trc_db_test_delete_wilds(trc_test *test);

extern trc_test *trc_db_walker_get_test(const te_trc_db_walker *walker);
extern trc_test_iter *trc_db_walker_get_iter(
                          const te_trc_db_walker *walker);

/**
 * Get users data of current TRC database element
 *
 * @param walker    TRC database walker
 *
 * @return
 *      Users data
 */
extern trc_users_data *trc_db_walker_users_data(
                           const te_trc_db_walker *walker);

/**
 * Get users data of parent of current TRC database element
 *
 * @param walker    TRC database walker
 *
 * @return
 *      Users data
 */
extern trc_users_data *trc_db_walker_parent_users_data(
                           const te_trc_db_walker *walker);


extern void *trc_db_test_get_user_data(const trc_test *test,
                                       unsigned int    user_id);

extern void *trc_db_iter_get_user_data(const trc_test_iter *iter,
                                       unsigned int         user_id);

/**
 * Set data associated by user with a given element in TRC database.
 *
 * @param db_item       Element in TRC database (test or test iteration)
 * @param is_iter       Is db_item iteration or not?
 * @param user_id       User ID
 * @param user_data     User data to associate
 *
 * @return Data associated by user or NULL.
 */
extern te_errno trc_db_set_user_data(void *db_item,
                                     te_bool is_iter,
                                     unsigned int user_id,
                                     void *user_data);

/**
 * Set data associated by user with a given iteration in TRC database.
 *
 * @param iter          Test iteration handler
 * @param user_id       User ID
 * @param user_data     User data to associate
 *
 * @return Data associated by user or NULL.
 */
extern te_errno trc_db_iter_set_user_data(trc_test_iter *iter,
                                          unsigned int user_id,
                                          void *user_data);
/**
 * Set data associated by user with a given iteration in TRC database.
 *
 * @param test          Test handler
 * @param user_id       User ID
 * @param user_data     User data to associate
 *
 * @return Data associated by user or NULL.
 */
extern te_errno trc_db_test_set_user_data(trc_test *test,
                                          unsigned int user_id,
                                          void *user_data);

/**
 * Match TRC database arguments vs arguments specified by caller.
 *
 * @param db_args       List with TRC database arguments
 * @parma n_args        Number of elements in @a names and @a values
 *                      arrays
 * @param names         Array with names of arguments
 * @param is_strict     Can arguments be omitted in wildcard or not?
 *
 * @retval ITER_NO_MATCH        No matching
 * @retval ITER_EXACT_MATCH     Exact matching of all arguments values
 * @retval ITER_WILD_MATCH      Matching to a wildcard record in TRC
 */
extern int test_iter_args_match(const trc_test_iter_args  *db_args,
                                unsigned int               n_args,
                                trc_report_argument       *args,
                                te_bool                    is_strict);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_DB_H__ */
