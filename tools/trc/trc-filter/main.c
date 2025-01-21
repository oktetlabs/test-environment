/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing Results Comparator
 *
 * Main module of TRC DB filter tool.
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
#error "popt library is required for TRC filter tool"
#endif

#include "logger_api.h"
#include "logger_file.h"
#include "te_trc.h"
#include "trc_tools.h"
#include "tq_string.h"

/** Command line options */
enum {
    /** Show version */
    TRC_OPT_VERSION = 1,
    /** Path to the database to be filtered */
    TRC_OPT_DB,
    /** TRC tag */
    TRC_OPT_TAG,
    /**
     * If specified, remove expressions mentioning tags (not negated),
     * otherwise - not mentioning
     */
    TRC_OPT_REVERSE,
    /*
     * If specified, only tests and iterations having some expected
     * results should be left in database.
     */
    TRC_OPT_DEL_NO_RES,
    /** Where to save resulting database */
    TRC_OPT_OUTPUT,
};

/** Path to source TRC database */
static const char *trc_db_path = NULL;
/** Tags which filtered expressions should mention */
static tqh_strings trc_tags = TAILQ_HEAD_INITIALIZER(trc_tags);
/** Flags for trc_db_filter_by_tags() (see trc_filter_flags) */
static unsigned int filter_flags = 0;
/** Path to the location for the resulting database */
static const char *out_path = NULL;

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option Table" that should be updated
 * when new options are added.
 *
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
process_cmd_line_opts(int argc, char **argv)
{
    int result = EXIT_FAILURE;
    poptContext optCon;
    int opt;
    const char *s;
    te_errno rc;

    /* Option Table */
    struct poptOption options_table[] = {
        { "db", 'd', POPT_ARG_STRING, NULL, TRC_OPT_DB,
          "Specify path to the TRC database main file.",
          "FILENAME" },

        { "tag", 't', POPT_ARG_STRING, NULL, TRC_OPT_TAG,
          "Specify TRC tag.", "TAGNAME" },

        { "reverse", 'r', POPT_ARG_NONE, NULL, TRC_OPT_REVERSE,
          "If specified, remove expressions mentioning tags.", NULL },

        { "del-no-res", 'd', POPT_ARG_NONE, NULL, TRC_OPT_DEL_NO_RES,
          "If specified, remove tests and iterations having no expected "
          "results.", NULL },

        { "output", 'o', POPT_ARG_STRING, NULL, TRC_OPT_OUTPUT,
          "Specify path to the resulting database.",
          "FILENAME" },

        { "version", '\0', POPT_ARG_NONE, NULL, TRC_OPT_VERSION,
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
            case TRC_OPT_DB:
                trc_db_path = poptGetOptArg(optCon);
                break;

            case TRC_OPT_TAG:
                s = poptGetOptArg(optCon);
                rc = tq_strings_add_uniq(&trc_tags, s);
                if (rc != 0)
                {
                    ERROR("Failed to add tag %s to the queue", s);
                    goto exit;
                }
                break;

            case TRC_OPT_REVERSE:
                filter_flags |= TRC_FILTER_REVERSE;
                break;

            case TRC_OPT_DEL_NO_RES:
                filter_flags |= TRC_FILTER_DEL_NO_RES;
                break;

            case TRC_OPT_OUTPUT:
                out_path = poptGetOptArg(optCon);
                break;

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

    if ((s = poptGetArg(optCon)) != NULL)
    {
        ERROR("Unexpected arguments in command line: %s", s);
        goto exit;
    }

    if (trc_db_path == NULL)
    {
        ERROR("Path to TRC database was not specified");
        goto exit;
    }

    if (out_path == NULL)
    {
        ERROR("Path for resulting database was not specified");
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:
    poptFreeContext(optCon);
    return result;
}

/**
 * Application entry point.
 *
 * @param argc      Number of command-line parameters.
 * @param argv      Array with command-line parameters as strings.
 *
 * @return Exit status.
 *
 * @retval EXIT_SUCCESS     Success
 * @retval EXIT_FAILURE     Failure
 */
int
main(int argc, char *argv[])
{
    int result = EXIT_FAILURE;
    te_trc_db *db = NULL;
    te_errno rc;

    te_log_init("TRC FILTER", te_log_message_file);

    if (process_cmd_line_opts(argc, argv) != EXIT_SUCCESS)
        goto exit;

    rc = trc_db_open_ext(trc_db_path, &db, TRC_OPEN_FIX_XINCLUDE);
    if (rc != 0)
    {
        ERROR("Failed to open TRC database %s: %r",
              trc_db_path, rc);
        goto exit;
    }

    trc_db_filter_by_tags(db, &trc_tags, filter_flags);

    rc = trc_db_save(db, out_path,
                     TRC_SAVE_UPDATE_OLD | TRC_SAVE_RESULTS |
                     TRC_SAVE_GLOBALS | TRC_SAVE_COMMENTS |
                     TRC_SAVE_NO_VOID_XINCL,
                     0, NULL, NULL, NULL, TRUE);
    if (rc != 0)
    {
        ERROR("Failed to save resulting database: %r", rc);
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:

    trc_db_close(db);
    tq_strings_free(&trc_tags, NULL);

    return result;
}
