/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator: update tool
 *
 * Definition of TRC report tool types and related routines.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TRC_UPDATE_H__
#define __TE_TRC_UPDATE_H__

#include "te_defs.h"
#include "te_queue.h"
#include "te_errno.h"
#include "tq_string.h"
#include "te_trc.h"
#include "trc_db.h"
#include "trc_report.h"

/* TODO: possibly missing some headers */

#ifdef __cplusplus
extern "C" {
#endif

enum trc_update_log_parse_flags {
    TRC_UPDATE_TAGS_BY_LOGS = (1LLU << 0),  /**< If tag expression for
                                                 log is not specified,
                                                 derive it from its file
                                                 name */
    TRC_UPDATE_FAKE_LOG     = (1LLU << 1),  /**< Parse log of fake
                                                 Tester run */
    TRC_UPDATE_MERGE_LOG    = (1LLU << 2),  /**< Merge iterations from
                                                 log into TRC DB
                                                 performing TRC
                                                 update */
    TRC_UPDATE_RULES_ALL    = (1LLU << 3),  /**< Generate updating rules
                                                 for all possible results
                                                 (not only those for
                                                 which there are new
                                                 results in logs) */
    TRC_UPDATE_USE_RULE_IDS = (1LLU << 4),  /**< Insert updating rule
                                                 ID in user_attr
                                                 attribute of test
                                                 iterations in
                                                 generated TRC
                                                 to simplify applying
                                                 of edited rules */
    TRC_UPDATE_NO_GEN_WILDS = (1LLU << 5),  /**< Do not replace test
                                                 iterations with
                                                 wildcards in
                                                 generated TRC */
    TRC_UPDATE_LOG_WILDS    = (1LLU << 6),  /**< Generate wildcards for
                                                 results from logs, not
                                                 from TRC DB */
    TRC_UPDATE_LOG_WILDS_UNEXP
                            = (1LLU << 7),  /**< Generate wildcards for
                                                 unexpected results from
                                                 logs only */
    TRC_UPDATE_COPY_OLD     = (1LLU << 8),  /**< Copy results from
                                                 current TRC DB in <new>
                                                 section of updating
                                                 rule */
    TRC_UPDATE_COPY_CONFLS  = (1LLU << 9),  /**< Copy conflicting results
                                                 from logs in <new>
                                                 section of updating
                                                 rule */
    TRC_UPDATE_COPY_OLD_FIRST
                            = (1LLU << 10), /**< This flag is useful
                                                 only if both
                                                 TRC_UDPATE_COPY_OLD and
                                                 TRC_UPDATE_COPY_CONFLS
                                                 are specified.
                                                 If this flag is set,
                                                 then when copying results
                                                 to <new> section, firstly
                                                 results from current TRC DB
                                                 should be processed;
                                                 otherwise firstly
                                                 conflicting results
                                                 should be processed */
    TRC_UPDATE_COPY_BOTH    = (1LLU << 11), /**< This flag is useful
                                                 only if both
                                                 TRC_UDPATE_COPY_OLD and
                                                 TRC_UPDATE_COPY_CONFLS
                                                 are specified. If this
                                                 flag is set, then both
                                                 types of results are
                                                 copied to <new>, in
                                                 order specified by
                                                 TRC_UPDATE_COPY_OLD_FIRST
                                                 flag. If it is not
                                                 set, then only the
                                                 first type is copied */
    TRC_UPDATE_CONFLS_ALL   = (1LLU << 12), /**< Treat all results from
                                                 logs as unexpected
                                                 ones */
    TRC_UPDATE_TAGS_STR     = (1LLU << 13), /**< Do not change string
                                                 representation of
                                                 tags */
    TRC_UPDATE_GEN_APPLY    = (1LLU << 14), /**< Apply updating rules
                                                 after generating
                                                 them */
    TRC_UPDATE_RULES_CONFL  = (1LLU << 15), /**< If applying of a rule
                                                 leads to replacing
                                                 some alredy existing
                                                 expected results with
                                                 different ones,
                                                 do not replace them
                                                 but treat results
                                                 from '<new>' section
                                                 of rule as conflicting
                                                 results from logs */
    TRC_UPDATE_RRESULTS    = (1LLU << 16),  /**< Generate updating
                                                 rules of type @c
                                                 TRC_UPDATE_RRESULTS */
    TRC_UPDATE_RRESULT     = (1LLU << 17),  /**< Generate updating
                                                 rules of type @c
                                                 TRC_UPDATE_RRESULT */
    TRC_UPDATE_RRENTRY     = (1LLU << 18),  /**< Generate updating
                                                 rules of type @c
                                                 TRC_UPDATE_RRENTRY */
    TRC_UPDATE_RVERDICT    = (1LLU << 19),  /**< Generate updating
                                                 rules of type @c
                                                 TRC_UPDATE_RVERDICT */
    TRC_UPDATE_PRINT_PATHS = (1LLU << 19),  /**< Output test paths
                                                 encountered in logs
                                                 and exit */
    TRC_UPDATE_NO_PE       = (1LLU << 21),  /**< Do not take into
                                                 consideration prologues
                                                 and epilogues */
    TRC_UPDATE_RULE_UPD_ONLY
                           = (1LLU << 22),  /**< Save only tests for
                                                 which iterations at
                                                 least one rule was
                                                 applied */
    TRC_UPDATE_SKIPPED     = (1LLU << 23),  /**< Show skipped unexpected
                                                 results */
    TRC_UPDATE_NO_SKIP_ONLY
                           = (1LLU << 24),  /**< Do not create rules with
                                                 <conflicts/> containing
                                                 skipped only results */
    TRC_UPDATE_NO_EXP_ONLY = (1LLU << 25),  /**< Do not create rules with
                                                 <conflicts/> containing
                                                 expected only results
                                                 if CONFLS_ALL is turned
                                                 on */
    TRC_UPDATE_GEN_TAGS    = (1LLU << 27),  /**< Generate tags for
                                                 logs */
    TRC_UPDATE_EXT_WILDS   = (1LLU << 28),  /**< Specify a value for
                                                 each argument in
                                                 wildcard where it is
                                                 possible for a given
                                                 wildcard */
    TRC_UPDATE_SIMPL_TAGS  = (1LLU << 29),  /**< Simplify tag
                                                 expressions in lists
                                                 of unexpected results
                                                 from logs */
    TRC_UPDATE_INTERSEC_WILDS
                           = (1LLU << 32),  /**< It's allowed for
                                                 iteration to have more
                                                 than one wildcard
                                                 describing it */
    TRC_UPDATE_NO_GEN_FSS  = (1LLU << 33),  /**< Do not try to
                                                 find out subsets
                                                 corresponding to
                                                 every possible
                                                 iteration record, do
                                                 not use algorithms
                                                 based on it */
    TRC_UPDATE_FSS_UNLIM   = (1LLU << 34),  /**< Do not resrict amount
                                                 of time used to
                                                 find out subsets for
                                                 every possible
                                                 iteration record */
    TRC_UPDATE_NO_R_FAIL   = (1LLU << 35),  /**< Do not consider
                                                 results of kind
                                                 "FAILED without
                                                  verdicts" */
    TRC_UPDATE_NO_INCOMPL  = (1LLU << 36),  /**< Do not consider
                                                 INCOMPLETE
                                                 results */
    TRC_UPDATE_NO_INT_ERR  = (1LLU << 37),  /**< Do not consider
                                                 results with
                                                 internal error */
    TRC_UPDATE_FILT_LOG    = (1LLU << 39),  /**< Log to be used
                                                 for filtering out
                                                 iterations not
                                                 appearing in it */
    TRC_UPDATE_RULE_ARGS   = (1LLU << 40),  /**< Generate <args>
                                                 tags for generated
                                                 rules */
    TRC_UPDATE_TAGS_GATHER = (1LLU << 41),  /**< Gather tags from
                                                 logs and print
                                                 them */
};

/** All rule type flags */
#define TRC_UPDATE_RTYPES \
    (TRC_UPDATE_RRESULTS | TRC_UPDATE_RRESULT | \
     TRC_UPDATE_RRENTRY | TRC_UPDATE_RVERDICT)


/** Group of logs with the same tag expression */
typedef struct trc_update_tag_logs {
    TAILQ_ENTRY(trc_update_tag_logs)    links;     /**< Queue links */
    char                               *tags_str;  /**< Tag expression
                                                        in string */
    logic_expr                         *tags_expr; /**< Logical tag
                                                        expression */
    tqh_strings                         logs;      /**< Logs paths */
} trc_update_tag_logs;

/** Queue of groups of logs */
typedef TAILQ_HEAD(trc_update_tags_logs,
                   trc_update_tag_logs) trc_update_tags_logs;

/** Entry of list of wildcards used in updating rules */
typedef struct trc_update_wilds_list_entry {
    SLIST_ENTRY(trc_update_wilds_list_entry)  links;  /**< List links */

    trc_test_iter_args      *args;   /**< Wildcard arguments */

    te_bool is_strict; /**< Can arguments be omitted in wildcard or not */
} trc_update_wilds_list_entry;

/** List of wildcards used in updating rules */
typedef SLIST_HEAD(trc_update_wilds_list, trc_update_wilds_list_entry)
                                                    trc_update_wilds_list;

/** TRC updating rule types */
typedef enum trc_update_rtype {
    TRC_UPDATE_RULE_RESULTS,    /**< Applicable to all iteration
                                     results as a whole */
    TRC_UPDATE_RULE_RESULT,     /**< Applicable to content of single
                                     <results> tags */
    TRC_UPDATE_RULE_ENTRY,     /**< Applicable to content of single
                                    <result> tags */
    TRC_UPDATE_RULE_VERDICT,    /**< Applicable to content of single
                                     <verdict> tags */
    TRC_UPDATE_RULE_UNKNOWN,    /**< Unknown */
} trc_update_rtype;

/** TRC updating rule */
typedef struct trc_update_rule {
    TAILQ_ENTRY(trc_update_rule)    links;       /**< Queue links */
    trc_exp_result                 *def_res;     /**< Default results */
    trc_exp_results                *old_res;     /**< Results in TRC */
    trc_exp_result_entry           *old_re;      /**< Content of a <result>
                                                      tag in TRC */
    char                           *old_v;       /**< Content of a
                                                      <verdict> tag in
                                                      TRC */

    trc_exp_results                *confl_res;   /**< Not-matching TRC
                                                      results from logs */

    trc_exp_results                *new_res;     /**< Results to replace
                                                      results in TRC */
    trc_exp_result_entry           *new_re;      /**< Replacement for
                                                      content of a <result>
                                                      tag in TRC */
    char                           *new_v;       /**< Replacement for
                                                      content of a
                                                      <verdict> tag in
                                                      TRC */

    trc_update_wilds_list          *wilds;       /**< Wildcards */
    tqh_strings                    *match_exprs; /**< Matching
                                                      expressions */
    te_bool                         apply;       /**< Should this rule be
                                                      applied or not */
    int                             rule_id;     /**< Rule ID */
    trc_update_rtype                type;        /**< Rule type */
} trc_update_rule;

/** TRC updating rules queue */
typedef TAILQ_HEAD(trc_update_rules, trc_update_rule) trc_update_rules;

/** Results simplification status */
typedef enum res_simpl_stat {
    RES_NO_SIMPLE = 0,  /**< Not simplified yet */
    RES_TO_REPLACE,     /**< Should be replaced
                             with already known
                             simplified version */
    RES_SIMPLE,         /**< Already simplified */
} res_simpl_stat;

/** Predeclaration, see definition below */
struct trc_update_args_group;
typedef struct trc_update_args_group trc_update_args_group;

/** List of TRC DB wildcards */
typedef SLIST_HEAD(trc_update_args_groups, trc_update_args_group)
                                                trc_update_args_groups;

/** TRC Update test iteration data attached to iteration in TRC DB */
typedef struct trc_update_test_iter_data {
    trc_exp_results       new_results; /**< Non-matching test results from
                                            logs */
    trc_exp_results       df_results;  /**< Test results from the second
                                            group of logs which was not
                                            found in the first group of
                                            logs */
    trc_update_rule      *rule;        /**< Updating rule for this
                                            iteration */
    int                   rule_id;     /**< It needs to be saved for correct
                                            "user_attr" attribute setting
                                            because rules itself are
                                            cleared before saving
                                            resulting XML file */
    te_bool               to_save;     /**< Should this iteration
                                            be saved? */
    te_bool               to_save_old; /**< Previous value of to_save */
    int                   counter;     /**< This counter is used for
                                            discovering skipped iterations
                                            */
    int                   results_id;  /**< Results ID (used in wildcards
                                            generation) */
    te_bool               in_wildcard; /**< Whether this iteration in
                                            some wildcard already or not */
    te_bool               filtered;    /**< Iteration was found in fake
                                            filter log */
    res_simpl_stat        r_simple;    /**< Results simplification status */

    trc_update_args_groups  all_wilds; /**< Here all possible wildcards
                                            defining the same iteration(s)
                                            can be stored */

    /*
     * We store this kind of representation of arguments to make
     * use of existing TRC arguments matching function for wildcards
     * generation
     */
    trc_report_argument  *args;        /**< Iteration arguments */
    unsigned int          args_n;      /**< Number of arguments */
    unsigned int          args_max;    /**< Count of elements for
                                            each space was allocated
                                            in arguments array */

    int     *set_nums;      /**< Numbers of sets in which this iteration
                                 is included */
    int      nums_cnt;      /**< Count of sets */
    int      nums_max;      /**< Maximum count of sets before reallocation
                                 of memory will be required */
} trc_update_test_iter_data;

/** TRC Update test data attached to test in TRC DB */
typedef struct trc_update_test_data {
    te_bool     to_save; /**< Should this test be saved? */
} trc_update_test_data;

/** Entry of queue containing information about tests to be updated */
typedef struct trc_update_test_entry {
    TAILQ_ENTRY(trc_update_test_entry)   links; /**< List links */
    trc_test                            *test;  /**< Test in TRC DB */

    trc_update_args_groups  *sets;      /**< Sets of iterations
                                             described by all possible
                                             iteration records */
    int                      sets_cnt;  /**< Count of sets */
    int                      sets_max;  /**< Maximum count of sets before
                                             reallocation will be
                                             required */

} trc_update_test_entry;

/** Queue containing information about tests to be updated */
typedef TAILQ_HEAD(trc_update_test_entries,
                   trc_update_test_entry) trc_update_test_entries;

/**
 * Entry of queue containing information about groups of tests
 * to be updated
 */
typedef struct trc_update_tests_group {
    TAILQ_ENTRY(trc_update_tests_group)  links; /**< List links */
    trc_update_test_entries              tests; /**< Related tests */
    char                                *path;  /**< Path in TRC DB */
    trc_update_rules                    *rules; /**< Updating rules */
} trc_update_tests_group;

/** Queue containing information about groups of tests to be updated */
typedef TAILQ_HEAD(trc_update_tests_groups,
                   trc_update_tests_group) trc_update_tests_groups;

/** Group of iteration arguments (describes wildcard) */
struct trc_update_args_group {
    SLIST_ENTRY(trc_update_args_group)  links;       /**< List links */
    trc_test_iter_args                 *args;        /**< Arguments */

    trc_exp_results  *exp_results; /**< Expected results of iterations
                                        matching wildcard */
    trc_exp_result   *exp_default; /**< Default result of iterations
                                        matching wildcard */
    int               group_id;    /**< Group ID */

    unsigned int first_iter_num;   /**< Number of the first matching
                                        iteration */
};

/** Context of TRC Update tool */
typedef struct trc_update_ctx {
    uint64_t                 flags;           /**< Flags */
    te_trc_db               *db;              /**< TRC DB */
    unsigned int             db_uid;          /**< TRC DB user ID */
    tqh_strings              test_names;      /**< Test paths */
    tqh_strings              tags_gen_list;   /**< List of tags for
                                                   automatical determining
                                                   of tag expression for
                                                   a log */
    tqh_strings              tags;            /**< Tags from currently
                                                   processed logs */
    tqh_strings              collected_tags;  /**< Full list of tags
                                                   appeared in processed
                                                   logs */
    trc_update_tags_logs     tags_logs;       /**< Queue of logs grouped by
                                                   tag expressions */
    char                    *fake_log;        /**< Tester fake run XML log
                                                   path */
    char                    *fake_filt_log;   /**< Tester fake run XML log
                                                   path (this log is used
                                                   for filtering out
                                                   iterations not matching
                                                   some reqs) */
    char                    *rules_load_from; /**< Path to file with
                                                   updating rules to
                                                   apply */
    char                    *rules_save_to;   /**< Path to file where
                                                   generated updating
                                                   rules should be saved */
    char                    *cmd;             /**< Command used to run
                                                   TRC Update Tool */

    func_args_match_ptr      func_args_match; /**< Function to match
                                                   iterations in TRC with
                                                   iterations from logs */
    char                    *tags_gather_to;  /**< Where to save gathered
                                                   tags */
    char                    *logs_dump;       /**< Path to logs dump */

    logic_expr                 *merge_expr; /**< Tag expression with
                                                 which new results
                                                 should be merged into
                                                 existing database */
    char                       *merge_str;  /**< String representation
                                                 of tag expression */

    trc_update_tests_groups  updated_tests; /**< Groups of tests to be
                                                 updated */
    trc_update_rules         global_rules;  /**< Updating rules which
                                                 can be applied to any
                                                 iteration of any test */

    int                 cur_lnum;   /**< Number of currently parsed
                                         log */

} trc_update_ctx;

/**
 * Initialize TRC Update tool context.
 *
 * @param ctx_p     Context pointer
 */
extern void trc_update_init_ctx(trc_update_ctx *ctx_p);

/**
 * Free TRC Update tool context.
 *
 * @param ctx_p     Context pointer
 */
extern void trc_update_free_ctx(trc_update_ctx *ctx_p);

/**
 * Initialize structure describing group of logs.
 *
 * @param tag_logs  Structure to be initialized
 */
extern void tag_logs_init(trc_update_tag_logs *tag_logs);

/**
 * Free structure describing group of logs.
 *
 * @param tag_logs  Structure to be freed
 */
extern void trc_update_tag_logs_free(trc_update_tag_logs *tag_logs);

/**
 * Free queue of Tester run logs paths.
 *
 * @param tags_logs     Queue pointer
 */
extern void trc_update_tags_logs_free(trc_update_tags_logs *tags_logs);

/**
 * Remove from a queue of tags those having no logs attached.
 *
 * @param tags_logs     Queue pointer
 */
extern void trc_update_tags_logs_remove_empty(
                                trc_update_tags_logs *tags_logs);

/**
 * Initialize TRC Update test iteration data.
 *
 * @param data      Data to be initialized.
 */
extern void trc_update_init_test_iter_data(
                        trc_update_test_iter_data *data);

/**
 * Free TRC Update test iteration data.
 *
 * @param data      Data to be freed.
 */
extern void trc_update_free_test_iter_data(
                        trc_update_test_iter_data *data);

/**
 * Free entry of wildcards list.
 *
 * @param entry     List entry to be freed
 */
extern void trc_update_wilds_list_entry_free(
                        trc_update_wilds_list_entry *entry);

/**
 * Free list of TRC DB wildcards.
 *
 * @param list  List to be freed
 */
extern void trc_update_wilds_list_free(trc_update_wilds_list *list);

/**
 * Free TRC updating rule.
 *
 * @param rule  Rule to be freed
 */
extern void trc_update_rule_free(trc_update_rule *rule);

/**
 * Free queue of updating rules.
 *
 * @param rules     Queue to be freed
 */
extern void trc_update_rules_free(trc_update_rules *rules);

/**
 * Compare test iteration results.
 *
 * @param p     The first result
 * @param q     The second result
 *
 * @result -1 if the first result is 'less' the second one,
 *          0 if they are 'equal',
 *          1 if the first result is 'greater' than the
 *          second one.
 */
extern int te_test_result_cmp(te_test_result *p,
                              te_test_result *q);

/**
 * Compare test iteration result entries (content of single
 * <result> tags).
 *
 * @param p     The first result
 * @param q     The second result
 *
 * @result -1 if the first result is 'less' the second one,
 *          0 if they are 'equal',
 *          1 if the first result is 'greater' than the
 *          second one.
 */
extern int trc_update_rentry_cmp(trc_exp_result_entry *p,
                                 trc_exp_result_entry *q);

/**
 * Compare expected results of iterations (used for ordering).
 *
 * @param p         First expected result
 * @param q         Second expected result
 * @param tags_cmp  Whether to compare string representation of
 *                  tag expressions or not
 *
 * @return -1, 0 or 1 as a result of comparison
 */
extern int trc_update_result_cmp_gen(trc_exp_result *p,
                                     trc_exp_result *q,
                                     te_bool tags_cmp);
/**
 * Compare expected results of iterations (used for ordering).
 *
 * @param p         First expected result
 * @param q         Second expected result
 *
 * @return -1, 0 or 1 as a result of comparison
 */
extern int trc_update_result_cmp(trc_exp_result *p, trc_exp_result *q);

/**
 * Compare expected results of iterations (used for ordering),
 * do not consider tag expressions in comparison.
 *
 * @param p         First expected result
 * @param q         Second expected result
 *
 * @return -1, 0 or 1 as a result of comparison
 */
extern int trc_update_result_cmp_no_tags(trc_exp_result *p,
                                         trc_exp_result *q);

/**
 * Compare lists of expected results (used for ordering).
 *
 * @param p         First expected results list.
 * @param q         Second expected results list.
 *
 * @return -1, 0 or 1 as a result of comparison
 */
extern int trc_update_results_cmp(trc_exp_results *p,
                                  trc_exp_results *q);

/**
 * Compare updating rules.
 *
 * @param p     The first rule
 * @param q     The second rule
 *
 * @return -1, 0 or 1 as a result of comparison
 */
extern int trc_update_rules_cmp(trc_update_rule *p,
                                trc_update_rule *q);

/**
 * Insert rule in a queue in proper place
 * (so that the queue will remain be sorted in increasing order).
 *
 * @param rule          Updating rule
 * @param rules         Rules queue where to insert
 * @param rules_cmp     Comparing function
 *
 * @return -1, 0 or 1 as a result of comparison
 */
extern te_errno trc_update_ins_rule(trc_update_rule *rule,
                                    trc_update_rules *rules,
                                    int (*rules_cmp)(trc_update_rule *,
                                                     trc_update_rule *));

/**
 * Free entry of queue of tests to be updated.
 *
 * @param test_entry   Queue entry to be freed
 */
extern void trc_update_test_entry_free(trc_update_test_entry *test_entry);

/**
 * Free queue of tests to be updated.
 *
 * @param tests     Queue to be freed
 */
extern void trc_update_test_entries_free(trc_update_test_entries *tests);

/**
 * Free structure describing group of tests to be updated.
 *
 * @param group     Pointer to structure describing group of tests
 */
extern void trc_update_tests_group_free(trc_update_tests_group *group);

/**
 * Free structure describing group of tests to be updated.
 *
 * @param group     Pointer to structure describing group of tests
 */
extern void trc_update_tests_group_free(trc_update_tests_group *group);

/**
 * Free queue of groups of tests to be updated.
 *
 * @param groups     Pointer to queue of groups of tests
 */
extern void trc_update_tests_groups_free(trc_update_tests_groups *groups);

/**
 * Free group of arguments (wildcard).
 *
 * @param args_group    Group to be freed
 */
extern void trc_update_args_group_free(trc_update_args_group *args_group);

/**
 * Free list of group of arguments (wildcards).
 *
 * @param args_groups   List to be freed
 */
extern void trc_update_args_groups_free(
                                trc_update_args_groups *args_groups);

/**
 * Duplicate TRC DB iteration arguments.
 *
 * @param args  Arguments to be duplicated
 *
 * @return Duplicate of arguments
 */
extern trc_test_iter_args *trc_update_args_wild_dup(
                                            trc_test_iter_args *args);

/**
 * Determine whether to save a given element of TRC DB (test or
 * iteration).
 *
 * @param data      User data attached to element
 * @param is_iter   Is element iteration or not
 *
 * @return TRUE if element should be saved,
 *         FALSE otherwise
 */
extern te_bool trc_update_is_to_save(void *data, te_bool is_iter);

/**
 * Function returning value of "user_attr" attribute to be set
 * on iteration or test of required.
 *
 * @param data      TRC Update data attached to TRC DB entry.
 * @param is_iter   Is it iteration or not?
 *
 * @return String representing value or NULL
 */
extern char *trc_update_set_user_attr(void *data, te_bool is_iter);

/**
 * Process test iteration.
 *
 * @param ctx         TRC Update context
 * @param db_walker   TRC DB walker
 * @param iter        Test iteration
 * @param entry       Iteration result
 *
 * @return Status code
 */
extern te_errno trc_update_process_iter(trc_update_ctx *ctx,
                                        te_trc_db_walker *db_walker,
                                        trc_test_iter *iter,
                                        trc_report_test_iter_entry *entry);

/**
 * Process TE log file with obtained results of fake tester run.
 *
 * @param ctx           TRC update context
 *
 * @return Status code.
 */
extern te_errno trc_update_process_logs(trc_update_ctx *gctx);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_UPDATE_H__ */
