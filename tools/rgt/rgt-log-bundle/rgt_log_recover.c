/** @file
 * @brief Test Environment: recovering raw log from its fragments.
 *
 * This program recovers original raw log from fragments produced by
 * rgt-log-split.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <popt.h>

#include <inttypes.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "rgt_log_bundle_common.h"

#include "te_string.h"

/** Where to find raw log fragments and recover_list file */
static char *split_log_path = NULL;
/** Where to store recovered raw log */
static char *output_path = NULL;

/**
 * Parse command line.
 *
 * @param argc    Number of arguments
 * @param argv    Array of command line arguments
 */
static void
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon; /* Context for parsing command-line options */
    int          rc;

    /* Option Table */
    struct poptOption optionsTable[] = {
        { "split-log", 's', POPT_ARG_STRING, NULL, 's',
          "Path to split raw log.", NULL },

        { "output", 'o', POPT_ARG_STRING, NULL, 'o',
          "Output file.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            optionsTable, 0);

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 's')
            split_log_path = poptGetOptArg(optCon);
        else if (rc == 'o')
            output_path = poptGetOptArg(optCon);
    }

    if (split_log_path == NULL || output_path == NULL)
        usage(optCon, 1, "Specify all the required parameters", NULL);

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        exit(1);
    }

    if (poptPeekArg(optCon) != NULL)
        usage(optCon, 1, "Too many parameters specified", NULL);

    poptFreeContext(optCon);
}

int
main(int argc, char **argv)
{
    FILE       *f_recover;
    FILE       *f_result;
    FILE       *f_frag;
    char        s[DEF_STR_LEN];
    te_string   path = TE_STRING_INIT;

    char      frag_name[DEF_STR_LEN];
    uint64_t  raw_offset;
    uint64_t  raw_length;
    uint64_t  frag_offset;

    te_log_init("RGT LOG RECOVER");

    process_cmd_line_opts(argc, argv);

    /**
     * In recover_list file we list of raw log blocks is stored,
     * where for each block it is specified in which log fragment file
     * it can be found, what is its offset and length, and at which offset
     * it should appear in recovered raw log. Restoring original raw log
     * from this data is straightforward.
     */
    te_string_append(&path, "%s/recover_list", split_log_path);
    f_recover = fopen(path.ptr, "r");
    if (f_recover == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for reading\n", path.ptr);
        exit(1);
    }
    te_string_free(&path);

    f_result = fopen(output_path, "w");
    if (f_result == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for writing\n", output_path);
        exit(1);
    }

    while (!feof(f_recover))
    {
        if (fgets(s, sizeof(s), f_recover) == NULL)
            break;

        if (sscanf(s, "%" PRIu64 " %" PRIu64 " %s %" PRIu64,
                   &raw_offset, &raw_length,
                   frag_name, &frag_offset) <= 0)
            break;

        te_string_append(&path, "%s/%s", split_log_path, frag_name);
        f_frag = fopen(path.ptr, "r");
        if (f_frag == NULL)
        {
            fprintf(stderr, "Failed to open '%s' for reading\n", path.ptr);
            exit(1);
        }
        te_string_free(&path);
        file2file(f_result, f_frag, raw_offset, frag_offset, raw_length);
        fclose(f_frag);
    }

    fclose(f_result);
    fclose(f_recover);
    free(split_log_path);
    free(output_path);
    return 0;
}
