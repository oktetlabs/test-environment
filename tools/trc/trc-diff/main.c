/** @file
 * @brief Testing Results Comparator
 *
 * Show differencies between two set of tags.
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
#include "logger_api.h"
#include "te_trc.h"

#include "trc_diff.h"
#include "re_subst.h"


DEFINE_LGR_ENTITY("TRC DIFF");


/** TRC tool command line options */
enum {
    TRC_DIFF_OPT_VERSION = 1,
    TRC_DIFF_OPT_TAG0,
    TRC_DIFF_OPT_TAG1,
    TRC_DIFF_OPT_TAG2,
    TRC_DIFF_OPT_TAG3,
    TRC_DIFF_OPT_TAG4,
    TRC_DIFF_OPT_TAG5,
    TRC_DIFF_OPT_TAG6,
    TRC_DIFF_OPT_TAG7,
    TRC_DIFF_OPT_TAG8,
    TRC_DIFF_OPT_TAG9,
    TRC_DIFF_OPT_NAME0,
    TRC_DIFF_OPT_NAME1,
    TRC_DIFF_OPT_NAME2,
    TRC_DIFF_OPT_NAME3,
    TRC_DIFF_OPT_NAME4,
    TRC_DIFF_OPT_NAME5,
    TRC_DIFF_OPT_NAME6,
    TRC_DIFF_OPT_NAME7,
    TRC_DIFF_OPT_NAME8,
    TRC_DIFF_OPT_NAME9,
    TRC_DIFF_OPT_SHOW_KEYS0,
    TRC_DIFF_OPT_SHOW_KEYS1,
    TRC_DIFF_OPT_SHOW_KEYS2,
    TRC_DIFF_OPT_SHOW_KEYS3,
    TRC_DIFF_OPT_SHOW_KEYS4,
    TRC_DIFF_OPT_SHOW_KEYS5,
    TRC_DIFF_OPT_SHOW_KEYS6,
    TRC_DIFF_OPT_SHOW_KEYS7,
    TRC_DIFF_OPT_SHOW_KEYS8,
    TRC_DIFF_OPT_SHOW_KEYS9,
    TRC_DIFF_OPT_EXCLUDE0,
    TRC_DIFF_OPT_EXCLUDE1,
    TRC_DIFF_OPT_EXCLUDE2,
    TRC_DIFF_OPT_EXCLUDE3,
    TRC_DIFF_OPT_EXCLUDE4,
    TRC_DIFF_OPT_EXCLUDE5,
    TRC_DIFF_OPT_EXCLUDE6,
    TRC_DIFF_OPT_EXCLUDE7,
    TRC_DIFF_OPT_EXCLUDE8,
    TRC_DIFF_OPT_EXCLUDE9,
    TRC_DIFF_OPT_KEY2HTML,
};


/** Name of the file with expected testing result database */
static const char *trc_diff_db_fn = NULL;
/** Name of the file with report in HTML format */
static const char *trc_diff_html_fn = NULL;
/** Title of the report in HTML format */
static const char *trc_diff_title = NULL;


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
process_cmd_line_opts(int argc, char **argv, trc_diff_ctx *ctx)
{
    poptContext optCon;
    int         rc;

    /* Option Table */
    struct poptOption options_table[] = {
        { "db", 'd', POPT_ARG_STRING, &trc_diff_db_fn, 0,
          "Specify name of the file with expected testing results "
          "database.",
          "FILENAME" },

        { "html", 'h', POPT_ARG_STRING, &trc_diff_html_fn, 0,
          "Name of the file for report in HTML format.",
          "FILENAME" },

        { "title", 't', POPT_ARG_STRING, &trc_diff_title, 0,
          "Title of the HTML report to be generate.", "TITLE" },

        { "key2html", '\0', POPT_ARG_STRING, NULL, TRC_DIFF_OPT_KEY2HTML,
          "File with regular expressions to apply when output keys to "
          "HTML report.", "FILENAME" },

#define TRC_DIFF_SET_OPTS(id_) \
        { #id_ "-tag", '0' + id_, POPT_ARG_STRING, NULL,            \
          TRC_DIFF_OPT_TAG##id_,                                    \
          "Name of the tag from corresponding set.", "TAG" },       \
        { #id_ "-name", '\0', POPT_ARG_STRING, NULL,                \
          TRC_DIFF_OPT_NAME##id_,                                   \
          "Name of the corresponding set of tags.", "NAME" },       \
        { #id_ "-show-keys", '\0', POPT_ARG_NONE, NULL,             \
          TRC_DIFF_OPT_SHOW_KEYS##id_,                              \
          "Show table with keys which cause differences.",  NULL }, \
        { #id_ "-exclude", '\0', POPT_ARG_STRING, NULL,             \
          TRC_DIFF_OPT_EXCLUDE##id_,                                \
          "Exclude from report entries with key by pattern.",       \
          "PATTERN" }

        TRC_DIFF_SET_OPTS(0),
        TRC_DIFF_SET_OPTS(1),
        TRC_DIFF_SET_OPTS(2),
        TRC_DIFF_SET_OPTS(3),
        TRC_DIFF_SET_OPTS(4),
        TRC_DIFF_SET_OPTS(5),
        TRC_DIFF_SET_OPTS(6),
        TRC_DIFF_SET_OPTS(7),
        TRC_DIFF_SET_OPTS(8),
        TRC_DIFF_SET_OPTS(9),

#undef TRC_DIFF_SET_OPTS

        { "version", '\0', POPT_ARG_NONE, NULL, TRC_DIFF_OPT_VERSION, 
          "Display version information.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };
    
    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        switch (rc)
        {
            case TRC_DIFF_OPT_TAG0:
            case TRC_DIFF_OPT_TAG1:
            case TRC_DIFF_OPT_TAG2:
            case TRC_DIFF_OPT_TAG3:
            case TRC_DIFF_OPT_TAG4:
            case TRC_DIFF_OPT_TAG5:
            case TRC_DIFF_OPT_TAG6:
            case TRC_DIFF_OPT_TAG7:
            case TRC_DIFF_OPT_TAG8:
            case TRC_DIFF_OPT_TAG9:
                if (trc_diff_add_tag(&ctx->sets, rc - TRC_DIFF_OPT_TAG0,
                                     poptGetOptArg(optCon)) != 0)
                {
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

            case TRC_DIFF_OPT_NAME0:
            case TRC_DIFF_OPT_NAME1:
            case TRC_DIFF_OPT_NAME2:
            case TRC_DIFF_OPT_NAME3:
            case TRC_DIFF_OPT_NAME4:
            case TRC_DIFF_OPT_NAME5:
            case TRC_DIFF_OPT_NAME6:
            case TRC_DIFF_OPT_NAME7:
            case TRC_DIFF_OPT_NAME8:
            case TRC_DIFF_OPT_NAME9:
                if (trc_diff_set_name(&ctx->sets, rc - TRC_DIFF_OPT_NAME0,
                                      poptGetOptArg(optCon)) != 0)
                {
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

            case TRC_DIFF_OPT_SHOW_KEYS0:
            case TRC_DIFF_OPT_SHOW_KEYS1:
            case TRC_DIFF_OPT_SHOW_KEYS2:
            case TRC_DIFF_OPT_SHOW_KEYS3:
            case TRC_DIFF_OPT_SHOW_KEYS4:
            case TRC_DIFF_OPT_SHOW_KEYS5:
            case TRC_DIFF_OPT_SHOW_KEYS6:
            case TRC_DIFF_OPT_SHOW_KEYS7:
            case TRC_DIFF_OPT_SHOW_KEYS8:
            case TRC_DIFF_OPT_SHOW_KEYS9:
                if (trc_diff_show_keys(&ctx->sets,
                                       rc - TRC_DIFF_OPT_SHOW_KEYS0) != 0)
                {
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

            case TRC_DIFF_OPT_EXCLUDE0:
            case TRC_DIFF_OPT_EXCLUDE1:
            case TRC_DIFF_OPT_EXCLUDE2:
            case TRC_DIFF_OPT_EXCLUDE3:
            case TRC_DIFF_OPT_EXCLUDE4:
            case TRC_DIFF_OPT_EXCLUDE5:
            case TRC_DIFF_OPT_EXCLUDE6:
            case TRC_DIFF_OPT_EXCLUDE7:
            case TRC_DIFF_OPT_EXCLUDE8:
            case TRC_DIFF_OPT_EXCLUDE9:
                if (trc_diff_add_ignore(&ctx->sets,
                                        rc - TRC_DIFF_OPT_EXCLUDE0,
                                        poptGetOptArg(optCon)) != 0)
                {
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

            case TRC_DIFF_OPT_KEY2HTML:
            {
                const char *key2html_fn = poptGetOptArg(optCon);

                if (trc_re_substs_read(key2html_fn, &key_substs) != 0)
                {
                    ERROR("Failed to get key substitutions from "
                          "file '%s'", key2html_fn);
                    free((void *)key2html_fn);
                    return EXIT_FAILURE;
                }
                free((void *)key2html_fn);
                break;
            }

            case TRC_DIFF_OPT_VERSION:
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

    poptFreeContext(optCon);

    return EXIT_SUCCESS;
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
    int             result = EXIT_FAILURE;
    trc_diff_ctx   *ctx;
    trc_diff_set   *diff_set;

    /* Initialize diff context */
    ctx = trc_diff_ctx_new();
    if (ctx == NULL)
        goto exit;

    /* Process and validate command-line options */
    if (process_cmd_line_opts(argc, argv, ctx) != EXIT_SUCCESS)
        goto exit;

    /* Make sure that all sets have name */
    TAILQ_FOREACH(diff_set, &ctx->sets, links)
    {
        if ((diff_set->name == NULL) &&
            (asprintf(&(diff_set->name), "Set %u", diff_set->id) == -1))
        {
            ERROR("asprintf() failed");
            goto exit;
        }
    }

    if (trc_diff_db_fn == NULL)
    {
        ERROR("Missing name of the file with expected testing results");
        goto exit;
    }

    /* Parse expected testing results database */
    if (trc_db_open(trc_diff_db_fn, &ctx->db) != 0)
    {
        ERROR("Failed to load expected testing results database");
        goto exit;
    }

    /* Generate reports in HTML format */
    if (trc_diff_do(ctx) != 0)
    {
        ERROR("Failed to generate diff");
        goto exit;
    }

    /* Generate reports in HTML format */
    if (trc_diff_report_to_html(ctx, trc_diff_html_fn,
                                trc_diff_title) != 0)
    {
        ERROR("Failed to generate report in HTML format");
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:

    trc_db_close(ctx->db);
    trc_diff_ctx_free(ctx);
    trc_re_substs_free(&key_substs);

    return result;
}
