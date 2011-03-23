/** @file
 * @brief Testing Results Comparator
 *
 * Main module
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

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include "te_trc.h"
#include "trc_report.h"
#include "trc_tools.h"
#include "re_subst.h"


DEFINE_LGR_ENTITY("TRC RG");


/** TRC tool command line options */
enum {
    TRC_OPT_VERSION = 1,
    TRC_OPT_UPDATE,
    TRC_OPT_QUIET,
    TRC_OPT_INIT,
    TRC_OPT_DB,
    TRC_OPT_TAG,
    TRC_OPT_TXT,
    TRC_OPT_HTML,
    TRC_OPT_HTML_HEADER,
    TRC_OPT_HTML_TITLE,
    TRC_OPT_HTML_LOGS,
    TRC_OPT_KEY2HTML,
    TRC_OPT_NO_TOTAL_STATS,
    TRC_OPT_NO_PACKAGES_ONLY,
    TRC_OPT_STATS_ONLY,
    TRC_OPT_NO_SCRIPTS,
    TRC_OPT_NO_UNSPEC,
    TRC_OPT_NO_SKIPPED,
    TRC_OPT_NO_EXP_PASSED,
    TRC_OPT_NO_EXPECTED,
    TRC_OPT_NO_STATS_NOT_RUN,
    TRC_OPT_NO_KEYS,
    TRC_OPT_KEYS_ONLY,
    TRC_OPT_KEYS_FAILURES,
    TRC_OPT_KEYS_SANITY,
    TRC_OPT_KEYS_EXPECTED,
    TRC_OPT_KEYS_UNEXPECTED,
    TRC_OPT_IGNORE_LOG_TAGS,
    TRC_OPT_COMPARISON,
    TRC_OPT_MERGE,
    TRC_OPT_CUT,
    TRC_OPT_SHOW_CMD_FILE,
};

/** HTML report configuration */
typedef struct trc_report_html {
    TAILQ_ENTRY(trc_report_html)    links;  /**< List links */

    char           *filename;   /**< Name of the file for report */
    char           *title;      /**< Report title */
    unsigned int    flags;      /**< Report options */
    FILE           *header;     /**< File with header */
} trc_report_html;


/** Should database be initialized from scratch */
static te_bool init_db = FALSE;
/** Be quiet, do not output grand total statistics to stdout */
static te_bool quiet = FALSE;

/** Name of the file with expected testing result database */
static char *db_fn = NULL;
/** Name of the file with XML log to be analyzed */
static const char *xml_log_fn = NULL;
/** Name of the file with report in TXT format */
static char *txt_fn = NULL;

/** List of HTML reports to generate */
static TAILQ_HEAD(trc_report_htmls, trc_report_html) reports;

/** TRC report context */
static trc_report_ctx ctx;


/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated 
 * if so me new options are going to be added.
 *
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
trc_report_process_cmd_line_opts(int argc, char **argv)
{
    int                 result = EXIT_FAILURE;
    poptContext         optCon;
    int                 opt;
    trc_report_html    *report = NULL;

    /* Option Table */
    struct poptOption options_table[] = {
        { "update", 'u', POPT_ARG_NONE, NULL, TRC_OPT_UPDATE,
          "Update expected testing results database.", NULL },

        { "init", 'i', POPT_ARG_NONE, NULL, TRC_OPT_INIT,
          "Initialize expected testing results database.", NULL },

        { "quiet", 'q', POPT_ARG_NONE, NULL, TRC_OPT_QUIET,
          "Be quiet.", NULL },

        { "db", 'd', POPT_ARG_STRING, NULL, TRC_OPT_DB,
          "Specify name of the file with expected testing results "
          "database.",
          "FILENAME" },

        { "tag", 'T', POPT_ARG_STRING, NULL, TRC_OPT_TAG,
          "Name of the tag to get specific expected result.",
          "TAG" },
        { "ignore-log-tags", 'I', POPT_ARG_NONE, NULL,
          TRC_OPT_IGNORE_LOG_TAGS, "Ignore tags from log.", NULL },

        { "txt", 't', POPT_ARG_STRING, NULL, TRC_OPT_TXT,
          "Specify name of the file to report in text format.",
          "FILENAME" },

        { "merge", 'm', POPT_ARG_STRING, NULL, TRC_OPT_MERGE,
          "Name of the XML log file for merge.",
          "FILENAME" },

        { "cut", 'c', POPT_ARG_STRING, NULL, TRC_OPT_CUT,
          "Cut off results of package/test specified by path.",
          "TESTPATH" },

        { "html", 'h', POPT_ARG_STRING, NULL, TRC_OPT_HTML,
          "Name of the file for report in HTML format.",
          "FILENAME" },

        { "html-logs", '\0', POPT_ARG_STRING, NULL, TRC_OPT_HTML_LOGS,
          "Path to test logs in HTML format.",
          "DIRNAME" },

        { "html-title", '\0', POPT_ARG_STRING, NULL, TRC_OPT_HTML_TITLE,
          "Title of the HTML report.",
          "FILENAME" },

        { "html-header", '\0', POPT_ARG_STRING, NULL, TRC_OPT_HTML_HEADER,
          "Name of the file with header for the HTML report.",
          "FILENAME" },

        { "key2html", '\0', POPT_ARG_STRING, NULL, TRC_OPT_KEY2HTML,
          "File with regular expressions to apply when output keys to "
          "HTML report.", "FILENAME" },

        { "no-total", '\0', POPT_ARG_NONE, NULL, TRC_OPT_NO_TOTAL_STATS,
          "Do not include grand total statistics.",
          NULL },

        { "no-packages-only", '\0', POPT_ARG_NONE, NULL,
          TRC_OPT_NO_PACKAGES_ONLY,
          "Do not include packages only statistics.",
          NULL },

        { "stats-only", '\0', POPT_ARG_NONE, NULL, TRC_OPT_STATS_ONLY,
          "Do not include details about iterations, statistics only.",
          NULL },

        { "no-scripts", '\0', POPT_ARG_NONE, NULL, TRC_OPT_NO_SCRIPTS,
          "Do not include information about scripts in the report.",
          NULL },

        { "no-unspec", '\0', POPT_ARG_NONE, NULL, TRC_OPT_NO_UNSPEC,
          "Do not include scripts with got unspecified result (not run).",
          NULL },

        { "no-skipped", '\0', POPT_ARG_NONE, NULL, TRC_OPT_NO_SKIPPED,
          "Do not include skipped scripts.",
          NULL },

        { "no-expected", '\0', POPT_ARG_NONE, NULL, TRC_OPT_NO_EXPECTED,
          "Do not include scripts with expected results.",
          NULL },

        { "no-exp-passed", '\0', POPT_ARG_NONE, NULL, TRC_OPT_NO_EXP_PASSED,
          "Do not include scripts with passed as expected results.",
          NULL },

        { "no-stats-not-run", '\0', POPT_ARG_NONE, NULL,
          TRC_OPT_NO_STATS_NOT_RUN,
          "Do not entries with unexpected 'not run' statistics.",
          NULL },

        { "no-keys", '\0', POPT_ARG_NONE, NULL, TRC_OPT_NO_KEYS,
          "Do not add any key tables to the report.", NULL },

        { "keys-only", '\0', POPT_ARG_NONE, NULL, TRC_OPT_KEYS_ONLY,
          "Generate keys table only.", NULL },

        { "keys", '\0', POPT_ARG_NONE, NULL, TRC_OPT_KEYS_FAILURES,
          "Add regular keys table to the report.", NULL },

        { "keys-sanity", '\0', POPT_ARG_NONE, NULL, TRC_OPT_KEYS_SANITY,
          "Perform sanity check for keys table.", NULL },

        { "keys-expected", '\0', POPT_ARG_NONE, NULL, TRC_OPT_KEYS_EXPECTED,
          "Show keys for expected test results.", NULL },

        { "keys-unexpected", '\0', POPT_ARG_NONE, NULL,
          TRC_OPT_KEYS_UNEXPECTED,
          "Show keys for unexpected test results.", NULL },

        { "comparison", '\0', POPT_ARG_STRING, NULL, TRC_OPT_COMPARISON,
          "Parameter comparison method (default is 'exact').",
          "exact|casefold|normalised|tokens" },

        { "show-cmd-file", '\0', POPT_ARG_STRING, NULL,
          TRC_OPT_SHOW_CMD_FILE,
          "Verbose command line for report generation into report.",
          "STRING" },

        { "version", '\0', POPT_ARG_NONE, NULL, TRC_OPT_VERSION, 
          "Display version information.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);

    poptSetOtherOptionHelp(optCon, "<xml-log>");

    while ((opt = poptGetNextOpt(optCon)) >= 0)
    {
        switch (opt)
        {
            case TRC_OPT_QUIET:
                quiet = TRUE;
                break;

            case TRC_OPT_INIT:
                init_db = TRUE;
                /* Fall throught */

            case TRC_OPT_UPDATE:
                ctx.flags |= TRC_REPORT_UPDATE_DB;
                break;

            case TRC_OPT_DB:
                db_fn = (char *)poptGetOptArg(optCon);
                break;

            case TRC_OPT_COMPARISON:
            {
                const char *method = poptGetOptArg(optCon);

                if (strcmp(method, "exact") == 0)
                {
                    trc_db_compare_values = strcmp;
                }
                else if (strcmp(method, "casefold") == 0)
                {
                    trc_db_compare_values = strcasecmp;
                }
                else if (strcmp(method, "normalised") == 0)
                {
                    trc_db_compare_values = trc_db_strcmp_normspace;
                }
                else if (strcmp(method, "tokens") == 0)
                {
                    trc_db_compare_values = trc_db_strcmp_tokens;
                }
                else
                {
                    ERROR("Invalid comparison method: %s", method);
                    goto exit;
                }
                break;
            }

            case TRC_OPT_MERGE:
            {
                tqe_string *p = TE_ALLOC(sizeof(*p));

                if (p == NULL)
                {
                    goto exit;
                }
                TAILQ_INSERT_TAIL(&ctx.merge_fns, p, links);
                p->v = (char *)poptGetOptArg(optCon);
                if (p->v == NULL)
                {
                    ERROR("Empty value of --merge option");
                    goto exit;
                }
                else
                    VERB("Parsed merge option: --merge=%s", p->v);

                break;
            }

            case TRC_OPT_CUT:
            {
                tqe_string *p = TE_ALLOC(sizeof(*p));

                if (p == NULL)
                {
                    goto exit;
                }
                TAILQ_INSERT_TAIL(&ctx.cut_paths, p, links);
                p->v = (char *)poptGetOptArg(optCon);
                if (p->v == NULL)
                {
                    ERROR("Empty value of --cut option");
                    goto exit;
                }
                else
                    VERB("Parsed cut option: --cut=%s", p->v);

                break;
            }

            case TRC_OPT_TAG:
            {
                tqe_string *p = TE_ALLOC(sizeof(*p));

                if (p == NULL)
                {
                    goto exit;
                }
                TAILQ_INSERT_TAIL(&ctx.tags, p, links);
                p->v = (char *)poptGetOptArg(optCon);
                if (p->v == NULL)
                {
                    ERROR("Empty option value of --tag option");
                    goto exit;
                }
                break;
            }

            case TRC_OPT_IGNORE_LOG_TAGS:
                ctx.flags |= TRC_REPORT_IGNORE_LOG_TAGS;
                break;

            case TRC_OPT_TXT:
                txt_fn = (char *)poptGetOptArg(optCon);
                break;

            case TRC_OPT_HTML:
            {
                report = TE_ALLOC(sizeof(*report));
                if (report == NULL)
                {
                    goto exit;
                }
                TAILQ_INSERT_TAIL(&reports, report, links);
                report->filename = (char *)poptGetOptArg(optCon);
                report->flags = 0;
                break;
            }

            case TRC_OPT_HTML_TITLE:
                if (report == NULL)
                {
                    ERROR("HTML report title should be specified after "
                          "the file name for report");
                    goto exit;
                }
                if (report->title != NULL)
                {
                    ERROR("Title of the HTML report '%s' has already "
                          "been specified", report->filename);
                    goto exit;
                }
                report->title = (char *)poptGetOptArg(optCon);
                break;

            case TRC_OPT_HTML_HEADER:
            {
                const char *trc_html_header_fn = poptGetOptArg(optCon);

                if (report == NULL)
                {
                    ERROR("HTML report header should be specified after "
                          "the file name for report");
                    goto exit;
                }
                if (report->header != NULL)
                {
                    ERROR("File with HTML header has already been "
                          "specified");
                    free((void *)trc_html_header_fn);
                    goto exit;
                }
                report->header = fopen(trc_html_header_fn, "r");
                if (report->header == NULL)
                {
                    ERROR("Failed to open file '%s'", trc_html_header_fn);
                    free((void *)trc_html_header_fn);
                    goto exit;
                }
                free((void *)trc_html_header_fn);
                break;
            }

            case TRC_OPT_HTML_LOGS:
            {
                const char *trc_html_logs_path = poptGetOptArg(optCon);

                if (ctx.html_logs_path != NULL)
                {
                    ERROR("Directory with File with HTML header has "
                          "already been specified");
                    free((void *)trc_html_logs_path);
                    goto exit;
                }
                ctx.html_logs_path = trc_html_logs_path;
                break;
            }

            case TRC_OPT_KEY2HTML:
            {
                const char *key2html_fn = poptGetOptArg(optCon);

                if (trc_key_substs_read(key2html_fn) != 0)
                {
                    ERROR("Failed to get key substitutions from "
                          "file '%s'", key2html_fn);
                    free((void *)key2html_fn);
                    break;
                }
                free((void *)key2html_fn);
                break;
            }

            case TRC_OPT_SHOW_CMD_FILE:
            {
                ctx.show_cmd_file = poptGetOptArg(optCon);
                break;
            }


#define TRC_OPT_FLAG(flag_) \
            case TRC_OPT_##flag_:                                       \
                if (report == NULL)                                     \
                {                                                       \
                    ERROR("HTML report modifiers should be specified "  \
                          "after the file name for report");            \
                    goto exit;                                          \
                }                                                       \
                report->flags |= TRC_REPORT_##flag_;                    \
                break;

            TRC_OPT_FLAG(NO_TOTAL_STATS);
            TRC_OPT_FLAG(NO_PACKAGES_ONLY);
            TRC_OPT_FLAG(STATS_ONLY);
            TRC_OPT_FLAG(NO_SCRIPTS);
            TRC_OPT_FLAG(NO_UNSPEC);
            TRC_OPT_FLAG(NO_SKIPPED);
            TRC_OPT_FLAG(NO_EXP_PASSED);
            TRC_OPT_FLAG(NO_EXPECTED);
            TRC_OPT_FLAG(NO_STATS_NOT_RUN);
            TRC_OPT_FLAG(NO_KEYS);
            TRC_OPT_FLAG(KEYS_ONLY);
            TRC_OPT_FLAG(KEYS_FAILURES);
            TRC_OPT_FLAG(KEYS_SANITY);
            TRC_OPT_FLAG(KEYS_EXPECTED);
            TRC_OPT_FLAG(KEYS_UNEXPECTED);

#undef TRC_OPT_FLAG

            case TRC_OPT_VERSION:
                printf("Test Environment: %s\n\n%s\n", PACKAGE_STRING,
                       TE_COPYRIGHT);
                goto exit;

            default:
                ERROR("Unexpected option number %d", opt);
                goto exit;
        }
    }

    if (opt < -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s", poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(opt));
        goto exit;
    }

    /* Get name of the file with log */
    xml_log_fn = poptGetArg(optCon);
    if ((xml_log_fn != NULL) && (strcmp(xml_log_fn, "-") == 0))
    {
        xml_log_fn = NULL;
    }

    if (poptGetArg(optCon) != NULL)
    {
        ERROR("Unexpected arguments in command line");
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:
    poptFreeContext(optCon);
    return result;
}

/**
 * Output statistics in plain format to a file.
 *
 * @param f         File opened for writing
 * @param stats     Statistics to output
 *
 * @retval Status code (always 0).
 */
static int
trc_report_stats_to_txt(FILE *f, const trc_report_stats *stats)
{
    static const char * const fmt =
"\n"
"Run (total)                          %6u\n"
"  Passed, as expected                %6u\n"
"  Failed, as expected                %6u\n"
"  Passed unexpectedly                %6u\n"
"  Failed unexpectedly                %6u\n"
"  Aborted (no useful result)         %6u\n"
"  New (expected result is not known) %6u\n"
"Not Run (total)                      %6u\n"
"  Skipped, as expected               %6u\n"
"  Skipped unexpectedly               %6u\n"
"\n";

    fprintf(f, fmt,
            TRC_STATS_RUN(stats),
            stats->pass_exp, stats->fail_exp,
            stats->pass_une, stats->fail_une,
            stats->aborted, stats->new_run,
            TRC_STATS_NOT_RUN(stats), stats->skip_exp, stats->skip_une);

    return 0;
}


/**
 * Application entry point.
 *
 * @param argc      Number of command-line parameters
 * @param argv      Array with command-line parameters as strings
 *
 * @retval EXIT_SUCCESS     Success
 * @retval EXIT_FAILURE     Failure
 */
int
main(int argc, char *argv[])
{
    int                 result = EXIT_FAILURE;
    trc_report_html    *report;
    tqe_string         *merge_fn;
    tqe_string         *cut_path;

    TAILQ_INIT(&reports);

    trc_report_init_ctx(&ctx);

    /* Process and validate command-line options */
    if (trc_report_process_cmd_line_opts(argc, argv) != EXIT_SUCCESS)
        goto exit;
    if (db_fn == NULL)
    {
        ERROR("Missing name of the file with TRC database");
        goto exit;
    }

    /* Initialize a new or open existing TRC database */
    if (init_db)
    {
        if (trc_db_init(&ctx.db) != 0)
        {
            ERROR("Failed to initialize a new TRC database");
            goto exit;
        }
    }
    else if (trc_db_open(db_fn, &ctx.db) != 0)
    {
        ERROR("Failed to open TRC database '%s'", db_fn);
        goto exit;
    }

    /* Allocate TRC database user ID */
    ctx.db_uid = trc_db_new_user(ctx.db);

    /* Process log */
    if (trc_report_process_log(&ctx, xml_log_fn) != 0)
    {
        ERROR("Failed to process XML log");
        goto exit;
    }

    /* Process cut operations */
    TAILQ_FOREACH(cut_path, &ctx.cut_paths, links)
    {
        if (trc_tools_cut_db(ctx.db, ctx.db_uid, cut_path->v, FALSE) != 0)
        {
            ERROR("Failed to remove tests by path %s", cut_path->v);
            goto exit;
        }
    }

    /* Process merge operations */
    TAILQ_FOREACH(merge_fn, &ctx.merge_fns, links)
    {
        VERB("Merging with %s", merge_fn->v);
        if (trc_report_merge(&ctx, merge_fn->v) != 0)
        {
            ERROR("Failed to merge with %s", merge_fn->v);
            goto exit;
        }
    }


    if (trc_report_collect_stats(&ctx) != 0)
    {
        ERROR("Collect of TRC report statistics failed");
        goto exit;
    }

    /* Output grand total statistics to stdout */
    if (!quiet && trc_report_stats_to_txt(stdout, &ctx.stats) != 0)
    {
        ERROR("Failed to output grand total statistics to stdout");
        /* Try to continue */
    }
    if (txt_fn != NULL)
    {
        FILE *f = fopen(txt_fn, "w");

        if (f != NULL)
        {
            if (trc_report_stats_to_txt(f, &ctx.stats) != 0)
            {
                ERROR("Failed to output grand total statistics to %s",
                      txt_fn);
                /* Continue */
            }
        }
        else
        {
            ERROR("Failed to open file '%s' for writing", txt_fn);
            /* Continue */
        }
        (void)fclose(f);
    }

    /* Generate reports in HTML format */
    TAILQ_FOREACH(report, &reports, links)
    {
        if (trc_report_to_html(&ctx, report->filename, report->title,
                               report->header, report->flags) != 0)
        {
            ERROR("Failed to generate report in HTML format");
            goto exit;
        }
    }

    /* Update expected testing results database, if requested */
    if ((ctx.flags & TRC_REPORT_UPDATE_DB) &&
        (trc_db_save(ctx.db, db_fn) != 0))
    {
        ERROR("Failed to save TRC database to '%s'", db_fn);
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:

    if (ctx.db != NULL)
    {
        trc_db_free_user_data(ctx.db, ctx.db_uid, free,
             (void (*)(void *))trc_report_free_test_iter_data);
        trc_db_free_user(ctx.db, ctx.db_uid);
        trc_db_close(ctx.db);
    }
    tq_strings_free(&ctx.tags, free);

    while ((report = TAILQ_FIRST(&reports)) != NULL)
    {
        TAILQ_REMOVE(&reports, report, links);
        if (report->header != NULL)
            (void)fclose(report->header);
        free(report->filename);
        free(report->title);
        free(report);
    }

    free(db_fn);
    free(txt_fn);

    xmlCleanupParser();
    logic_expr_int_lex_destroy();

    trc_key_substs_free();

    return result;
}
