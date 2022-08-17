/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * TRC update tool
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include "trc_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for TRC tool
#endif

#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_file.h"
#include "te_trc.h"
#include "trc_tools.h"
#include "log_parse.h"
#include "re_subst.h"
#include "logic_expr.h"
#include "te_string.h"
#include "tq_string.h"

#include "trc_report.h"
#include "trc_update.h"

#ifdef HAVE_LIBPERL
#define PERL_NO_GET_CONTEXT
#include <EXTERN.h>
#include <perl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

/** Where to print detailed logs. */
#define TRC_UPDATE_LOG_FILE "trc_update_log.txt"

/** TRC update tool command line options */
enum {
    TRC_UPDATE_OPT_VERSION = 1,     /**< Display version information */
    TRC_UPDATE_OPT_CMD,             /**< Used to pass command with which
                                         tool was actually runned */
    TRC_UPDATE_OPT_DB,              /**< Specify TRC DB path */
    TRC_UPDATE_OPT_TEST_NAME,       /**< Specify full name of the test */
    TRC_UPDATE_OPT_TAGS,            /**< Specify tags expression */
    TRC_UPDATE_OPT_LOG,             /**< Tester run log in XML format */
    TRC_UPDATE_OPT_RULES,           /**< Path to file with updating
                                         rules to be applied */
    TRC_UPDATE_OPT_RULES_SAVE,      /**< Where to save generated updating
                                         rules */
    TRC_UPDATE_OPT_RULES_ALL,       /**< Generate updating rules for all
                                         iterations in TRC (including
                                         those for which there is no
                                         conflicting results in logs) */
    TRC_UPDATE_OPT_CONFLS_ALL,      /**< Treat all results from logs as
                                         unexpected ones */
    TRC_UPDATE_OPT_NO_USE_IDS,      /**< Do not set user_attr attribute
                                         to updating rule ID in
                                         generated TRC XML */
    TRC_UPDATE_OPT_FILL_NEW,        /**< Specify format for autofilling
                                         <new> section of updating rules */
    TRC_UPDATE_OPT_NO_WILDS,        /**< Do not generate wildcards */
    TRC_UPDATE_OPT_LOG_WILDS,       /**< Generate wildcards for results
                                         from logs, not from TRC DB */
    TRC_UPDATE_OPT_LOG_WILDS_UNEXP, /**< Generate wildcards for unexpected
                                         results from logs only */
    TRC_UPDATE_OPT_FAKE_LOG,        /**< Fake Tester run log */
    TRC_UPDATE_OPT_FAKE_FILT_LOG,   /**< Fake Tester run log used for
                                         filtering out iterations
                                         not matching some reqs */
    TRC_UPDATE_OPT_MATCHING_EXPR,   /**< Expression to match iterations
                                         in TRC with iterations in logs */
    TRC_UPDATE_OPT_MATCHING_PERL,   /**< Path to iterations matching perl
                                         script */
    TRC_UPDATE_OPT_MATCHING_OTH,    /**< Path to iteration matching
                                         program */
    TRC_UPDATE_OPT_TRC_SAVE,        /**< Path to file where resulting TRC
                                         should be saved */
    TRC_UPDATE_OPT_TAGS_STR,        /**< Do not change string
                                         representation of tags */
    TRC_UPDATE_OPT_NO_POS_ATTR,     /**< Do not set "pos"
                                         attribute */
    TRC_UPDATE_OPT_GEN_APPLY,       /**< Apply updating rules after
                                         generating them */
    TRC_UPDATE_OPT_RULES_CONFL,     /**< If applying of updating rule
                                         leads to replacing some already
                                         existing results, do not replace
                                         them but instead treat results
                                         from '<new>' section of the rule
                                         as if they were conflicting
                                         results from logs */
    TRC_UPDATE_OPT_RRESULTS,        /**< Generate updating rules of
                                         type @c TRC_UPDATE_RRESULTS */
    TRC_UPDATE_OPT_RRESULT,         /**< Generate updating rules of
                                         type @c TRC_UPDATE_RRESULT */
    TRC_UPDATE_OPT_RRENTRY,         /**< Generate updating rules of
                                         type @c TRC_UPDATE_RRENTRY */
    TRC_UPDATE_OPT_RVERDICT,        /**< Generate updating rules of
                                         type @c TRC_UPDATE_RVERDICT */
    TRC_UPDATE_OPT_RULE_ARGS,       /**< Generate <args> tags for generated
                                         rules */
    TRC_UPDATE_OPT_PRINT_PATHS,     /**< Print paths of tests encountered
                                         in logs and exit */
    TRC_UPDATE_OPT_PE,              /**< Take into consideration prologues
                                         and epilogues */
    TRC_UPDATE_OPT_RULE_UPD_ONLY,   /**< Save only tests for which
                                         iterations at least one rule was
                                         applied */
    TRC_UPDATE_OPT_SKIPPED,         /**< Show skipped unexpected results */
    TRC_UPDATE_OPT_NO_SKIP_ONLY,    /**< Do not create rules with
                                         <conflicts/> containing
                                         skipped only results */
    TRC_UPDATE_OPT_NO_EXP_ONLY,     /**< Do not create rules with
                                         <conflicts/> containing
                                         expected only results
                                         if CONFLS_ALL is turned on */
    TRC_UPDATE_OPT_NO_R_FAIL,       /**< Do not consider results of kind
                                         "FAILED without verdicts */
    TRC_UPDATE_OPT_NO_INCOMPL,      /**< Do not consider INCOMPLETE results */
    TRC_UPDATE_OPT_NO_INT_ERR,      /**< Do not consider results with internal
                                         error */
    TRC_UPDATE_OPT_TAGS_LIST,       /**< Specify list of tags to be
                                         used for automatical generation
                                         of tag expression for a log */
    TRC_UPDATE_OPT_TAGS_LIST_FILE,  /**< Specify file with list of tags
                                         to be used for automatical
                                         generation of tag expression
                                         for a log */
    TRC_UPDATE_OPT_TAGS_GATHER,     /**< Gather all the different tags
                                         from the specified logs and
                                         printf them */
    TRC_UPDATE_OPT_TAGS_BY_LOGS,    /**< If --tag is not specified,
                                         derive it from log file name */
    TRC_UPDATE_OPT_EXT_WILDS,       /**< Extend generated wildcards:
                                         specify values for all the
                                         arguments having only
                                         single possible value in
                                         all the iterations described by
                                         this wildcard */
    TRC_UPDATE_OPT_SIMPL_TAGS,      /**< Simplify tag expressions in
                                         lists of unexpected results from
                                         logs */
    TRC_UPDATE_OPT_FROM_FILE,       /**< Load additional options from a
                                         file */
    TRC_UPDATE_OPT_LOGS_DUMP,       /**< Path to file with logs dump
                                         (file in binary format containing
                                         information about tests results
                                         gathered from several logs)*/
    TRC_UPDATE_OPT_NO_GEN_FSS,      /**< Do not try to find out all subsets
                                         of iterations corresponding to
                                         every possible iteration record,
                                         do not use algorithms based on it
                                         (obsolete; turned on by default) */
    TRC_UPDATE_OPT_GEN_FSS,         /**< Try to find out all subsets of
                                         iterations corresponding to
                                         every possible iteration record
                                         and use algorithms based on it */
    TRC_UPDATE_OPT_FSS_UNLIM,       /**< Do not resrict amount of time used
                                         to find out subsets for every
                                         possible iteration record */
};

#ifdef HAVE_LIBPERL
/** Test path */
static SV *test_path = NULL;
/** Arguments of an iteration stored in TRC DB */
static HV *olds = NULL;
/** Arguments of an iteration from a log */
static HV *news = NULL;
/** Arguments which should have the same values in matching iterations */
static HV *commons = NULL;
/** Arguments which can be omitted in an iteration stored in TRC DB */
static HV *notcomm_olds = NULL;
/** Arguments which can be omitted in an iteration from a log */
static HV *notcomm_news = NULL;
#endif

/** Name of the file with expected testing result database */
static char *db_fn = NULL;

/** Name of the file in which resulting TRC should be saved */
static char *trc_save_to = NULL;

/** Expression to match iterations in TRC with iterations in logs */
static char *perl_expr = NULL;
/** Perl script to match iterations in TRC with iterations in logs */
static char *perl_script = NULL;
/** Program script to match iterations in TRC with iterations in logs */
static char *oth_prog = NULL;

/** TRC update context */
static trc_update_ctx ctx;

#ifdef HAVE_LIBPERL
/** Perl interpreter */
static PerlInterpreter *perl_interp = NULL;
#endif

/*
 * Queue of all possible iteration arguments names
 */
static tqh_strings     args_registered;

/**
 * Whether to set "pos" attribute in generated XML
 * or not
 */
static te_bool         set_pos_attr = TRUE;

/**
 * Add new group of logs.
 *
 * @param tags_str  Tag expression
 *
 * @return
 *      Pointer to structure describing group of logs or NULL
 */
static trc_update_tag_logs *
add_new_tag_logs(char *tags_str)
{
    trc_update_tag_logs     *tag_logs;

    tag_logs = TE_ALLOC(sizeof(*tag_logs));

    if (tag_logs == NULL)
    {
        ERROR("Failed to allocate memory");
        return NULL;
    }

    tag_logs_init(tag_logs);

    tag_logs->tags_str = tags_str;
    if (logic_expr_parse(tag_logs->tags_str,
                         &tag_logs->tags_expr) != 0)
    {
        ERROR("Incorrect tag expression: %s",
              tag_logs->tags_str);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&ctx.tags_logs, tag_logs, links);

    return tag_logs;
}

/**
 * Get list of tags from a command line argument.
 *
 * @param tags_list     List of tags to be filled
 * @param tags          Value of command line argument
 *
 * @return Status code
 */
static te_errno
parse_tags_list(char *tags_list, tqh_strings *tags)
{
    char        *s = tags_list;
    char        *t;
    char        *p;
    tqe_string  *tqs = NULL;

    while (s != NULL && s[0] != '\0')
    {
        t = strchr(s, ',');
        if (t == NULL)
            p = s + strlen(s);
        else
            p = t;

        tqs = TE_ALLOC(sizeof(*tqs));
        tqs->v = TE_ALLOC(p - s + 1);
        memcpy(tqs->v, s, p - s);
        tqs->v[p - s] = '\0';

        TAILQ_INSERT_TAIL(tags, tqs, links);

        if (t == NULL)
            s = t;
        else
            s = t + 1;
    }

    return 0;
}

/**
 * Get list of tags from a file.
 *
 * @param fname         File name
 * @param tags          Value of command line argument
 *
 * @return Status code
 */
static te_errno
get_tags_list_from_file(char *fname, tqh_strings *tags)
{
    FILE        *f = fopen(fname, "r");
    int          max_tag_len = 100;
    char         s[max_tag_len];
    char        *end_line;
    tqe_string  *tqs = NULL;

    while (!feof(f))
    {
        if (fgets(s, max_tag_len, f) == NULL)
            break;

        end_line = strchr(s, '\n');
        if (end_line != NULL)
            *end_line = '\0';
        if (strlen(s) == 0)
            continue;

        tqs = TE_ALLOC(sizeof(*tqs));
        tqs->v = strdup(s);
        TAILQ_INSERT_TAIL(tags, tqs, links);
    }

    fclose(f);

    return 0;
}

/* Predeclaration, see description below */
static int trc_update_process_cmd_line_opts(int argc, char **argv,
                                            te_bool main_call);

/**
 * Gets additional options from a file.
 *
 * @param fname         File name
 *
 * @return Status code
 */
static te_errno
get_opts_from_file(char *fname)
{
    FILE        *f = fopen(fname, "r");
    off_t        opts_len = 0;
    char        *s;
    int          argc;
    char       **argv;
    int          rc;

    if (f == NULL)
    {
        printf("%s(): failed to open %s\n", __FUNCTION__, fname);
        return -1;
    }

    fseeko(f, 0LL, SEEK_END);
    opts_len = ftell(f) + 1 /* for '\0' */;
    fseeko(f, 0LL, SEEK_SET);

    s = TE_ALLOC(sizeof(*s) * opts_len);
    if (s == NULL)
    {
        printf("%s(): memory allocation failed\n", __FUNCTION__);
        return -1;
    }

    if (fgets(s, opts_len, f) == NULL)
    {
        printf("%s(): failed to read additional options from %s\n",
               __FUNCTION__, fname);
        return -1;
    }
    if (poptParseArgvString(s, &argc, (const char ***)&argv) != 0)
    {
        printf("%s(): failed to parse additional options from %s\n",
               __FUNCTION__, fname);
        return -1;
    }

    rc = trc_update_process_cmd_line_opts(argc, argv, FALSE);

    free(argv);
    free(s);
    fclose(f);

    return rc;
}

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated
 * if some new options are going to be added.
 *
 * @param argc          Number of elements in array "argv".
 * @param argv          Array of strings that represents all command line
 *                      arguments.
 * @param main_call     Whether this call is from main() or not.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
trc_update_process_cmd_line_opts(int argc, char **argv, te_bool main_call)
{
    int             result = EXIT_FAILURE;
    poptContext     optCon;
    int             opt;
    int             rc;

    static trc_update_tag_logs     *tag_logs;
    tqe_string                     *tqe_str;

    char           *s;
    static te_bool  no_use_ids = FALSE;
    static te_bool  log_specified = FALSE;
    static uint64_t rtype_flags = 0;

    struct poptOption options_table[] = {
        { "test-name", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_TEST_NAME,
          "Specify full name of the test", NULL },

        { "no-omit-pe", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_PE,
          "Take into consideration prologues and epilogues", NULL },

        { "matching-expr", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_MATCHING_EXPR,
          "Specify expression to match old iterations from TRC with "
          "new ones (it makes sense when set of parameters was changed)",
          NULL },

        { "matching-perl", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_MATCHING_PERL,
          "Specify path to a perl script matching old iterations from "
          "TRC with new ones", NULL },

        { "matching-prog", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_MATCHING_OTH,
          "Specify path to a program matching old iterations from TRC "
          "with new ones", NULL },

        { "rules", 'r', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_RULES,
          "Specify updating rules file in XML format", NULL },

        { "rules-save", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_RULES_SAVE,
          "Specify where to save updating rules for editing", NULL },

        { "rules-all", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RULES_ALL,
          "Create updating rules for all results (not only for those "
          "which are to be merged with new ones)", NULL },

        { "rules-iter-res", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RRESULTS,
          "Create updating rules for iteration results",
          NULL },

        { "rules-results", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RRESULT,
          "Create updating rules for <results> tags",
          NULL },

        { "rules-result", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RRENTRY,
          "Create updating rules for <result> tags",
          NULL },

        { "rules-verdict", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RVERDICT,
          "Create updating rules for <verdict> tags",
          NULL },

        { "rule-args", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RULE_ARGS,
          "Create <args> tags for updating rules",
          NULL },

        { "rules-confl", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RULES_CONFL,
          "If applying of updating rule leads to replacing some "
          "already existing results, do not replace them but "
          "instead treat results from '<new>' section of "
          "the rule as if they were conflicting results from "
          "logs", NULL },

        { "confls-all", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_CONFLS_ALL,
          "Treat all results from logs as unexpected ones", NULL },

        { "no-exp-only", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_EXP_ONLY,
          "Do not create rules with <conflicts/> containing "
          "expected only results in if --confls-all is specified",
          NULL },

        { "no-r-fail", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_R_FAIL,
          "Do not consider FAILED without verdicts results",
          NULL },

        { "no-incompl", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_INCOMPL,
          "Do not consider INCOMPLETE results",
          NULL },

       { "no-int-err", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_INT_ERR,
          "Do not consider results with internal error",
          NULL },

        { "gen-apply", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_GEN_APPLY,
          "Apply updating rules after generating them", NULL },

        { "rule-upd-only", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RULE_UPD_ONLY,
          "Save only tests to which iterations at least one rule "
          "was applied", NULL },

        { "skipped", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_SKIPPED,
          "Show skipped unexpected results", NULL },

        { "no-skip-only", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_SKIP_ONLY,
          "Do not create rules with <conflicts/> containing "
          "skipped only results",
          NULL },

        { "tags-list", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_TAGS_LIST,
          "Specify tags list for automatic generation of tag expression "
          "for logs", NULL },

        { "tags-list-file", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_TAGS_LIST_FILE,
          "Specify a file with tags list for automatic generation "
          "of tag expression for logs",
          NULL },

        { "tags-gather", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_TAGS_GATHER,
          "Gather all the different tags from the specified logs and "
          "save them in a specified file", NULL },

        { "simpl-tags", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_SIMPL_TAGS,
          "Simplify tag expressions in lists of unexpected results "
          "from logs", NULL },

        { "tags-by-logs", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_TAGS_BY_LOGS,
          "Derive tags expressions for logs from their file names", NULL },

        { "no-use-ids", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_USE_IDS,
          "Do not use \"id\" attribute of rules and \"user_attr\" "
          "attribute of test iterations to match rules and "
          "iterations in \"raw\" TRC generated by this tool", NULL },

        { "no-pos", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_POS_ATTR,
          "Do not set \"pos\" attribute in TRC XML "
          "generated by this tool", NULL },

        { "fill-new", '\0', POPT_ARG_STRING, NULL,
           TRC_UPDATE_OPT_FILL_NEW, "Specify how to fill <new> "
           "section of updating rules (see Wiki for more info, "
           "default value is 'old')", NULL },

        { "no-wilds", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_WILDS,
          "Do not generate wildcards in resulting TRC",
          NULL },

        { "ext-wilds", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_EXT_WILDS,
          "In every generated wildcard specify value for any "
          "argument having only the single value in all the iterations "
          "described by the wildcard", NULL },

        { "log-wilds", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_LOG_WILDS,
          "Generate wildcards for results from logs, not from TRC DB",
          NULL },

        { "log-wilds-unexp", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_LOG_WILDS_UNEXP,
          "Generate wildcards for unexpected results from logs only",
          NULL },

        { "tags-str", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_TAGS_STR,
          "Do not change string representation of tags",
          NULL },

        { "tags", 't', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_TAGS,
          "Specify tag expression", NULL },

        { "log", 'l', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_LOG,
          "Specify log file", NULL },

        { "fake-log", '\0', POPT_ARG_STRING | POPT_ARGFLAG_DOC_HIDDEN,
          NULL, TRC_UPDATE_OPT_FAKE_LOG,
          "Specify log file of fake Tester run in XML format", NULL },

        { "fake-filter-log", '\0',
          POPT_ARG_STRING | POPT_ARGFLAG_DOC_HIDDEN,
          NULL, TRC_UPDATE_OPT_FAKE_FILT_LOG,
          "Specify log file of fake Tester run in XML format "
          "(to be used for filtering out some iterations)",
          NULL },

        { "no-gen-fss", '\0', POPT_ARG_NONE | POPT_ARGFLAG_DOC_HIDDEN, NULL,
          TRC_UPDATE_OPT_NO_GEN_FSS,
          "Obsolete",
          NULL },

        { "gen-fss", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_GEN_FSS,
          "Try to find out subsets corresponding to every possible "
          "iteration record and use algorithms based on it",
          NULL },

        { "fss-unlim", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_FSS_UNLIM,
          "Do not resrict amount of time used to find out subsets for "
          "every possible iteration record",
          NULL },

        { "print-paths", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_PRINT_PATHS,
          "Print paths of all test scripts encountered in logs "
          "and terminate", NULL },

        { "trc-save", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_TRC_SAVE,
          "Specify file to save resulting TRC", NULL },

        { "cmd", '\0', POPT_ARG_STRING | POPT_ARGFLAG_DOC_HIDDEN,
          NULL, TRC_UPDATE_OPT_CMD, NULL, NULL },

        { "db", 'd', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_DB,
          "Specify name of the file with expected testing results "
          "database.",
          "FILENAME" },

        { "opts-file", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_FROM_FILE,
          "Specify a file with additional options", NULL },

        { "logs-dump", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_LOGS_DUMP,
          "Specify a file with logs dump", NULL },

        { "version", '\0', POPT_ARG_NONE, NULL, TRC_UPDATE_OPT_VERSION,
          "Display version information.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);

    while ((opt = poptGetNextOpt(optCon)) >= 0)
    {
        switch (opt)
        {
            case TRC_UPDATE_OPT_PE:
                ctx.flags &= ~TRC_UPDATE_NO_PE;
                break;

            case TRC_UPDATE_OPT_TEST_NAME:
                tqe_str = TE_ALLOC(sizeof(tqe_string));
                tqe_str->v = (char *)poptGetOptArg(optCon);
                s = strchr(tqe_str->v, ':');
                if (s != NULL)
                    *s = '\0';
                s = strchr(tqe_str->v, '%');
                if (s != NULL)
                    *s = '\0';
                s = tqe_str->v + strlen(tqe_str->v) - 1;
                if (*s == '/')
                    *s = '\0';
                TAILQ_INSERT_TAIL(&ctx.test_names, tqe_str, links);
                break;

            case TRC_UPDATE_OPT_TAGS:
                tag_logs = add_new_tag_logs(poptGetOptArg(optCon));
                if (tag_logs == NULL)
                    goto exit;
                break;

            case TRC_UPDATE_OPT_LOG:

                tqe_str = malloc(sizeof(tqe_string));
                tqe_str->v = poptGetOptArg(optCon);

                if (TAILQ_EMPTY(&ctx.tags_logs))
                {
                    const char *def_tags = "UNSPEC";

                    if (ctx.flags & TRC_UPDATE_TAGS_BY_LOGS)
                        def_tags = tqe_str->v;

                    tag_logs = add_new_tag_logs(strdup(def_tags));
                    if (tag_logs == NULL)
                        goto exit;
                }
                else
                    tag_logs = TAILQ_LAST(&ctx.tags_logs,
                                          trc_update_tags_logs);

                TAILQ_INSERT_TAIL(&tag_logs->logs, tqe_str, links);

                log_specified = TRUE;

                break;

            case TRC_UPDATE_OPT_NO_GEN_FSS:
                // Obsolete, do nothing
                break;

            case TRC_UPDATE_OPT_GEN_FSS:
                ctx.flags &= ~TRC_UPDATE_NO_GEN_FSS;
                break;

            case TRC_UPDATE_OPT_FSS_UNLIM:
                ctx.flags |= TRC_UPDATE_FSS_UNLIM;
                break;

            case TRC_UPDATE_OPT_FAKE_LOG:
                ctx.fake_log = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_FAKE_FILT_LOG:
                ctx.fake_filt_log = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_PRINT_PATHS:
                ctx.flags |= TRC_UPDATE_PRINT_PATHS;
                break;

            case TRC_UPDATE_OPT_MATCHING_EXPR:
#ifndef HAVE_LIBPERL
                fprintf(stderr, "ERROR: libperl is missed. You cannot "
                        "use --matching-expr.\n");
                goto exit;
#endif
                perl_expr = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_MATCHING_PERL:
#ifndef HAVE_LIBPERL
                fprintf(stderr, "ERROR: libperl is missed. You cannot "
                        "use --matching-perl.\n");
                goto exit;
#endif
                perl_script = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_MATCHING_OTH:
                oth_prog = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_TAGS_LIST:
                s = poptGetOptArg(optCon);
                tq_strings_free(&ctx.tags_gen_list, free);
                parse_tags_list(s, &ctx.tags_gen_list);
                free(s);
                ctx.flags |= TRC_UPDATE_GEN_TAGS;
                break;

            case TRC_UPDATE_OPT_TAGS_LIST_FILE:
                s = poptGetOptArg(optCon);
                tq_strings_free(&ctx.tags_gen_list, free);
                get_tags_list_from_file(s, &ctx.tags_gen_list);
                free(s);
                ctx.flags |= TRC_UPDATE_GEN_TAGS;
                break;

            case TRC_UPDATE_OPT_TAGS_GATHER:
                ctx.flags |= TRC_UPDATE_TAGS_GATHER;
                ctx.tags_gather_to = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_SIMPL_TAGS:
                ctx.flags |= TRC_UPDATE_SIMPL_TAGS;
                break;

            case TRC_UPDATE_OPT_TAGS_BY_LOGS:
                ctx.flags |= TRC_UPDATE_TAGS_BY_LOGS;
                break;

            case TRC_UPDATE_OPT_RULES:
                ctx.rules_load_from = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_RULES_SAVE:
                ctx.rules_save_to = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_FILL_NEW:
                s = poptGetOptArg(optCon);
                ctx.flags &= ~(TRC_UPDATE_COPY_CONFLS |
                               TRC_UPDATE_COPY_OLD |
                               TRC_UPDATE_COPY_BOTH |
                               TRC_UPDATE_COPY_OLD_FIRST);

                if (strcmp(s, "o") == 0 || strcmp(s, "old") == 0)
                    ctx.flags |= TRC_UPDATE_COPY_OLD;
                else if (strcmp(s, "c") == 0 ||
                         strcmp(s, "conflicts") == 0)
                    ctx.flags |= TRC_UPDATE_COPY_CONFLS;
                else if (strcmp(s, "o/c") == 0 || strcmp(s, "o,c") == 0 ||
                         strcmp(s, "old/conflicts") == 0 ||
                         strcmp(s, "old,conflicts") == 0)
                {
                    ctx.flags |= TRC_UPDATE_COPY_OLD |
                                 TRC_UPDATE_COPY_CONFLS |
                                 TRC_UPDATE_COPY_OLD_FIRST;
                    if (strchr(s, ',') != 0)
                        ctx.flags |= TRC_UPDATE_COPY_BOTH;
                }
                else if (strcmp(s, "c/o") == 0 || strcmp(s, "c,o") == 0 ||
                         strcmp(s, "conflicts/old") == 0 ||
                         strcmp(s, "conflicts,old") == 0)
                {
                    ctx.flags |= TRC_UPDATE_COPY_OLD |
                                 TRC_UPDATE_COPY_CONFLS;
                    if (strchr(s, ',') != 0)
                        ctx.flags |= TRC_UPDATE_COPY_BOTH;
                }
                else if (s[0] != '\0')
                {
                    ERROR("Incorrect value \"%s\" of --fill-new option",
                          s);
                    free(s);
                    goto exit;
                }

                free(s);
                break;

            case TRC_UPDATE_OPT_RULES_ALL:
                ctx.flags |= TRC_UPDATE_RULES_ALL;
                break;

            case TRC_UPDATE_OPT_RRESULTS:
                rtype_flags |= TRC_UPDATE_RRESULTS;
                break;

            case TRC_UPDATE_OPT_RRESULT:
                rtype_flags |= TRC_UPDATE_RRESULT;
                break;

            case TRC_UPDATE_OPT_RRENTRY:
                rtype_flags |= TRC_UPDATE_RRENTRY;
                break;

            case TRC_UPDATE_OPT_RVERDICT:
                rtype_flags |= TRC_UPDATE_RVERDICT;
                break;

            case TRC_UPDATE_OPT_RULE_ARGS:
                ctx.flags |= TRC_UPDATE_RULE_ARGS;
                break;

            case TRC_UPDATE_OPT_RULES_CONFL:
                ctx.flags |= TRC_UPDATE_RULES_CONFL;
                break;

            case TRC_UPDATE_OPT_CONFLS_ALL:
                ctx.flags |= TRC_UPDATE_CONFLS_ALL;
                break;

            case TRC_UPDATE_OPT_GEN_APPLY:
                ctx.flags |= TRC_UPDATE_GEN_APPLY;
                break;

            case TRC_UPDATE_OPT_RULE_UPD_ONLY:
                ctx.flags |= TRC_UPDATE_RULE_UPD_ONLY;
                break;

            case TRC_UPDATE_OPT_SKIPPED:
                ctx.flags |= TRC_UPDATE_SKIPPED;
                break;

            case TRC_UPDATE_OPT_NO_SKIP_ONLY:
                ctx.flags |= TRC_UPDATE_NO_SKIP_ONLY;
                break;

            case TRC_UPDATE_OPT_NO_EXP_ONLY:
                ctx.flags |= TRC_UPDATE_NO_EXP_ONLY;
                break;

            case TRC_UPDATE_OPT_NO_R_FAIL:
                ctx.flags |= TRC_UPDATE_NO_R_FAIL;
                break;

            case TRC_UPDATE_OPT_NO_INCOMPL:
                ctx.flags |= TRC_UPDATE_NO_INCOMPL;
                break;

            case TRC_UPDATE_OPT_NO_INT_ERR:
                ctx.flags |= TRC_UPDATE_NO_INT_ERR;
                break;

            case TRC_UPDATE_OPT_NO_USE_IDS:
                no_use_ids = TRUE;
                break;

            case TRC_UPDATE_OPT_NO_POS_ATTR:
                set_pos_attr = FALSE;
                break;

            case TRC_UPDATE_OPT_NO_WILDS:
                ctx.flags |= TRC_UPDATE_NO_GEN_WILDS;
                break;

            case TRC_UPDATE_OPT_EXT_WILDS:
                ctx.flags |= TRC_UPDATE_EXT_WILDS;
                break;

            case TRC_UPDATE_OPT_LOG_WILDS:
                ctx.flags |= TRC_UPDATE_LOG_WILDS;
                break;

            case TRC_UPDATE_OPT_LOG_WILDS_UNEXP:
                ctx.flags |= TRC_UPDATE_LOG_WILDS |
                             TRC_UPDATE_LOG_WILDS_UNEXP;
                break;

            case TRC_UPDATE_OPT_TAGS_STR:
                ctx.flags |= TRC_UPDATE_TAGS_STR;
                break;

            case TRC_UPDATE_OPT_CMD:
                ctx.cmd = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_DB:
                db_fn = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_TRC_SAVE:
                trc_save_to = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_FROM_FILE:
                s = poptGetOptArg(optCon);
                rc = get_opts_from_file(s);
                if (rc < 0)
                    goto exit;
                break;

            case TRC_UPDATE_OPT_LOGS_DUMP:
                ctx.logs_dump = poptGetOptArg(optCon);
                log_specified = TRUE;
                break;

            case TRC_UPDATE_OPT_VERSION:
                printf("Test Environment: %s\n\n%s\n", PACKAGE_STRING,
                       TE_COPYRIGHT);
                goto exit;

            default:
                ERROR("Unexpected option number %d", opt);
                goto exit;
        }
    }

    if (main_call)
    {
        if (!no_use_ids && (log_specified ||
            ((ctx.flags & TRC_UPDATE_RULES_CONFL) &&
             !(ctx.flags & TRC_UPDATE_GEN_APPLY) &&
             ctx.rules_save_to != NULL)))
            ctx.flags |= TRC_UPDATE_USE_RULE_IDS;

        if (!log_specified && ctx.rules_load_from == NULL)
            ctx.flags |= TRC_UPDATE_RULES_ALL;

        if (rtype_flags != 0)
            ctx.flags = (ctx.flags & ~TRC_UPDATE_RTYPES) | rtype_flags;
    }

    if (opt != -1)
    {
        ERROR("%s:%s", poptBadOption(optCon, 0), poptStrerror(opt));
        goto exit;
    }

    if ((s = (char *)poptGetArg(optCon)) != NULL)
    {
        ERROR("Unexpected arguments in command line: %s", s);
        goto exit;
    }

    trc_update_tags_logs_remove_empty(&ctx.tags_logs);

    result = EXIT_SUCCESS;

exit:
    poptFreeContext(optCon);
    return result;
}

#ifdef HAVE_LIBPERL
static int
perl_prepare()
{
    te_string   te_str;

    dTHX;

    if (perl_expr != NULL || perl_script != NULL)
    {
        olds = get_hv("old", GV_ADD);
        news = get_hv("new", GV_ADD);
        commons = get_hv("commons", GV_ADD);
        notcomm_news = get_hv("notcomm_new", GV_ADD);
        notcomm_olds = get_hv("notcomm_old", GV_ADD);
        test_path = get_sv("test_path", GV_ADD);

        eval_pv("sub old_wild_eq"
                "{"
                "   my @arr = @_;"
                ""
                "   return 1 if (old($arr[0]) eq $arr[1]);"
                "   return 2 if (exists($old{$arr[0]}) && "
                "                length($old{$arr[0]}) == 0);"
                "   return 0;"
                "}",
                TRUE);

        eval_pv("sub notcomm_old"
                "{"
                "   my $arg;"
                "   foreach $arg (@_)"
                "   {"
                "       $notcomm_old{$arg} = 1;"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub notcomm_new"
                "{"
                "   my $arg;"
                "   foreach $arg (@_)"
                "   {"
                "       $notcomm_new{$arg} = 1;"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub notcomm"
                "{"
                "   my $arg;"
                "   foreach $arg (@_)"
                "   {"
                "       $notcomm_old{$arg} = 1;"
                "       $notcomm_new{$arg} = 1;"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub comm_inc"
                "{"
                "   my $arg;"
                "   foreach $arg (keys %commons)"
                "   {"
                "       $commons{$arg} = 0;"
                "   }"
                "   foreach $arg (@_)"
                "   {"
                "       $commons{$arg} = 1;"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub comm_exc"
                "{"
                "   my $arg;"
                "   foreach $arg (@_)"
                "   {"
                "       if (exists($commons{$arg})) "
                "       {"
                "           $commons{$arg} = 0;"
                "       }"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub comm_eq"
                "{"
                "   my $rc = 1;"
                "   my $eq_r = 0;"
                "   my $arg;"
                "   foreach $arg (keys %commons)"
                "   {"
                "       if ($commons{$arg} == 1)"
                "       {"
                "           $eq_r = old_wild_eq($arg, $new{$arg});"
                "           return 0 if ($eq_r == 0);"
                "           if ($eq_r > $rc)"
                "           {"
                "               $rc = $eq_r;"
                "           }"
                "       }"
                "   }"
                "   return $rc;"
                "}",
                TRUE);

        eval_pv("sub notcomm_chk"
                "{"
                "   my $arg;"
                "   foreach $arg (keys %old)"
                "   {"
                "       if (!exists($commons{$arg}) &&"
                "           $notcomm_old{$arg} != 1)"
                "       {"
                "           return 0;"
                "       }"
                "   }"
                "   foreach $arg (keys %new)"
                "   {"
                "       if (!exists($commons{$arg}) &&"
                "           $notcomm_new{$arg} != 1)"
                "       {"
                "           return 0;"
                "       }"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub test_path { return $test_path; }", TRUE);
        eval_pv("sub old { return $old{$_[0]}; }", TRUE);
        eval_pv("sub new { return $new{$_[0]}; }", TRUE);
        eval_pv("sub old_e { return exists($old{$_[0]}); }", TRUE);
        eval_pv("sub new_e { return exists($new{$_[0]}); }", TRUE);

        eval_pv("sub add_val"
                "{"
                "   my @arr = @_;"
                "   my $rc = 0;"
                "   my $i = 0;"
                ""
                "   for ($i = 2; $i < scalar @arr; $i++)"
                "   {"
                "       $rc = $rc || (new($arr[0]) eq $arr[$i]);"
                "   }"
                "   if (scalar @arr <= 2)"
                "   {"
                "       $rc = 1;"
                "   }"
                "   if ($rc)"
                "   {"
                "       if (old_wild_eq($arr[0], $arr[1]))"
                "       {"
                "           comm_exc($arr[0]);"
                "           return 1;"
                "       }"
                "       else"
                "       {"
                "           return 0;"
                "       }"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub add_arg"
                "{"
                "   my @arr = @_;"
                "   my $rc = 0;"
                "   my $i = 0;"
                ""
                "   if (exists($new{$arr[0]}) && !exists($old{$arr[0]}))"
                "   {"
                "       for ($i = 1; $i < scalar @arr; $i++)"
                "       {"
                "           $rc = $rc || (new($arr[0]) eq $arr[$i]);"
                "       }"
                "       if (scalar @arr <= 1)"
                "       {"
                "           $rc = 1;"
                "       }"
                "       return notcomm($arr[0]) && $rc;"
                "   }"
                "   else"
                "   {"
                "       return 1;"
                "   }"
                "}",
                TRUE);

        eval_pv("sub del_arg"
                "{"
                "   my @arr = @_;"
                "   my $rc = 0;"
                "   my $i = 0;"
                ""
                "   if (exists($old{$arr[0]}) && !exists($new{$arr[0]}))"
                "   {"
                "       for ($i = 1; $i < scalar @arr; $i++)"
                "       {"
                "           $rc = $rc || old_wild_eq($arr[0], $arr[$i]);"
                "       }"
                "       if (scalar @arr <= 1)"
                "       {"
                "           $rc = 1;"
                "       }"
                "       return notcomm($arr[0]) && $rc;"
                "   }"
                "   else"
                "   {"
                "       return 1;"
                "   }"
                "}",
                TRUE);

        eval_pv("my $rc;\n"
                "my $filter;\n",
                TRUE);

        eval_pv("sub arg_diff\n"
                "{\n"
                "   my $arg = @_[0];\n"
                "   my $val1 = @_[1];\n"
                "   my $val2 = @_[2];\n"
                "\n"
                "   $filter = (new($arg) eq $val2);\n"
                "   return comm_exc($arg) && old_wild_eq($arg, $val1) &&\n"
                "          new($arg) eq $val2;\n"
                "}\n",
                TRUE);
        memset(&te_str, 0, sizeof(te_str));
        te_str.ptr = NULL;

        if (perl_expr != NULL)
        {
            te_string_append(&te_str,
                             "sub set_filter"
                             "{"
                             "    $filter = @_[0];\n"
                             "    return 1;\n"
                             "}\n"
                             "sub get_rc"
                             "{"
                             "    my $eq_r;"
                             "    $rc = (%s);\n"
                             "    $eq_r = comm_eq();"
                             "    return ($rc &&"
                             "            notcomm_chk()) ? $eq_r : 0;\n"
                             "}\n"
                             "sub get_filter"
                             "{"
                             "    $filter = 1;"
                             "    get_rc();"
                             "    return $filter;\n"
                             "}\n",
                             perl_expr);
            eval_pv(te_str.ptr, TRUE);
        }
        else
        {
            FILE               *f = NULL;
            unsigned long int   flen;
            char               *script_text = NULL;

            f = fopen(perl_script, "rb");
            if (f == NULL)
            {
                perror("Failed to open file with perl script");
                exit(1);
            }

            fseek(f, 0, SEEK_END);
            flen = ftell(f);
            fseek(f, 0, SEEK_SET);

            script_text = calloc(flen, sizeof(*script_text));
            if (script_text == NULL)
            {
                printf("Out of memory allocating space "
                       "for perl script\n");
                exit(1);
            }

            if (fread(script_text, sizeof(*script_text), flen, f) != flen)
            {
                printf("Failed to read a script from %s\n", perl_script);
                exit(1);
            }

            te_string_append(&te_str,
                             "sub get_vals"
                             "{"
                             "    %s\n"
                             "}"
                             "sub get_rc"
                             "{"
                             "    my $eq_r;"
                             "    $rc = 0;"
                             "    get_vals();"
                             "    $eq_r = comm_eq();"
                             "    return ($rc &&"
                             "            notcomm_chk()) ? $eq_r : 0;\n"
                             "}"
                             "sub get_filter"
                             "{"
                             "    $filter = 1;\n"
                             "    get_vals();"
                             "    return $filter;\n"
                             "}",
                             script_text);

            eval_pv(te_str.ptr, TRUE);

            free(script_text);
            fclose(f);
        }

        te_string_free(&te_str);
    }

    return 0;
}
#endif

/**
 * Function to match iteration in TRC with iteration from logs
 * using one of perl_expr, perl_script, oth_prog.
 *
 * @param iter          Iteration in TRC
 * @param n_args        Number of arguments of iteration from log
 * @param args          Arguments of iteration from log
 * @param filter_mode   Whether to use this function for filtering
 *                      out instead of matching?
 *
 * @return ITER_NO_MATCH if not matching, ITER_EXACT_MATCH or
 *         ITER_WILD_MATCH if matching.
 */
static int
func_args_match(const void *iter_ptr,
                unsigned int n_args, trc_report_argument *args,
                te_bool filter_mode)
{
    trc_test_iter             *iter = (trc_test_iter *)iter_ptr;
    trc_test_iter_arg         *arg;

    te_string   te_str;
    int         i = 0;

#ifndef HAVE_LIBPERL
    UNUSED(filter_mode);
#endif

    assert(iter != NULL);

    if (perl_expr != NULL || perl_script != NULL)
    {
#ifdef HAVE_LIBPERL
        int             rc = 0;
        SV             *val;
        tqh_strings     arg_names;
        tqh_strings     common_args;
        tqe_string     *arg_name;

        dTHX;

        TAILQ_INIT(&arg_names);
        TAILQ_INIT(&common_args);

        hv_clear(olds);
        sv_setpv(test_path, iter->parent->path);

        TAILQ_FOREACH(arg, &iter->args.head, links)
        {
            val = newSVpv(arg->value, strlen(arg->value));
            if (hv_store(olds, arg->name, strlen(arg->name),
                         val, 0) == NULL)
            {
                ERROR("%s(): hv_store() failed");
                return ITER_NO_MATCH;
            }

            tq_strings_add_uniq_dup(&arg_names, arg->name);
        }

        hv_clear(news);
        hv_clear(commons);

        hv_clear(notcomm_news);
        hv_clear(notcomm_olds);

        for (i = 0; i < (int)n_args; i++)
        {
            val = newSVpv(args[i].value, strlen(args[i].value));
            if (hv_store(news, args[i].name, strlen(args[i].name),
                         val, 0) == NULL)
            {
                ERROR("%s(): hv_store() failed");
                return ITER_NO_MATCH;
            }

            if (tq_strings_add_uniq_dup(&arg_names,
                                        args[i].name) == 1)
            {
                val = newSViv(1);
                if (hv_store(commons, args[i].name,
                             strlen(args[i].name), val, 0) == NULL)
                {
                    ERROR("%s(): hv_store() failed");
                    return ITER_NO_MATCH;
                }
            }
        }

        TAILQ_FOREACH(arg_name, &arg_names, links)
        {
            if (tq_strings_add_uniq_dup(&args_registered,
                                        arg_name->v) == 0)
            {
                memset(&te_str, 0, sizeof(te_str));
                te_str.ptr = NULL;

                te_string_append(&te_str, "sub %s { return \"%s\"; }",
                                 arg_name->v, arg_name->v);
                eval_pv(te_str.ptr, TRUE);
                te_string_free(&te_str);
            }
        }

        dSP;
        ENTER;
        SAVETMPS;
        PUSHMARK(SP);
        PUTBACK;

        if (!filter_mode)
            rc = call_pv("get_rc", G_SCALAR | G_NOARGS);
        else
            rc = call_pv("get_filter", G_SCALAR | G_NOARGS);

        SPAGAIN;
        if (rc != 1)
        {
            printf("Perl function returned unexpected "
                   "result\n");
            exit(1);
        }

        rc = POPi;
        PUTBACK;
        FREETMPS;
        LEAVE;

        tq_strings_free(&arg_names, free);
        tq_strings_free(&common_args, free);

        if (rc == 1)
            return ITER_EXACT_MATCH;
        else if (rc == 2)
            return ITER_WILD_MATCH;
        else
            return ITER_NO_MATCH;
#endif
    }
    else if (oth_prog != NULL)
    {
        int   status = 0;
        pid_t child = fork();

        if (child != 0)
        {
            waitpid(child, &status, 0);
            if (status == 0)
                return ITER_WILD_MATCH;
            else if (status == 1)
                return ITER_EXACT_MATCH;
            else
                return ITER_NO_MATCH;
        }
        else
        {
            char **argv;
            int    j = 0;

            TAILQ_FOREACH(arg, &iter->args.head, links)
                i++;

            argv = TE_ALLOC(sizeof(*args) * (i + n_args + 2));

            j = 0;
            argv[j++] = oth_prog;

            TAILQ_FOREACH(arg, &iter->args.head, links)
            {
                memset(&te_str, 0, sizeof(te_str));
                te_str.ptr = NULL;
                te_string_append(&te_str, "--old-%s=%s",
                                 arg->name, arg->value);
                argv[j++] = te_str.ptr;
            }

            for (i = 0; i < (int)n_args; i++)
            {
                memset(&te_str, 0, sizeof(te_str));
                te_str.ptr = NULL;
                te_string_append(&te_str, "--new-%s=%s",
                                 args[i].name, args[i].value);
                argv[j++] = te_str.ptr;
            }

            execv(oth_prog, argv);
        }
    }

    return ITER_NO_MATCH;
}

int
main(int argc, char **argv, char **envp)
{
    int     result = EXIT_FAILURE;

    FILE *log_f = NULL;

#ifdef HAVE_LIBPERL
    char   *perl_embed_params[] = { "", "-e", "0" };

    PERL_SYS_INIT3(&argc, &argv, &envp);
#else
    UNUSED(envp);
#endif

    te_log_init("TRC UPD", te_log_message_file);

    log_f = fopen(TRC_UPDATE_LOG_FILE, "w");
    if (log_f == NULL)
    {
        perror("failed to open " TRC_UPDATE_LOG_FILE);
        return EXIT_FAILURE;
    }
    te_log_message_file_out = log_f;

    TAILQ_INIT(&args_registered);
    trc_update_init_ctx(&ctx);

    ctx.flags |= TRC_UPDATE_COPY_OLD | TRC_UPDATE_COPY_OLD_FIRST |
                 TRC_UPDATE_RRESULTS | TRC_UPDATE_NO_PE |
                 TRC_UPDATE_NO_GEN_FSS;

    if (trc_update_process_cmd_line_opts(argc, argv, TRUE) != EXIT_SUCCESS)
        goto exit;

    if (ctx.cmd == NULL)
    {
        te_string   te_str;
        int         i;

        memset(&te_str, 0, sizeof(te_str));
        te_str.ptr = NULL;

        te_string_append(&te_str, "%s", argv[0]);

        for (i = 1; i < argc; i++)
            te_string_append(&te_str, " %s", argv[i]);

        ctx.cmd = te_str.ptr;
    }

    if (db_fn == NULL)
    {
        ERROR("Missing name of the file with TRC database");
        goto exit;
    }

    if (trc_db_open_ext(db_fn, &ctx.db, TRC_OPEN_FIX_XINCLUDE) != 0)
    {
        ERROR("Failed to open TRC database '%s'", db_fn);
        goto exit;
    }

    /* Allocate TRC database user ID */
    ctx.db_uid = trc_db_new_user(ctx.db);

    if (perl_expr != NULL || perl_script != NULL || oth_prog != NULL)
        ctx.func_args_match = func_args_match;

#ifdef HAVE_LIBPERL
    /* Allocate and initialize perl interpreter */
    perl_interp = perl_alloc();
    perl_construct(perl_interp);
    perl_parse(perl_interp, NULL, 3, perl_embed_params, NULL);
    dTHX;
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
    perl_prepare();
#endif

    if (trc_update_process_logs(&ctx) != 0)
    {
        ERROR("Failed to process XML logs");
        goto exit;
    }

    if (!(ctx.flags & TRC_UPDATE_PRINT_PATHS))
    {
        if (trc_save_to == NULL)
            trc_save_to = "tmp_trc_db.xml";

        if (trc_db_save(ctx.db, trc_save_to,
                        TRC_SAVE_UPDATE_OLD | TRC_SAVE_RESULTS |
                        TRC_SAVE_GLOBALS | TRC_SAVE_DEL_XINCL |
                        TRC_SAVE_NO_VOID_XINCL |
                        (set_pos_attr ? TRC_SAVE_POS_ATTR : 0),
                        ctx.db_uid, &trc_update_is_to_save,
                        (ctx.flags & TRC_UPDATE_USE_RULE_IDS) ?
                                &trc_update_set_user_attr : NULL,
                        ctx.cmd) != 0)
        {
            ERROR("Failed to save TRC database '%s'", db_fn);
            goto exit;
        }
    }

    result = EXIT_SUCCESS;

exit:

    if (ctx.db != NULL)
    {
        trc_db_free_user_data(ctx.db, ctx.db_uid, free,
                              (void (*)(void *))
                              &trc_update_free_test_iter_data);
        trc_db_free_user(ctx.db, ctx.db_uid);
        trc_db_close(ctx.db);
    }

#ifdef HAVE_LIBPERL
    if (perl_interp != NULL)
    {
        perl_destruct(perl_interp);
        perl_free(perl_interp);
    }
    PERL_SYS_TERM();
#endif

    trc_update_free_ctx(&ctx);

    free(db_fn);
    free(perl_expr);
    free(perl_script);
    free(oth_prog);
    tq_strings_free(&args_registered, free);

    if (log_f != NULL)
        fclose(log_f);

    if (result != EXIT_SUCCESS)
        fprintf(stderr, "TRC Update failed. See %s for details\n",
                TRC_UPDATE_LOG_FILE);

    return result;
}
