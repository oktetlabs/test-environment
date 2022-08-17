/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * Definition of data types and API to use TRC.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TRC_H__
#define __TE_TRC_H__

#include "te_errno.h"
#include "te_queue.h"
#include "tq_string.h"
#include "te_test_result.h"
#include "logic_expr.h"
#include "logger_api.h"
#include <libxml/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Test iteration argument */
typedef struct trc_test_iter_arg {

    xmlNodePtr  node;   /**< XML node with this element */

    TAILQ_ENTRY(trc_test_iter_arg)   links;  /**< List links */

    char       *name;   /**< Argument name */
    char       *value;  /**< Argument value */

} trc_test_iter_arg;

/** Head of queue of test iteration arguments */
typedef TAILQ_HEAD(trc_test_iter_args_head, trc_test_iter_arg)
                                            trc_test_iter_args_head;

/** Head of the list with test iteration arguments */
typedef struct trc_test_iter_args {

    xmlNodePtr  node;   /**< XML node with this element */

    trc_test_iter_args_head     head;  /**< Head of the list */

    tqh_strings save_order; /**< Argument names listed in order
                                 in which they should be saved to XML.
                                 They can be stored in different
                                 order in memory when they are
                                 sorted to simplify matching
                                 TRC records. */
} trc_test_iter_args;

/** TE log test argument */
typedef struct trc_report_argument {
    char    *name;              /**< Argument name */
    char    *value;             /**< Argument value */
    te_bool  variable;          /**< Is this argument in fact a variable */
} trc_report_argument;

/**
 * Single test result with auxiliary information for TRC.
 */
typedef struct trc_exp_result_entry {
    TAILQ_ENTRY(trc_exp_result_entry)   links;  /**< List links */

    te_test_result  result; /**< Test result */

    /* Auxiliary information */
    char       *key;            /**< BugID-like information */
    char       *notes;          /**< Any kind of notes */
    te_bool     is_expected;    /**< Is this result expected
                                     (used by TRC Update) */
} trc_exp_result_entry;

/** Head of queue of expected test results */
typedef TAILQ_HEAD(trc_exp_result_entry_head, trc_exp_result_entry)
                                            trc_exp_result_entry_head;
/**
 * Expected test result.
 */
typedef struct trc_exp_result {
    STAILQ_ENTRY(trc_exp_result) links; /**< List links */

    char        *tags_str;   /**< String representation of tags logical
                                 expression */
    logic_expr  *tags_expr;  /**< Tags logical expression */
    tqh_strings *tags;       /**< Tag strings merged when updating
                                  from logs */

    trc_exp_result_entry_head  results; /**< Results expected for such
                                             tags logical expression */

    /* Auxiliary information common for expected results */
    char       *key;        /**< BugID-like information */
    char       *notes;      /**< Any kind of notes */
} trc_exp_result;


/*
 * Testing Results Comparator database interface
 */

/* Forward declaration of the TRC database */
struct te_trc_db;
/** Short alias for TRC database type */
typedef struct te_trc_db te_trc_db;

/**
 * Initialize a new TRC database.
 *
 * @param p_trc_db      Location for TRC database instance handle
 *
 * @return Status code.
 *
 * @sa trc_db_save, trc_db_close
 */
extern te_errno trc_db_init(te_trc_db **p_trc_db);

/**
 * Open TRC database.
 *
 * @param location      Location of the database
 * @param db            Location for TRC database instance handle
 *
 * @return Status code.
 *
 * @sa trc_db_save, trc_db_close
 */
extern te_errno trc_db_open(const char *location, te_trc_db **db);

/**
 * Open TRC database.
 *
 * @param location      Location of the database
 * @param db            Location for TRC database instance handle
 * @param flags         Flags (see trc_open_flags).
 *
 * @return Status code.
 *
 * @sa trc_db_save, trc_db_close
 */
extern te_errno trc_db_open_ext(const char *location, te_trc_db **db,
                                int flags);


/**
 * Close TRC database.
 *
 * @param trc_db        TRC database instance handle
 */
extern void trc_db_close(te_trc_db *trc_db);


/*
 * Testing Results Comparator database walker
 */

/* Forward declaration of the position in TRC database tree */
struct te_trc_db_walker;
/** Short alias for type to represent position in TRC database tree */
typedef struct te_trc_db_walker te_trc_db_walker;

/**
 * Allocate a new walker to traverse TRC database tree.
 *
 * @param trc_db        TRC database instance
 *
 * @return Pointer to allocated walker.
 */
extern te_trc_db_walker *trc_db_new_walker(te_trc_db *trc_db);

/**
 * Make a copy of existing database walker. It is helpful when
 * you need to save current position in TRC database to work
 * with it again later.
 *
 * @note If you remove something from database, saved walker
 *       may become invalid.
 *
 * @param walker        Walker instance
 *
 * @return Copy of the walker instance.
 */
extern te_trc_db_walker *trc_db_walker_copy(const te_trc_db_walker *walker);

/**
 * Release resources allocated for TRC database tree walker.
 *
 * @param walker        TRC database tree walker
 */
extern void trc_db_free_walker(te_trc_db_walker *walker);

/**
 * Is walker located on a test iteration (or test itself)?
 *
 * @param walker        Current walker position
 */
extern te_bool trc_db_walker_is_iter(const te_trc_db_walker *walker);

/**
 * Move walker from the current position to the child test with
 * specified name.
 *
 * @attention The function has to be called from initial walker
 *            position or iteration of any test.
 *
 * @param walker        Current walker position
 * @param test_name     Name of the test
 * @param force         Force to create DB entry, if it does not
 *                      exist (if resources allocation fails, FALSE is
 *                      returned, else TRUE is returned)
 *
 * @return Is walker in a known place in TRC database tree?
 */
extern te_bool trc_db_walker_step_test(te_trc_db_walker *walker,
                                       const char       *test_name,
                                       te_bool           force);

/**
 * Typedef for function matching iterations in TRC with
 * iterations from XML log. The first argument is pointer to
 * TRC iteration, the second one is the number of arguments
 * in the array pointer to which is passed as the third argument.
 * The last argument indicates whether to use function for
 * filtering out some iterations instead of matching.
 */
typedef int (*func_args_match_ptr)(const void *,
                                   unsigned int, trc_report_argument *,
                                   te_bool);

/**
 * Flags passed to trc_db_walker_step_iter() function.
 */
typedef enum step_iter_flags {
    STEP_ITER_NO_MATCH_OLD   = 0x1,     /**< Do not match to iterations
                                             from existing TRC DB */
    STEP_ITER_NO_MATCH_WILD  = 0x2,     /**< Do not match to wildcard
                                             iterations */
    STEP_ITER_NO_MATCH_NEW   = 0x4,     /**< Do not match to iterations
                                             created by a tool */
    STEP_ITER_CREATE_NFOUND  = 0x8,     /**< Create iteration if it
                                             cannot be found in DB. */
    STEP_ITER_CREATE_UNSPEC  = 0x10,    /**< If a new iteration is created
                                             and expected results are
                                             not known, set default expected
                                             result to TE_TEST_UNSPEC,
                                             not TE_TEST_PASSED. */
    STEP_ITER_SPLIT_RESULTS  = 0x20,    /**< For each result: split tag
                                             expression of results in
                                             conjuncts, replace single
                                             result with its copies marked
                                             by different conjuncts. */
} step_iter_flags;

/**
 * Flags from step_iter_flags related to matching iterations to DB
 * records.
 */
#define STEP_ITER_MATCH_FLAGS \
    (STEP_ITER_NO_MATCH_OLD | STEP_ITER_NO_MATCH_WILD | \
     STEP_ITER_NO_MATCH_NEW)

/**
 * Move walker from the current position to the test iteration with
 * specified arguments.
 *
 * @attention The function has to be called from the test entry
 *            position only.
 *
 * @param walker            Current walker position
 * @param n_args            Number of arguments
 * @param args              Test arguments
 * @param flags             Flags (see step_iter_flags)
 * @param db_uid            TRC database user ID
 * @param func_args_match   Function to be used instead
 *                          @b test_iter_args_match() if required.
 *
 * @return Is walker in a known place in TRC database tree?
 */
extern te_bool trc_db_walker_step_iter(te_trc_db_walker *walker,
                                       unsigned int n_args,
                                       trc_report_argument *args,
                                       uint32_t flags,
                                       unsigned int db_uid,
                                       func_args_match_ptr
                                                func_args_match);

/**
 * Move walker one step back.
 *
 * @param walker        Current walker position
 */
extern void trc_db_walker_step_back(te_trc_db_walker *walker);


/** Types of motion of the TRC database walker */
typedef enum trc_db_walker_motion {
    TRC_DB_WALKER_SON,      /**< To son */
    TRC_DB_WALKER_BROTHER,  /**< To brother */
    TRC_DB_WALKER_FATHER,   /**< To father */
    TRC_DB_WALKER_ROOT,     /**< Nowhere */
} trc_db_walker_motion;

/**
 * If previous motion is to nowhere, move to the root.
 * If previous motion is not to the parent, try to move to the first son.
 * Otherwise, move to the next brother, or to the parent, or nowhere.
 *
 * @param walker        Current walker position
 *
 * @return Made motion.
 */
extern trc_db_walker_motion trc_db_walker_move(te_trc_db_walker *walker);

/**
 * Get test iteration expected result at the current TRC database
 * walker position.
 *
 * @param walker        Current walker position
 * @param tags          Tags which identify IUT
 *
 * @return Expected result.
 * @return NULL         The test/iteration is unknown for TRC database.
 */
extern const trc_exp_result *trc_db_walker_get_exp_result(
                                 const te_trc_db_walker *walker,
                                 const tqh_strings      *tags);


/*
 * Comparison routines.
 */

/**
 * Is obtained result equal to another?
 *
 * @param lhv   Left hand result value
 * @param rhv   Right hand result value
 *
 * @return      TRUE if results are equal, FALSE otherwise.
 */
extern te_bool
te_test_results_equal(const te_test_result *lhv,
                      const te_test_result *rhv);

/**
 * Is obtained result equal to one of expected?
 *
 * @param expected      Expected results
 * @param obtained      Obtained result
 *
 * @return Pointer to the entry in expected result which is equal
 *         to obtained result.
 */
extern const trc_exp_result_entry *trc_is_result_expected(
                                       const trc_exp_result *expected,
                                       const te_test_result *obtained);

/**
 * Is expected result equal to skipped (without any verdicts).
 *
 * @param result        Expected result to check
 */
extern te_bool trc_is_exp_result_skipped(const trc_exp_result *result);


/*
 * Attached user data management.
 */

/**
 * Allocate a new user ID.
 *
 * @param db            TRC database handle
 *
 * @return Allocated user ID.
 */
extern unsigned int trc_db_new_user(te_trc_db *db);

/**
 * Free user ID.
 *
 * @param db            TRC database handle
 * @param user_id       User ID to free
 */
extern void trc_db_free_user(te_trc_db *db, unsigned int user_id);

/**
 * Get data associated by user with current position in TRC database.
 *
 * @param walker        TRC database walker
 * @param user_id       User ID
 *
 * @return Data associated by user or NULL.
 */
extern void *trc_db_walker_get_user_data(const te_trc_db_walker *walker,
                                         unsigned int            user_id);

/**
 * Get data associated by user with parent of current
 * element in TRC database.
 *
 * @param walker        TRC database walker
 * @param user_id       User ID
 *
 * @return Data associated by user or NULL.
 */
extern void *trc_db_walker_get_parent_user_data(const
                                                    te_trc_db_walker
                                                               *walker,
                                                unsigned int    user_id);
/**
 * Set data associated by user with current position in TRC database.
 *
 * @param walker        TRC database walker
 * @param user_id       User ID
 * @param user_data     User data to associate
 *
 * @return Status code.
 */
extern te_errno trc_db_walker_set_user_data(
                    const te_trc_db_walker *walker,
                    unsigned int            user_id,
                    void                   *user_data);

/**
 * Set data associated by user with current element in TRC database
 * and all its parents.
 *
 * @param walker        TRC database walker
 * @param user_id       User ID
 * @param user_data     User data to associate
 * @param data_gen      Function to generate user data for
 *                      ancestors
 *
 * @return Status code.
 */
extern te_errno trc_db_walker_set_prop_ud(
                    const te_trc_db_walker *walker,
                    unsigned int            user_id,
                    void                   *user_data,
                    void                   *(*data_gen)(void *, te_bool));

/**
 * Free user data associated by user with current position in TRC
 * database.
 *
 * @param walker        TRC database walker
 * @param user_id       User ID
 * @param user_free     Function to be used to free user data or NULL
 */
extern void trc_db_walker_free_user_data(te_trc_db_walker *walker,
                                         unsigned int user_id,
                                         void (*user_free)(void *));

/**
 * Free all data of specified user associated with elements of TRC
 * database.
 *
 * @param db            TRC database handle
 * @param user_id       User ID
 * @param test_free     Function to be used to free user data
 *                      associated with test entries or NULL
 * @param iter_free     Function to be used to free user data
 *                      associated with test iterations or NULL
 *
 * @return Status code.
 */
extern te_errno trc_db_free_user_data(te_trc_db *db,
                                      unsigned int user_id,
                                      void (*test_free)(void *),
                                      void (*iter_free)(void *));

extern int (*trc_db_compare_values)(const char *s1, const char *s2);

extern int trc_db_strcmp_tokens(const char *s1, const char *s2);
extern int trc_db_strcmp_normspace(const char *s1, const char *s2);



/**
 * Add TRC tag into the list.
 *
 * @param tags          List with TRC tags
 * @param name          Name of the tag to add
 *
 * @return Status code.
 */
static inline te_errno trc_add_tag(tqh_strings *tags, const char *name)
{
    tqe_string *p = calloc(1, sizeof(*p));
    tqe_string *tag;
    char       *col;

    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
        return TE_ENOMEM;
    }
    if (name == NULL)
    {
        ERROR("Wrong tag name given: NULL");
        return TE_EINVAL;
    }

    p->v = strdup(name);
    if (p->v == NULL)
    {
        ERROR("strdup(%s) failed", name);
        return TE_ENOMEM;
    }

    if ((col = strchr(p->v, ':')) != NULL)
        *col = '\0';

    /* do we have this tag? */
    /* memory loss here, nobody cares */
    TAILQ_FOREACH(tag, tags, links)
    {
        char *c = strchr(tag->v, ':');

        if (strncmp(p->v, tag->v,
                    c ? MAX((unsigned)(c - tag->v), strlen(p->v)) :
                    MAX(strlen(tag->v), strlen(p->v))) == 0)
        {
            tag->v = p->v;
            free(p);
            p = NULL;
            break;
        }
    }
    if (p != NULL)
        TAILQ_INSERT_TAIL(tags, p, links);

    if (col != NULL)
        *col = ':';

    return 0;
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_H__ */
