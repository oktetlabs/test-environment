/** @file
 * @brief Testing Results Comparator
 *
 * Main module
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
#include "trc_log.h"
#include "trc_db.h"


/** Should database be update */
te_bool trc_update_db = FALSE;
/** Should database be initialized from scratch */
te_bool trc_init_db = FALSE;


/** Name of the file with XML log to be analyzed */
static char *trc_xml_log_fn = NULL;
/** Name of the file with expected testing result database */
static char *trc_db_fn = NULL;


/** TRC tool command line options */
enum {
    TRC_OPT_VERSION,
    TRC_OPT_UPDATE,
    TRC_OPT_INIT,
    TRC_OPT_DB,
};

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated 
 * if some new options are going to be added.
 *
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon;
    int          rc;
    const char  *cfg_file;

    /* Option Table */
    struct poptOption options_table[] = {
        { "update", 'u', POPT_ARG_NONE, NULL, TRC_OPT_UPDATE,
          "Update expected testing results database.", NULL },

        { "init", 'i', POPT_ARG_NONE, NULL, TRC_OPT_INIT,
          "Initialize expected testing results database.", NULL },

        { "db", 'd', POPT_ARG_STRING, NULL, TRC_OPT_DB,
          "Specify name of the file with expected testing results database.",
          "FILENAME" },

        { "version", '\0', POPT_ARG_NONE, NULL, TRC_OPT_VERSION, 
          "Display version information.", NULL },

        POPT_AUTOHELP
        
        { NULL, 0, 0, NULL, 0, NULL, 0 },
    };
    
    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);
  
    poptSetOtherOptionHelp(optCon, "<xml-log>");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        switch (rc)
        {
            case TRC_OPT_INIT:
                trc_init_db = TRUE;
                /* Fall throught */

            case TRC_OPT_UPDATE:
                trc_update_db = TRUE;
                break;

            case TRC_OPT_DB:
                trc_db_fn = strdup(poptGetOptArg(optCon));
                break;
            
            case TRC_OPT_VERSION:
                printf("Test Environment: %s\n\n%s\n", PACKAGE_STRING,
                       TE_COPYRIGHT);
                poptFreeContext(optCon);
                return EXIT_FAILURE;

            default:
                ERROR("Unexpected option number %d", rc);
                poptFreeContext(optCon);
                return EXIT_FAILURE;
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s", poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    /* Get Tester configuration file names */
    trc_xml_log_fn = strdup(poptGetArg(optCon));

    if (poptGetArg(optCon) != NULL)
    {
        ERROR("Unexpected arguments in command line");
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    poptFreeContext(optCon);

    return EXIT_SUCCESS;
}


static void trc_collect_iters_stats(test_iters *iters, trc_stats *stats);
static void trc_collect_tests_stats(test_runs *tests, trc_stats *stats);

/**
 * Add one statistics to another.
 *
 * @param stats     Total statistics
 * @param add       Statistics to add
 */
static void
trc_stats_add(trc_stats *stats, const trc_stats *add)
{
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

/**
 * Collect statistics for list of iterations.
 *
 * @param iters     List of iterations
 * @param stats     Statistics to be updated
 */
static void
trc_collect_iters_stats(test_iters *iters, trc_stats *stats)
{
    test_iter *p;

    for (p = iters->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        trc_collect_tests_stats(&p->tests, &p->stats);
        trc_stats_add(stats, &p->stats);
    }
}

/**
 * Collect statistics for list of tests.
 *
 * @param tests     List of tests
 * @param stats     Statistics to be updated
 */
static void
trc_collect_tests_stats(test_runs *tests, trc_stats *stats)
{
    test_run *p;

    for (p = tests->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        trc_collect_iters_stats(&p->iters, &p->stats);
        trc_stats_add(stats, &p->stats);
    }
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
    int         result = EXIT_FAILURE;

    memset(&trc_db, 0, sizeof(trc_db));
    TAILQ_INIT(&trc_db.tests.head);

    /* Process and validate command-line options */
    if (process_cmd_line_opts(argc, argv) != EXIT_SUCCESS)
        goto exit;
    if (trc_db_fn == NULL)
    {
        ERROR("Missing name of the file with expected testing results");
        goto exit;
    }
    if (trc_xml_log_fn == NULL)
    {
        ERROR("Missing name of the file with XML log to analyze");
        goto exit;
    }

    /* Parse expected testing results database */
    if (!trc_init_db && (trc_parse_db(trc_db_fn) != 0))
    {
        ERROR("Failed to parse expected testing results database");
        goto exit;
    }

    if (trc_parse_log(trc_xml_log_fn) != 0)
    {
        ERROR("Failed to parse XML log");
        goto exit;
    }

    /* Collect total statistics */
    trc_collect_tests_stats(&trc_db.tests, &trc_db.stats);

    /* Generate report in HTML format */
    if (report_to_html("test.html", &trc_db) != 0)
    {
        ERROR("Failed to generate report in HTML format");
        goto exit;
    }

    /* Update expected testing results database, if requested */
    if (trc_update_db && (trc_dump_db(trc_db_fn) != 0))
    {
        ERROR("Failed to update expected testing results database");
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:

    free(trc_db_fn);
    free(trc_xml_log_fn);

    return result;
}
