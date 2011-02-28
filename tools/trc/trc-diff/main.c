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
    TRC_DIFF_OPT_TAG10,
    TRC_DIFF_OPT_TAG11,
    TRC_DIFF_OPT_TAG12,
    TRC_DIFF_OPT_TAG13,
    TRC_DIFF_OPT_TAG14,
    TRC_DIFF_OPT_TAG15,
    TRC_DIFF_OPT_TAG16,
    TRC_DIFF_OPT_TAG17,
    TRC_DIFF_OPT_TAG18,
    TRC_DIFF_OPT_TAG19,
    TRC_DIFF_OPT_TAG20,
    TRC_DIFF_OPT_TAG21,
    TRC_DIFF_OPT_TAG22,
    TRC_DIFF_OPT_TAG23,
    TRC_DIFF_OPT_TAG24,
    TRC_DIFF_OPT_TAG25,
    TRC_DIFF_OPT_TAG26,
    TRC_DIFF_OPT_TAG27,
    TRC_DIFF_OPT_TAG28,
    TRC_DIFF_OPT_TAG29,
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
    TRC_DIFF_OPT_NAME10,
    TRC_DIFF_OPT_NAME11,
    TRC_DIFF_OPT_NAME12,
    TRC_DIFF_OPT_NAME13,
    TRC_DIFF_OPT_NAME14,
    TRC_DIFF_OPT_NAME15,
    TRC_DIFF_OPT_NAME16,
    TRC_DIFF_OPT_NAME17,
    TRC_DIFF_OPT_NAME18,
    TRC_DIFF_OPT_NAME19,
    TRC_DIFF_OPT_NAME20,
    TRC_DIFF_OPT_NAME21,
    TRC_DIFF_OPT_NAME22,
    TRC_DIFF_OPT_NAME23,
    TRC_DIFF_OPT_NAME24,
    TRC_DIFF_OPT_NAME25,
    TRC_DIFF_OPT_NAME26,
    TRC_DIFF_OPT_NAME27,
    TRC_DIFF_OPT_NAME28,
    TRC_DIFF_OPT_NAME29,
    TRC_DIFF_OPT_LOG0,
    TRC_DIFF_OPT_LOG1,
    TRC_DIFF_OPT_LOG2,
    TRC_DIFF_OPT_LOG3,
    TRC_DIFF_OPT_LOG4,
    TRC_DIFF_OPT_LOG5,
    TRC_DIFF_OPT_LOG6,
    TRC_DIFF_OPT_LOG7,
    TRC_DIFF_OPT_LOG8,
    TRC_DIFF_OPT_LOG9,
    TRC_DIFF_OPT_LOG10,
    TRC_DIFF_OPT_LOG11,
    TRC_DIFF_OPT_LOG12,
    TRC_DIFF_OPT_LOG13,
    TRC_DIFF_OPT_LOG14,
    TRC_DIFF_OPT_LOG15,
    TRC_DIFF_OPT_LOG16,
    TRC_DIFF_OPT_LOG17,
    TRC_DIFF_OPT_LOG18,
    TRC_DIFF_OPT_LOG19,
    TRC_DIFF_OPT_LOG20,
    TRC_DIFF_OPT_LOG21,
    TRC_DIFF_OPT_LOG22,
    TRC_DIFF_OPT_LOG23,
    TRC_DIFF_OPT_LOG24,
    TRC_DIFF_OPT_LOG25,
    TRC_DIFF_OPT_LOG26,
    TRC_DIFF_OPT_LOG27,
    TRC_DIFF_OPT_LOG28,
    TRC_DIFF_OPT_LOG29,
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
    TRC_DIFF_OPT_SHOW_KEYS10,
    TRC_DIFF_OPT_SHOW_KEYS11,
    TRC_DIFF_OPT_SHOW_KEYS12,
    TRC_DIFF_OPT_SHOW_KEYS13,
    TRC_DIFF_OPT_SHOW_KEYS14,
    TRC_DIFF_OPT_SHOW_KEYS15,
    TRC_DIFF_OPT_SHOW_KEYS16,
    TRC_DIFF_OPT_SHOW_KEYS17,
    TRC_DIFF_OPT_SHOW_KEYS18,
    TRC_DIFF_OPT_SHOW_KEYS19,
    TRC_DIFF_OPT_SHOW_KEYS20,
    TRC_DIFF_OPT_SHOW_KEYS21,
    TRC_DIFF_OPT_SHOW_KEYS22,
    TRC_DIFF_OPT_SHOW_KEYS23,
    TRC_DIFF_OPT_SHOW_KEYS24,
    TRC_DIFF_OPT_SHOW_KEYS25,
    TRC_DIFF_OPT_SHOW_KEYS26,
    TRC_DIFF_OPT_SHOW_KEYS27,
    TRC_DIFF_OPT_SHOW_KEYS28,
    TRC_DIFF_OPT_SHOW_KEYS29,
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
    TRC_DIFF_OPT_EXCLUDE10,
    TRC_DIFF_OPT_EXCLUDE11,
    TRC_DIFF_OPT_EXCLUDE12,
    TRC_DIFF_OPT_EXCLUDE13,
    TRC_DIFF_OPT_EXCLUDE14,
    TRC_DIFF_OPT_EXCLUDE15,
    TRC_DIFF_OPT_EXCLUDE16,
    TRC_DIFF_OPT_EXCLUDE17,
    TRC_DIFF_OPT_EXCLUDE18,
    TRC_DIFF_OPT_EXCLUDE19,
    TRC_DIFF_OPT_EXCLUDE20,
    TRC_DIFF_OPT_EXCLUDE21,
    TRC_DIFF_OPT_EXCLUDE22,
    TRC_DIFF_OPT_EXCLUDE23,
    TRC_DIFF_OPT_EXCLUDE24,
    TRC_DIFF_OPT_EXCLUDE25,
    TRC_DIFF_OPT_EXCLUDE26,
    TRC_DIFF_OPT_EXCLUDE27,
    TRC_DIFF_OPT_EXCLUDE28,
    TRC_DIFF_OPT_EXCLUDE29,
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
        { #id_ "-log", '\0', POPT_ARG_STRING, NULL,                 \
          TRC_DIFF_OPT_LOG##id_,                                    \
          "Name of the corresponding set of tags.", "LOG" },        \
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
        TRC_DIFF_SET_OPTS(10),
        TRC_DIFF_SET_OPTS(11),
        TRC_DIFF_SET_OPTS(12),
        TRC_DIFF_SET_OPTS(13),
        TRC_DIFF_SET_OPTS(14),
        TRC_DIFF_SET_OPTS(15),
        TRC_DIFF_SET_OPTS(16),
        TRC_DIFF_SET_OPTS(17),
        TRC_DIFF_SET_OPTS(18),
        TRC_DIFF_SET_OPTS(19),
        TRC_DIFF_SET_OPTS(20),
        TRC_DIFF_SET_OPTS(21),
        TRC_DIFF_SET_OPTS(22),
        TRC_DIFF_SET_OPTS(23),
        TRC_DIFF_SET_OPTS(24),
        TRC_DIFF_SET_OPTS(25),
        TRC_DIFF_SET_OPTS(26),
        TRC_DIFF_SET_OPTS(27),
        TRC_DIFF_SET_OPTS(28),
        TRC_DIFF_SET_OPTS(29),

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
            case TRC_DIFF_OPT_TAG10:
            case TRC_DIFF_OPT_TAG11:
            case TRC_DIFF_OPT_TAG12:
            case TRC_DIFF_OPT_TAG13:
            case TRC_DIFF_OPT_TAG14:
            case TRC_DIFF_OPT_TAG15:
            case TRC_DIFF_OPT_TAG16:
            case TRC_DIFF_OPT_TAG17:
            case TRC_DIFF_OPT_TAG18:
            case TRC_DIFF_OPT_TAG19:
            case TRC_DIFF_OPT_TAG20:
            case TRC_DIFF_OPT_TAG21:
            case TRC_DIFF_OPT_TAG22:
            case TRC_DIFF_OPT_TAG23:
            case TRC_DIFF_OPT_TAG24:
            case TRC_DIFF_OPT_TAG25:
            case TRC_DIFF_OPT_TAG26:
            case TRC_DIFF_OPT_TAG27:
            case TRC_DIFF_OPT_TAG28:
            case TRC_DIFF_OPT_TAG29:
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
            case TRC_DIFF_OPT_NAME10:
            case TRC_DIFF_OPT_NAME11:
            case TRC_DIFF_OPT_NAME12:
            case TRC_DIFF_OPT_NAME13:
            case TRC_DIFF_OPT_NAME14:
            case TRC_DIFF_OPT_NAME15:
            case TRC_DIFF_OPT_NAME16:
            case TRC_DIFF_OPT_NAME17:
            case TRC_DIFF_OPT_NAME18:
            case TRC_DIFF_OPT_NAME19:
            case TRC_DIFF_OPT_NAME20:
            case TRC_DIFF_OPT_NAME21:
            case TRC_DIFF_OPT_NAME22:
            case TRC_DIFF_OPT_NAME23:
            case TRC_DIFF_OPT_NAME24:
            case TRC_DIFF_OPT_NAME25:
            case TRC_DIFF_OPT_NAME26:
            case TRC_DIFF_OPT_NAME27:
            case TRC_DIFF_OPT_NAME28:
            case TRC_DIFF_OPT_NAME29:
                if (trc_diff_set_name(&ctx->sets, rc - TRC_DIFF_OPT_NAME0,
                                      poptGetOptArg(optCon)) != 0)
                {
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

            case TRC_DIFF_OPT_LOG0:
            case TRC_DIFF_OPT_LOG1:
            case TRC_DIFF_OPT_LOG2:
            case TRC_DIFF_OPT_LOG3:
            case TRC_DIFF_OPT_LOG4:
            case TRC_DIFF_OPT_LOG5:
            case TRC_DIFF_OPT_LOG6:
            case TRC_DIFF_OPT_LOG7:
            case TRC_DIFF_OPT_LOG8:
            case TRC_DIFF_OPT_LOG9:
            case TRC_DIFF_OPT_LOG10:
            case TRC_DIFF_OPT_LOG11:
            case TRC_DIFF_OPT_LOG12:
            case TRC_DIFF_OPT_LOG13:
            case TRC_DIFF_OPT_LOG14:
            case TRC_DIFF_OPT_LOG15:
            case TRC_DIFF_OPT_LOG16:
            case TRC_DIFF_OPT_LOG17:
            case TRC_DIFF_OPT_LOG18:
            case TRC_DIFF_OPT_LOG19:
            case TRC_DIFF_OPT_LOG20:
            case TRC_DIFF_OPT_LOG21:
            case TRC_DIFF_OPT_LOG22:
            case TRC_DIFF_OPT_LOG23:
            case TRC_DIFF_OPT_LOG24:
            case TRC_DIFF_OPT_LOG25:
            case TRC_DIFF_OPT_LOG26:
            case TRC_DIFF_OPT_LOG27:
            case TRC_DIFF_OPT_LOG28:
            case TRC_DIFF_OPT_LOG29:
                if (trc_diff_set_log(&ctx->sets, rc - TRC_DIFF_OPT_LOG0,
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
            case TRC_DIFF_OPT_SHOW_KEYS10:
            case TRC_DIFF_OPT_SHOW_KEYS11:
            case TRC_DIFF_OPT_SHOW_KEYS12:
            case TRC_DIFF_OPT_SHOW_KEYS13:
            case TRC_DIFF_OPT_SHOW_KEYS14:
            case TRC_DIFF_OPT_SHOW_KEYS15:
            case TRC_DIFF_OPT_SHOW_KEYS16:
            case TRC_DIFF_OPT_SHOW_KEYS17:
            case TRC_DIFF_OPT_SHOW_KEYS18:
            case TRC_DIFF_OPT_SHOW_KEYS19:
            case TRC_DIFF_OPT_SHOW_KEYS20:
            case TRC_DIFF_OPT_SHOW_KEYS21:
            case TRC_DIFF_OPT_SHOW_KEYS22:
            case TRC_DIFF_OPT_SHOW_KEYS23:
            case TRC_DIFF_OPT_SHOW_KEYS24:
            case TRC_DIFF_OPT_SHOW_KEYS25:
            case TRC_DIFF_OPT_SHOW_KEYS26:
            case TRC_DIFF_OPT_SHOW_KEYS27:
            case TRC_DIFF_OPT_SHOW_KEYS28:
            case TRC_DIFF_OPT_SHOW_KEYS29:
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
            case TRC_DIFF_OPT_EXCLUDE10:
            case TRC_DIFF_OPT_EXCLUDE11:
            case TRC_DIFF_OPT_EXCLUDE12:
            case TRC_DIFF_OPT_EXCLUDE13:
            case TRC_DIFF_OPT_EXCLUDE14:
            case TRC_DIFF_OPT_EXCLUDE15:
            case TRC_DIFF_OPT_EXCLUDE16:
            case TRC_DIFF_OPT_EXCLUDE17:
            case TRC_DIFF_OPT_EXCLUDE18:
            case TRC_DIFF_OPT_EXCLUDE19:
            case TRC_DIFF_OPT_EXCLUDE20:
            case TRC_DIFF_OPT_EXCLUDE21:
            case TRC_DIFF_OPT_EXCLUDE22:
            case TRC_DIFF_OPT_EXCLUDE23:
            case TRC_DIFF_OPT_EXCLUDE24:
            case TRC_DIFF_OPT_EXCLUDE25:
            case TRC_DIFF_OPT_EXCLUDE26:
            case TRC_DIFF_OPT_EXCLUDE27:
            case TRC_DIFF_OPT_EXCLUDE28:
            case TRC_DIFF_OPT_EXCLUDE29:
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

                if (trc_key_substs_read(key2html_fn) != 0)
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

    if (trc_diff_db_fn == NULL)
    {
        ERROR("Missing name of the file with expected testing results");
        goto exit;
    }

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

    /* Parse expected testing results database */
    if (trc_db_open(trc_diff_db_fn, &ctx->db) != 0)
    {
        ERROR("Failed to load expected testing results database");
        goto exit;
    }

    /* Make sure that all sets with logs have db_uid allocated */
    TAILQ_FOREACH(diff_set, &ctx->sets, links)
    {
        if ((diff_set->db_uid = trc_db_new_user(ctx->db)) < 0)
        {
            ERROR("trc_db_new_user() failed");
            goto exit;
        }
    }

    /* Parse logs for each diff set */
    if (trc_diff_process_logs(ctx) != 0)
    {
        ERROR("Failed to read logs");
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
    trc_key_substs_free();

    return result;
}
