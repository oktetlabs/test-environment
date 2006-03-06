/** @file
 * @brief Testing Results Comparator
 *
 * Show differencies between two set of tags.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for TRC tool
#endif

#include "te_defs.h"
#include "te_queue.h"
#include "trc_log.h"
#include "trc_tag.h"
#include "trc_db.h"


/** TRC tool command line options */
enum {
    TRC_DIFF_OPT_VERSION = 1,
    TRC_DIFF_OPT_EXCLUDE,
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
};


/** Title of the report in HTML format */
char *trc_diff_title = NULL;
/** Templates of keys to be excluded */
lh_string trc_diff_exclude_keys;

/** Name of the file with expected testing result database */
static char *trc_diff_db_fn = NULL;
/** Name of the file with report in HTML format */
static char *trc_diff_html_fn = NULL;

/** Report options */
static unsigned int trc_diff_flags = 0;


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
    poptContext optCon;
    int         rc;

    /* Option Table */
    struct poptOption options_table[] = {
        { "db", 'd', POPT_ARG_STRING, &trc_diff_db_fn, 0,
          "Specify name of the file with expected testing results "
          "database.",
          "FILENAME" },

        { "title", 't', POPT_ARG_STRING, &trc_diff_title, 0,
          "Title of the HTML report to be generate.", "TITLE" },

        { "html", 'h', POPT_ARG_STRING, &trc_diff_html_fn, 0,
          "Name of the file for report in HTML format.",
          "FILENAME" },

        { "exclude", 'e', POPT_ARG_STRING, NULL, TRC_DIFF_OPT_EXCLUDE,
          "Exclude from report entries with key by template or all "
          "(if template is empty).", NULL },

#define TRC_DIFF_TAG_OPT(id_) \
        { "tag" #id_, '0' + id_,                            \
          POPT_ARG_STRING, NULL, TRC_DIFF_OPT_TAG##id_,     \
          "Name of the tag from corresponding set.", "TAG" }

        TRC_DIFF_TAG_OPT(0),
        TRC_DIFF_TAG_OPT(1),
        TRC_DIFF_TAG_OPT(2),
        TRC_DIFF_TAG_OPT(3),
        TRC_DIFF_TAG_OPT(4),
        TRC_DIFF_TAG_OPT(5),
        TRC_DIFF_TAG_OPT(6),
        TRC_DIFF_TAG_OPT(7),
        TRC_DIFF_TAG_OPT(8),
        TRC_DIFF_TAG_OPT(9),

#undef TRC_DIFF_TAG_OPT

#define TRC_DIFF_NAME_OPT(id_) \
        { "name" #id_, '\0',                                \
          POPT_ARG_STRING, NULL, TRC_DIFF_OPT_NAME##id_,    \
          "Name of the corresponding set of tags.", "NAME" }

        TRC_DIFF_NAME_OPT(0),
        TRC_DIFF_NAME_OPT(1),
        TRC_DIFF_NAME_OPT(2),
        TRC_DIFF_NAME_OPT(3),
        TRC_DIFF_NAME_OPT(4),
        TRC_DIFF_NAME_OPT(5),
        TRC_DIFF_NAME_OPT(6),
        TRC_DIFF_NAME_OPT(7),
        TRC_DIFF_NAME_OPT(8),
        TRC_DIFF_NAME_OPT(9),

#undef TRC_DIFF_NAME_OPT

#define TRC_DIFF_SHOW_KEYS_OPT(id_) \
        { "show-keys" #id_, '\0',                                  \
          POPT_ARG_NONE, NULL, TRC_DIFF_OPT_SHOW_KEYS##id_,        \
          "Show table with keys which cause differences.",  NULL }

        TRC_DIFF_SHOW_KEYS_OPT(0),
        TRC_DIFF_SHOW_KEYS_OPT(1),
        TRC_DIFF_SHOW_KEYS_OPT(2),
        TRC_DIFF_SHOW_KEYS_OPT(3),
        TRC_DIFF_SHOW_KEYS_OPT(4),
        TRC_DIFF_SHOW_KEYS_OPT(5),
        TRC_DIFF_SHOW_KEYS_OPT(6),
        TRC_DIFF_SHOW_KEYS_OPT(7),
        TRC_DIFF_SHOW_KEYS_OPT(8),
        TRC_DIFF_SHOW_KEYS_OPT(9),

#undef TRC_DIFF_SHOW_KEYS_OPT

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
            case TRC_DIFF_OPT_EXCLUDE:
            {
                le_string  *no_bugid = calloc(1, sizeof(*no_bugid));

                if (no_bugid == NULL)
                {
                    ERROR("calloc() failed");
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                LIST_INSERT_HEAD(&trc_diff_exclude_keys, no_bugid, links);
                no_bugid->str = strdup(poptGetOptArg(optCon));
                break;
            }

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
                if (trc_diff_add_tag(&tags_diff, rc - TRC_DIFF_OPT_TAG0,
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
                if (trc_diff_set_name(&tags_diff, rc - TRC_DIFF_OPT_NAME0,
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
                if (trc_diff_show_keys(&tags_diff,
                                       rc - TRC_DIFF_OPT_SHOW_KEYS0) != 0)
                {
                    poptFreeContext(optCon);
                    return EXIT_FAILURE;
                }
                break;

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
    trc_tags_entry *p;

    memset(&trc_db, 0, sizeof(trc_db));
    TAILQ_INIT(&trc_db.tests.head);
    TAILQ_INIT(&tags);
    TAILQ_INIT(&tags_diff);
    LIST_INIT(&trc_diff_exclude_keys);

    /* Process and validate command-line options */
    if (process_cmd_line_opts(argc, argv) != EXIT_SUCCESS)
        goto exit;
    if (trc_diff_db_fn == NULL)
    {
        ERROR("Missing name of the file with expected testing results");
        goto exit;
    }
    if (trc_diff_html_fn == NULL)
    {
        ERROR("Missing name of the file for HTML report");
        goto exit;
    }

    /* Add tag of the default result */
    if (trc_add_tag(&tags, "result") != 0)
    {
        ERROR("Failed to add tag of the default result in the set 1");
        goto exit;
    }
    for (p = tags_diff.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (trc_add_tag(&p->tags, "result") != 0)
        {
            ERROR("Failed to add tag of the default result in the "
                  "set with ID=%d", p->id);
            goto exit;
        }
    }

    /* Parse expected testing results database */
    if (trc_parse_db(trc_diff_db_fn) != 0)
    {
        ERROR("Failed to parse expected testing results database");
        goto exit;
    }

    /* Generate reports in HTML format */
    if (trc_diff_report_to_html(&trc_db, trc_diff_flags,
                                trc_diff_html_fn) != 0)
    {
        ERROR("Failed to generate report in HTML format");
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:

    trc_free_db(&trc_db);
    free(trc_diff_db_fn);
    free(trc_diff_html_fn);
    trc_free_tags(&tags);
    trc_diff_free_tags(&tags_diff);

    return result;
}
