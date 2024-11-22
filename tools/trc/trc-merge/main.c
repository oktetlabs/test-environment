/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Testing Results Comparator
 *
 * Main module of TRC DB merging tool.
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
#error "popt library is required for TRC merge tool"
#endif

#include "logger_api.h"
#include "logger_file.h"
#include "te_trc.h"
#include "trc_tools.h"
#include "te_vector.h"

/** TRC merge tool command line options */
enum {
    /** Show version */
    TRC_OPT_VERSION = 1,
    /** Path to one of the merged databases */
    TRC_OPT_DB,
    /** Where to save resulting database */
    TRC_OPT_OUTPUT,
};

/** Array of TRC databases paths specified in command line */
static te_vec trc_dbs = TE_VEC_INIT(const char *);
/** Path to the location for the resulting database */
static const char *out_fn = NULL;

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
    const char *db_fn;
    const char *s;

    /* Option Table */
    struct poptOption options_table[] = {
        { "db", 'd', POPT_ARG_STRING, NULL, TRC_OPT_DB,
          "Specify path to the TRC database main file.",
          "FILENAME" },

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
                db_fn = poptGetOptArg(optCon);
                TE_VEC_APPEND(&trc_dbs, db_fn);
                break;

            case TRC_OPT_OUTPUT:
                out_fn = poptGetOptArg(optCon);
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
    const char *db_path;
    const char **db_path_ptr;
    bool first = true;
    te_errno rc;

    te_log_init("TRC MERGE", te_log_message_file);

    if (process_cmd_line_opts(argc, argv) != EXIT_SUCCESS)
        goto exit;

    if (out_fn == NULL)
    {
        ERROR("Name of the output file is missing");
        goto exit;
    }
    if (te_vec_size(&trc_dbs) == 0)
    {
        ERROR("Input TRC databases are missing");
        goto exit;
    }

    TE_VEC_FOREACH(&trc_dbs, db_path_ptr)
    {
        db_path = *db_path_ptr;

        if (first)
        {
            rc = trc_db_open(db_path, &db);
            if (rc != 0)
            {
                ERROR("Failed to open TRC database %s: %r",
                      db_path, rc);
                goto exit;
            }
        }
        else
        {
            rc = trc_db_open_merge(db, db_path, 0);
            if (rc != 0)
            {
                ERROR("Failed to merge TRC database %s: %r",
                      db_path, rc);
                goto exit;
            }
        }

        first = FALSE;
    }

    rc = trc_db_save(db, out_fn,
                     TRC_SAVE_UPDATE_OLD | TRC_SAVE_RESULTS |
                     TRC_SAVE_GLOBALS | TRC_SAVE_DEL_XINCL |
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
    te_vec_free(&trc_dbs);

    return result;
}
